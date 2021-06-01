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

#include "expressions/conditional_assembly/terms/ca_var_sym.h"
#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::processing {

statement_provider::statement_provider(const statement_provider_kind kind)
    : kind(kind)
{}

bool statement_provider::try_trigger_attribute_lookahead(const semantics::instruction_si& instruction,
    expressions::evaluation_context eval_ctx,
    processing::processing_state_listener& listener)
{
    auto references = process_instruction(instruction, eval_ctx);

    if (references.empty())
        return false;

    trigger_attribute_lookahead(std::move(references), eval_ctx, listener);

    return true;
}

bool statement_provider::try_trigger_attribute_lookahead(const context::hlasm_statement& statement,
    expressions::evaluation_context eval_ctx,
    processing::processing_state_listener& listener)
{
    const semantics::label_si* label;
    std::set<context::id_index> references;

    if (statement.kind == context::statement_kind::DEFERRED)
    {
        auto def_stmt = statement.access_deferred();
        label = &def_stmt->label_ref();
    }
    else
    {
        auto res_stmt = statement.access_resolved();
        label = &res_stmt->label_ref();

        references.merge(process_operands(res_stmt->operands_ref(), eval_ctx));
    }

    references.merge(process_label(*label, eval_ctx));

    if (references.empty())
        return false;

    trigger_attribute_lookahead(std::move(references), eval_ctx, listener);

    return true;
}

void statement_provider::trigger_attribute_lookahead(std::set<context::id_index> references,
    const expressions::evaluation_context& eval_ctx,
    processing::processing_state_listener& listener)
{
    auto&& [statement_position, snapshot] = eval_ctx.hlasm_ctx.get_begin_snapshot(false);

    listener.start_lookahead(lookahead_start_data(std::move(references), statement_position, std::move(snapshot)));
}

std::set<context::id_index> statement_provider::process_label(
    const semantics::label_si& label, const expressions::evaluation_context& eval_ctx)
{
    switch (label.type)
    {
        case semantics::label_si_type::CONC:
            return semantics::concatenation_point::get_undefined_attributed_symbols(
                std::get<semantics::concat_chain>(label.value), eval_ctx);
        case semantics::label_si_type::VAR:
            return expressions::ca_var_sym::get_undefined_attributed_symbols_vs(
                std::get<semantics::vs_ptr>(label.value), eval_ctx);
        default:
            return {};
    }
}

std::set<context::id_index> statement_provider::process_instruction(
    const semantics::instruction_si& instruction, const expressions::evaluation_context& eval_ctx)
{
    if (instruction.type != semantics::instruction_si_type::CONC)
        return {};

    const auto& chain = std::get<semantics::concat_chain>(instruction.value);

    return semantics::concatenation_point::get_undefined_attributed_symbols(chain, eval_ctx);
}

std::set<context::id_index> statement_provider::process_operands(
    const semantics::operands_si& operands, const expressions::evaluation_context& eval_ctx)
{
    std::set<context::id_index> ret;
    for (const auto& op : operands.value)
    {
        switch (op->type)
        {
            case semantics::operand_type::MODEL:
                ret.merge(semantics::concatenation_point::get_undefined_attributed_symbols(
                    op->access_model()->chain, eval_ctx));
                break;
            case semantics::operand_type::MAC:
                if (op->access_mac()->kind == semantics::mac_kind::CHAIN)
                    ret.merge(semantics::concatenation_point::get_undefined_attributed_symbols(
                        op->access_mac()->access_chain()->chain, eval_ctx));
                break;
            case semantics::operand_type::CA:
                ret.merge(op->access_ca()->get_undefined_attributed_symbols(eval_ctx));
                break;
            default:
                break;
        }
    }
    return ret;
}

} // namespace hlasm_plugin::parser_library::processing
