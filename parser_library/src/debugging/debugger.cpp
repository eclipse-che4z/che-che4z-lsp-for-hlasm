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
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "analyzer.h"
#include "context/hlasm_context.h"
#include "context/ordinary_assembly/ordinary_assembly_dependency_solver.h"
#include "context/variables/system_variable.h"
#include "context/well_known.h"
#include "debug_lib_provider.h"
#include "debug_types.h"
#include "debugger_configuration.h"
#include "diagnostic.h"
#include "diagnostic_consumer.h"
#include "expressions/evaluation_context.h"
#include "lexing/string_with_newlines.h"
#include "lexing/tools.h"
#include "library_info_transitional.h"
#include "output_handler.h"
#include "parsing/parser_impl.h"
#include "processing/op_code.h"
#include "processing/statement.h"
#include "processing/statement_analyzers/statement_analyzer.h"
#include "processing/statement_fields_parser.h"
#include "semantics/operand_impls.h"
#include "utils/async_busy_wait.h"
#include "utils/factory.h"
#include "utils/string_operations.h"
#include "utils/task.h"
#include "variable.h"
#include "workspace_manager.h"
#include "workspace_manager_response.h"
#include "workspaces/file_manager.h"
#include "workspaces/file_manager_vfm.h"


using namespace hlasm_plugin::parser_library::workspaces;

namespace hlasm_plugin::parser_library::debugging {

evaluated_expression evaluated_expression_value(std::string s = {}, var_reference_t var_def = 0)
{
    return {
        .result = std::move(s),
        .var_ref = var_def,
    };
}

evaluated_expression evaluated_expression_error(std::string s)
{
    return {
        .result = std::move(s),
        .error = true,
    };
}

// Implements DAP for macro tracing. Starts analyzer in a separate thread
// then controls the flow of analyzer by implementing processing_tracer
// interface.
class debugger::impl final : public processing::statement_analyzer, output_handler
{
    bool debug_ended_ = false;
    bool continue_ = false;

    // Specifies whether the debugger stops on the next statement call.
    bool stop_on_next_stmt_ = false;
    bool stop_on_stack_changes_ = false;
    std::pair<context::processing_stack_t, std::optional<utils::resource::resource_location>> stop_on_stack_condition_;

    // True, if disconnect request was received
    bool disconnected_ = false;

    // Provides a way to inform outer world about debugger events
    debug_event_consumer* event_ = nullptr;

    // Debugging information retrieval
    context::hlasm_context* ctx_ = nullptr;
    parse_lib_provider* lib_provider_ = nullptr;
    std::string opencode_source_uri_;
    std::vector<stack_frame> stack_frames_;
    std::vector<scope> scopes_;
    std::unordered_map<frame_id_t, context::system_variable_map> last_system_variables_;

    std::unordered_map<size_t, std::vector<variable>> variables_;
    size_t next_var_ref_ = 1;
    context::processing_stack_details_t proc_stack_;

    std::unordered_map<utils::resource::resource_location, std::vector<breakpoint>> breakpoints_;

    std::unordered_set<std::string, utils::hashers::string_hasher, std::equal_to<>> function_breakpoints_;

    size_t add_variable(std::vector<variable> vars)
    {
        variables_[next_var_ref_] = std::move(vars);
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
        workspaces::file_manager_vfm vfm(*dc.fm);

        if (auto prefetch = debug_provider.prefetch_libraries(); prefetch.valid())
            co_await std::move(prefetch);

        analyzer a(open_code_text.value(),
            analyzer_options {
                std::move(open_code_location),
                &debug_provider,
                std::move(dc.opts),
                std::move(dc.pp_opts),
                &vfm,
                static_cast<output_handler*>(this),
            });

        a.register_stmt_analyzer(this);

        ctx_ = a.context().hlasm_ctx.get();
        lib_provider_ = &debug_provider;

        co_await a.co_analyze();
    }

    void mnote(unsigned char level, std::string_view text) override
    {
        if (event_)
            event_->mnote(level, text);
    }

    void punch(std::string_view text) override
    {
        if (event_)
            event_->punch(text);
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
        dc_provider.provide_debugger_configuration(source, conf_resp);
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

        const auto& op_code = resolved_stmt->opcode_ref().value;
        // Continue only for non-empty statements
        if (op_code.empty())
            return false;

        const bool actr_limit = ctx_->get_branch_counter() < 0;

        const bool function_breakpoint_hit = function_breakpoints_.contains(op_code.to_string_view());

        range stmt_range = resolved_stmt->stmt_range_ref();

        bool breakpoint_hit = false;

        auto stack_node = ctx_->processing_stack();
        auto stack = ctx_->processing_stack_details();

        for (const auto& bp : breakpoints(stack.back().resource_loc))
        {
            if (bp.line >= stmt_range.start.line && bp.line <= stmt_range.end.line)
                breakpoint_hit = true;
        }

        const auto stack_condition_violated = [&cond = stop_on_stack_condition_](context::processing_stack_t cur) {
            auto last = cur;
            for (cur = cur.parent(); !cur.empty(); last = cur, cur = cur.parent())
                if (cur == cond.first)
                    break;
            return cond.first != cur || (cond.second && *cond.second != last.frame().resource_loc);
        };

        // breakpoint check
        if (stop_on_next_stmt_ || breakpoint_hit || function_breakpoint_hit || actr_limit
            || (stop_on_stack_changes_ && stack_condition_violated(stack_node)))
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
            stop_on_stack_condition_ = std::make_pair(stack_node, std::nullopt);

            continue_ = false;

            static constexpr std::string_view reasons[] = {
                "entry",
                "breakpoint",
                "function breakpoint",
                "breakpoint",
            };
            const auto reason_id = breakpoint_hit + 2 * function_breakpoint_hit;

            if (event_)
            {
                if (actr_limit)
                    event_->stopped("exception", "ACTR limit reached");
                else
                    event_->stopped(reasons[reason_id], "");
            }
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

    static std::string fpt_to_string(context::file_processing_type fpt)
    {
        switch (fpt)
        {
            case context::file_processing_type::OPENCODE:
                return "OPENCODE";
            case context::file_processing_type::MACRO:
                return "MACRO";
            case context::file_processing_type::COPY:
                return "COPY";
            default:
                return "";
        }
    }

    // Retrieval of current context.
    const std::vector<stack_frame>& stack_frames()
    {
        stack_frames_.clear();
        if (debug_ended_)
            return stack_frames_;
        for (size_t i = proc_stack_.size() - 1; i != (size_t)-1; --i)
        {
            const auto& frame = proc_stack_[i];

            stack_frames_.emplace_back(frame.pos.line,
                frame.pos.line,
                (uint32_t)i,
                fpt_to_string(frame.proc_type),
                source(frame.resource_loc.get_uri()));
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

        std::vector<variable> scope_vars;
        std::vector<variable> globals;
        std::vector<variable> ordinary_symbols;
        // we show only global variables that are valid for current scope,
        // moreover if we show variable in globals, we do not show it in locals

        const auto& current_scope = proc_stack_[frame_id].scope;

        if (current_scope.is_in_macro())
        {
            for (const auto& [name, value] : current_scope.this_macro->named_params)
            {
                if (name.empty())
                    continue;
                scope_vars.push_back(generate_macro_param_variable(*value, std::vector<context::A_t> {}));
            }
        }

        for (const auto& [_, value_type] : current_scope.variables)
        {
            const auto& [ref, _data, global] = value_type;
            if (global)
                globals.push_back(generate_set_symbol_variable(*ref));
            else
                scope_vars.push_back(generate_set_symbol_variable(*ref));
        }

        auto [sysvars, _] = last_system_variables_.try_emplace(frame_id, ctx_->get_system_variables(current_scope));
        for (const auto& [__, value_type] : sysvars->second)
        {
            const auto& [value, global] = value_type;
            if (global)
                globals.push_back(generate_macro_param_variable(*value, std::vector<context::A_t> {}));
            else
                scope_vars.push_back(generate_macro_param_variable(*value, std::vector<context::A_t> {}));
            // fetch all vars
        }

        for (const auto& it : ctx_->ord_ctx.symbols())
            if (const auto* sym = std::get_if<context::symbol>(&it.second))
                ordinary_symbols.push_back(generate_ordinary_symbol_variable(*sym));

        std::ranges::sort(globals, {}, &variable::name);
        std::ranges::sort(scope_vars, {}, &variable::name);
        std::ranges::sort(ordinary_symbols, {}, &variable::name);

        scopes_.emplace_back("Globals", add_variable(std::move(globals)), source(opencode_source_uri_));
        scopes_.emplace_back("Locals", add_variable(std::move(scope_vars)), source(opencode_source_uri_));
        scopes_.emplace_back(
            "Ordinary symbols", add_variable(std::move(ordinary_symbols)), source(opencode_source_uri_));

        return scopes_;
    }

    std::span<const hlasm_plugin::parser_library::debugging::variable> variables(var_reference_t var_ref)
    {
        if (debug_ended_)
            return {};

        auto it = variables_.find(var_ref);
        if (it == variables_.end())
            return {};

        for (auto& var : it->second)
        {
            if (var.is_scalar())
                continue;

            var.var_reference = add_variable(var.values());
        }

        return it->second;
    }

    struct error_collector final : diagnostic_op_consumer
    {
        std::string& msg;
        explicit error_collector(std::string& msg)
            : msg(msg)
        {}

        void add_diagnostic(diagnostic_op d) override
        {
            if (d.severity != diagnostic_severity::error)
                return;
            if (!msg.empty())
                msg.push_back('\n');
            msg.append(d.code).append(": ").append(d.message);
        }
    };

    evaluated_expression evaluate_expression(const expressions::ca_expression& expr,
        const context::code_scope& scope,
        const context::system_variable_map& sysvars) const
    {
        std::string error_msg;
        error_collector diags(error_msg);
        library_info_transitional lib_info(*lib_provider_);

        auto eval = expr.evaluate({ *ctx_, lib_info, diags, scope, sysvars });

        if (!error_msg.empty())
            return evaluated_expression_error(std::move(error_msg));

        using enum context::SET_t_enum;
        switch (eval.type())
        {
            case UNDEF_TYPE:
                return evaluated_expression_value("<UNDEFINED>");

            case A_TYPE:
                return evaluated_expression_value(std::to_string(eval.access_a()));

            case B_TYPE:
                return evaluated_expression_value(eval.access_b() ? "TRUE" : "FALSE");

            case C_TYPE:
                return evaluated_expression_value(std::move(eval.access_c()));
        }
    }

    static std::string to_string(const context::address& reloc)
    {
        std::string text;

        auto bases = std::vector<context::address::base_entry>(reloc.bases().begin(), reloc.bases().end());
        std::ranges::sort(bases, {}, [](const auto& e) { return e.owner->name; });
        bool first = true;
        for (const auto& [qualifier, owner, d] : bases)
        {
            if (owner->name.empty() || d == 0)
                continue;

            bool was_first = std::exchange(first, false);
            if (d < 0)
                text.append(was_first ? "-" : " - ");
            else if (!was_first)
                text.append(" + ");

            if (d != 1 && d != -1)
                text.append(std::to_string(d < 0 ? -(unsigned)d : (unsigned)d)).append("*");

            if (!qualifier.empty())
                text.append(qualifier.to_string_view()).append(".");
            text.append(owner->name.to_string_view());
        }
        if (!first)
            text.append(" + ");

        text.append("X'");
        uint32_t offset = reloc.offset();
        size_t len = text.size();
        do
        {
            text.push_back("0123456789ABCDEF"[offset & 0xf]);
            offset >>= 4;

        } while (offset);
        std::reverse(text.begin() + len, text.end());
        text.push_back('\'');

        return text;
    }

    evaluated_expression evaluate_expression(const expressions::mach_expression& expr) const
    {
        std::string error_msg;
        error_collector diags(error_msg);
        library_info_transitional lib_info(*lib_provider_);
        context::ordinary_assembly_dependency_solver dep_solver(ctx_->ord_ctx,
            // do not introduce a private section by accident
            // TODO: the alignment will be wrong
            //       - it is instruction and argument dependent which is not available yet
            ctx_->ord_ctx.current_section() ? ctx_->ord_ctx.align(context::no_align, lib_info) : context::address(),
            lib_info);

        if (auto d = expr.get_dependencies(dep_solver); d.has_error)
            return evaluated_expression_error("Expression cannot be evaluated");
        else if (d.contains_dependencies())
            return evaluated_expression_error("Expression has unresolved dependencies");

        auto eval = expr.evaluate(dep_solver, diags).ignore_qualification();

        if (!error_msg.empty())
            return evaluated_expression_error(std::move(error_msg));

        using enum context::symbol_value_kind;
        switch (eval.value_kind())
        {
            case UNDEF:
                return evaluated_expression_value("<UNDEFINED>");

            case ABS:
                return evaluated_expression_value(std::to_string(eval.get_abs()));

            case RELOC:
                return evaluated_expression_value(to_string(eval.get_reloc()));
        }
        // unreachable
    }

    static constexpr processing::processing_status mach_status = {
        processing::processing_format(processing::processing_kind::ORDINARY, processing::processing_form::MACH),
        processing::op_code()
    };
    static constexpr processing::processing_status seta_status = {
        processing::processing_format(processing::processing_kind::ORDINARY, processing::processing_form::CA),
        processing::op_code(context::well_known::SETA, context::instruction_type::CA)
    };
    static constexpr processing::processing_status setb_status = {
        processing::processing_format(processing::processing_kind::ORDINARY, processing::processing_form::CA),
        processing::op_code(context::well_known::SETB, context::instruction_type::CA)
    };
    static constexpr processing::processing_status setc_status = {
        processing::processing_format(processing::processing_kind::ORDINARY, processing::processing_form::CA),
        processing::op_code(context::well_known::SETC, context::instruction_type::CA)
    };

    evaluated_expression evaluate_exact_match(
        std::string_view var_name, const context::code_scope& scope, const context::system_variable_map& sysvars)
    {
        std::optional<debugging::variable> var;

        if (scope.is_in_macro())
        {
            for (const auto& [name, value] : scope.this_macro->named_params)
            {
                if (name.to_string_view() != var_name)
                    continue;
                var = generate_macro_param_variable(*value, std::vector<context::A_t> {});
                break;
            }
        }

        if (!var)
        {
            for (const auto& [name, value] : scope.variables)
            {
                if (name.to_string_view() != var_name)
                    continue;
                var = generate_set_symbol_variable(*value.ref);
                break;
            }
        }

        if (!var)
        {
            for (const auto& [name, value] : sysvars)
            {
                if (name.to_string_view() != var_name)
                    continue;
                var = generate_macro_param_variable(*value.first, std::vector<context::A_t> {});
                break;
            }
        }

        if (!var)
            return evaluated_expression_error("Variable not found");

        return evaluated_expression_value(std::move(var->value), var->is_scalar() ? 0 : add_variable(var->values()));
    }

    evaluated_expression evaluate(std::string_view expr, frame_id_t frame_id)
    {
        if (debug_ended_ || expr.empty())
            return evaluated_expression_value();

        if (frame_id == (size_t)-1)
            frame_id = proc_stack_.size() - 1;

        if (frame_id >= proc_stack_.size())
            return evaluated_expression_error("Invalid frame id");

        const auto& scope = proc_stack_[frame_id].scope;
        auto [sysvars, _] = last_system_variables_.try_emplace(
            frame_id, utils::factory([this, &scope]() { return ctx_->get_system_variables(scope); }));

        if (expr.starts_with("&") && lexing::is_valid_symbol_name(expr.substr(1)))
            return evaluate_exact_match(utils::to_upper_copy(expr.substr(1)), scope, sysvars->second);

        static constexpr auto stringy_attribute = [](std::string_view s) {
            return s.starts_with("T'") || s.starts_with("O'") || s.starts_with("t'") || s.starts_with("o'");
        };

        const auto& status = [&expr]() -> const auto& {
            if (const auto pos = expr.find("&"); pos == (size_t)-1)
                return mach_status;
            else if (expr.front() == '(' && expr.back() == ')')
                return setb_status;
            else if (expr.front() == '\'' || stringy_attribute(expr))
                return setc_status;
            else
                return seta_status;
        }();

        std::string error_msg;
        error_collector diags(error_msg);

        auto p = parsing::parser_holder(*ctx_, nullptr);
        p.prepare_parser(
            lexing::u8string_view_with_newlines(expr), *ctx_, &diags, semantics::range_provider(), range(), 1, status);

        semantics::operand_ptr op =
            status.first.form == processing::processing_form::CA ? p.ca_op_expr() : p.operand_mach();

        static constexpr auto ca_expr = [](const semantics::operand_ptr& o) -> const expressions::ca_expression* {
            if (const auto* ca_op = o->access_ca())
                return ca_op->access_expr()->expression.get();
            return nullptr;
        };
        static constexpr auto mach_expr = [](const semantics::operand_ptr& o) -> const expressions::mach_expression* {
            if (const auto* mach_op = o->access_mach(); mach_op && mach_op->is_single_expression())
                return mach_op->displacement.get();
            return nullptr;
        };

        if (!error_msg.empty())
            return evaluated_expression_error(std::move(error_msg));
        else if (!op)
            return evaluated_expression_error("Single expression expected");
        else if (const auto* caop = ca_expr(op))
            return evaluate_expression(*caop, scope, sysvars->second);
        else if (const auto* mop = mach_expr(op))
            return evaluate_expression(*mop);
        else
            return evaluated_expression_error("Unexpected operand format");
    }

    void breakpoints(const utils::resource::resource_location& source, std::span<const breakpoint> bps)
    {
        breakpoints_[source].assign(bps.begin(), bps.end());
    }

    [[nodiscard]] std::span<const breakpoint> breakpoints(const utils::resource::resource_location& source) const
    {
        if (auto it = breakpoints_.find(source); it != breakpoints_.end())
            return it->second;
        return {};
    }

    void function_breakpoints(std::span<const function_breakpoint> bps)
    {
        function_breakpoints_.clear();
        for (const auto& bp : bps)
            function_breakpoints_.emplace(utils::to_upper_copy(bp.name));
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

void debugger::launch(std::string_view source,
    debugger_configuration_provider& dc_provider,
    bool stop_on_entry,
    workspace_manager_response<bool> resp)
{
    pimpl->launch(source, dc_provider, stop_on_entry, std::move(resp));
}

void debugger::set_event_consumer(debug_event_consumer* event) { pimpl->set_event_consumer(event); }

void debugger::next() { pimpl->next(); }
void debugger::step_in() { pimpl->step_in(); }
void debugger::step_out() { pimpl->step_out(); }
void debugger::disconnect() { pimpl->disconnect(); }
void debugger::continue_debug() { pimpl->continue_debug(); }
void debugger::pause() { pimpl->pause(); }
void debugger::analysis_step(const std::atomic<unsigned char>* yield_indicator) { pimpl->step(yield_indicator); }


void debugger::breakpoints(std::string_view source, std::span<const breakpoint> bps)
{
    pimpl->breakpoints(utils::resource::resource_location(source), bps);
}
std::span<const breakpoint> debugger::breakpoints(std::string_view source) const
{
    return pimpl->breakpoints(utils::resource::resource_location(source));
}

void debugger::function_breakpoints(std::span<const function_breakpoint> bps) { pimpl->function_breakpoints(bps); }

std::span<const stack_frame> debugger::stack_frames() const { return pimpl->stack_frames(); }
std::span<const scope> debugger::scopes(frame_id_t frame_id) const { return pimpl->scopes(frame_id); }
std::span<const variable> debugger::variables(var_reference_t var_ref) const { return pimpl->variables(var_ref); }

evaluated_expression debugger::evaluate(std::string_view expr, frame_id_t id) { return pimpl->evaluate(expr, id); }

} // namespace hlasm_plugin::parser_library::debugging
