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

#include "diagnosable_ctx.h"
#include "workspaces/parse_lib_provider.h"

namespace hlasm_plugin::parser_library::expressions {

// structure holding required objects to correcly perform evaluation of expressions
struct evaluation_context : public diagnosable_ctx
{
    analyzing_context ctx;
    context::hlasm_context& hlasm_ctx;
    workspaces::parse_lib_provider& lib_provider;

    evaluation_context(analyzing_context ctx, workspaces::parse_lib_provider& lib_provider)
        : diagnosable_ctx(*ctx.hlasm_ctx)
        , ctx(ctx)
        , hlasm_ctx(*ctx.hlasm_ctx)
        , lib_provider(lib_provider)
    {}

    evaluation_context(const evaluation_context& oth)
        : diagnosable_ctx(oth)
        , ctx(oth.ctx)
        , hlasm_ctx(*ctx.hlasm_ctx)
        , lib_provider(oth.lib_provider)
    {}

    void collect_diags() const override
    {
        // nothing to collect
    }
};

} // namespace hlasm_plugin::parser_library::expressions

#endif
