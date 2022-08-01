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

namespace hlasm_plugin::parser_library::processing {

copy_statement_provider::copy_statement_provider(analyzing_context ctx,
    statement_fields_parser& parser,
    workspaces::parse_lib_provider& lib_provider,
    processing::processing_state_listener& listener,
    diagnostic_op_consumer& diag_consumer)
    : members_statement_provider(
        statement_provider_kind::COPY, std::move(ctx), parser, lib_provider, listener, diag_consumer)
{}

bool copy_statement_provider::finished() const
{
    const auto& current_stack = ctx.hlasm_ctx->current_copy_stack();
    return current_stack.empty() || ctx.hlasm_ctx->in_opencode() && current_stack.back().suspended();
}

context::statement_cache* copy_statement_provider::get_next()
{
    auto& invo = ctx.hlasm_ctx->current_copy_stack().back();

    ++invo.current_statement;
    if (invo.current_statement == invo.cached_definition()->size())
    {
        ctx.hlasm_ctx->leave_copy_member();
        return nullptr;
    }

    return &invo.cached_definition()->at(invo.current_statement);
}

std::vector<diagnostic_op> copy_statement_provider::filter_cached_diagnostics(
    const semantics::deferred_statement& stmt) const
{
    auto diags = stmt.diagnostics_without_operands();
    return std::vector<diagnostic_op>(diags.begin(), diags.end());
}

} // namespace hlasm_plugin::parser_library::processing
