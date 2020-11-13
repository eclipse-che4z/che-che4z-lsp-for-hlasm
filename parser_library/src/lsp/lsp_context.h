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

#include "feature_provider.h"
#include "file_info.h"
#include "opencode_info.h"

namespace hlasm_plugin::parser_library::lsp {

class lsp_context : public feature_provider
{
    opencode_info_ptr opencode_;
    std::unordered_map<std::string, file_info_ptr> files_;
    std::unordered_map<context::id_index, macro_info_ptr> macros_;

public:
    void add_copy(context::copy_member_ptr copy);
    void add_macro(macro_info_ptr macro_i);
    void add_opencode(opencode_info_ptr opencode_i);

    void update_file_info(const std::string& name, const occurence_storage& occurences);

    virtual position_uri definition(const char* document_uri, const position pos) override;
    virtual position_uris references(const char* document_uri, const position pos) override;
    virtual string_array hover(const char* document_uri, const position pos) override;
    virtual completion_list completion(
        const char* document_uri, const position pos, const char trigger_char, int trigger_kind) override;

private:
    void add_file(file_info file_i);
    void distribute_macro_i(macro_info_ptr macro_i);

    std::optional<location> find_definition_position(const symbol_occurence& occ, macro_info_ptr macro_i);
};

} // namespace hlasm_plugin::parser_library::lsp

#endif