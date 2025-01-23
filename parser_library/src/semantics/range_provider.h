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

#ifndef SEMANTICS_RANGE_PROVIDER_H
#define SEMANTICS_RANGE_PROVIDER_H

#include <utility>
#include <vector>

#include "lexing/logical_line.h"
#include "range.h"

namespace hlasm_plugin::parser_library::semantics {

// state of range adjusting
enum class adjusting_state
{
    NONE,
    SUBSTITUTION,
    MACRO_REPARSE,
    MODEL_REPARSE,
};

// structure for computing range
struct range_provider
{
public:
    range original_range;
    std::vector<range> original_operand_ranges;
    std::vector<std::pair<std::pair<size_t, bool>, range>> model_substitutions;
    std::vector<size_t> line_limits;
    adjusting_state state;
    size_t m_continued_code_line_column = 15;

    explicit range_provider(range original_field_range, adjusting_state state, size_t continued_code_line_column = 15);
    explicit range_provider(range original_field_range,
        std::vector<range> original_operand_ranges,
        adjusting_state state,
        std::vector<size_t> line_limits,
        size_t continued_code_line_column = 15);
    explicit range_provider(
        std::vector<std::pair<std::pair<size_t, bool>, range>> model_substitutions, std::vector<size_t> line_limits);
    explicit range_provider();

    [[nodiscard]] range adjust_range(range r) const noexcept;

private:
    [[nodiscard]] position adjust_position(position pos, bool end) const noexcept;
    [[nodiscard]] position adjust_model_position(position pos, bool end) const noexcept;

    [[nodiscard]] size_t get_line_limit(size_t relative_line) const noexcept;
};

template<typename It>
range text_range(const It& b, const It& e, size_t lineno_offset)
{
    assert(std::ranges::distance(b, e) >= 0);

    const auto [bx, by] = b.get_coordinates();
    position b_pos(by + lineno_offset, bx);
    if (b == e) // empty range
        return range(std::move(b_pos));
    else
    {
        const auto [ex, ey] = std::prev(e).get_coordinates();
        return range(std::move(b_pos), position(ey + lineno_offset, ex + 1));
    }
}

} // namespace hlasm_plugin::parser_library::semantics
#endif
