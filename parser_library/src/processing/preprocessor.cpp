/*
 * Copyright (c) 2022 Broadcom.
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

#include "preprocessor.h"

#include <iterator>

#include "lexing/logical_line.h"
#include "protocol.h"
#include "semantics/source_info_processor.h"
#include "semantics/statement.h"
#include "utils/unicode_text.h"

namespace hlasm_plugin::parser_library::processing {

preprocessor::line_iterator preprocessor::extract_nonempty_logical_line(
    lexing::logical_line<std::string_view::iterator>& out,
    line_iterator it,
    line_iterator end,
    const lexing::logical_line_extractor_args& opts)
{
    out.clear();

    while (it != end)
    {
        if (!append_to_logical_line(out, it++->text(), opts).first)
            break;
    }

    finish_logical_line(out, opts);

    return it;
}

bool preprocessor::is_continued(std::string_view s)
{
    const auto cont = utils::utf8_substr(s, lexing::default_ictl_copy.end, 1).str;
    return !cont.empty() && cont != " ";
}

void preprocessor::reset()
{
    m_statements.clear();
    m_inc_members.clear();
}

void preprocessor::set_statement(std::shared_ptr<semantics::preprocessor_statement_si> stmt)
{
    m_statements.emplace_back(std::move(stmt));
}

void preprocessor::set_statements(std::vector<std::shared_ptr<semantics::preprocessor_statement_si>> stmts)
{
    m_statements.insert(
        m_statements.end(), std::make_move_iterator(stmts.begin()), std::make_move_iterator(stmts.end()));
}

std::vector<std::shared_ptr<semantics::preprocessor_statement_si>> preprocessor::take_statements()
{
    return std::move(m_statements);
}

void preprocessor::do_highlighting(const semantics::preprocessor_statement_si& stmt,
    semantics::source_info_processor& src_proc,
    size_t continue_column) const
{
    const auto& details = stmt.m_details;

    if (!details.label.name.empty())
        src_proc.add_hl_symbol(token_info(details.label.r, hl_scopes::label), continue_column);

    src_proc.add_hl_symbol(token_info(details.instruction.nr.r, hl_scopes::instruction), continue_column);

    for (const auto& operand : details.operands)
        src_proc.add_hl_symbol(token_info(operand.r, hl_scopes::operand), continue_column);

    for (const auto& remark_r : details.remarks)
        src_proc.add_hl_symbol(token_info(remark_r, hl_scopes::remark), continue_column);
}

void preprocessor::do_highlighting(const semantics::preprocessor_statement_si& stmt,
    const lexing::logical_line<std::string_view::iterator>& ll,
    semantics::source_info_processor& src_proc,
    size_t continue_column) const
{
    do_highlighting(stmt, src_proc, continue_column);

    constexpr const auto continuation_column = lexing::default_ictl.end;
    constexpr const auto ignore_column = lexing::default_ictl.end + 1;
    for (size_t i = 0, lineno = stmt.m_details.stmt_r.start.line; i < ll.segments.size(); ++i, ++lineno)
    {
        const auto& segment = ll.segments[i];

        const bool continuation = segment.continuation != segment.ignore;
        if (continuation)
            src_proc.add_hl_symbol(
                token_info(range(position(lineno, continuation_column), position(lineno, ignore_column)),
                    hl_scopes::continuation));

        if (const size_t ignore_len = std::ranges::distance(segment.ignore, segment.end); ignore_len)
            src_proc.add_hl_symbol(token_info(
                range(position(lineno, ignore_column), position(lineno, ignore_column + ignore_len - !continuation)),
                hl_scopes::ignored));
    }
}

void preprocessor::append_included_member(std::unique_ptr<included_member_details> details)
{
    m_inc_members.emplace_back(std::move(details));
}

void preprocessor::append_included_members(std::vector<std::unique_ptr<included_member_details>> details)
{
    m_inc_members.insert(
        m_inc_members.end(), std::make_move_iterator(details.begin()), std::make_move_iterator(details.end()));
}

void preprocessor::capture_included_members(preprocessor& preproc)
{
    append_included_members(std::move(preproc.m_inc_members));
}

const std::vector<std::unique_ptr<preprocessor::included_member_details>>& preprocessor::view_included_members()
{
    return m_inc_members;
}

} // namespace hlasm_plugin::parser_library::processing
