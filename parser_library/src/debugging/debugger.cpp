/*
 * Copyright (c) 2019 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

#include "debugger.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "analyzer.h"
#include "context/hlasm_context.h"
#include "context/variables/system_variable.h"
#include "debug_lib_provider.h"
#include "debug_types.h"
#include "debugger_configuration.h"
#include "macro_param_variable.h"
#include "ordinary_symbol_variable.h"
#include "processing/statement.h"
#include "processing/statement_analyzers/statement_analyzer.h"
#include "set_symbol_variable.h"
#include "utils/async_busy_wait.h"
#include "utils/task.h"
#include "variable.h"
#include "virtual_file_monitor.h"
#include "workspace_manager.h"
#include "workspace_manager_response.h"
#include "workspaces/file_manager.h"
#include "workspaces/file_manager_vfm.h"


using namespace hlasm_plugin::parser_library::workspaces;

namespace hlasm_plugin::parser_library::debugging {

class breakpoints_t::impl
{
    friend class debugger;
    friend class breakpoints_t;

    std::vector<breakpoint> m_breakpoints;
};

breakpoints_t::breakpoints_t()
    : pimpl(new impl())
{}
breakpoints_t::breakpoints_t(breakpoints_t&& b) noexcept
    : pimpl(std::exchange(b.pimpl, nullptr))
{}
breakpoints_t& breakpoints_t::operator=(breakpoints_t&& b) & noexcept
{
    breakpoints_t tmp(std::move(b));
    std::swap(pimpl, tmp.pimpl);
    return *this;
};
breakpoints_t::~breakpoints_t()
{
    if (pimpl)
        delete pimpl;
}
const breakpoint* breakpoints_t::begin() const { return pimpl->m_breakpoints.data(); }
const breakpoint* breakpoints_t::end() const { return pimpl->m_breakpoints.data() + pimpl->m_breakpoints.size(); }
std::size_t breakpoints_t::size() const { return pimpl->m_breakpoints.size(); }

// Implements DAP for macro tracing. Starts analyzer in a separate thread
// then controls the flow of analyzer by implementing processing_tracer
// interface.
class debugger::impl final : public processing::statement_analyzer
{
    bool debug_ended_ = false;
    bool continue_ = false;

    // Specifies whether the debugger stops on the next statement call.
    bool stop_on_next_stmt_ = false;
    bool stop_on_stack_changes_ = false;
    std::pair<context::processing_stack_t, const utils::resource::resource_location*> stop_on_stack_condition_;

    // True, if disconnect request was received
    bool disconnected_ = false;

    // Provides a way to inform outer world about debugger events
    debug_event_consumer* event_ = nullptr;

    // Debugging information retrieval
    context::hlasm_context* ctx_ = nullptr;
    std::string opencode_source_uri_;
    std::vector<stack_frame> stack_frames_;
    std::vector<scope> scopes_;
    context::hlasm_context::sysvar_map last_system_variables_;

    std::unordered_map<size_t, variable_store> variables_;
    size_t next_var_ref_ = 1;
    context::processing_stack_details_t proc_stack_;

    std::unordered_map<utils::resource::resource_location,
        std::vector<breakpoint>,
        utils::resource::resource_location_hasher>
        breakpoints_;

    size_t add_variable(std::vector<variable_ptr> vars)
    {
        variables_[next_var_ref_].variables = std::move(vars);
        return next_var_ref_++;
    }

    utils::task analyzer_task;

    utils::task start_main_analyzer(utils::resource::resource_location open_code_location,
        workspace_manager_response<bool> resp,
        debugger_configuration dc)
    {
        if (!dc.fm)
        {
            resp.provide(false);
            co_return;
        }
        auto open_code_text = co_await dc.fm->get_file_content(open_code_location);
        if (!open_code_text.has_value())
        {
            resp.provide(false);
            co_return;
        }
        resp.provide(true);
        debug_lib_provider debug_provider(std::move(dc.libraries), *dc.fm);
        workspaces::file_manager_vfm vfm(*dc.fm, std::move(dc.workspace_uri));

        if (auto prefetch = debug_provider.prefetch_libraries(); prefetch.valid())
            co_await std::move(prefetch);

        analyzer a(open_code_text.value(),
            analyzer_options {
                std::move(open_code_location),
                &debug_provider,
                std::move(dc.opts),
                std::move(dc.pp_opts),
                &vfm,
            });

        a.register_stmt_analyzer(this);

        ctx_ = a.context().hlasm_ctx.get();

        co_await a.co_analyze();
    }

public:
    impl() = default;

    void launch(std::string_view source,
        debugger_configuration_provider& dc_provider,
        bool stop_on_entry,
        workspace_manager_response<bool> resp)
    {
        opencode_source_uri_ = source;
        continue_ = true;
        stop_on_next_stmt_ = stop_on_entry;
        stop_on_stack_changes_ = false;

        struct conf_t
        {
            debugger_configuration conf;

            void provide(debugger_configuration v) { conf = std::move(v); }
            void error(int, const char*) const noexcept {}
        };
        auto [conf_resp, conf] = make_workspace_manager_response(std::in_place_type<conf_t>);
        dc_provider.provide_debugger_configuration(sequence<char>(source), conf_resp);
        analyzer_task =
            utils::async_busy_wait(std::move(conf_resp), &conf->conf)
                .then(std::bind_front(
                    &impl::start_main_analyzer, this, utils::resource::resource_location(source), std::move(resp)));
    }

    void step(const std::atomic<unsigned char>* yield_indicator)
    {
        if (!analyzer_task.valid() || debug_ended_)
            return;

        if (!continue_)
            return;

        if (!analyzer_task.done())
        {
            analyzer_task.resume(yield_indicator);
            if (!analyzer_task.done())
                return;
        }

        analyzer_task = {};

        if (!disconnected_ && event_)
            event_->exited(0);
        debug_ended_ = true;
    }

    void set_event_consumer(debug_event_consumer* event) { event_ = event; }

    bool analyze(const context::hlasm_statement& statement,
        processing::statement_provider_kind,
        processing::processing_kind proc_kind,
        bool evaluated_model) override
    {
        if (disconnected_)
            return false;

        if (evaluated_model)
            return false; // we already stopped on the model itself

        // Continue only for ordinary processing kind (i.e. when the statement is executed, skip
        // lookahead and copy/macro definitions)
        if (proc_kind != processing::processing_kind::ORDINARY)
            return false;

        const auto* resolved_stmt = statement.access_resolved();
        if (!resolved_stmt)
            return false;

        // Continue only for non-empty statements
        if (resolved_stmt->opcode_ref().value.empty())
            return false;

        range stmt_range = resolved_stmt->stmt_range_ref();

        bool breakpoint_hit = false;

        auto stack_node = ctx_->processing_stack();
        auto stack = ctx_->processing_stack_details();

        for (const auto& bp : breakpoints(*stack.back().resource_loc))
        {
            if (bp.line >= stmt_range.start.line && bp.line <= stmt_range.end.line)
                breakpoint_hit = true;
        }

        const auto stack_condition_violated = [cond = stop_on_stack_condition_](context::processing_stack_t cur) {
            auto last = cur;
            for (cur = cur.parent(); !cur.empty(); last = cur, cur = cur.parent())
                if (cur == cond.first)
                    break;
            return cond.first != cur || (cond.second && cond.second != last.frame().resource_loc);
        };

        // breakpoint check
        if (stop_on_next_stmt_ || breakpoint_hit || (stop_on_stack_changes_ && stack_condition_violated(stack_node)))
        {
            variables_.clear();
            stack_frames_.clear();
            scopes_.clear();
            proc_stack_ = std::move(stack);
            last_system_variables_.clear();

            if (disconnected_)
                return false;
            stop_on_next_stmt_ = false;
            stop_on_stack_changes_ = false;
            stop_on_stack_condition_ = std::make_pair(stack_node, nullptr);

            continue_ = false;
            if (event_)
                event_->stopped("entry", "");
        }
        return !continue_;
    }

    void analyze_aread_line(const utils::resource::resource_location&, size_t, std::string_view) override {}

    // User controls of debugging.
    void next()
    {
        stop_on_stack_changes_ = true;
        continue_ = true;
    }

    void step_in()
    {
        stop_on_next_stmt_ = true;
        continue_ = true;
    }

    void step_out()
    {
        if (!stop_on_stack_condition_.first.empty())
        {
            stop_on_stack_changes_ = true;
            stop_on_stack_condition_ = std::make_pair(
                stop_on_stack_condition_.first.parent(), stop_on_stack_condition_.first.frame().resource_loc);
        }
        else
            stop_on_next_stmt_ = false; // step out in the opencode is equivalent to continue
        continue_ = true;
    }

    void disconnect()
    {
        // set cancellation token and wake up the thread,
        // so it peacefully finishes, we then wait for it and join
        disconnected_ = true;
        continue_ = true;
    }

    void continue_debug()
    {
        stop_on_next_stmt_ = false;
        continue_ = true;
    }

    void pause() { stop_on_next_stmt_ = true; }

    // Retrieval of current context.
    const std::vector<stack_frame>& stack_frames()
    {
        stack_frames_.clear();
        if (debug_ended_)
            return stack_frames_;
        for (size_t i = proc_stack_.size() - 1; i != (size_t)-1; --i)
        {
            source source(proc_stack_[i].resource_loc->get_uri());
            std::string name;
            switch (proc_stack_[i].proc_type)
            {
                case context::file_processing_type::OPENCODE:
                    name = "OPENCODE";
                    break;
                case context::file_processing_type::MACRO:
                    name = "MACRO";
                    break;
                case context::file_processing_type::COPY:
                    name = "COPY";
                    break;
                default:
                    name = "";
                    break;
            }

            stack_frames_.emplace_back(
                proc_stack_[i].pos.line, proc_stack_[i].pos.line, (uint32_t)i, std::move(name), std::move(source));
        }


        return stack_frames_;
    }

    const std::vector<scope>& scopes(frame_id_t frame_id)
    {
        scopes_.clear();
        if (debug_ended_)
            return scopes_;

        if (frame_id >= proc_stack_.size())
            return scopes_;

        std::vector<variable_ptr> scope_vars;
        std::vector<variable_ptr> globals;
        std::vector<variable_ptr> ordinary_symbols;
        // we show only global variables that are valid for current scope,
        // moreover if we show variable in globals, we do not show it in locals

        if (proc_stack_[frame_id].scope.is_in_macro())
        {
            for (const auto& [name, value] : proc_stack_[frame_id].scope.this_macro->named_params)
            {
                if (name.empty())
                    continue;
                scope_vars.push_back(std::make_unique<macro_param_variable>(*value, std::vector<context::A_t> {}));
            }
        }

        for (const auto& [_, value_type] : proc_stack_[frame_id].scope.variables)
        {
            const auto& [ref, _data, global] = value_type;
            if (global)
                globals.push_back(std::make_unique<set_symbol_variable>(*ref));
            else
                scope_vars.push_back(std::make_unique<set_symbol_variable>(*ref));
        }

        last_system_variables_ = ctx_->get_system_variables(proc_stack_[frame_id].scope);
        for (const auto& [_, value_type] : last_system_variables_)
        {
            const auto& [value, global] = value_type;
            if (global)
                globals.push_back(std::make_unique<macro_param_variable>(*value, std::vector<context::A_t> {}));
            else
                scope_vars.push_back(std::make_unique<macro_param_variable>(*value, std::vector<context::A_t> {}));
            // fetch all vars
        }

        for (const auto& it : ctx_->ord_ctx.symbols())
            if (const auto* sym = std::get_if<context::symbol>(&it.second))
                ordinary_symbols.push_back(std::make_unique<ordinary_symbol_variable>(*sym));

        constexpr auto compare_variables = [](const variable_ptr& l, const variable_ptr& r) {
            return l->get_name() < r->get_name();
        };

        std::sort(globals.begin(), globals.end(), compare_variables);
        std::sort(scope_vars.begin(), scope_vars.end(), compare_variables);
        std::sort(ordinary_symbols.begin(), ordinary_symbols.end(), compare_variables);

        scopes_.emplace_back("Globals", add_variable(std::move(globals)), source(opencode_source_uri_));
        scopes_.emplace_back("Locals", add_variable(std::move(scope_vars)), source(opencode_source_uri_));
        scopes_.emplace_back(
            "Ordinary symbols", add_variable(std::move(ordinary_symbols)), source(opencode_source_uri_));

        return scopes_;
    }

    const hlasm_plugin::parser_library::debugging::variable_store& variables(var_reference_t var_ref)
    {
        static const hlasm_plugin::parser_library::debugging::variable_store empty_variables;

        if (debug_ended_)
            return empty_variables;
        auto it = variables_.find(var_ref);
        if (it == variables_.end())
            return empty_variables;
        for (const auto& var : it->second.variables)
        {
            if (var->is_scalar())
                continue;

            var->var_reference = add_variable(var->values());
        }

        return it->second;
    }

    void breakpoints(const utils::resource::resource_location& source, std::vector<breakpoint> bps)
    {
        breakpoints_[source] = std::move(bps);
    }

    [[nodiscard]] std::vector<breakpoint> breakpoints(const utils::resource::resource_location& source) const
    {
        if (auto it = breakpoints_.find(source); it != breakpoints_.end())
            return it->second;
        return {};
    }

    ~impl() { disconnect(); }
};

debugger::debugger()
    : pimpl(new impl())
{}
debugger::debugger(debugger&& d) noexcept
    : pimpl(std::exchange(d.pimpl, nullptr))
{}
debugger& debugger::operator=(debugger&& d) & noexcept
{
    debugger tmp(std::move(d));
    std::swap(pimpl, tmp.pimpl);
    return *this;
}
debugger::~debugger()
{
    if (pimpl)
        delete pimpl;
}

void debugger::launch(sequence<char> source,
    debugger_configuration_provider& dc_provider,
    bool stop_on_entry,
    workspace_manager_response<bool> resp)
{
    pimpl->launch(std::string_view(source), dc_provider, stop_on_entry, std::move(resp));
}

void debugger::set_event_consumer(debug_event_consumer* event) { pimpl->set_event_consumer(event); }

void debugger::next() { pimpl->next(); }
void debugger::step_in() { pimpl->step_in(); }
void debugger::step_out() { pimpl->step_out(); }
void debugger::disconnect() { pimpl->disconnect(); }
void debugger::continue_debug() { pimpl->continue_debug(); }
void debugger::pause() { pimpl->pause(); }
void debugger::analysis_step(const std::atomic<unsigned char>* yield_indicator) { pimpl->step(yield_indicator); }


void debugger::breakpoints(sequence<char> source, sequence<breakpoint> bps)
{
    pimpl->breakpoints(utils::resource::resource_location(std::string(source)), std::vector<breakpoint>(bps));
}
breakpoints_t debugger::breakpoints(sequence<char> source) const
{
    breakpoints_t result;

    result.pimpl->m_breakpoints = pimpl->breakpoints(utils::resource::resource_location(std::string(source)));


    return result;
}

stack_frames_t debugger::stack_frames() const
{
    const auto& frames = pimpl->stack_frames();
    return stack_frames_t(frames.data(), frames.size());
}
scopes_t debugger::scopes(frame_id_t frame_id) const
{
    const auto& s = pimpl->scopes(frame_id);
    return scopes_t(s.data(), s.size());
}
variables_t debugger::variables(var_reference_t var_ref) const
{
    const auto& v = pimpl->variables(var_ref);
    return variables_t(&v, v.variables.size());
}

} // namespace hlasm_plugin::parser_library::debugging
