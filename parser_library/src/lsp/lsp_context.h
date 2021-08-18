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

#include "completion_item.h"
#include "document_symbol_item.h"
#include "feature_provider.h"
#include "file_info.h"
#include "location.h"
#include "opencode_info.h"

namespace hlasm_plugin::parser_library::lsp {

class lsp_context final : public feature_provider
{
    opencode_info_ptr opencode_;
    std::unordered_map<std::string, file_info_ptr> files_;
    std::unordered_map<context::macro_def_ptr, macro_info_ptr> macros_;

    std::shared_ptr<context::hlasm_context> hlasm_ctx_;

public:
    explicit lsp_context(std::shared_ptr<context::hlasm_context> h_ctx);

    void add_copy(context::copy_member_ptr copy, text_data_ref_t text_data);
    void add_macro(macro_info_ptr macro_i, text_data_ref_t text_data = text_data_ref_t());
    void add_opencode(opencode_info_ptr opencode_i, text_data_ref_t text_data);

    [[nodiscard]] macro_info_ptr get_macro_info(context::id_index macro_name) const;
    [[nodiscard]] const file_info* get_file_info(const std::string& file_name) const;

    location definition(const std::string& document_uri, position pos) const override;
    location_list references(const std::string& document_uri, position pos) const override;
    hover_result hover(const std::string& document_uri, position pos) const override;
    completion_list_s completion(const std::string& document_uri,
        position pos,
        char trigger_char,
        completion_trigger_kind trigger_kind) const override;
    document_symbol_list_s document_symbol(const std::string& document_uri) const override;

private:
    void add_file(file_info file_i);
    void distribute_macro_i(macro_info_ptr macro_i);
    void distribute_file_occurences(const file_occurences_t& occurences);

    occurence_scope_t find_occurence_with_scope(const std::string& document_uri, const position pos) const;

    std::optional<location> find_definition_location(const symbol_occurence& occ, macro_info_ptr macro_i) const;
    hover_result find_hover(const symbol_occurence& occ, macro_info_ptr macro_i) const;

    completion_list_s complete_var(const file_info& file, position pos) const;
    completion_list_s complete_seq(const file_info& file, position pos) const;
    completion_list_s complete_instr(const file_info& file, position pos) const;

    bool is_continued_line(std::string_view line) const;
    bool should_complete_instr(const text_data_ref_t& text, const position pos) const;
    std::string get_macro_documentation(const macro_info& m) const;

    document_symbol_list_s document_symbol_macro(const std::string& document_uri) const;
    document_symbol_list_s document_symbol_macro(const std::string& document_uri, const range& r) const;
    document_symbol_list_s document_symbol_copy(
        const std::vector<symbol_occurence>& occurence_list, const std::string& document_uri) const;
    document_symbol_list_s document_symbol_copy(
        const std::vector<symbol_occurence>& occurence_list, const std::string& document_uri, const range& r) const;
    std::vector<std::pair<symbol_occurence, std::vector<context::id_index>>> copy_occurences(
        const std::string& document_uri) const;
    void modify_with_copy(document_symbol_list_s& modified,
        const context::id_index& sym_name,
        const std::vector<std::pair<symbol_occurence, std::vector<context::id_index>>>& copy_occs,
        const document_symbol_kind kind) const;
    std::string find_macro_copy_id(const context::processing_stack_t& stack, unsigned long i) const;
    void document_symbol_symbol(document_symbol_list_s& modified,
        const document_symbol_list_s& children,
        const context::id_index& id,
        const context::symbol& sym,
        const document_symbol_kind kind,
        unsigned long i) const;
    document_symbol_list_s document_symbol_opencode_ord_symbol() const;
    void document_symbol_opencode_var_seq_symbol(const std::string& document_uri, document_symbol_list_s& result) const;
    void document_symbol_opencode_var_seq_symbol_aux(document_symbol_list_s& result) const;
    bool belongs_to_copyfile(const std::string& document_uri, position pos, const context::id_index& id) const;
};

} // namespace hlasm_plugin::parser_library::lsp

#endif