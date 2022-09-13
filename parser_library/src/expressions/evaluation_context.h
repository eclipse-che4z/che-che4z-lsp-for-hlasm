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

#ifndef HLASMPLUGIN_PARSER_HLASM_EVALUATION_CONTEXT_H
#define HLASMPLUGIN_PARSER_HLASM_EVALUATION_CONTEXT_H

#include "context/common_types.h"
#include "diagnosable_ctx.h"
#include "library_info.h"

namespace hlasm_plugin::parser_library::expressions {

// structure holding required objects to correctly perform evaluation of expressions
struct evaluation_context
{
    context::hlasm_context& hlasm_ctx;
    const library_info& lib_info;
    diagnostic_op_consumer& diags;

    evaluation_context(context::hlasm_context& ctx, const library_info& lib_info, diagnostic_op_consumer& diags)
        : hlasm_ctx(ctx)
        , lib_info(lib_info)
        , diags(diags)
    {}

    evaluation_context(const evaluation_context& oth) = delete;
};

} // namespace hlasm_plugin::parser_library::expressions

#endif
