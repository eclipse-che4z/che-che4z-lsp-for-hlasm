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
    processing::processing_state_listener& listener)
    : members_statement_provider(statement_provider_kind::COPY, std::move(ctx), parser, lib_provider, listener)
{}

bool copy_statement_provider::finished() const
{
    return ctx.hlasm_ctx->current_copy_stack().empty()
        || m_suspended && &ctx.hlasm_ctx->current_copy_stack() == &ctx.hlasm_ctx->opencode_copy_stack();
}

void copy_statement_provider::suspend()
{
    assert(!m_suspended);
    m_suspended = true;
}

bool copy_statement_provider::try_resume_at(position_t line_no, resume_copy resume_opts)
{
    assert(m_suspended);

    if (resume_opts == resume_copy::ignore_line)
    {
        m_suspended = false;
        return true;
    }

    auto& opencode_copy_stack = ctx.hlasm_ctx->opencode_copy_stack().back();

    opencode_copy_stack.current_statement = -1;

    for (const auto& stmt : *opencode_copy_stack.cached_definition)
    {
        const auto stmt_line_no = stmt.get_base()->statement_position().line;
        if (stmt_line_no == line_no)
        {
            m_suspended = false;
            return true;
        }
        else if (stmt_line_no > line_no)
        {
            switch (resume_opts)
            {
                case resume_copy::exact_line_match:
                    return false;
                case resume_copy::exact_or_next_line:
                    m_suspended = false;
                    return true;
            }
        }

        ++opencode_copy_stack.current_statement;
    }
    return false;
}

context::statement_cache* copy_statement_provider::get_next()
{
    auto& invo = ctx.hlasm_ctx->current_copy_stack().back();

    ++invo.current_statement;
    if ((size_t)invo.current_statement == invo.cached_definition->size())
    {
        ctx.hlasm_ctx->leave_copy_member();
        return nullptr;
    }

    return &invo.cached_definition->at(invo.current_statement);
}

} // namespace hlasm_plugin::parser_library::processing
