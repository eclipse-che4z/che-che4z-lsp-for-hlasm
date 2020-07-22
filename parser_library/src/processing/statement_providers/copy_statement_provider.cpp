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

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

copy_statement_provider::copy_statement_provider(context::hlasm_context& hlasm_ctx, statement_fields_parser& parser)
    : common_statement_provider(statement_provider_kind::COPY, hlasm_ctx, parser)
{}

void copy_statement_provider::process_next(statement_processor& processor)
{
    if (finished())
        throw std::runtime_error("provider already finished");

    auto& invo = hlasm_ctx.current_copy_stack().back();

    ++invo.current_statement;
    if ((size_t)invo.current_statement == invo.cached_definition.size())
    {
        hlasm_ctx.leave_copy_member();
        return;
    }

    auto& cache = invo.cached_definition[invo.current_statement];

    switch (cache.get_base()->kind)
    {
        case context::statement_kind::COMPLETE:
            processor.process_statement(cache.get_base());
            break;
        case context::statement_kind::PARTIAL:
            preprocess_deferred(processor, cache);
            break;
        default:
            break;
    }
}

bool copy_statement_provider::finished() const { return hlasm_ctx.current_copy_stack().empty(); }
