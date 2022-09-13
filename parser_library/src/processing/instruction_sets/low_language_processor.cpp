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

#include <algorithm>
#include <optional>
#include <type_traits>

#include "checking/using_label_checker.h"
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
    auto [label, ops, literals] = preprocess_inner(*stmt);
    return rebuilt_statement(std::move(stmt), std::move(label), std::move(ops), std::move(literals));
}

context::id_index low_language_processor::find_label_symbol(const rebuilt_statement& stmt) const
{
    if (const auto& label = stmt.label_ref(); label.type == semantics::label_si_type::ORD)
    {
        diagnostic_consumer_transform diags([this](diagnostic_op d) { add_diagnostic(std::move(d)); });
        auto [valid, id] = hlasm_ctx.try_get_symbol_name(*std::get<semantics::ord_symbol_string>(label.value).symbol);
        if (!valid)
            diags.add_diagnostic(diagnostic_op::error_E065(label.field_range));
        return id;
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
    auto sym_loc = hlasm_ctx.processing_stack_top().get_location();
    sym_loc.pos.column = 0;
    bool ok = hlasm_ctx.ord_ctx.create_symbol(
        symbol_name, std::move(value), std::move(attributes), std::move(sym_loc), lib_info);

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
    preprocessed_part result;

    std::string new_label;
    // label
    switch (const auto& label_ref = stmt.label_ref(); label_ref.type)
    {
        case label_si_type::CONC:
            new_label = concatenation_point::evaluate(std::get<concat_chain>(label_ref.value), eval_ctx);
            if (!trim_right(new_label))
                result.label.emplace(label_ref.field_range);
            else
            {
                auto ord_id = hlasm_ctx.ids().add(new_label);
                result.label.emplace(label_ref.field_range, ord_symbol_string { ord_id, std::move(new_label) });
            }
            break;
        case label_si_type::VAR:
            new_label = var_sym_conc::evaluate(std::get<vs_ptr>(label_ref.value)->evaluate(eval_ctx));
            if (!trim_right(new_label))
                result.label.emplace(label_ref.field_range);
            else
            {
                auto ord_id = hlasm_ctx.ids().add(new_label);
                result.label.emplace(label_ref.field_range, ord_symbol_string { ord_id, std::move(new_label) });
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
        auto [operands, _, literals] = parser.parse_operand_field(std::move(field),
            true,
            range_provider(operands_ref.value[0]->operand_range, adjusting_state::SUBSTITUTION),
            processing_status(stmt.format_ref(), stmt.opcode_ref()),
            *this);
        result.operands.emplace(std::move(operands));
        result.literals.emplace(std::move(literals));
    }

    return result;
}

check_org_result hlasm_plugin::parser_library::processing::check_address_for_ORG(
    const context::address& addr_to_check, const context::address& curr_addr, size_t boundary, int offset)
{
    int al = boundary ? (int)((boundary - (addr_to_check.offset() % boundary)) % boundary) : 0;

    bool underflow = !addr_to_check.has_dependant_space() && addr_to_check.offset() + al + offset < 0;
    if (!curr_addr.in_same_loctr(addr_to_check) || underflow)
        return check_org_result::underflow;

    if (!addr_to_check.is_simple())
        return check_org_result::invalid_address;

    return check_org_result::valid;
}

low_language_processor::transform_result low_language_processor::transform_mnemonic(const resolved_statement& stmt,
    context::dependency_solver& dep_solver,
    const context::mnemonic_code& mnemonic,
    const diagnostic_collector& add_diagnostic)
{
    // operands obtained from the user
    const auto& operands = stmt.operands_ref().value;
    // the name of the instruction (mnemonic) obtained from the user
    const auto& instr_name = *stmt.opcode_ref().value;
    // the machine instruction structure associated with the given instruction name
    auto curr_instr = mnemonic.instruction();

    auto transforms = mnemonic.operand_transformations();

    // check size of mnemonic operands
    if (auto [low, high] = mnemonic.operand_count(); operands.size() < low || operands.size() > high)
    {
        add_diagnostic(
            diagnostic_op::error_optional_number_of_operands(instr_name, high - low, high, stmt.stmt_range_ref()));
        return std::nullopt;
    }
    assert(operands.size() <= context::machine_instruction::max_operand_count);

    struct operand_info
    {
        int value;
        bool failed;
        range r;
    };

    std::array<checking::check_op_ptr, context::machine_instruction::max_operand_count> po;
    std::array<operand_info, context::machine_instruction::max_operand_count> provided_operand_values {};
    for (size_t op_id = 0, po_id = 0, repl_id = 0, processed = 0; const auto &operand : operands)
    {
        while (repl_id < transforms.size() && processed == transforms[repl_id].skip && transforms[repl_id].insert)
        {
            ++repl_id;
            ++op_id;
            processed = 0;
        }
        auto& t = po[po_id];
        provided_operand_values[po_id].r = operand->operand_range;
        if (operand->type == semantics::operand_type::EMPTY) // if operand is empty
        {
            t = std::make_unique<checking::empty_operand>(operand->operand_range);
            provided_operand_values[po_id].failed = true;
        }
        else // if operand is not empty
        {
            t = get_check_op(operand.get(), dep_solver, add_diagnostic, stmt, op_id, &mnemonic);
            if (!t)
                return std::nullopt; // contains dependencies
            t->operand_range = operand->operand_range;
            if (const auto* ao = dynamic_cast<const checking::one_operand*>(t.get()); ao)
                provided_operand_values[po_id].value = ao->value;
            else
                provided_operand_values[po_id].failed = true;
        }
        if (repl_id < transforms.size() && processed == transforms[repl_id].skip)
        {
            ++repl_id;
            processed = 0;
        }
        else
            ++processed;
        ++op_id;
        ++po_id;
    }
    std::span<checking::check_op_ptr> provided_operands(po.data(), operands.size());

    // create vector of empty operands
    std::vector<checking::check_op_ptr> result(curr_instr->operands().size());

    // add other
    for (size_t processed = 0; auto& op : result)
    {
        if (!transforms.empty() && transforms.front().skip == processed)
        {
            const auto transform = transforms.front();
            transforms = transforms.subspan(1);
            processed = 0;

            const auto& [expr_value, failed, r] = provided_operand_values[transform.source];
            int value = transform.value;
            switch (transform.type)
            {
                case context::mnemonic_transformation_kind::value:
                    break;
                case context::mnemonic_transformation_kind::copy:
                    value = expr_value;
                    break;
                case context::mnemonic_transformation_kind::or_with:
                    value |= expr_value;
                    break;
                case context::mnemonic_transformation_kind::add_to:
                    value += expr_value;
                    break;
                case context::mnemonic_transformation_kind::subtract_from:
                    value -= expr_value;
                    break;
                case context::mnemonic_transformation_kind::complement:
                    value = 1 + ~(unsigned)expr_value & (1u << value) - 1;
                    break;
            }
            if (!transform.has_source() && transform.insert || !failed)
                op = std::make_unique<checking::one_operand>(value, r);
            else
                op = std::make_unique<checking::empty_operand>(r);

            if (!transform.insert && !provided_operands.empty())
                provided_operands = provided_operands.subspan(1); // consume updated operand
            continue;
        }
        if (provided_operands.empty())
            break;

        op = std::move(provided_operands.front());
        provided_operands = provided_operands.subspan(1);
        ++processed;
    }
    std::erase_if(result, [](const auto& x) { return !x; });
    return result;
}

low_language_processor::transform_result low_language_processor::transform_default(
    const resolved_statement& stmt, context::dependency_solver& dep_solver, const diagnostic_collector& add_diagnostic)
{
    std::vector<checking::check_op_ptr> operand_vector;
    for (auto& op : stmt.operands_ref().value)
    {
        // check whether operand isn't empty
        if (op->type == semantics::operand_type::EMPTY)
        {
            operand_vector.push_back(std::make_unique<checking::empty_operand>(op->operand_range));
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
    const diagnostic_collector& add_diagnostic,
    const resolved_statement& stmt,
    size_t op_position,
    const context::mnemonic_code* mnemonic)
{
    diagnostic_consumer_transform diags([&add_diagnostic](diagnostic_op d) { add_diagnostic(std::move(d)); });

    const auto& ev_op = dynamic_cast<const semantics::evaluable_operand&>(*op);

    auto tmp = context::instruction::find_assembler_instructions(*stmt.opcode_ref().value);
    const bool can_have_ord_syms = tmp ? tmp->has_ord_symbols() : true;
    const bool postpone_dependencies = tmp ? tmp->postpone_dependencies() : false;

    std::vector<context::id_index> missing_symbols;
    if (can_have_ord_syms && !postpone_dependencies && ev_op.has_dependencies(dep_solver, &missing_symbols))
    {
        for (const auto& symbol : missing_symbols)
            add_diagnostic(diagnostic_op::error_E010("ordinary symbol", *symbol, ev_op.operand_range));
        if (missing_symbols.empty()) // this is a fallback message if somehow non-symbolic deps are not resolved
            add_diagnostic(diagnostic_op::error_E016(ev_op.operand_range));
        return nullptr;
    }

    checking::check_op_ptr uniq;

    checking::using_label_checker lc(dep_solver, diags);
    ev_op.apply_mach_visitor(lc);

    if (auto mach_op = dynamic_cast<const semantics::machine_operand*>(&ev_op))
    {
        // TODO: this is less than ideal, we should probably create operand structures
        // with the correct "type" while parsing and reject incompatible arguments
        // early when the syntax is incompatible
        const auto* instr = mnemonic ? mnemonic->instruction()
                                     : &context::instruction::get_machine_instructions(*stmt.opcode_ref().value);
        if (op_position < instr->operands().size())
        {
            uniq = mach_op->get_operand_value(dep_solver, instr->operands()[op_position], diags);
        }
        else
            uniq = ev_op.get_operand_value(dep_solver, diags);
    }
    else if (auto expr_op = dynamic_cast<const semantics::expr_assembler_operand*>(&ev_op))
    {
        uniq = expr_op->get_operand_value(dep_solver, can_have_ord_syms, diags);
    }
    else
    {
        uniq = ev_op.get_operand_value(dep_solver, diags);
    }

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
        operand_vector = transform_mnemonic(stmt, dep_solver, *mnem_tmp, collector);
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
