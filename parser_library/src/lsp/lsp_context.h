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

#include "branch_info.h"
#include "completion_list_source.h"
#include "context/id_index.h"
#include "context/macro.h"
#include "document_symbol_item.h"
#include "file_info.h"
#include "location.h"
#include "opencode_info.h"
#include "range.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::parser_library {
enum class completion_trigger_kind;
class parse_lib_provider;
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::lsp {

class lsp_context final
{
    opencode_info_ptr m_opencode;
    std::unordered_map<utils::resource::resource_location, file_info_ptr, utils::resource::resource_location_hasher>
        m_files;
    std::unordered_map<const context::macro_definition*, macro_info_ptr> m_macros;

    std::shared_ptr<context::hlasm_context> m_hlasm_ctx;

    std::unordered_map<context::id_index, utils::resource::resource_location> m_instr_like;

    template<typename T>
    struct vector_set
    {
        std::vector<T> data;

        bool contains(const T& v) const { return std::ranges::binary_search(data, v); }
    };

    struct title_details
    {
        std::string title;
        context::processing_stack_t stack;

        title_details(std::string title, context::processing_stack_t stack) // llvm-14
            : title(std::move(title))
            , stack(std::move(stack))
        {}
    };

    std::vector<title_details> m_titles;

public:
    explicit lsp_context(std::shared_ptr<context::hlasm_context> h_ctx);

    void add_copy(context::copy_member_ptr copy, text_data_view text_data);
    void add_macro(macro_info_ptr macro_i, text_data_view text_data = text_data_view());
    void add_opencode(opencode_info_ptr opencode_i, text_data_view text_data, parse_lib_provider& libs);
    void add_title(std::string title, context::processing_stack_t stack);

    [[nodiscard]] macro_info_ptr get_macro_info(
        context::id_index macro_name, context::opcode_generation gen = context::opcode_generation::current) const;
    [[nodiscard]] const file_info* get_file_info(const utils::resource::resource_location& file_loc) const;

    location definition(const utils::resource::resource_location& document_loc, position pos) const;
    std::vector<location> references(const utils::resource::resource_location& document_loc, position pos) const;
    std::string hover(const utils::resource::resource_location& document_loc, position pos) const;
    completion_list_source completion(const utils::resource::resource_location& document_uri,
        position pos,
        char trigger_char,
        completion_trigger_kind trigger_kind) const;
    std::vector<document_symbol_item> document_symbol(const utils::resource::resource_location& document_loc) const;

    const context::hlasm_context& get_related_hlasm_context() const { return *m_hlasm_ctx; }

    const std::unordered_map<const context::macro_definition*, macro_info_ptr>& macros() const { return m_macros; };

    std::vector<branch_info> get_opencode_branch_info() const;

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
    std::string find_hover(const symbol_occurrence& occ,
        macro_info_ptr macro_i,
        const line_occurence_details* ld,
        std::optional<location> definition) const;

    completion_list_source complete_var(const file_info& file, position pos) const;
    completion_list_source complete_seq(const file_info& file, position pos) const;
    completion_list_source complete_instr(const file_info& file, position pos) const;

    bool should_complete_instr(const text_data_view& text, position pos) const;

    std::string hover_for_macro(const macro_info& macro) const;
    std::string hover_for_instruction(context::id_index name) const;

    bool have_suggestions_for_instr_like(context::id_index name) const;
};

} // namespace hlasm_plugin::parser_library::lsp

#endif
