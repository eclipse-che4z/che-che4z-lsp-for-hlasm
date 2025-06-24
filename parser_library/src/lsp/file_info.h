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

#include <compare>
#include <utility>
#include <variant>
#include <vector>

#include "context/copy_member.h"
#include "context/statement_id.h"
#include "macro_info.h"
#include "symbol_occurrence.h"
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

    auto operator<=>(const line_range&) const noexcept = default;
};

struct macro_range
{
    context::statement_id begin;
    context::statement_id end;
};

struct file_slice_t
{
    scope_type type;

    const macro_info* macro_context;

    // range of slice within macro definition
    macro_range macro_lines;
    // range of slice within file
    line_range file_lines;

    static file_slice_t transform_slice(
        const macro_slice_t& slice, const macro_info& macro_i, const utils::resource::resource_location& f);
};

enum class file_type
{
    MACRO,
    COPY,
    OPENCODE
};

class file_info;

using occurrence_scope_t = std::pair<const symbol_occurrence*, const macro_info*>;

class file_info
{
public:
    // first variant is monostate as there is no storing of opencode statements in the code yet
    using owner_t = std::variant<std::monostate, context::macro_def_ptr, context::copy_member_ptr>;

    file_type type;
    owner_t owner;
    text_data_view data;


    explicit file_info(text_data_view text_data);
    explicit file_info(context::macro_def_ptr owner, text_data_view text_data);
    explicit file_info(context::copy_member_ptr owner, text_data_view text_data);

    static bool is_in_range(const position& pos, const range& r);

    occurrence_scope_t find_occurrence_with_scope(position pos) const;
    const macro_info* find_scope(position pos) const;
    static std::vector<position> find_references(
        const symbol_occurrence& occurrence, const std::vector<symbol_occurrence>& occurrences);

    void update_occurrences(const std::vector<symbol_occurrence>& occurrences_upd,
        const std::vector<line_occurence_details>& line_details_upd);
    const std::vector<symbol_occurrence>& get_occurrences() const;
    void process_occurrences();
    void collect_instruction_like_references(
        std::unordered_map<context::id_index, utils::resource::resource_location>& m) const;

    const symbol_occurrence* find_closest_instruction(position pos) const noexcept;
    std::pair<const context::section*, index_t<context::using_collection>> find_reachable_sections(position pos) const;

    const line_occurence_details* get_line_details(size_t l) const noexcept;
    const std::vector<line_occurence_details>& get_line_details() const noexcept { return line_details; }

    std::vector<bool> macro_map() const;

    static void distribute_macro_slices(
        const std::unordered_map<const context::macro_definition*, macro_info_ptr>& macros,
        std::unordered_map<utils::resource::resource_location, file_info>& files);

private:
    std::vector<file_slice_t> slices;
    std::vector<symbol_occurrence> occurrences;
    std::vector<line_occurence_details> line_details;
    std::vector<size_t> occurrences_start_limit;
};

} // namespace hlasm_plugin::parser_library::lsp

#endif
