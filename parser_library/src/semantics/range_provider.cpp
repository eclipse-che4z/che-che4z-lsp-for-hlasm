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

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::semantics;

range range_provider::get_range(const antlr4::Token* start, const antlr4::Token* stop) const
{
    range ret;

    ret.start.line = start->getLine();
    ret.start.column = start->getCharPositionInLine();

    if (stop)
    {
        ret.end.line = stop->getLine();
        ret.end.column = stop->getCharPositionInLine() + stop->getStopIndex() - stop->getStartIndex() + 1;
    }
    else // empty rule
    {
        ret.end = ret.start;
    }
    return adjust_range(ret);
}

range range_provider::get_range(const antlr4::Token* terminal) const { return get_range(terminal, terminal); }

range range_provider::get_range(antlr4::ParserRuleContext* non_terminal) const
{
    return get_range(non_terminal->getStart(), non_terminal->getStop());
}

range range_provider::get_empty_range(const antlr4::Token* start) const
{
    range ret;
    ret.start.line = start->getLine();
    ret.start.column = start->getCharPositionInLine();
    ret.end = ret.start;
    return adjust_range(ret);
}

range range_provider::adjust_range(range r) const
{
    if (state == adjusting_state::MACRO_REPARSE)
        return range(adjust_position(r.start), adjust_position(r.end));
    else if (state == adjusting_state::SUBSTITUTION)
        return original_range;
    else if (state == adjusting_state::NONE)
        return r;
    assert(false);
    return r;
}

position range_provider::adjust_position(position pos) const
{
    size_t idx = 1;
    auto orig_range = original_operand_ranges.empty() ? original_range : original_operand_ranges.front();

    auto offset = pos.column - orig_range.start.column;

    if (original_operand_ranges.size())
        while (true)
        {
            auto tmp_lines = orig_range.end.line - orig_range.start.line;
            auto tmp_cols = (orig_range.end.column + 72 * tmp_lines) - orig_range.start.column;

            if (tmp_cols >= offset)
                break;
            else
            {
                offset -= tmp_cols + 1;
                orig_range = original_operand_ranges[idx++];
            }
        }

    auto column_start = orig_range.start.column;
    auto line_start = orig_range.start.line;

    while (true)
    {
        auto rest = 71 - column_start;
        if (offset > rest)
        {
            offset -= rest;
            column_start = 15;
            ++line_start;
        }
        else
        {
            column_start += offset;
            break;
        }
    }
    return position(line_start, column_start);
}

range_provider::range_provider(range original_range, adjusting_state state)
    : original_range(original_range)
    , state(state)
{}

range_provider::range_provider(
    range original_field_range, std::vector<range> original_operand_ranges, adjusting_state state)
    : original_range(original_field_range)
    , original_operand_ranges(std::move(original_operand_ranges))
    , state(state)
{}

range_provider::range_provider()
    : original_range()
    , state(adjusting_state::NONE)
{}
