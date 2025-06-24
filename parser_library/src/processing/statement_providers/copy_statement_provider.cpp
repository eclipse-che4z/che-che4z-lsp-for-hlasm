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

#include "copy_statement_provider.h"

#include "context/hlasm_context.h"

namespace hlasm_plugin::parser_library::processing {

copy_statement_provider::copy_statement_provider(const analyzing_context& ctx,
    statement_fields_parser& parser,
    parse_lib_provider& lib_provider,
    processing::processing_state_listener& listener,
    diagnostic_op_consumer& diag_consumer)
    : members_statement_provider(statement_provider_kind::COPY, ctx, parser, lib_provider, listener, diag_consumer)
{}

bool copy_statement_provider::finished() const
{
    const auto& current_stack = m_ctx.hlasm_ctx->current_copy_stack();
    return current_stack.empty() || m_ctx.hlasm_ctx->in_opencode() && current_stack.back().suspended();
}

std::pair<context::statement_cache*, std::optional<std::optional<context::id_index>>>
copy_statement_provider::get_next()
{
    // LIFETIME: copy stack should not move even if source stack changes
    // due to std::vector iterator invalidation rules for move
    auto& invo = m_ctx.hlasm_ctx->current_copy_stack().back();

    invo.current_statement.value += !m_resolved_instruction.has_value();
    if (invo.current_statement.value == invo.cached_definition()->size())
    {
        m_resolved_instruction.reset();
        m_ctx.hlasm_ctx->leave_copy_member();
        return {};
    }

    return { &invo.cached_definition()->at(invo.current_statement.value), std::exchange(m_resolved_instruction, {}) };
}

std::vector<diagnostic_op> copy_statement_provider::filter_cached_diagnostics(
    const semantics::deferred_statement& stmt, bool) const
{
    auto diags = stmt.diagnostics_without_operands();
    return std::vector<diagnostic_op>(diags.begin(), diags.end());
}

} // namespace hlasm_plugin::parser_library::processing
