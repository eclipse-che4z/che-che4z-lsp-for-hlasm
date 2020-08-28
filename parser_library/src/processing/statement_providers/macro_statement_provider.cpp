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

#include "processing/processing_manager.h"

namespace hlasm_plugin::parser_library::processing {

macro_statement_provider::macro_statement_provider(context::hlasm_context& hlasm_ctx, statement_fields_parser& parser)
    : common_statement_provider(statement_provider_kind::MACRO, hlasm_ctx, parser)
{}

bool macro_statement_provider::finished() const { return hlasm_ctx.scope_stack().size() == 1; }

context::cached_statement_storage* macro_statement_provider::get_next()
{
    auto& invo = hlasm_ctx.scope_stack().back().this_macro;
    assert(invo);

    ++invo->current_statement;
    if ((size_t)invo->current_statement == invo->cached_definition.size())
    {
        hlasm_ctx.leave_macro();
        return nullptr;
    }

    return &invo->cached_definition[invo->current_statement];
}

} // namespace hlasm_plugin::parser_library::processing
