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

#include <memory>
#include <optional>
#include <regex>
#include <string_view>
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

template<typename PREPROC_STATEMENT, typename ITERATOR>
std::shared_ptr<PREPROC_STATEMENT> get_preproc_statement(
    const std::match_results<ITERATOR>& matches, const stmt_part_ids& ids, size_t lineno, size_t continue_column = 15);

template<typename It>
static std::true_type same_line_detector(const It& t, decltype(t.same_line(t)) = false);
static std::false_type same_line_detector(...);

template<typename It>
static bool same_line(const It& l, const It& r)
{
    if constexpr (decltype(same_line_detector(l))::value)
        return l.same_line(r);
    else
        return true;
}

} // namespace hlasm_plugin::parser_library::processing

#endif