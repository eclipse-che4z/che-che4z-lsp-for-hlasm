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

#include "lexing/logical_line.h"
#include "semantics/source_info_processor.h"
#include "semantics/statement.h"
#include "utils/unicode_text.h"

namespace hlasm_plugin::parser_library::processing {

preprocessor::line_iterator preprocessor::extract_nonempty_logical_line(
    lexing::logical_line& out, line_iterator it, line_iterator end, const lexing::logical_line_extractor_args& opts)
{
    out.clear();

    while (it != end)
    {
        auto text = it++->text();
        if (!append_to_logical_line(out, text, opts))
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

void preprocessor::clear_statements() { m_statements.clear(); }

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

void preprocessor::do_highlighting(
    const semantics::preprocessor_statement_si& stmt, semantics::source_info_processor& src_proc) const
{
    if (stmt.label_ref().type != semantics::label_si_type::EMPTY)
        src_proc.add_hl_symbol(token_info(stmt.label_ref().field_range, semantics::hl_scopes::label));

    src_proc.add_hl_symbol(token_info(stmt.instruction_ref().field_range, semantics::hl_scopes::instruction));

    if (stmt.operands_ref().value.size())
        src_proc.add_hl_symbol(token_info(stmt.operands_ref().field_range, semantics::hl_scopes::operand));

    if (stmt.remarks_ref().value.size())
        src_proc.add_hl_symbol(token_info(stmt.remarks_ref().field_range, semantics::hl_scopes::remark));
}

namespace {
std::vector<semantics::preproc_details::name_range> get_operands_list(
    std::string_view operands, size_t column_offset, size_t lineno)
{
    std::vector<semantics::preproc_details::name_range> operand_list;

    while (operands.size())
    {
        auto space = operands.find_first_of(" (),'");

        if (space == std::string_view::npos)
        {
            operand_list.emplace_back(operands,
                range((position(lineno, column_offset)), (position(lineno, column_offset + operands.length()))));
            break;
        }

        operand_list.emplace_back(operands.substr(0, space),
            range((position(lineno, column_offset)), (position(lineno, column_offset + space))));


        operands.remove_prefix(space);
        column_offset += space;

        space = operands.find_first_not_of(" (),'");
        if (space == std::string_view::npos)
            break;

        operands.remove_prefix(space);
        column_offset += space;
    }

    return operand_list;
}

template<typename ITERATOR>
semantics::preproc_details::name_range get_stmt_part_name_range(
    const std::match_results<ITERATOR>& matches, size_t index, size_t line_no)
{
    semantics::preproc_details::name_range nr;

    if (index < matches.size() && matches[index].length())
    {
        nr.name = std::string_view(std::to_address(&*matches[index].first), matches[index].length());
        nr.r = range(position(line_no, std::distance(matches[0].first, matches[index].first)),
            position(line_no, std::distance(matches[0].first, matches[index].second)));
    }

    return nr;
}

template<typename ITERATOR>
std::optional<semantics::preproc_details> get_preproc_details(const std::match_results<ITERATOR>& matches,
    std::optional<size_t> label_id,
    std::vector<size_t> instruction_ids,
    size_t operands_id,
    std::optional<size_t> remarks_id,
    size_t lineno,
    context::id_storage& ids)
{
    if (!matches.size())
        return std::nullopt;

    semantics::preproc_details details;

    details.stmt_r = range({ lineno, 0 }, { lineno, matches[0].str().length() });

    if (label_id)
        details.label = get_stmt_part_name_range<ITERATOR>(matches, *label_id, lineno);

    if (instruction_ids.size())
    {
        // Let's store the complete instruction range and only the last instruction part which is unique
        details.instruction = get_stmt_part_name_range<ITERATOR>(matches, instruction_ids.back(), lineno);
        details.instruction.r.start =
            get_stmt_part_name_range<ITERATOR>(matches, instruction_ids.front(), lineno).r.start;
    }

    if (matches[operands_id].length())
    {
        auto [ops_text, op_range] = get_stmt_part_name_range<ITERATOR>(matches, operands_id, lineno);
        details.operands.first = get_operands_list(ops_text, op_range.start.column, lineno);
        details.operands.second = std::move(op_range);
    }

    if (remarks_id && matches.size() == *remarks_id + 1 && matches[*remarks_id].length())
    {
        details.remarks.second = get_stmt_part_name_range<ITERATOR>(matches, *remarks_id, lineno).r;

        details.remarks.first.emplace_back(details.remarks.second);
    }

    return details;
}
} // namespace

template<typename PREPROC_STATEMENT, typename ITERATOR>
std::shared_ptr<PREPROC_STATEMENT> preprocessor::get_preproc_statement(
    const std::match_results<ITERATOR>& matches, stmt_part_ids part_ids, size_t lineno, context::id_storage& ids) const
{
    if (!matches.size())
        return nullptr;

    semantics::preproc_details details;

    details.stmt_r = range({ lineno, 0 }, { lineno, matches[0].str().length() });

    if (part_ids.label)
        details.label = get_stmt_part_name_range<ITERATOR>(matches, *part_ids.label, lineno);

    if (part_ids.instruction.size())
    {
        // Let's store the complete instruction range and only the last instruction part which is unique
        details.instruction = get_stmt_part_name_range<ITERATOR>(matches, part_ids.instruction.back(), lineno);
        details.instruction.r.start =
            get_stmt_part_name_range<ITERATOR>(matches, part_ids.instruction.front(), lineno).r.start;
    }

    if (matches[part_ids.operands].length())
    {
        auto [ops_text, op_range] = get_stmt_part_name_range<ITERATOR>(matches, part_ids.operands, lineno);
        details.operands.first = get_operands_list(ops_text, op_range.start.column, lineno);
        details.operands.second = std::move(op_range);
    }

    if (part_ids.remarks && matches.size() == *part_ids.remarks + 1 && matches[*part_ids.remarks].length())
    {
        details.remarks.second = get_stmt_part_name_range<ITERATOR>(matches, *part_ids.remarks, lineno).r;

        details.remarks.first.emplace_back(details.remarks.second);
    }

    return std::make_shared<PREPROC_STATEMENT>(std::move(details), ids);
}

template std::shared_ptr<semantics::endevor_statement_si>
preprocessor::get_preproc_statement<semantics::endevor_statement_si, std::string_view::iterator>(
    const std::match_results<std::string_view::iterator>& matches,
    stmt_part_ids part_ids,
    size_t lineno,
    context::id_storage& ids) const;

template std::shared_ptr<semantics::cics_statement_si>
preprocessor::get_preproc_statement<semantics::cics_statement_si, lexing::logical_line::const_iterator>(
    const std::match_results<lexing::logical_line::const_iterator>& matches,
    stmt_part_ids part_ids,
    size_t lineno,
    context::id_storage& ids) const;

} // namespace hlasm_plugin::parser_library::processing
