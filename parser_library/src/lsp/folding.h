/*
 * Copyright (c) 2024 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_LSP_FOLDING_H
#define HLASMPLUGIN_PARSERLIBRARY_LSP_FOLDING_H

#include <span>
#include <string_view>
#include <vector>

#include "folding_range.h"

namespace hlasm_plugin::parser_library::lsp {

struct line_entry
{
    size_t lineno;
    size_t end_lineno;
    signed char comment_offset;
    signed char indent;
    bool comment : 1;
    bool blank : 1;
    bool blank_comment : 1;
    bool separator : 1;
    bool has_label : 1;
    bool suspicious : 1;

    bool operator==(const line_entry&) const noexcept = default;
};

struct fold_data
{
    size_t indentation = 0;
    size_t comment = 0;
    size_t notcomment = 0;
    bool small_structure = false;

    bool operator==(const fold_data&) const noexcept = default;
};

// pre: data.size()>=max lineno
void folding_by_indentation(std::vector<fold_data>& data, std::span<const line_entry> lines);
// pre: data.size()>=max lineno
void folding_by_comments(std::vector<fold_data>& data, std::span<const line_entry> lines);
// pre: data.size()>=max lineno
void folding_between_comments(std::vector<fold_data>& data, std::span<const line_entry> lines);
void adjust_folding_data(std::span<fold_data> data);

std::vector<line_entry> generate_indentation_map(std::string_view text);
void mark_suspicious(std::vector<lsp::line_entry>& lines);
std::vector<fold_data> compute_folding_data(std::span<const line_entry> lines);
std::vector<folding_range> generate_folding_ranges(std::span<const fold_data> data);

} // namespace hlasm_plugin::parser_library::lsp

#endif
