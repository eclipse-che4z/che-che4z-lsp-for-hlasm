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
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "analyzer.h"
#include "debug_lib_provider.h"
#include "debug_types.h"
#include "macro_param_variable.h"
#include "ordinary_symbol_variable.h"
#include "set_symbol_variable.h"
#include "variable.h"
#include "virtual_file_monitor.h"
#include "workspaces/file_manager.h"
#include "workspaces/file_manager_vfm.h"
#include "workspaces/workspace.h"

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
    mutable std::mutex control_mtx;
    mutable std::mutex variable_mtx_;
    mutable std::mutex breakpoints_mutex_;

    std::thread thread_;

    // these are used in conditional variable to stop execution
    // of analyzer and wait for user input
    std::condition_variable con_var;
    std::atomic<bool> continue_ = false;

    bool debug_ended_ = false;

    // Specifies whether the debugger stops on the next statement call.
    std::atomic<bool> stop_on_next_stmt_ = false;
    std::atomic<bool> step_over_ = false;
    size_t step_over_depth_ = 0;

    // Range of statement that is about to be processed by analyzer.
    range next_stmt_range_;

    // True, if disconnect request was received
    bool disconnected_ = false;

    // Cancellation token for parsing
    std::atomic<bool> cancel_ = false;

    // Provides a way to inform outer world about debugger events
    debug_event_consumer* event_ = nullptr;

    // Debugging information retrieval
    context::hlasm_context* ctx_ = nullptr;
    std::string opencode_source_uri_;
    std::vector<stack_frame> stack_frames_;
    std::vector<scope> scopes_;

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

public:
    impl() = default;

    bool launch(std::string_view source, workspaces::workspace& workspace, bool stop_on_entry)
    {
        // still has data races
        utils::resource::resource_location open_code_location(source);
        auto open_code_text = workspace.get_file_manager().get_file_content(open_code_location);
        if (!open_code_text.has_value())
        {
            debug_ended_ = true;
            return false;
        }
        opencode_source_uri_ = open_code_location.get_uri();
        stop_on_next_stmt_ = stop_on_entry;

        struct debugger_thread_data
        {
            utils::resource::resource_location open_code_location;
            std::string open_code_text;
            debug_lib_provider debug_provider;
            asm_option asm_opts;
            std::vector<preprocessor_options> pp_opts;
            workspaces::file_manager_vfm vfm;
            analyzer_options opts;
        };

        auto data = std::make_unique<debugger_thread_data>(debugger_thread_data {
            open_code_location,
            std::move(open_code_text).value(),
            debug_lib_provider(workspace.get_libraries(open_code_location), workspace.get_file_manager(), &cancel_),
            workspace.get_asm_options(open_code_location),
            workspace.get_preprocessor_options(open_code_location),
            workspaces::file_manager_vfm(
                workspace.get_file_manager(), utils::resource::resource_location(workspace.uri())),
        });

        thread_ = std::thread([this, data = std::move(data)]() {
            std::lock_guard<std::mutex> guard(variable_mtx_); // Lock the mutex while analyzer is running, unlock once
                                                              // it is stopped and waiting in the statement method

            analyzer a(data->open_code_text,
                analyzer_options {
                    std::move(data->open_code_location),
                    &data->debug_provider,
                    std::move(data->asm_opts),
                    std::move(data->pp_opts),
                    &data->vfm,
                });

            a.register_stmt_analyzer(this);

            ctx_ = a.context().hlasm_ctx.get();

            a.analyze(&cancel_);

            if (!disconnected_ && event_)
                event_->exited(0);
            debug_ended_ = true;
        });

        return true;
    }

    void set_event_consumer(debug_event_consumer* event) { event_ = event; }

    void analyze(const context::hlasm_statement& statement,
        processing::statement_provider_kind,
        processing::processing_kind proc_kind,
        bool evaluated_model) override
    {
        if (disconnected_)
            return;

        if (evaluated_model)
            return; // we already stopped on the model itself

        // Continue only for ordinary processing kind (i.e. when the statement is executed, skip
        // lookahead and copy/macro definitions)
        if (proc_kind != processing::processing_kind::ORDINARY)
            return;

        const auto* resolved_stmt = statement.access_resolved();
        if (!resolved_stmt)
            return;

        // Continue only for non-empty statements
        if (resolved_stmt->opcode_ref().value.empty())
            return;

        range stmt_range = resolved_stmt->stmt_range_ref();

        bool breakpoint_hit = false;

        auto stack = ctx_->processing_stack_details();
        const auto stack_depth = stack.size();

        for (const auto& bp : breakpoints(*stack.back().resource_loc))
        {
            if (bp.line >= stmt_range.start.line && bp.line <= stmt_range.end.line)
                breakpoint_hit = true;
        }

        // breakpoint check
        if (stop_on_next_stmt_ || breakpoint_hit || (step_over_ && stack_depth <= step_over_depth_))
        {
            variables_.clear();
            stack_frames_.clear();
            scopes_.clear();
            proc_stack_ = std::move(stack);
            variable_mtx_.unlock();

            std::unique_lock<std::mutex> lck(control_mtx);
            if (disconnected_)
                return;
            stop_on_next_stmt_ = false;
            step_over_ = false;
            next_stmt_range_ = stmt_range;
            step_over_depth_ = stack_depth;

            continue_ = false;
            if (event_)
                event_->stopped("entry", "");

            con_var.wait(lck, [&] { return !!continue_; });

            variable_mtx_.lock();
        }
    }

    // User controls of debugging.
    void next()
    {
        {
            std::lock_guard<std::mutex> lck(control_mtx);
            step_over_ = true;
            continue_ = true;
        }
        con_var.notify_all();
    }

    void step_in()
    {
        {
            std::lock_guard<std::mutex> lck(control_mtx);
            stop_on_next_stmt_ = true;
            continue_ = true;
        }
        con_var.notify_all();
    }

    void disconnect()
    {
        // set cancellation token and wake up the thread,
        // so it peacefully finishes, we then wait for it and join
        {
            std::lock_guard<std::mutex> lck(control_mtx);

            disconnected_ = true;
            continue_ = true;
            cancel_ = true;
        }
        con_var.notify_all();

        if (thread_.joinable())
            thread_.join();
    }

    void continue_debug()
    {
        stop_on_next_stmt_ = false;
        continue_ = true;
        con_var.notify_all();
    }

    void pause() { stop_on_next_stmt_ = true; }

    // Retrieval of current context.
    const std::vector<stack_frame>& stack_frames()
    {
        std::lock_guard<std::mutex> guard(variable_mtx_);
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
        std::lock_guard<std::mutex> guard(variable_mtx_);

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
            for (auto it : proc_stack_[frame_id].scope.this_macro->named_params)
            {
                if (it.first.empty())
                    continue;
                scope_vars.push_back(std::make_unique<macro_param_variable>(*it.second, std::vector<size_t> {}));
            }

        for (auto it : proc_stack_[frame_id].scope.variables)
        {
            if (it.second->is_global)
                globals.push_back(std::make_unique<set_symbol_variable>(*it.second));
            else
                scope_vars.push_back(std::make_unique<set_symbol_variable>(*it.second));
        }

        for (auto it : proc_stack_[frame_id].scope.system_variables)
        {
            if (it.second->is_global)
                globals.push_back(std::make_unique<macro_param_variable>(*it.second, std::vector<size_t> {}));
            else
                scope_vars.push_back(std::make_unique<macro_param_variable>(*it.second, std::vector<size_t> {}));
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

        std::lock_guard<std::mutex> guard(variable_mtx_);
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
        std::lock_guard g(breakpoints_mutex_);
        breakpoints_[source] = std::move(bps);
    }

    [[nodiscard]] std::vector<breakpoint> breakpoints(const utils::resource::resource_location& source) const
    {
        std::lock_guard g(breakpoints_mutex_);
        if (auto it = breakpoints_.find(source); it != breakpoints_.end())
            return it->second;
        return {};
    }

    ~impl()
    {
        if (thread_.joinable())
            disconnect();
    }
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

bool debugger::launch(sequence<char> source, workspaces::workspace& source_workspace, bool stop_on_entry)
{
    return pimpl->launch(std::string_view(source), source_workspace, stop_on_entry);
}

void debugger::set_event_consumer(debug_event_consumer* event) { pimpl->set_event_consumer(event); }

void debugger::next() { pimpl->next(); }
void debugger::step_in() { pimpl->step_in(); }
void debugger::disconnect() { pimpl->disconnect(); }
void debugger::continue_debug() { pimpl->continue_debug(); }
void debugger::pause() { pimpl->pause(); }


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