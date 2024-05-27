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

#include "preprocessor_utils.h"

#include <algorithm>
#include <string>
#include <tuple>
#include <utility>

#include "lexing/logical_line.h"
#include "utils/string_operations.h"

namespace hlasm_plugin::parser_library::processing {
namespace {
size_t get_quoted_string_end(std::string_view s)
{
    auto closing_quote = std::string_view::npos;

    s.remove_prefix(1);
    while (closing_quote == std::string_view::npos)
    {
        closing_quote = s.find_first_of("'");

        if (closing_quote == std::string_view::npos || closing_quote == s.length() - 1
            || s[closing_quote + 1] != '\'') // ignore double quotes
            break;

        s = s.substr(closing_quote + 1);
        closing_quote = std::string_view::npos;
    }

    return closing_quote;
}

size_t get_argument_length(std::string_view s)
{
    auto string_end_pos = std::string_view::npos;
    if (auto string_start_pos = s.find_first_of("'"); string_start_pos != std::string_view::npos)
        string_end_pos = get_quoted_string_end(s);

    return string_end_pos == std::string_view::npos ? s.length() : s.find_first_of(")", string_end_pos) + 1;
}

std::string_view extract_operand_and_argument(std::string_view s)
{
    static constexpr std::string_view separators = " ,";

    auto separator_pos = s.find_first_of(separators);
    if (separator_pos == std::string_view::npos)
        return s;

    auto parenthesis = s.find_first_of("(");
    if (parenthesis == std::string_view::npos)
        return s.substr(0, separator_pos);

    if (parenthesis > separator_pos)
        if (auto prev_char = s.find_last_not_of(separators, parenthesis - 1); prev_char > separator_pos)
            return s.substr(0, separator_pos);

    return s.substr(0, get_argument_length(s));
}

std::pair<std::string_view, size_t> remove_separators(std::string_view s)
{
    auto trimmed = hlasm_plugin::utils::trim_left(s);
    if (!s.empty() && s.front() == ',')
    {
        s.remove_prefix(1);
        trimmed += 1;
    }

    return { s, trimmed };
}
} // namespace

std::vector<semantics::preproc_details::name_range> get_operands_list(
    std::string_view operands, size_t op_column_start, const semantics::range_provider& rp)
{
    std::vector<semantics::preproc_details::name_range> operand_list;
    auto lineno = rp.original_range.start.line;

    while (!operands.empty())
    {
        size_t trimmed = 0;
        std::tie(operands, trimmed) = remove_separators(operands);
        if (operands.empty())
            break;

        op_column_start += trimmed;

        auto operand_view = extract_operand_and_argument(operands);
        std::string operand;
        operand.reserve(operand_view.length());
        std::ranges::remove_copy_if(
            operand_view, std::back_inserter(operand), [](unsigned char c) { return utils::isblank32(c); });

        operand_list.emplace_back(std::move(operand),
            rp.adjust_range(range {
                position(lineno, op_column_start),
                position(lineno, op_column_start + operand_view.length()),
            }));

        operands.remove_prefix(operand_view.length());
        op_column_start += operand_view.length();
    }

    return operand_list;
}

namespace {
template<typename ITERATOR>
semantics::preproc_details::name_range get_stmt_part_name_range(
    std::span<const std::pair<ITERATOR, ITERATOR>> matches, size_t index, const semantics::range_provider& rp)
{
    semantics::preproc_details::name_range nr;
    auto lineno = rp.original_range.start.line;
    ++index;
    if (index < matches.size() && (index == 0 || matches[index].first != matches[index].second))
    {
        nr.name = std::string(matches[index].first, matches[index].second);
        nr.r = rp.adjust_range(range {
            position(lineno, std::ranges::distance(matches[1].first, matches[index].first)),
            position(lineno, std::ranges::distance(matches[1].first, matches[index].second)),
        });
    }

    return nr;
}
} // namespace

template<typename PREPROC_STATEMENT, typename ITERATOR>
std::shared_ptr<PREPROC_STATEMENT> get_preproc_statement(std::span<const std::pair<ITERATOR, ITERATOR>> matches,
    const stmt_part_ids& ids,
    size_t lineno,
    bool contains_preproc_specific_instruction,
    size_t continue_column)
{
    assert(!matches.empty() && (ids.operands < matches.size() || ids.operands == -1)
        && (!ids.remarks || *ids.remarks < matches.size() || *ids.remarks == -1));

    const auto matches_ = [&matches](size_t n) { return matches[1 + n]; };
    const auto lengths_ = [&matches](size_t n) {
        const auto& [b, e] = matches[1 + n];
        return std::ranges::distance(b, e);
    };

    semantics::preproc_details details;

    details.stmt_r = range({ lineno, 0 }, { lineno, static_cast<size_t>(lengths_(0)) });
    auto rp = semantics::range_provider(details.stmt_r, semantics::adjusting_state::MACRO_REPARSE, continue_column);

    if (ids.label)
        details.label = get_stmt_part_name_range<ITERATOR>(matches, *ids.label, rp);

    if (ids.instruction.size())
    {
        // Let's store the complete instruction range and only the last word of the instruction as it is unique
        details.instruction.nr = get_stmt_part_name_range<ITERATOR>(matches, ids.instruction.back(), rp);

        auto front_instr_r = get_stmt_part_name_range<ITERATOR>(matches, ids.instruction.front(), rp).r;
        details.instruction.nr.r.start = front_instr_r.start;

        if (contains_preproc_specific_instruction)
            details.instruction.preproc_specific_r = std::move(front_instr_r);
    }

    if (lengths_(ids.operands))
        details.operands = get_operands_list(get_stmt_part_name_range<ITERATOR>(matches, ids.operands, rp).name,
            std::ranges::distance(matches_(0).first, matches_(ids.operands).first),
            rp);

    if (ids.remarks && lengths_(*ids.remarks))
        details.remarks.emplace_back(get_stmt_part_name_range<ITERATOR>(matches, *ids.remarks, rp).r);

    return std::make_shared<PREPROC_STATEMENT>(std::move(details));
}

template std::shared_ptr<semantics::preprocessor_statement_si>
get_preproc_statement<semantics::preprocessor_statement_si,
    lexing::logical_line<std::string_view::iterator>::const_iterator>(
    std::span<const std::pair<lexing::logical_line<std::string_view::iterator>::const_iterator,
        lexing::logical_line<std::string_view::iterator>::const_iterator>> matches,
    const stmt_part_ids& ids,
    size_t lineno,
    bool contains_preproc_specific_instruction,
    size_t continuation_column);

template std::shared_ptr<semantics::endevor_statement_si>
get_preproc_statement<semantics::endevor_statement_si, std::string_view::iterator>(
    std::span<const std::pair<std::string_view::iterator, std::string_view::iterator>> matches,
    const stmt_part_ids& ids,
    size_t lineno,
    bool contains_preproc_specific_instruction,
    size_t continuation_column);
} // namespace hlasm_plugin::parser_library::processing
