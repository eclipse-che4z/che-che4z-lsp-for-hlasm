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

copy_statement_provider::copy_statement_provider(context::hlasm_context& hlasm_ctx, statement_fields_parser& parser)
    : members_statement_provider(statement_provider_kind::COPY, hlasm_ctx, parser)
{}

bool copy_statement_provider::finished() const { return hlasm_ctx.current_copy_stack().empty(); }

context::cached_statement_storage* copy_statement_provider::get_next()
{
    auto& invo = hlasm_ctx.current_copy_stack().back();

    ++invo.current_statement;
    if ((size_t)invo.current_statement == invo.cached_definition.size())
    {
        hlasm_ctx.leave_copy_member();
        return nullptr;
    }

    return &invo.cached_definition[invo.current_statement];
}

} // namespace hlasm_plugin::parser_library::processing
