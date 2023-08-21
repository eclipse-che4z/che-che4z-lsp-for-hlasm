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

#ifndef LSP_CONTEXT_H
#define LSP_CONTEXT_H

#include <memory>
#include <span>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "completion_list_source.h"
#include "context/id_index.h"
#include "context/macro.h"
#include "document_symbol_item.h"
#include "file_info.h"
#include "location.h"
#include "opencode_info.h"
#include "range.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::parser_library::workspaces {
class parse_lib_provider;
} // namespace hlasm_plugin::parser_library::workspaces

namespace hlasm_plugin::parser_library::lsp {

class lsp_context final
{
    opencode_info_ptr m_opencode;
    std::unordered_map<utils::resource::resource_location, file_info_ptr, utils::resource::resource_location_hasher>
        m_files;
    std::unordered_map<context::macro_def_ptr, macro_info_ptr> m_macros;

    std::shared_ptr<context::hlasm_context> m_hlasm_ctx;

    std::unordered_map<context::id_index, utils::resource::resource_location> m_instr_like;

    template<typename T>
    struct vector_set
    {
        std::vector<T> data;

        bool contains(const T& v) const { return std::binary_search(data.begin(), data.end(), v); }
    };

    struct document_symbol_cache
    {
        std::unordered_map<utils::resource::resource_location,
            std::vector<std::pair<symbol_occurrence, vector_set<context::id_index>>>,
            utils::resource::resource_location_hasher>
            occurrences;

        std::unordered_map<const file_info*, std::vector<const symbol_occurrence*>> occurrences_by_name;
    };

public:
    explicit lsp_context(std::shared_ptr<context::hlasm_context> h_ctx);

    void add_copy(context::copy_member_ptr copy, text_data_view text_data);
    void add_macro(macro_info_ptr macro_i, text_data_view text_data = text_data_view());
    void add_opencode(opencode_info_ptr opencode_i, text_data_view text_data, workspaces::parse_lib_provider& libs);

    [[nodiscard]] macro_info_ptr get_macro_info(
        context::id_index macro_name, context::opcode_generation gen = context::opcode_generation::current) const;
    [[nodiscard]] const file_info* get_file_info(const utils::resource::resource_location& file_loc) const;

    location definition(const utils::resource::resource_location& document_loc, position pos) const;
    location_list references(const utils::resource::resource_location& document_loc, position pos) const;
    std::string hover(const utils::resource::resource_location& document_loc, position pos) const;
    completion_list_source completion(const utils::resource::resource_location& document_uri,
        position pos,
        char trigger_char,
        completion_trigger_kind trigger_kind) const;
    document_symbol_list_s document_symbol(
        const utils::resource::resource_location& document_loc, long long limit) const;

    const context::hlasm_context& get_related_hlasm_context() const { return *m_hlasm_ctx; }

    const std::unordered_map<context::macro_def_ptr, macro_info_ptr>& macros() const { return m_macros; };

private:
    void add_file(file_info file_i);
    void distribute_macro_i(macro_info_ptr macro_i);
    void distribute_file_occurrences(const file_occurrences_t& occurrences);

    occurrence_scope_t find_occurrence_with_scope(
        const utils::resource::resource_location& document_loc, position pos) const;
    const line_occurence_details* find_line_details(
        const utils::resource::resource_location& document_loc, size_t l) const;

    std::optional<location> find_definition_location(const symbol_occurrence& occ,
        macro_info_ptr macro_i,
        const utils::resource::resource_location& document_loc,
        position pos) const;
    location find_symbol_definition_location(
        const context::symbol& sym, const utils::resource::resource_location& document_loc, position pos) const;
    std::string find_hover(
        const symbol_occurrence& occ, macro_info_ptr macro_i, const line_occurence_details* ld) const;

    completion_list_source complete_var(const file_info& file, position pos) const;
    completion_list_source complete_seq(const file_info& file, position pos) const;
    completion_list_source complete_instr(const file_info& file, position pos) const;

    bool should_complete_instr(const text_data_view& text, position pos) const;

    void document_symbol_macro(document_symbol_list_s& result,
        const utils::resource::resource_location& document_loc,
        std::optional<range> r,
        long long& limit,
        document_symbol_cache& cache) const;
    void document_symbol_copy(document_symbol_list_s& result,
        const std::vector<symbol_occurrence>& occurrence_list,
        const utils::resource::resource_location& document_loc,
        std::optional<range> r,
        long long& limit) const;
    void document_symbol_other(document_symbol_list_s& result,
        const utils::resource::resource_location& document_loc,
        long long& limit,
        document_symbol_cache& cache) const;

    const std::vector<std::pair<symbol_occurrence, vector_set<context::id_index>>>& copy_occurrences(
        const utils::resource::resource_location& document_loc, document_symbol_cache& cache) const;
    void fill_cache(
        std::vector<std::pair<symbol_occurrence, lsp_context::vector_set<context::id_index>>>& copy_occurrences,
        const utils::resource::resource_location& document_loc,
        document_symbol_cache& cache) const;
    std::span<const symbol_occurrence* const> get_occurrences_by_name(
        const file_info& document, context::id_index name, document_symbol_cache& cache) const;

    void modify_with_copy(document_symbol_list_s& modified,
        context::id_index sym_name,
        const std::vector<std::pair<symbol_occurrence, lsp_context::vector_set<context::id_index>>>& copy_occs,
        const document_symbol_kind kind,
        long long& limit) const;
    std::string find_macro_copy_id(const std::vector<context::processing_frame>& stack, unsigned long i) const;
    void document_symbol_symbol(document_symbol_list_s& modified,
        document_symbol_list_s children,
        context::id_index id,
        const std::vector<context::processing_frame>& sym_stack,
        const document_symbol_kind kind,
        unsigned long i,
        long long& limit) const;
    void document_symbol_opencode_ord_symbol(document_symbol_list_s& result, long long& limit) const;
    void document_symbol_opencode_var_seq_symbol_aux(document_symbol_list_s& result,
        const std::unordered_map<std::string_view, utils::resource::resource_location>& name_to_location_cache,
        long long& limit,
        document_symbol_cache& cache) const;
    bool belongs_to_copyfile(
        const utils::resource::resource_location& document_loc, position pos, context::id_index id) const;

    std::string hover_for_macro(const macro_info& macro) const;
    std::string hover_for_instruction(context::id_index name) const;

    bool have_suggestions_for_instr_like(context::id_index name) const;
};

} // namespace hlasm_plugin::parser_library::lsp

#endif
