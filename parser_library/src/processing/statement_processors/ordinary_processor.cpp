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

#include "ordinary_processor.h"

#include <stdexcept>

#include "checking/diagnostic_collector.h"
#include "checking/instruction_checker.h"
#include "checking/machine_check.h"
#include "checking/using_label_checker.h"
#include "context/hlasm_context.h"
#include "context/literal_pool.h"
#include "context/ordinary_assembly/location_counter.h"
#include "context/ordinary_assembly/ordinary_assembly_dependency_solver.h"
#include "context/ordinary_assembly/symbol_dependency_tables.h"
#include "diagnostic_consumer.h"
#include "ebcdic_encoding.h"
#include "instructions/instruction.h"
#include "lsp/lsp_context.h"
#include "processing/instruction_sets/postponed_statement_impl.h"
#include "processing/processing_manager.h"
#include "semantics/operand_impls.h"
#include "utils/projectors.h"
#include "utils/truth_table.h"

namespace hlasm_plugin::parser_library::processing {

ordinary_processor::ordinary_processor(const analyzing_context& ctx,
    branching_provider& branch_provider,
    parse_lib_provider& lib_provider,
    processing_state_listener& state_listener,
    statement_fields_parser& parser,
    opencode_provider& open_code,
    processing_manager& proc_mgr,
    output_handler* output,
    diagnosable_ctx& diag_ctx)
    : statement_processor(processing_kind::ORDINARY, ctx, diag_ctx)
    , branch_provider_(branch_provider)
    , lib_info(lib_provider)
    , eval_ctx { *ctx.hlasm_ctx, lib_info, diag_ctx }
    , ca_proc_(ctx, branch_provider, lib_provider, state_listener, open_code, diag_ctx)
    , mac_proc_(ctx, branch_provider, lib_provider, diag_ctx)
    , asm_proc_(ctx, branch_provider, lib_provider, parser, open_code, proc_mgr, output, diag_ctx)
    , mach_proc_(ctx, branch_provider, lib_provider, parser, proc_mgr, diag_ctx)
    , finished_flag_(false)
    , listener_(state_listener)
    , proc_mgr(proc_mgr)
{}


std::optional<context::id_index> ordinary_processor::resolve_concatenation(
    const semantics::concat_chain& concat, const range& r) const
{
    return resolve_instruction_concat_chain(concat, r);
}

std::optional<processing_status> ordinary_processor::get_processing_status(
    const std::optional<context::id_index>& instruction, const range&) const
{
    assert(instruction.has_value());
    const auto& id = *instruction;

    context::id_index suggestion;

    if (auto status = get_instruction_processing_status(id, hlasm_ctx, &suggestion); status.has_value())
        return *status;

    if (suggestion.empty())
        return std::make_pair(processing_format(processing_kind::ORDINARY, processing_form::UNKNOWN),
            op_code(id, context::instruction_type::UNDEF));

    auto found = branch_provider_.request_external_processing(suggestion, processing_kind::MACRO, {});
    if (!found.has_value())
        return std::nullopt;

    if (!found.value())
        return std::make_pair(processing_format(processing_kind::ORDINARY, processing_form::UNKNOWN),
            op_code(suggestion, context::instruction_type::UNDEF));

    if (const auto mp = hlasm_ctx.find_macro(suggestion))
        return std::make_pair(
            processing_format(processing_kind::ORDINARY, processing_form::MAC), op_code(suggestion, mp->get()));
    else
        return std::make_pair(processing_format(processing_kind::ORDINARY, processing_form::MAC),
            op_code(suggestion, context::instruction_type::UNDEF));
}

void ordinary_processor::process_statement(context::shared_stmt_ptr s)
{
    for (const auto& d : s->diagnostics())
        add_diagnostic(d);

    bool fatal = check_fatals(range(s->statement_position()));
    if (fatal)
        return;
    if (s->kind != context::statement_kind::RESOLVED)
        return;

    auto statement = std::static_pointer_cast<const processing::resolved_statement>(std::move(s));

    if (hlasm_ctx.get_end_reached())
    {
        if (statement->label_ref().type != semantics::label_si_type::EMPTY
            || statement->instruction_ref().type != semantics::instruction_si_type::EMPTY
            || !statement->operands_ref().value.empty())
        {
            add_diagnostic(diagnostic_op::warning_W015(range(statement->statement_position())));
            finished_flag_ = true;
        }
        return;
    }

    using enum context::instruction_type;
    switch (statement->opcode_ref().type)
    {
        case UNDEF:
            add_diagnostic(diagnostic_op::error_E049(
                statement->opcode_ref().value.to_string_view(), statement->instruction_ref().field_range));
            return;
        case CA:
            ca_proc_.process(std::move(statement));
            return;
        case MAC:
            mac_proc_.process(std::move(statement));
            return;
        case ASM:
            asm_proc_.process(std::move(statement));
            return;
        case MACH:
        case MNEMO:
            mach_proc_.process(std::move(statement));
            return;
        default:
            assert(false);
            return;
    }
}

void ordinary_processor::process_postponed_statements(
    const std::vector<std::pair<context::post_stmt_ptr, context::dependency_evaluation_context>>& stmts)
{
    proc_mgr.process_postponed_statements(stmts);
    check_postponed_statements(stmts);
}

void ordinary_processor::end_processing()
{
    if (hlasm_ctx.ord_ctx.literals().get_pending_count())
    {
        hlasm_ctx.ord_ctx.set_location_counter(hlasm_ctx.ord_ctx.implicit_ltorg_target());
        hlasm_ctx.ord_ctx.set_available_location_counter_value(lib_info);

        hlasm_ctx.ord_ctx.generate_pool(diag_ctx, hlasm_ctx.using_current(), lib_info);
    }

    hlasm_ctx.ord_ctx.start_reporting_label_candidates();

    if (!hlasm_ctx.ord_ctx.symbol_dependencies().check_loctr_cycle(lib_info))
        add_diagnostic(diagnostic_op::error_E033(range())); // TODO: at least we say something

    hlasm_ctx.ord_ctx.symbol_dependencies().add_defined(context::id_index(), &diag_ctx, lib_info);

    hlasm_ctx.ord_ctx.finish_module_layout(&diag_ctx, lib_info);

    hlasm_ctx.ord_ctx.symbol_dependencies().resolve_all_as_default();

    // do not replace stack trace in the messages - it is already provided
    diagnostic_consumer_transform raw_diags([this](diagnostic d) { diag_ctx.add_raw_diagnostic(std::move(d)); });

    hlasm_ctx.using_resolve(raw_diags, lib_info);
    hlasm_ctx.validate_psect_registrations(raw_diags);

    process_postponed_statements(hlasm_ctx.ord_ctx.symbol_dependencies().collect_postponed());

    hlasm_ctx.pop_statement_processing();

    listener_.finish_opencode();

    finished_flag_ = true;
}

bool ordinary_processor::terminal_condition(const statement_provider_kind prov_kind) const
{
    return prov_kind == statement_provider_kind::OPEN;
}

bool ordinary_processor::finished() { return finished_flag_; }

struct processing_status_visitor
{
    context::id_index id;

    std::optional<processing_status> return_value(processing_form f, bool no_ops, auto arg) const noexcept
    {
        const auto o = no_ops ? operand_occurrence::ABSENT : operand_occurrence::PRESENT;
        return std::optional<processing_status> {
            std::in_place,
            processing_format(processing_kind::ORDINARY, f, o),
            op_code(id, arg),
        };
    }

    std::optional<processing_status> operator()(const instructions::assembler_instruction* i) const noexcept
    {
        using enum processing_form;
        static constexpr std::pair<context::id_index, processing_form> forms[] = {
            { context::id_index("DC"), DAT },
            { context::id_index("DS"), DAT },
            { context::id_index("USING"), ASM_USING },
            { context::id_index("ALIAS"), ASM_ALIAS },
            { context::id_index("DXD"), DAT },
            { context::id_index("END"), ASM_END },
            { context::id_index(), ASM_GENERIC },
        };
        const auto it = std::ranges::find(forms, forms + std::size(forms) - 1, id, utils::first_element);
        return return_value(it->second, i->max_operands() == 0, i);
    }
    std::optional<processing_status> operator()(const instructions::machine_instruction* i) const noexcept
    {
        return return_value(processing_form::MACH, i->operands().empty(), i);
    }
    std::optional<processing_status> operator()(const instructions::ca_instruction* i) const noexcept
    {
        return return_value(processing_form::CA, i->operandless(), context::instruction_type::CA);
    }
    std::optional<processing_status> operator()(const instructions::mnemonic_code* i) const noexcept
    {
        return return_value(processing_form::MACH, i->operand_count().second == 0, i);
    }
    std::optional<processing_status> operator()(context::macro_definition* mac) const noexcept
    {
        return return_value(processing_form::MAC, false, mac);
    }
    std::optional<processing_status> operator()(std::monostate) const noexcept { return std::nullopt; }
};

std::optional<processing_status> ordinary_processor::get_instruction_processing_status(
    context::id_index instruction, context::hlasm_context& hlasm_ctx, context::id_index* ext_suggestion)
{
    if (instruction.empty())
        return std::make_pair(
            processing_format(processing_kind::ORDINARY, processing_form::CA, operand_occurrence::ABSENT),
            op_code(context::id_index(), context::instruction_type::CA));

    const auto code = hlasm_ctx.get_operation_code(instruction, ext_suggestion);

    return std::visit(processing_status_visitor { code.opcode }, code.opcode_detail);
}

checking::check_op_ptr get_check_op(const semantics::operand* op,
    context::dependency_solver& dep_solver,
    const diagnostic_collector& add_diagnostic,
    const instructions::assembler_instruction& instr)
{
    diagnostic_consumer_transform diags([&add_diagnostic](diagnostic_op d) { add_diagnostic(std::move(d)); });

    const auto& ev_op = dynamic_cast<const semantics::evaluable_operand&>(*op);

    const bool can_have_ord_syms = instr.has_ord_symbols();

    std::vector<context::id_index> missing_symbols;
    if (can_have_ord_syms && !instr.postpone_dependencies() && ev_op.has_dependencies(dep_solver, &missing_symbols))
    {
        for (const auto& symbol : missing_symbols)
            add_diagnostic(diagnostic_op::error_E010("ordinary symbol", symbol.to_string_view(), ev_op.operand_range));
        if (missing_symbols.empty()) // this is a fallback message if somehow non-symbolic deps are not resolved
            add_diagnostic(diagnostic_op::error_E016(ev_op.operand_range));
        return nullptr;
    }

    checking::check_op_ptr uniq;

    checking::using_label_checker lc(dep_solver, diags);
    ev_op.apply_mach_visitor(lc);

    if (auto expr_op = dynamic_cast<const semantics::expr_assembler_operand*>(&ev_op))
    {
        uniq = expr_op->get_operand_value(dep_solver, can_have_ord_syms, diags);
    }
    else
    {
        uniq = ev_op.get_operand_value(dep_solver, diags);
    }

    return uniq;
}

bool transform_asm(std::vector<checking::check_op_ptr>& result,
    const semantics::operand_list& ops,
    const instructions::assembler_instruction& instr,
    context::dependency_solver& dep_solver,
    const diagnostic_collector& add_diagnostic)
{
    for (const auto& op : ops)
    {
        // check whether operand isn't empty
        if (op->type == semantics::operand_type::EMPTY)
        {
            result.push_back(std::make_unique<checking::empty_operand>(op->operand_range));
            continue;
        }

        auto uniq = get_check_op(op.get(), dep_solver, add_diagnostic, instr);

        if (!uniq)
            return false; // contains dependencies

        uniq->operand_range = op.get()->operand_range;
        result.push_back(std::move(uniq));
    }
    return true;
}

void ordinary_processor::check_postponed_statements(
    const std::vector<std::pair<context::post_stmt_ptr, context::dependency_evaluation_context>>& stmts)
{
    std::vector<const checking::asm_operand*> operand_asm_vector;
    std::vector<const checking::machine_operand*> operand_mach_vector;
    std::vector<checking::check_op_ptr> operand_vector;

    for (const auto& [stmt, dep_ctx] : stmts)
    {
        if (!stmt)
            continue;

        context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, dep_ctx, lib_info);

        const auto* rs = stmt->resolved_stmt;
        diagnostic_collector collector(&diag_ctx, stmt->location_stack);

        operand_vector.clear();

        const auto& opcode = rs->opcode_ref();
        const auto instruction_name = opcode.value.to_string_view();

        using enum context::instruction_type;
        switch (opcode.type)
        {
            case MACH:
                checking::check_machine_instruction_operands(*opcode.instr_mach,
                    instruction_name,
                    rs->operands_ref().value,
                    rs->stmt_range_ref(),
                    dep_solver,
                    collector);
                break;

            case MNEMO:
                checking::check_mnemonic_code_operands(*opcode.instr_mnemo,
                    instruction_name,
                    rs->operands_ref().value,
                    rs->stmt_range_ref(),
                    dep_solver,
                    collector);
                break;

            case ASM:
                if (!transform_asm(operand_vector, rs->operands_ref().value, *opcode.instr_asm, dep_solver, collector))
                    continue;
                operand_asm_vector.clear();
                for (const auto& op : operand_vector)
                    operand_asm_vector.push_back(dynamic_cast<const checking::asm_operand*>(op.get()));
                checking::check_asm_ops(instruction_name, operand_asm_vector, rs->stmt_range_ref(), collector);
                break;

            default:
                assert(false);
                break;
        }
    }
}

bool ordinary_processor::check_fatals(range line_range)
{
    if (!hlasm_ctx.next_statement())
    {
        add_diagnostic(diagnostic_op::error_E077(line_range));
        finished_flag_ = true;
        return true;
    }

    if (hlasm_ctx.scope_stack().size() > NEST_LIMIT)
    {
        while (hlasm_ctx.is_in_macro())
            hlasm_ctx.leave_macro();

        add_diagnostic(diagnostic_op::error_E055(line_range));
        return true;
    }

    if (hlasm_ctx.get_branch_counter() < 0)
    {
        add_diagnostic(diagnostic_op::error_E056(line_range));
        if (hlasm_ctx.is_in_macro())
            hlasm_ctx.leave_macro();
        else
            finished_flag_ = true;

        return true;
    }

    return false;
}

context::id_index ordinary_processor::resolve_instruction_concat_chain(
    const semantics::concat_chain& chain, range instruction_range) const
{
    auto tmp = semantics::concatenation_point::evaluate(chain, eval_ctx);

    // trimright
    while (tmp.size() && tmp.back() == ' ')
        tmp.pop_back();

    // trimleft
    size_t i;
    for (i = 0; i < tmp.size() && tmp[i] == ' '; i++)
        ;
    tmp.erase(0U, i);

    static constexpr auto valid_values =
        utils::create_truth_table(" $_#@abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

    if (tmp.empty())
    {
        add_diagnostic(diagnostic_op::error_E074(instruction_range));
        return context::id_index();
    }
    else if (auto pos = utils::find_mismatch(tmp, valid_values); pos != std::string_view::npos)
    {
        add_diagnostic(diagnostic_op::error_E075(tmp, instruction_range));
        return context::id_index();
    }
    else if (tmp.find(' ') != std::string::npos)
    {
        add_diagnostic(diagnostic_op::error_E067(instruction_range));
        return context::id_index();
    }

    return hlasm_ctx.add_id(std::move(tmp));
}

} // namespace hlasm_plugin::parser_library::processing
