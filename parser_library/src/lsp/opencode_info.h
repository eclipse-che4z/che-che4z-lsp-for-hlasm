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

#include "context/hlasm_context.h"
#include "macro_info.h"

namespace hlasm_plugin::parser_library::lsp {

struct opencode_info;

using opencode_info_ptr = std::unique_ptr<opencode_info>;

struct opencode_info
{
    vardef_storage variable_definitions;
    context::hlasm_context& hlasm_ctx;

    opencode_info(vardef_storage variable_definitions, context::hlasm_context& hlasm_ctx)
        : variable_definitions(std::move(variable_definitions))
        , hlasm_ctx(hlasm_ctx)
    {}
};

} // namespace hlasm_plugin::parser_library::lsp

#endif