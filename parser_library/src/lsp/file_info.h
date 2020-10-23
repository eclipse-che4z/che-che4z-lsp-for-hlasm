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

#include "macro_info.h"
#include "symbol_occurence.h"

namespace hlasm_plugin::parser_library::lsp {

enum class scope_type
{
    OPENCODE,
    MACRO,
    INNER_MACRO
};

struct file_slice
{
    scope_type type;

    macro_info_ptr macro_context;

    size_t begin_idx, end_idx;
    size_t begin_line, end_line;
};

struct file_info
{
    const std::string name;

    std::vector<file_slice> slices;
    std::vector<symbol_occurence> occurences;

    void update_slices(std::vector<macro_slices> slices);
};

} // namespace hlasm_plugin::parser_library::lsp

#endif