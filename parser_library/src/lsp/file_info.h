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

#ifndef LSP_FILE_INFO_H
#define LSP_FILE_INFO_H

#include <functional>
#include <map>
#include <memory>
#include <span>
#include <utility>
#include <variant>
#include <vector>

#include "context/copy_member.h"
#include "macro_info.h"
#include "symbol_occurence.h"
#include "text_data_view.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::parser_library::lsp {

enum class scope_type
{
    OPENCODE,
    MACRO,
    INNER_MACRO
};

struct line_range
{
    size_t begin;
    size_t end;
};

bool operator==(const line_range& lhs, const line_range& rhs);
bool operator<(const line_range& lhs, const line_range& rhs);

struct file_slice_t
{
    scope_type type;

    macro_info_ptr macro_context;

    // range of slice within macro definition
    line_range macro_lines;
    // range of slice within file
    line_range file_lines;

    static file_slice_t transform_slice(const macro_slice_t& slice, macro_info_ptr macro_i);
    static std::vector<file_slice_t> transform_slices(const std::vector<macro_slice_t>& slices, macro_info_ptr macro_i);
};

enum class file_type
{
    MACRO,
    COPY,
    OPENCODE
};

class file_info;

using file_info_ptr = std::unique_ptr<file_info>;

using occurence_scope_t = std::pair<const symbol_occurence*, macro_info_ptr>;

class file_info
{
public:
    // first variant is monostate as there is no storing of opencode statements in the code yet
    using owner_t = std::variant<std::monostate, context::macro_def_ptr, context::copy_member_ptr>;

    utils::resource::resource_location location;
    file_type type;
    owner_t owner;
    text_data_view data;


    explicit file_info(utils::resource::resource_location location, text_data_view text_data);
    explicit file_info(context::macro_def_ptr owner, text_data_view text_data);
    explicit file_info(context::copy_member_ptr owner, text_data_view text_data);

    static bool is_in_range(const position& pos, const range& r);

    occurence_scope_t find_occurence_with_scope(position pos) const;
    macro_info_ptr find_scope(position pos) const;
    static std::vector<position> find_references(
        const symbol_occurence& occurence, const std::vector<symbol_occurence>& occurences);

    void update_occurences(const occurence_storage& occurences_upd);
    void update_slices(const std::vector<file_slice_t>& slices);
    const std::vector<symbol_occurence>& get_occurences() const;
    void process_occurrences();
    void collect_instruction_like_references(
        std::unordered_map<context::id_index, utils::resource::resource_location>& m) const;

private:
    std::map<line_range, file_slice_t> slices;
    std::vector<symbol_occurence> occurences;
    std::vector<size_t> occurences_start_limit;
};

} // namespace hlasm_plugin::parser_library::lsp

#endif
