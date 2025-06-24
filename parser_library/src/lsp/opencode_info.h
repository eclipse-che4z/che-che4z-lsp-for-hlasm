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

#ifndef LSP_OPENCODE_INFO_H
#define LSP_OPENCODE_INFO_H

#include "macro_info.h"

namespace hlasm_plugin::parser_library::lsp {

struct opencode_info;

using opencode_info_ptr = std::unique_ptr<opencode_info>;

struct opencode_info
{
    vardef_storage variable_definitions;
    file_occurrences_t file_occurrences;

    opencode_info(vardef_storage variable_definitions, file_occurrences_t file_occurrences)
        : variable_definitions(std::move(variable_definitions))
        , file_occurrences(std::move(file_occurrences))
    {}
};

} // namespace hlasm_plugin::parser_library::lsp

#endif
