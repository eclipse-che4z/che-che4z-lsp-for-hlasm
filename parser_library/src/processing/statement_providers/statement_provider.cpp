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

#include "statement_provider.h"

#include "context/hlasm_context.h"
#include "expressions/conditional_assembly/terms/ca_var_sym.h"
#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::processing {

statement_provider::statement_provider(const statement_provider_kind kind)
    : kind(kind)
{}

bool statement_provider::try_trigger_attribute_lookahead(const semantics::instruction_si& instruction,
    expressions::evaluation_context eval_ctx,
    processing::processing_state_listener& listener,
    std::vector<context::id_index>&& references_buffer)
{
    references_buffer.clear();
    process_instruction(references_buffer, instruction, eval_ctx);

    if (references_buffer.empty())
        return false;

    trigger_attribute_lookahead(std::move(references_buffer), eval_ctx, listener);

    return true;
}

bool statement_provider::try_trigger_attribute_lookahead(const context::hlasm_statement& statement,
    expressions::evaluation_context eval_ctx,
    processing::processing_state_listener& listener,
    std::vector<context::id_index>&& references_buffer)
{
    references_buffer.clear();
    const semantics::label_si* label;

    if (auto def_stmt = statement.access_deferred())
    {
        label = &def_stmt->label_ref();
    }
    else if (auto res_stmt = statement.access_resolved())
    {
        label = &res_stmt->label_ref();

        process_operands(references_buffer, res_stmt->operands_ref(), eval_ctx);
    }
    else
        return false;

    process_label(references_buffer, *label, eval_ctx);

    if (references_buffer.empty())
        return false;

    trigger_attribute_lookahead(std::move(references_buffer), eval_ctx, listener);

    return true;
}

void statement_provider::trigger_attribute_lookahead(std::vector<context::id_index>&& references_buffer,
    const expressions::evaluation_context& eval_ctx,
    processing::processing_state_listener& listener)
{
    auto&& [statement_position, snapshot] = eval_ctx.hlasm_ctx.get_begin_snapshot(false);

    std::ranges::sort(references_buffer);

    listener.start_lookahead(
        lookahead_start_data(std::vector(references_buffer.begin(), std::ranges::unique(references_buffer).begin()),
            statement_position,
            std::move(snapshot)));
}

bool statement_provider::process_label(std::vector<context::id_index>& symbols,
    const semantics::label_si& label,
    const expressions::evaluation_context& eval_ctx)
{
    switch (label.type)
    {
        case semantics::label_si_type::CONC:
            return semantics::concatenation_point::get_undefined_attributed_symbols(
                symbols, std::get<semantics::concat_chain>(label.value), eval_ctx);
        case semantics::label_si_type::VAR:
            return expressions::ca_var_sym::get_undefined_attributed_symbols_vs(
                symbols, std::get<semantics::vs_ptr>(label.value), eval_ctx);
        default:
            return false;
    }
}

bool statement_provider::process_instruction(std::vector<context::id_index>& symbols,
    const semantics::instruction_si& instruction,
    const expressions::evaluation_context& eval_ctx)
{
    if (instruction.type != semantics::instruction_si_type::CONC)
        return false;

    const auto& chain = std::get<semantics::concat_chain>(instruction.value);

    return semantics::concatenation_point::get_undefined_attributed_symbols(symbols, chain, eval_ctx);
}

bool statement_provider::process_operands(std::vector<context::id_index>& symbols,
    const semantics::operands_si& operands,
    const expressions::evaluation_context& eval_ctx)
{
    bool result = false;
    for (const auto& op : operands.value)
    {
        switch (op->type)
        {
            case semantics::operand_type::MODEL:
                result |= semantics::concatenation_point::get_undefined_attributed_symbols(
                    symbols, op->access_model()->chain, eval_ctx);
                break;
            case semantics::operand_type::MAC:
                if (op->access_mac()->kind == semantics::mac_kind::CHAIN)
                    result |= semantics::concatenation_point::get_undefined_attributed_symbols(
                        symbols, op->access_mac()->access_chain()->chain, eval_ctx);
                break;
            case semantics::operand_type::CA:
                result |= op->access_ca()->get_undefined_attributed_symbols(symbols, eval_ctx);
                break;
            default:
                break;
        }
    }
    return result;
}

} // namespace hlasm_plugin::parser_library::processing
