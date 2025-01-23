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

#include "range_provider.h"

#include <cassert>

using namespace hlasm_plugin::parser_library;
namespace hlasm_plugin::parser_library::semantics {

range range_provider::adjust_range(range r) const noexcept
{
    if (state == adjusting_state::MACRO_REPARSE)
    {
        if (r.start != r.end)
            return range(adjust_position(r.start, false), adjust_position(r.end, true));

        auto adjusted = adjust_position(r.end, true);
        return range(adjusted, adjusted);
    }
    else if (state == adjusting_state::SUBSTITUTION)
        return original_range;
    else if (state == adjusting_state::NONE)
        return r;
    else if (state == adjusting_state::MODEL_REPARSE)
    {
        assert(r.start.line == 0 && r.end.line == 0);
        if (r.start != r.end)
            return range(adjust_model_position(r.start, false), adjust_model_position(r.end, true));

        auto adjusted = adjust_model_position(r.end, true);
        return range(adjusted, adjusted);
    }
    assert(false);
    return r;
}

position range_provider::adjust_model_position(position pos, bool end) const noexcept
{
    const auto& [d, r] = *std::prev(std::find_if(std::next(model_substitutions.begin()),
        model_substitutions.end(),
        [pos, end](const auto& s) { return pos.column < s.first.first + end; }));
    const auto& [column, var] = d;
    if (var)
        return end ? r.end : r.start;

    pos.column -= column;
    pos.column += r.start.column;
    while (true)
    {
        const size_t line_limit = get_line_limit(pos.line);
        if (pos.column < line_limit + end)
            break;
        pos.column -= line_limit - m_continued_code_line_column;
        ++pos.line;
    }
    pos.line += r.start.line;

    if (auto cmp = pos <=> r.end; cmp > 0 || end == false && cmp >= 0)
        pos = r.end;

    return pos;
}

size_t range_provider::get_line_limit(size_t relative_line) const noexcept
{
    return relative_line >= line_limits.size() ? 71 : line_limits[relative_line];
}

position range_provider::adjust_position(position pos, bool end) const noexcept
{
    auto [r, column] = [this, pos, end]() {
        for (auto column = pos.column - original_range.start.column; const auto& op_range : original_operand_ranges)
        {
            auto range_len = op_range.end.column - op_range.start.column;
            for (size_t i = op_range.start.line; i < op_range.end.line; ++i)
                range_len += get_line_limit(i - original_range.start.line) - m_continued_code_line_column;

            if (column < range_len + end)
                return std::pair(op_range, column);
            column -= range_len;
        }
        return std::pair(original_range, pos.column - original_range.start.column);
    }();

    auto column_start = r.start.column;
    size_t line_start = r.start.line - original_range.start.line;

    while (true)
    {
        auto rest = get_line_limit(line_start) - column_start;
        if (column < rest + end)
        {
            column_start += column;
            break;
        }
        else
        {
            column -= rest;
            column_start = m_continued_code_line_column;
            ++line_start;
        }
    }
    line_start += original_range.start.line;
    return position(line_start, column_start);
}

range_provider::range_provider(range original_range, adjusting_state state, size_t continued_code_line_column)
    : original_range(original_range)
    , state(state)
    , m_continued_code_line_column(continued_code_line_column)
{}

range_provider::range_provider(range original_field_range,
    std::vector<range> original_operand_ranges_,
    adjusting_state state,
    std::vector<size_t> line_limits,
    size_t continued_code_line_column)
    : original_range(original_field_range)
    , original_operand_ranges(std::move(original_operand_ranges_))
    , line_limits(std::move(line_limits))
    , state(state)
    , m_continued_code_line_column(continued_code_line_column)
{
    assert(original_operand_ranges.empty() || original_range.start == original_operand_ranges.front().start);
}

range_provider::range_provider(
    std::vector<std::pair<std::pair<size_t, bool>, range>> ms, std::vector<size_t> line_limits)
    : model_substitutions(std::move(ms))
    , line_limits(std::move(line_limits))
    , state(adjusting_state::MODEL_REPARSE)
{
    assert(!model_substitutions.empty());
}

range_provider::range_provider()
    : original_range()
    , state(adjusting_state::NONE)
{}
} // namespace hlasm_plugin::parser_library::semantics
