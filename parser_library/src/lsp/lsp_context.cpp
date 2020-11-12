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

#include "lsp_context.h"

#include <cassert>

namespace hlasm_plugin::parser_library::lsp {

void lsp_context::add_file(file_info file_i)
{
    std::string name = file_i.name;
    files_.try_emplace(std::move(name), std::make_unique<file_info>(std::move(file_i)));
}

void lsp_context::add_copy(context::copy_member_ptr copy) { add_file(file_info(std::move(copy))); }

void lsp_context::add_macro(macro_info_ptr macro_i)
{
    if (macro_i->external)
        add_file(file_info(macro_i->macro_definition));
    distribute_macro_i(macro_i);
}

void lsp_context::add_opencode(opencode_info_ptr opencode_i) { opencode_ = std::move(opencode_i); }

void lsp_context::update_file_info(const std::string& name, const occurence_storage& occurences)
{
    assert(files_.find(name) != files_.end());
    files_[name]->update_occurences(occurences);
}

position_uri lsp_context::definition(const char* document_uri, const position pos) { return position_uri(); }

position_uris lsp_context::references(const char* document_uri, const position pos) { return position_uris(); }

string_array lsp_context::hover(const char* document_uri, const position pos) { return string_array(); }

completion_list lsp_context::completion(
    const char* document_uri, const position pos, const char trigger_char, int trigger_kind)
{
    return completion_list();
}

bool files_present(const std::unordered_map<std::string, file_info_ptr>& files,
    const macro_file_scopes_t& scopes,
    const macro_file_occurences_t& occs)
{
    bool present = true;
    for (const auto& [file, _] : scopes)
        present |= files.find(file) != files.end();
    for (const auto& [file, _] : occs)
        present |= files.find(file) != files.end();
    return present;
}

void lsp_context::distribute_macro_i(macro_info_ptr macro_i)
{
    assert(files_present(files_, macro_i->file_scopes_, macro_i->file_occurences_));

    for (const auto& [file, slices] : macro_i->file_scopes_)
        files_[file]->update_slices(file_slice_t::transform_slices(slices, macro_i));

    for (const auto& [file, occs] : macro_i->file_occurences_)
        files_[file]->update_occurences(occs);
}

} // namespace hlasm_plugin::parser_library::lsp
