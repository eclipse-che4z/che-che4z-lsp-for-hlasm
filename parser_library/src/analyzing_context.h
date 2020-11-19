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

#ifndef HLASMPARSER_PARSERLIBRARY_ANALYZING_CONTEXT_H
#define HLASMPARSER_PARSERLIBRARY_ANALYZING_CONTEXT_H

#include "context/hlasm_context.h"
#include "lsp/lsp_context.h"

namespace hlasm_plugin {
namespace parser_library {

struct analyzing_context
{
    context::hlasm_ctx_ptr hlasm_ctx;
    lsp::lsp_ctx_ptr lsp_ctx;
};

} // namespace parser_library
} // namespace hlasm_plugin
#endif
