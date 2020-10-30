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

#include <variant>

#include "context/copy_member.h"
#include "macro_info.h"
#include "symbol_occurence.h"

namespace hlasm_plugin::parser_library::lsp {

enum class scope_type
{
    OPENCODE,
    MACRO,
    INNER_MACRO
};

struct file_slice_t
{
    scope_type type;

    macro_info_ptr macro_context;

    size_t begin_idx, end_idx;
    size_t begin_line, end_line;
};

struct file_info
{
    using owner_t = std::variant<macro_info_ptr, context::copy_member_ptr>;

    const std::string name;
    owner_t owner;

    std::vector<file_slice_t> slices;
    std::vector<symbol_occurence> occurences;

    file_info(macro_info_ptr macro_i, std::vector<macro_slice_t> slices, std::vector<symbol_occurence> occurences);
    file_info(context::copy_member_ptr owner);

    void update_slices(std::vector<macro_slice_t> slices);

    static file_slice_t transform_slice(const macro_slice_t& slice, macro_info_ptr macro_i);
    static std::vector<file_slice_t> transform_slices(const std::vector<macro_slice_t>& slices, macro_info_ptr macro_i);
};

} // namespace hlasm_plugin::parser_library::lsp

#endif