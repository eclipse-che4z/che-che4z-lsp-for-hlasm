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

namespace hlasm_plugin::parser_library::lsp {

void lsp_context::add_file(file_info file_i)
{
    std::string name = file_i.name;
    files_.emplace(std::move(name), std::move(file_i));
}

void lsp_context::add_copy(context::copy_member_ptr copy) { add_file(file_info(std::move(copy))); }

void lsp_context::add_macro(macro_info_ptr macro_i)
{
    if (macro_i->external)
        add_file(file_info(macro_i->macro_definition));
    distribute_macro_i(*macro_i);
}

void lsp_context::distribute_macro_i(const macro_info& macro_i)
{
    for (const auto& [file, slices] : macro_i.file_scopes_)
        files_[file].update_slices(file_slice_t::transform_slices(slices, macro_i));

    for (const auto& [file, occs] : macro_i.file_occurences_)
        files_[file].update_occurences(occs);
}

} // namespace hlasm_plugin::parser_library::lsp
