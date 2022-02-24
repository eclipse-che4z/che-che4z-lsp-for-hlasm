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

low_language_processor::low_language_processor(analyzing_context ctx,
    branching_provider& branch_provider,
    parse_lib_provider& lib_provider,
    statement_fields_parser& parser)
    : instruction_processor(std::move(ctx), branch_provider, lib_provider)
    , parser(parser)
{}

rebuilt_statement low_language_processor::preprocess(std::shared_ptr<const processing::resolved_statement> statement)
{
    auto stmt = std::static_pointer_cast<const resolved_statement>(statement);
    auto [label, ops] = preprocess_inner(*stmt);
    return rebuilt_statement(std::move(stmt), std::move(label), std::move(ops));
}

context::id_index low_language_processor::find_label_symbol(const rebuilt_statement& stmt) const
{
    if (const auto& label = stmt.label_ref(); label.type == semantics::label_si_type::ORD)
    {
        context_manager mngr(hlasm_ctx);
        auto ret = mngr.get_symbol_name(*std::get<semantics::ord_symbol_string>(label.value).symbol, label.field_range);
        collect_diags_from_child(mngr);
        return ret;
    }
    else
        return context::id_storage::empty_id;
}

context::id_index low_language_processor::find_using_label(const rebuilt_statement& stmt) const
{
    if (const auto& label = stmt.label_ref(); label.type == semantics::label_si_type::ORD)
    {
        if (auto [valid, id] =
                hlasm_ctx.try_get_symbol_name(*std::get<semantics::ord_symbol_string>(label.value).symbol);
            valid)
            return id;

        add_diagnostic(diagnostic_op::error_E065(label.field_range));
    }
    return nullptr;
}

bool low_language_processor::create_symbol(
    range err_range, context::id_index symbol_name, context::symbol_value value, context::symbol_attributes attributes)
{
    auto sym_loc = hlasm_ctx.processing_stack().back().proc_location;
    sym_loc.pos.column = 0;
    bool ok = hlasm_ctx.ord_ctx.create_symbol(symbol_name, std::move(value), std::move(attributes), std::move(sym_loc));

    if (!ok)
        add_diagnostic(diagnostic_op::error_E033(err_range));

    return ok;
}

// return true if the result is not empty
bool trim_right(std::string& s)
{
    auto last_non_space = s.find_last_not_of(' ');
    if (last_non_space != std::string::npos)
    {
        s.erase(last_non_space + 1);
        return true;
    }
    else
    {
        s.clear();
        return false;
    }
}

low_language_processor::preprocessed_part low_language_processor::preprocess_inner(const resolved_statement& stmt)
{
    using namespace semantics;
    std::optional<label_si> label;
    std::optional<operands_si> operands;

    std::string new_label;
    // label
    switch (const auto& label_ref = stmt.label_ref(); label_ref.type)
    {
        case label_si_type::CONC:
            new_label = concatenation_point::evaluate(std::get<concat_chain>(label_ref.value), eval_ctx);
            if (!trim_right(new_label))
                label.emplace(label_ref.field_range);
            else
            {
                auto ord_id = hlasm_ctx.ids().add(new_label);
                label.emplace(label_ref.field_range, ord_symbol_string { ord_id, std::move(new_label) });
            }
            break;
        case label_si_type::VAR:
            new_label = var_sym_conc::evaluate(std::get<vs_ptr>(label_ref.value)->evaluate(eval_ctx));
            if (!trim_right(new_label))
                label.emplace(label_ref.field_range);
            else
            {
                auto ord_id = hlasm_ctx.ids().add(new_label);
                label.emplace(label_ref.field_range, ord_symbol_string { ord_id, std::move(new_label) });
            }
            break;
        case label_si_type::MAC:
            add_diagnostic(diagnostic_op::error_E057(label_ref.field_range));
            break;
        case label_si_type::SEQ:
            branch_provider.register_sequence_symbol(
                std::get<seq_sym>(label_ref.value).name, std::get<seq_sym>(label_ref.value).symbol_range);
            break;
        default:
            break;
    }

    // operands
    if (const auto& operands_ref = stmt.operands_ref();
        !operands_ref.value.empty() && operands_ref.value[0]->type == operand_type::MODEL)
    {
        assert(operands_ref.value.size() == 1);
        std::string field(concatenation_point::evaluate(operands_ref.value[0]->access_model()->chain, eval_ctx));
        operands.emplace(parser
                             .parse_operand_field(std::move(field),
                                 true,
                                 range_provider(operands_ref.value[0]->operand_range, adjusting_state::SUBSTITUTION),
                                 processing_status(stmt.format_ref(), stmt.opcode_ref()),
                                 *this)
                             .first);
    }

    return std::make_pair(std::move(label), std::move(operands));
}

bool low_language_processor::check_address_for_ORG(range err_range,
    const context::address& addr_to_check,
    const context::address& curr_addr,
    size_t boundary,
    int offset)
{
    int al = boundary ? (int)((boundary - (addr_to_check.offset() % boundary)) % boundary) : 0;

    bool underflow = !addr_to_check.has_dependant_space() && addr_to_check.offset() + al + offset < 0;
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

void low_language_processor::resolve_unknown_loctr_dependency(context::space_ptr sp,
    const context::address& addr,
    range err_range,
    const context::dependency_evaluation_context& dep_ctx)
{
    auto tmp_loctr = hlasm_ctx.ord_ctx.current_section()->current_location_counter();

    hlasm_ctx.ord_ctx.set_location_counter(sp->owner.name, location());
    hlasm_ctx.ord_ctx.current_section()->current_location_counter().switch_to_unresolved_value(sp);

    if (!check_address_for_ORG(err_range,
            addr,
            hlasm_ctx.ord_ctx.align(context::no_align, dep_ctx),
            sp->previous_boundary,
            sp->previous_offset))
    {
        (void)hlasm_ctx.ord_ctx.current_section()->current_location_counter().restore_from_unresolved_value(sp);
        hlasm_ctx.ord_ctx.set_location_counter(tmp_loctr.name, location());
        return;
    }

    auto new_sp = hlasm_ctx.ord_ctx.set_location_counter_value_space(
        addr, sp->previous_boundary, sp->previous_offset, nullptr, nullptr, dep_ctx);

    auto ret = hlasm_ctx.ord_ctx.current_section()->current_location_counter().restore_from_unresolved_value(sp);
    hlasm_ctx.ord_ctx.set_location_counter(tmp_loctr.name, location());

    context::space::resolve(sp, std::move(ret));

    if (!hlasm_ctx.ord_ctx.symbol_dependencies.check_cycle(new_sp))
        add_diagnostic(diagnostic_op::error_E033(err_range));

    for (auto& sect : hlasm_ctx.ord_ctx.sections())
        for (auto& loctr : sect->location_counters())
            if (!loctr->check_underflow())
            {
                add_diagnostic(diagnostic_op::error_E068(err_range));
                return;
            }
}


low_language_processor::transform_result low_language_processor::transform_mnemonic(
    const resolved_statement& stmt, context::dependency_solver& dep_solver, diagnostic_collector add_diagnostic)
{
    // operands obtained from the user
    const auto& operands = stmt.operands_ref().value;
    // the name of the instruction (mnemonic) obtained from the user
    auto instr_name = *stmt.opcode_ref().value;
    // the associated mnemonic structure with the given name
    auto mnemonic = context::instruction::get_mnemonic_codes(instr_name);
    // the machine instruction structure associated with the given instruction name
    auto curr_instr = mnemonic.instruction();

    auto replaced = mnemonic.replaced_operands();

    // check whether substituted mnemonic values are ok

    // check size of mnemonic operands
    int diff = (int)curr_instr->operands().size() - (int)operands.size() - (int)replaced.size();
    if (std::abs(diff) > curr_instr->optional_operand_count())
    {
        auto curr_diag = diagnostic_op::error_optional_number_of_operands(instr_name,
            curr_instr->optional_operand_count(),
            (int)curr_instr->operands().size() - (int)replaced.size(),
            stmt.stmt_range_ref());

        add_diagnostic(curr_diag);
        return std::nullopt;
    }

    std::vector<checking::check_op_ptr> substituted_mnems;
    for (auto mnem : replaced)
        substituted_mnems.push_back(std::make_unique<checking::one_operand>((int)mnem.second));

    std::vector<checking::check_op_ptr> operand_vector;
    // create vector of empty operands
    for (size_t i = 0; i < curr_instr->operands().size() + curr_instr->optional_operand_count(); i++)
        operand_vector.push_back(nullptr);
    // add substituted
    for (size_t i = 0; i < replaced.size(); i++)
        operand_vector[replaced[i].first] = std::move(substituted_mnems[i]);
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
                auto uniq = get_check_op(operand.get(), dep_solver, add_diagnostic, stmt, j, &mnemonic);
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
    const resolved_statement& stmt, context::dependency_solver& dep_solver, diagnostic_collector add_diagnostic)
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

        auto uniq = get_check_op(op.get(), dep_solver, add_diagnostic, stmt, operand_vector.size());

        if (!uniq)
            return std::nullopt; // contains dependencies

        uniq->operand_range = op.get()->operand_range;
        operand_vector.push_back(std::move(uniq));
    }
    return operand_vector;
}

checking::check_op_ptr low_language_processor::get_check_op(const semantics::operand* op,
    context::dependency_solver& dep_solver,
    diagnostic_collector add_diagnostic,
    const resolved_statement& stmt,
    size_t op_position,
    const context::mnemonic_code* mnemonic)
{
    const auto& ev_op = dynamic_cast<const semantics::evaluable_operand&>(*op);

    auto tmp = context::instruction::find_assembler_instructions(*stmt.opcode_ref().value);
    const bool can_have_ord_syms = tmp ? tmp->has_ord_symbols() : true;
    const bool postpone_dependencies = tmp ? tmp->postpone_dependencies() : false;

    if (can_have_ord_syms && !postpone_dependencies && ev_op.has_dependencies(dep_solver))
    {
        add_diagnostic(diagnostic_op::error_E010("ordinary symbol", ev_op.operand_range));
        return nullptr;
    }

    checking::check_op_ptr uniq;

    if (auto mach_op = dynamic_cast<const semantics::machine_operand*>(&ev_op))
    {
        const auto* instr = mnemonic ? mnemonic->instruction()
                                     : &context::instruction::get_machine_instructions(*stmt.opcode_ref().value);
        if (op_position < instr->operands().size())
        {
            auto type = instr->operands()[op_position].identifier.type;
            uniq = mach_op->get_operand_value(dep_solver, type);
        }
        else
            uniq = ev_op.get_operand_value(dep_solver);
    }
    else if (auto expr_op = dynamic_cast<const semantics::expr_assembler_operand*>(&ev_op))
    {
        uniq = expr_op->get_operand_value(dep_solver, can_have_ord_syms);
    }
    else
    {
        uniq = ev_op.get_operand_value(dep_solver);
    }

    ev_op.collect_diags();
    for (auto& diag : ev_op.diags())
        add_diagnostic(std::move(diag));
    ev_op.diags().clear();

    return uniq;
}

bool low_language_processor::check(const resolved_statement& stmt,
    const context::processing_stack_t& processing_stack,
    context::dependency_solver& dep_solver,
    const checking::instruction_checker& checker,
    const diagnosable_ctx& diagnoser)
{
    diagnostic_collector collector(&diagnoser, processing_stack);

    std::vector<const checking::operand*> operand_ptr_vector;
    transform_result operand_vector;

    std::string_view instruction_name = *stmt.opcode_ref().value;

    if (auto mnem_tmp = context::instruction::find_mnemonic_codes(instruction_name))
    {
        operand_vector = transform_mnemonic(stmt, dep_solver, collector);
    }
    else
    {
        operand_vector = transform_default(stmt, dep_solver, collector);
    }

    if (!operand_vector)
        return false;

    for (const auto& op : *operand_vector)
        operand_ptr_vector.push_back(op.get());

    return checker.check(instruction_name, operand_ptr_vector, stmt.stmt_range_ref(), collector);
}
