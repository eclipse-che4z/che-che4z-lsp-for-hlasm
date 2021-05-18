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

#include "checking/instruction_checker.h"
#include "ebcdic_encoding.h"
#include "processing/instruction_sets/postponed_statement_impl.h"

namespace hlasm_plugin::parser_library::processing {

ordinary_processor::ordinary_processor(analyzing_context ctx,
    branching_provider& branch_provider,
    workspaces::parse_lib_provider& lib_provider,
    processing_state_listener& state_listener,
    statement_fields_parser& parser,
    opencode_provider& open_code)
    : statement_processor(processing_kind::ORDINARY, ctx)
    , eval_ctx { ctx, lib_provider }
    , ca_proc_(ctx, branch_provider, lib_provider, state_listener, open_code)
    , mac_proc_(ctx, branch_provider, lib_provider)
    , asm_proc_(ctx, branch_provider, lib_provider, parser, open_code)
    , mach_proc_(ctx, branch_provider, lib_provider, parser)
    , finished_flag_(false)
    , listener_(state_listener)
{}

processing_status ordinary_processor::get_processing_status(const semantics::instruction_si& instruction) const
{
    context::id_index id;
    if (instruction.type == semantics::instruction_si_type::CONC)
        id = resolve_instruction(std::get<semantics::concat_chain>(instruction.value), instruction.field_range);
    else
        id = std::get<context::id_index>(instruction.value);

    auto status = get_instruction_processing_status(id, hlasm_ctx);

    if (!status)
    {
        auto found =
            eval_ctx.lib_provider.parse_library(*id, ctx, workspaces::library_data { processing_kind::MACRO, id });
        processing_form f;
        context::instruction_type t;
        if (found)
        {
            f = processing_form::MAC;
            t = (hlasm_ctx.macros().find(id) != hlasm_ctx.macros().end()) ? context::instruction_type::MAC
                                                                          : context::instruction_type::UNDEF;
        }
        else
        {
            f = processing_form::UNKNOWN;
            t = context::instruction_type::UNDEF;
        }
        return std::make_pair(processing_format(processing_kind::ORDINARY, f), op_code(id, t));
    }
    else
        return *status;
}

void ordinary_processor::process_statement(context::shared_stmt_ptr statement)
{
    assert(statement->kind == context::statement_kind::RESOLVED);

    bool fatal = check_fatals(range(statement->statement_position()));
    if (fatal)
        return;

    switch (statement->access_resolved()->opcode_ref().type)
    {
        case context::instruction_type::UNDEF:
            add_diagnostic(diagnostic_op::error_E049(*statement->access_resolved()->opcode_ref().value,
                statement->access_resolved()->instruction_ref().field_range));
            return;
        case context::instruction_type::CA:
            ca_proc_.process(std::move(statement));
            return;
        case context::instruction_type::MAC:
            mac_proc_.process(std::move(statement));
            return;
        case context::instruction_type::ASM:
            asm_proc_.process(std::move(statement));
            return;
        case context::instruction_type::MACH:
            mach_proc_.process(std::move(statement));
            return;
        default:
            assert(false);
            return;
    }
}

void ordinary_processor::end_processing()
{
    hlasm_ctx.ord_ctx.symbol_dependencies.add_defined(&asm_proc_);

    hlasm_ctx.ord_ctx.finish_module_layout(&asm_proc_);

    hlasm_ctx.ord_ctx.symbol_dependencies.resolve_all_as_default();

    check_postponed_statements(hlasm_ctx.ord_ctx.symbol_dependencies.collect_postponed());

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
    context::hlasm_context& hlasm_ctx;

    processing_status return_value(processing_form f, operand_occurence o, context::instruction_type t) const
    {
        return std::make_pair(processing_format(processing_kind::ORDINARY, f, o), op_code(id, t));
    }

    processing_status operator()(const context::assembler_instruction* i) const
    {
        const auto f = id == hlasm_ctx.ids().add("DC") || id == hlasm_ctx.ids().add("DS") ? processing_form::DAT
                                                                                          : processing_form::ASM;
        const auto o = i->max_operands == 0 ? operand_occurence::ABSENT : operand_occurence::PRESENT;
        return return_value(f, o, context::instruction_type::ASM);
    }
    processing_status operator()(const context::machine_instruction* i) const
    {
        return return_value(processing_form::MACH,
            i->operands.empty() ? operand_occurence::ABSENT : operand_occurence::PRESENT,
            context::instruction_type::MACH);
    }
    processing_status operator()(const context::ca_instruction* i) const
    {
        return return_value(processing_form::CA,
            i->operandless ? operand_occurence::ABSENT : operand_occurence::PRESENT,
            context::instruction_type::CA);
    }
    processing_status operator()(const context::mnemonic_code* i) const
    {
        return return_value(processing_form::MACH,
            i->operand_count() == 0 ? operand_occurence::ABSENT : operand_occurence::PRESENT,
            context::instruction_type::MACH);
    }
    template<typename T>
    processing_status operator()(const T&) const
    {
        // opcode should already be found
        throw std::logic_error("processing_status_visitor: unexpected instruction type");
    }
};

std::optional<processing_status> ordinary_processor::get_instruction_processing_status(
    context::id_index instruction, context::hlasm_context& hlasm_ctx)
{
    auto code = hlasm_ctx.get_operation_code(instruction);

    if (std::holds_alternative<context::macro_def_ptr>(code.opcode_detail))
    {
        return std::make_pair(processing_format(processing_kind::ORDINARY, processing_form::MAC),
            op_code(instruction, context::instruction_type::MAC));
    }

    if (!code.opcode)
    {
        if (instruction == context::id_storage::empty_id)
            return std::make_pair(
                processing_format(processing_kind::ORDINARY, processing_form::CA, operand_occurence::ABSENT),
                op_code(context::id_storage::empty_id, context::instruction_type::CA));
        else
            return std::nullopt;
    }

    return std::visit(processing_status_visitor { code.opcode, hlasm_ctx }, code.opcode_detail);
}

void ordinary_processor::collect_diags() const
{
    collect_diags_from_child(ca_proc_);
    collect_diags_from_child(asm_proc_);
    collect_diags_from_child(mac_proc_);
    collect_diags_from_child(mach_proc_);
    collect_diags_from_child(eval_ctx);
}

void ordinary_processor::check_postponed_statements(std::vector<context::post_stmt_ptr> stmts)
{
    checking::assembler_checker asm_checker;
    checking::machine_checker mach_checker;

    for (auto& stmt : stmts)
    {
        if (!stmt)
            continue;


        assert(stmt->impl()->opcode_ref().type == context::instruction_type::ASM
            || stmt->impl()->opcode_ref().type == context::instruction_type::MACH);

        if (stmt->impl()->opcode_ref().type == context::instruction_type::ASM)
            low_language_processor::check(*stmt->impl(), hlasm_ctx, asm_checker, *this);
        else
            low_language_processor::check(*stmt->impl(), hlasm_ctx, mach_checker, *this);
    }
}

bool ordinary_processor::check_fatals(range line_range)
{
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

    if (hlasm_ctx.scope_stack().back().branch_counter_change > ACTR_LIMIT)
    {
        add_diagnostic(diagnostic_op::error_E063(line_range));
        finished_flag_ = true;
        return true;
    }

    return false;
}

context::id_index ordinary_processor::resolve_instruction(
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

    if (tmp.find(' ') != std::string::npos)
    {
        add_diagnostic(diagnostic_op::error_E067(instruction_range));
        return context::id_storage::empty_id;
    }

    return hlasm_ctx.ids().add(std::move(tmp));
}

} // namespace hlasm_plugin::parser_library::processing
