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

#include "low_language_processor.h"

#include <optional>

#include "processing/context_manager.h"
#include "processing/statement_processors/ordinary_processor.h"

using namespace hlasm_plugin::parser_library;
using namespace processing;
using namespace workspaces;

low_language_processor::low_language_processor(context::hlasm_context& hlasm_ctx,
    attribute_provider& attr_provider,
    branching_provider& branch_provider,
    parse_lib_provider& lib_provider,
    statement_fields_parser& parser)
    : instruction_processor(hlasm_ctx, attr_provider, branch_provider, lib_provider)
    , parser(parser)
{}

rebuilt_statement low_language_processor::preprocess(context::unique_stmt_ptr statement)
{
    auto& stmt = dynamic_cast<resolved_statement_impl&>(*statement->access_resolved());
    auto [label, ops] = preprocess_inner(stmt);
    return rebuilt_statement(std::move(stmt), std::move(label), std::move(ops));
}

rebuilt_statement low_language_processor::preprocess(context::shared_stmt_ptr statement)
{
    const auto& stmt = dynamic_cast<const resolved_statement_impl&>(*statement->access_resolved());
    auto [label, ops] = preprocess_inner(stmt);
    return rebuilt_statement(stmt, std::move(label), std::move(ops));
}

context::id_index low_language_processor::find_label_symbol(const rebuilt_statement& stmt) const
{
    if (stmt.label_ref().type == semantics::label_si_type::ORD)
    {
        context_manager mngr(hlasm_ctx);
        auto ret = mngr.get_symbol_name(std::get<std::string>(stmt.label_ref().value), stmt.label_ref().field_range);
        collect_diags_from_child(mngr);
        return ret;
    }
    else
        return context::id_storage::empty_id;
}

void low_language_processor::create_symbol(
    range err_range, context::id_index symbol_name, context::symbol_value value, context::symbol_attributes attributes)
{
    auto sym_loc = hlasm_ctx.processing_stack().back().proc_location;
    sym_loc.pos.column = 0;
    hlasm_ctx.ord_ctx.create_symbol(symbol_name, std::move(value), std::move(attributes), std::move(sym_loc));
}

low_language_processor::preprocessed_part low_language_processor::preprocess_inner(const resolved_statement_impl& stmt)
{
    context_manager mngr(hlasm_ctx);

    std::optional<semantics::label_si> label;
    std::optional<semantics::operands_si> operands;

    std::string new_label;
    // label
    switch (stmt.label_ref().type)
    {
        case semantics::label_si_type::CONC:
            label.emplace(stmt.label_ref().field_range,
                semantics::concatenation_point::evaluate(
                    std::get<semantics::concat_chain>(stmt.label_ref().value), eval_ctx));
            break;
        case semantics::label_si_type::VAR:
            new_label = semantics::var_sym_conc::evaluate(
                std::get<semantics::vs_ptr>(stmt.label_ref().value)->evaluate(eval_ctx));
            if (new_label.empty() || new_label[0] == ' ')
                label.emplace(stmt.label_ref().field_range);
            else
                label.emplace(stmt.label_ref().field_range, std::move(new_label));
            break;
        case semantics::label_si_type::MAC:
            add_diagnostic(diagnostic_op::error_E057(stmt.label_ref().field_range));
            break;
        case semantics::label_si_type::SEQ:
            branch_provider.register_sequence_symbol(std::get<semantics::seq_sym>(stmt.label_ref().value).name,
                std::get<semantics::seq_sym>(stmt.label_ref().value).symbol_range);
            break;
        default:
            break;
    }

    // operands
    if (!stmt.operands_ref().value.empty() && stmt.operands_ref().value[0]->type == semantics::operand_type::MODEL)
    {
        assert(stmt.operands_ref().value.size() == 1);
        std::string field(
            semantics::concatenation_point::evaluate(stmt.operands_ref().value[0]->access_model()->chain, eval_ctx));
        operands.emplace(parser
                             .parse_operand_field(&hlasm_ctx,
                                 std::move(field),
                                 true,
                                 semantics::range_provider(stmt.operands_ref().value[0]->operand_range,
                                     semantics::adjusting_state::SUBSTITUTION),
                                 processing_status(stmt.format, stmt.opcode))
                             .first);
    }

    for (auto& op : (operands ? operands->value : stmt.operands_ref().value))
    {
        if (auto simple_tmp = dynamic_cast<semantics::simple_expr_operand*>(op.get()))
            simple_tmp->expression->fill_location_counter(hlasm_ctx.ord_ctx.align(context::no_align));
        if (auto addr_tmp = dynamic_cast<semantics::address_machine_operand*>(op.get()))
        {
            if (addr_tmp->displacement)
                addr_tmp->displacement->fill_location_counter(hlasm_ctx.ord_ctx.align(context::no_align));
            if (addr_tmp->first_par)
                addr_tmp->first_par->fill_location_counter(hlasm_ctx.ord_ctx.align(context::no_align));
            if (addr_tmp->second_par)
                addr_tmp->second_par->fill_location_counter(hlasm_ctx.ord_ctx.align(context::no_align));
        }
    }

    collect_diags_from_child(mngr);

    return std::make_pair(std::move(label), std::move(operands));
}

bool low_language_processor::check_address_for_ORG(range err_range,
    const context::address& addr_to_check,
    const context::address& curr_addr,
    size_t boundary,
    int offset)
{
    int al = boundary ? (int)((boundary - (addr_to_check.offset % boundary)) % boundary) : 0;

    bool underflow = !addr_to_check.has_dependant_space() && addr_to_check.offset + al + offset < 0;
    if (!curr_addr.in_same_loctr(addr_to_check) || underflow)
    {
        add_diagnostic(diagnostic_op::error_E068(err_range));
        return false;
    }
    if (!addr_to_check.is_simple())
    {
        add_diagnostic(diagnostic_op::error_A115_ORG_op_format(err_range));
        return false;
    }
    return true;
}

void low_language_processor::check_loctr_dependencies(range err_range)
{
    if (hlasm_ctx.ord_ctx.symbol_dependencies.loctr_dependencies.empty())
        return;

    bool ok = true;
    auto tmp_sect = hlasm_ctx.ord_ctx.current_section();

    for (auto&& [sp, dep] : hlasm_ctx.ord_ctx.symbol_dependencies.loctr_dependencies)
    {
        hlasm_ctx.ord_ctx.set_section(sp->owner.owner.name, sp->owner.owner.kind, location());
        hlasm_ctx.ord_ctx.current_section()->current_location_counter().switch_to_unresolved_value(sp);

        if (!check_address_for_ORG(
                err_range, dep, hlasm_ctx.ord_ctx.align(context::no_align), sp->previous_boundary, sp->previous_offset))
        {
            (void)hlasm_ctx.ord_ctx.current_section()->current_location_counter().restore_from_unresolved_value(sp);
            continue;
        }

        auto new_sp = hlasm_ctx.ord_ctx.set_location_counter_value_space(
            dep, sp->previous_boundary, sp->previous_offset, nullptr, nullptr);
        auto ret = hlasm_ctx.ord_ctx.current_section()->current_location_counter().restore_from_unresolved_value(sp);
        context::space::resolve(sp, std::move(ret));
        ok &= hlasm_ctx.ord_ctx.symbol_dependencies.check_cycle(new_sp);
    }
    hlasm_ctx.ord_ctx.set_section(tmp_sect->name, tmp_sect->kind, location());
    hlasm_ctx.ord_ctx.symbol_dependencies.loctr_dependencies.clear();
    hlasm_ctx.ord_ctx.symbol_dependencies.add_defined();
    if (!ok)
        add_diagnostic(diagnostic_op::error_E033(err_range));

    ok = true;
    for (auto& sect : hlasm_ctx.ord_ctx.sections())
        for (auto& loctr : sect->location_counters())
            ok &= loctr->check_underflow();

    if (!ok)
        add_diagnostic(diagnostic_op::error_E068(err_range));
}


low_language_processor::transform_result low_language_processor::transform_mnemonic(
    const resolved_statement& stmt, context::hlasm_context& hlasm_ctx, diagnostic_collector add_diagnostic)
{
    // operands obtained from the user
    const auto& operands = stmt.operands_ref().value;
    // the name of the instruction (mnemonic) obtained from the user
    auto instr_name = *stmt.opcode_ref().value;
    // the associated mnemonic structure with the given name
    auto mnemonic = context::instruction::mnemonic_codes.at(instr_name);
    // the machine instruction structure associated with the given instruction name
    auto curr_instr = &context::instruction::machine_instructions.at(mnemonic.instruction);

    // check whether substituted mnemonic values are ok

    // check size of mnemonic operands
    int diff = (int)curr_instr->get()->operands.size() - (int)operands.size() - (int)mnemonic.replaced.size();
    if (std::abs(diff) > curr_instr->get()->no_optional)
    {
        auto curr_diag = diagnostic_op::error_optional_number_of_operands(instr_name,
            curr_instr->get()->no_optional,
            (int)curr_instr->get()->operands.size() - (int)mnemonic.replaced.size(),
            stmt.stmt_range_ref());

        add_diagnostic(curr_diag);
        return std::nullopt;
    }

    std::vector<checking::check_op_ptr> substituted_mnems;
    for (auto mnem : mnemonic.replaced)
        substituted_mnems.push_back(std::make_unique<checking::one_operand>((int)mnem.second));

    std::vector<checking::check_op_ptr> operand_vector;
    // create vector of empty operands
    for (size_t i = 0; i < curr_instr->get()->operands.size() + curr_instr->get()->no_optional; i++)
        operand_vector.push_back(nullptr);
    // add substituted
    for (size_t i = 0; i < mnemonic.replaced.size(); i++)
        operand_vector[mnemonic.replaced[i].first] = std::move(substituted_mnems[i]);
    // add other
    size_t real_op_idx = 0;
    for (size_t j = 0; j < operand_vector.size() && real_op_idx < operands.size(); j++)
    {
        if (operand_vector[j] == nullptr)
        {
            auto& operand = operands[real_op_idx++];
            if (operand->type == semantics::operand_type::EMPTY) // if operand is empty
            {
                operand_vector[j] = std::make_unique<checking::empty_operand>();
                operand_vector[j]->operand_range = operand->operand_range;
            }
            else // if operand is not empty
            {
                auto uniq = get_check_op(operand.get(), hlasm_ctx, add_diagnostic, stmt, j, &mnemonic.instruction);
                if (!uniq)
                    return std::nullopt; // contains dependencies

                uniq->operand_range = operand.get()->operand_range;
                operand_vector[j] = std::move(uniq);
            }
        }
    }
    return operand_vector;
}

low_language_processor::transform_result low_language_processor::transform_default(
    const resolved_statement& stmt, context::hlasm_context& hlasm_ctx, diagnostic_collector add_diagnostic)
{
    std::vector<checking::check_op_ptr> operand_vector;
    for (auto& op : stmt.operands_ref().value)
    {
        // check whether operand isn't empty
        if (op->type == semantics::operand_type::EMPTY)
        {
            operand_vector.push_back(std::make_unique<checking::empty_operand>());
            operand_vector.back()->operand_range = op->operand_range;
            continue;
        }

        auto uniq = get_check_op(op.get(), hlasm_ctx, add_diagnostic, stmt, operand_vector.size());

        if (!uniq)
            return std::nullopt; // contains dependencies

        uniq->operand_range = op.get()->operand_range;
        operand_vector.push_back(std::move(uniq));
    }
    return operand_vector;
}

checking::check_op_ptr low_language_processor::get_check_op(const semantics::operand* op,
    context::hlasm_context& hlasm_ctx,
    diagnostic_collector add_diagnostic,
    const resolved_statement& stmt,
    size_t op_position,
    const std::string* mnemonic)
{
    auto ev_op = dynamic_cast<const semantics::evaluable_operand*>(op);
    assert(ev_op);

    auto tmp = context::instruction::assembler_instructions.find(*stmt.opcode_ref().value);
    bool can_have_ord_syms =
        tmp != context::instruction::assembler_instructions.end() ? tmp->second.has_ord_symbols : true;

    if (can_have_ord_syms && ev_op->has_dependencies(hlasm_ctx.ord_ctx))
    {
        add_diagnostic(diagnostic_op::error_E010("ordinary symbol", ev_op->operand_range));
        return nullptr;
    }

    checking::check_op_ptr uniq;

    if (auto mach_op = dynamic_cast<const semantics::machine_operand*>(ev_op))
    {
        if (context::instruction::machine_instructions.at(mnemonic ? *mnemonic : *stmt.opcode_ref().value)
                ->operands.size()
            > op_position)
        {
            auto type = context::instruction::machine_instructions.at(mnemonic ? *mnemonic : *stmt.opcode_ref().value)
                            ->operands[op_position]
                            .identifier.type;
            uniq = mach_op->get_operand_value(hlasm_ctx.ord_ctx, type);
        }
        else
            uniq = ev_op->get_operand_value(hlasm_ctx.ord_ctx);
    }
    else
    {
        uniq = ev_op->get_operand_value(hlasm_ctx.ord_ctx);
    }

    ev_op->collect_diags();
    for (auto& diag : ev_op->diags())
        add_diagnostic(std::move(diag));
    ev_op->diags().clear();

    return uniq;
}

void low_language_processor::check(const resolved_statement& stmt,
    context::hlasm_context& hlasm_ctx,
    checking::instruction_checker& checker,
    const diagnosable_ctx& diagnoser)
{
    auto postponed_stmt = dynamic_cast<const context::postponed_statement*>(&stmt);
    diagnostic_collector collector(
        &diagnoser, postponed_stmt ? postponed_stmt->location_stack() : hlasm_ctx.processing_stack());

    std::vector<const checking::operand*> operand_ptr_vector;
    transform_result operand_vector;

    auto mnem_tmp = context::instruction::mnemonic_codes.find(*stmt.opcode_ref().value);
    const std::string* instruction_name;

    if (mnem_tmp != context::instruction::mnemonic_codes.end())
    {
        operand_vector = transform_mnemonic(stmt, hlasm_ctx, collector);
        // save the actual mnemonic name
        instruction_name = &mnem_tmp->first;
    }
    else
    {
        operand_vector = transform_default(stmt, hlasm_ctx, collector);
        instruction_name = stmt.opcode_ref().value;
    }

    if (!operand_vector)
        return;

    for (const auto& op : *operand_vector)
        operand_ptr_vector.push_back(op.get());

    checker.check(*instruction_name, operand_ptr_vector, stmt.stmt_range_ref(), collector);
}
