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

#include "context/hlasm_context.h"
#include "processing/processing_manager.h"
#include "processing/statement_processors/ordinary_processor.h"
#include "semantics/operand_impls.h"

using namespace hlasm_plugin::parser_library;
using namespace processing;
using namespace workspaces;

low_language_processor::low_language_processor(const analyzing_context& ctx,
    branching_provider& branch_provider,
    parse_lib_provider& lib_provider,
    statement_fields_parser& parser,
    const processing_manager& proc_mgr,
    diagnosable_ctx& diag_ctx)
    : instruction_processor(ctx, branch_provider, lib_provider, diag_ctx)
    , parser(parser)
    , proc_mgr(proc_mgr)
{}

rebuilt_statement low_language_processor::preprocess(std::shared_ptr<const processing::resolved_statement> stmt)
{
    auto [label, ops, literals, was_model] = preprocess_inner(*stmt);
    rebuilt_statement result(std::move(stmt), std::move(label), std::move(ops), std::move(literals));
    if (was_model)
        proc_mgr.run_analyzers(result, true);
    return result;
}

context::id_index low_language_processor::find_label_symbol(const rebuilt_statement& stmt) const
{
    if (const auto& label = stmt.label_ref(); label.type == semantics::label_si_type::ORD)
    {
        diagnostic_consumer_transform diags([this](diagnostic_op d) { add_diagnostic(std::move(d)); });
        auto [valid, id] = hlasm_ctx.try_get_symbol_name(std::get<semantics::ord_symbol_string>(label.value).symbol);
        if (!valid)
            diags.add_diagnostic(diagnostic_op::error_E065(label.field_range));
        return id;
    }
    else
        return context::id_index();
}

context::id_index low_language_processor::find_using_label(const rebuilt_statement& stmt) const
{
    if (const auto& label = stmt.label_ref(); label.type == semantics::label_si_type::ORD)
    {
        if (auto [valid, id] =
                hlasm_ctx.try_get_symbol_name(std::get<semantics::ord_symbol_string>(label.value).symbol);
            valid)
            return id;

        add_diagnostic(diagnostic_op::error_E065(label.field_range));
    }
    return context::id_index();
}

bool low_language_processor::create_symbol(
    range err_range, context::id_index symbol_name, context::symbol_value value, context::symbol_attributes attributes)
{
    bool ok = hlasm_ctx.ord_ctx.create_symbol(symbol_name, std::move(value), std::move(attributes), lib_info);

    if (!ok)
        add_diagnostic(diagnostic_op::error_E033(err_range));

    return ok;
}

low_language_processor::preprocessed_part low_language_processor::preprocess_inner(const resolved_statement& stmt)
{
    using namespace semantics;
    preprocessed_part result;

    const auto label_inserter = [&result, this](std::string&& label, const range& r) {
        label.erase(label.find_last_not_of(' ') + 1);
        if (label.empty())
            result.label.emplace(r);
        else
        {
            auto ord_id = hlasm_ctx.add_id(label);
            result.label.emplace(r, ord_symbol_string { ord_id, std::move(label) });
        }
    };

    // label
    switch (const auto& label_ref = stmt.label_ref(); label_ref.type)
    {
        case label_si_type::CONC:
            label_inserter(concatenation_point::evaluate(std::get<concat_chain>(label_ref.value), eval_ctx),
                label_ref.field_range);
            result.was_model = true;
            break;
        case label_si_type::VAR:
            label_inserter(
                var_sym_conc::evaluate(std::get<vs_ptr>(label_ref.value)->evaluate(eval_ctx)), label_ref.field_range);
            result.was_model = true;
            break;
        case label_si_type::MAC:
            if (stmt.opcode_ref().value.to_string_view() != "TITLE")
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
        const auto* model = operands_ref.value[0]->access_model();
        auto [field, map] = concatenation_point::evaluate_with_range_map(model->chain, eval_ctx);
        auto [operands, _, literals] = parser.parse_operand_field(lexing::u8string_view_with_newlines(field),
            true,
            range_provider(std::move(map), model->line_limits),
            0,
            processing_status(stmt.format_ref(), stmt.opcode_ref()),
            diag_ctx);
        result.operands.emplace(std::move(operands));
        result.literals.emplace(std::move(literals));
        result.was_model = true;
    }

    return result;
}

check_org_result hlasm_plugin::parser_library::processing::check_address_for_ORG(
    const context::address& addr_to_check, const context::address& curr_addr, size_t boundary, int offset)
{
    int addr_to_check_offset = addr_to_check.offset();

    int al = boundary ? (int)((boundary - (addr_to_check_offset % boundary)) % boundary) : 0;

    bool underflow = !addr_to_check.has_dependant_space() && addr_to_check_offset + al + offset < 0;
    if (underflow || !curr_addr.in_same_loctr(addr_to_check))
        return check_org_result::underflow;

    if (!addr_to_check.is_simple())
        return check_org_result::invalid_address;

    return check_org_result::valid;
}
