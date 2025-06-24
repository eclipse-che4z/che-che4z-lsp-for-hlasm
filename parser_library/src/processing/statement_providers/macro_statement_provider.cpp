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

#include "macro_statement_provider.h"

#include "context/hlasm_context.h"

namespace hlasm_plugin::parser_library::processing {

macro_statement_provider::macro_statement_provider(const analyzing_context& ctx,
    statement_fields_parser& parser,
    parse_lib_provider& lib_provider,
    processing::processing_state_listener& listener,
    diagnostic_op_consumer& diag_consumer)
    : members_statement_provider(statement_provider_kind::MACRO, ctx, parser, lib_provider, listener, diag_consumer)
{}

bool macro_statement_provider::finished() const { return m_ctx.hlasm_ctx->scope_stack().size() == 1; }

std::pair<context::statement_cache*, std::optional<std::optional<context::id_index>>>
macro_statement_provider::get_next()
{
    auto& invo = m_ctx.hlasm_ctx->scope_stack().back().this_macro;
    assert(invo);

    invo->current_statement.value += !m_resolved_instruction.has_value();
    if (invo->current_statement.value == invo->cached_definition.size())
    {
        m_resolved_instruction.reset();
        m_ctx.hlasm_ctx->leave_macro();
        return {};
    }

    return { &invo->cached_definition[invo->current_statement.value], std::exchange(m_resolved_instruction, {}) };
}

std::vector<diagnostic_op> macro_statement_provider::filter_cached_diagnostics(
    const semantics::deferred_statement& stmt, bool no_operands) const
{
    auto diags = no_operands ? stmt.diagnostics_without_operands() : stmt.diagnostics();
    return std::vector<diagnostic_op>(diags.begin(), diags.end());
}

} // namespace hlasm_plugin::parser_library::processing
