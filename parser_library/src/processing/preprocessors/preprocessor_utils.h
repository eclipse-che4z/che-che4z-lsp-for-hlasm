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

#ifndef HLASMPARSER_PARSERLIBRARY_PROCESSING_PREPROCESSOR_UTILS_H
#define HLASMPARSER_PARSERLIBRARY_PROCESSING_PREPROCESSOR_UTILS_H

#include <array>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "semantics/range_provider.h"
#include "semantics/statement.h"

namespace hlasm_plugin::parser_library::processing {

struct stmt_part_ids
{
    std::optional<size_t> label;
    std::vector<size_t> instruction;
    size_t operands;
    std::optional<size_t> remarks;
};

// This function returns a list of operands with their ranges while expecting to receive a string_view of a single line
// where operands are separated by spaces or commas
std::vector<semantics::preproc_details::name_range> get_operands_list(
    std::string_view operands, size_t op_column_start, const semantics::range_provider& rp);

template<size_t n>
auto make_preproc_matches(auto&& m)
{
    using it = decltype(m.suffix().first);
    std::array<std::pair<it, it>, 1 + n + 1> result { std::make_pair(m.suffix().first, m.suffix().second) };
    for (size_t i = 0; i <= n; ++i)
        result[1 + i] = std::make_pair(m[i].first, m[i].second);

    return result;
}

template<typename PREPROC_STATEMENT, typename ITERATOR>
std::shared_ptr<PREPROC_STATEMENT> get_preproc_statement(std::span<const std::pair<ITERATOR, ITERATOR>> matches,
    const stmt_part_ids& ids,
    size_t lineno,
    bool contains_preproc_specific_instruction,
    size_t continue_column = 15);

} // namespace hlasm_plugin::parser_library::processing

#endif
