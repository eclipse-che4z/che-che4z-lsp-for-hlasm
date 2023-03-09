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
#include "context/literal_pool.h"
#include "context/ordinary_assembly/ordinary_assembly_dependency_solver.h"
#include "diagnostic_consumer.h"
#include "ebcdic_encoding.h"
#include "processing/instruction_sets/postponed_statement_impl.h"
#include "utils/truth_table.h"

namespace hlasm_plugin::parser_library::processing {

ordinary_processor::ordinary_processor(analyzing_context ctx,
    branching_provider& branch_provider,
    workspaces::parse_lib_provider& lib_provider,
    processing_state_listener& state_listener,
    statement_fields_parser& parser,
    opencode_provider& open_code,
    const processing_manager& proc_mgr)
    : statement_processor(processing_kind::ORDINARY, ctx)
    , branch_provider_(branch_provider)
    , lib_info(lib_provider)
    , eval_ctx { *ctx.hlasm_ctx, lib_info, *this }
    , ca_proc_(ctx, branch_provider, lib_provider, state_listener, open_code)
    , mac_proc_(ctx, branch_provider, lib_provider)
    , asm_proc_(ctx, branch_provider, lib_provider, parser, open_code, proc_mgr)
    , mach_proc_(ctx, branch_provider, lib_provider, parser, proc_mgr)
    , finished_flag_(false)
    , listener_(state_listener)
{}

std::optional<processing_status> ordinary_processor::get_processing_status(
    const semantics::instruction_si& instruction) const
{
    context::id_index id = instruction.type == semantics::instruction_si_type::CONC
        ? resolve_instruction(std::get<semantics::concat_chain>(instruction.value), instruction.field_range)
        : std::get<context::id_index>(instruction.value);

    if (auto status = get_instruction_processing_status(id, hlasm_ctx); status.has_value())
        return *status;

    auto found = branch_provider_.request_external_processing(id, processing_kind::MACRO, {});
    if (!found.has_value())
        return std::nullopt;

    processing_form f;
    context::instruction_type t;
    if (found.value())
    {
        f = processing_form::MAC;
        t = hlasm_ctx.find_macro(id) ? context::instruction_type::MAC : context::instruction_type::UNDEF;
    }
    else
    {
        f = processing_form::UNKNOWN;
        t = context::instruction_type::UNDEF;
    }
    return std::make_pair(processing_format(processing_kind::ORDINARY, f), op_code(id, t));
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

    switch (statement->opcode_ref().type)
    {
        case context::instruction_type::UNDEF:
            add_diagnostic(diagnostic_op::error_E049(
                statement->opcode_ref().value.to_string_view(), statement->instruction_ref().field_range));
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
    if (hlasm_ctx.ord_ctx.literals().get_pending_count())
    {
        auto ltorg = hlasm_ctx.ord_ctx.implicit_ltorg_target();
        hlasm_ctx.ord_ctx.set_location_counter(ltorg->name, {}, lib_info);
        hlasm_ctx.ord_ctx.set_available_location_counter_value(lib_info);

        hlasm_ctx.ord_ctx.generate_pool(*this, hlasm_ctx.using_current(), lib_info);
    }

    hlasm_ctx.ord_ctx.start_reporting_label_candidates();

    if (!hlasm_ctx.ord_ctx.symbol_dependencies.check_loctr_cycle(lib_info))
        add_diagnostic(diagnostic_op::error_E033(range())); // TODO: at least we say something

    hlasm_ctx.ord_ctx.symbol_dependencies.add_defined(context::id_index(), &asm_proc_, lib_info);

    hlasm_ctx.ord_ctx.finish_module_layout(&asm_proc_, lib_info);

    hlasm_ctx.ord_ctx.symbol_dependencies.resolve_all_as_default();

    // do not replace stack trace in the messages - it is already provided
    diagnostic_consumer_transform using_diags(
        [this](diagnostic_s d) { diagnosable_impl::add_diagnostic(std::move(d)); });
    hlasm_ctx.using_resolve(using_diags, lib_info);

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

    processing_status return_value(processing_form f, operand_occurrence o, context::instruction_type t) const
    {
        return std::make_pair(processing_format(processing_kind::ORDINARY, f, o), op_code(id, t));
    }

    processing_status operator()(const context::assembler_instruction* i) const
    {
        const auto f = id == context::id_index("DC") || id == context::id_index("DS") || id == context::id_index("DXD")
            ? processing_form::DAT
            : processing_form::ASM;
        const auto o = i->max_operands() == 0 ? operand_occurrence::ABSENT : operand_occurrence::PRESENT;
        return return_value(f, o, context::instruction_type::ASM);
    }
    processing_status operator()(const context::machine_instruction* i) const
    {
        return return_value(processing_form::MACH,
            i->operands().empty() ? operand_occurrence::ABSENT : operand_occurrence::PRESENT,
            context::instruction_type::MACH);
    }
    processing_status operator()(const context::ca_instruction* i) const
    {
        return return_value(processing_form::CA,
            i->operandless() ? operand_occurrence::ABSENT : operand_occurrence::PRESENT,
            context::instruction_type::CA);
    }
    processing_status operator()(const context::mnemonic_code* i) const
    {
        return return_value(processing_form::MACH,
            i->operand_count().second == 0 ? operand_occurrence::ABSENT : operand_occurrence::PRESENT,
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

    if (code.opcode.empty())
    {
        if (instruction.empty())
            return std::make_pair(
                processing_format(processing_kind::ORDINARY, processing_form::CA, operand_occurrence::ABSENT),
                op_code(context::id_index(), context::instruction_type::CA));
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
}

void ordinary_processor::check_postponed_statements(
    const std::vector<std::pair<context::post_stmt_ptr, context::dependency_evaluation_context>>& stmts)
{
    static const checking::assembler_checker asm_checker;
    static const checking::machine_checker mach_checker;

    for (const auto& [stmt, dep_ctx] : stmts)
    {
        if (!stmt)
            continue;

        context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, dep_ctx, lib_info);

        const auto* rs = stmt->resolved_stmt();
        switch (rs->opcode_ref().type)
        {
            case hlasm_plugin::parser_library::context::instruction_type::MACH:
                low_language_processor::check(*rs, stmt->location_stack(), dep_solver, mach_checker, *this);
                break;

            case hlasm_plugin::parser_library::context::instruction_type::ASM:
                low_language_processor::check(*rs, stmt->location_stack(), dep_solver, asm_checker, *this);
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

    return hlasm_ctx.ids().add(std::move(tmp));
}

} // namespace hlasm_plugin::parser_library::processing
