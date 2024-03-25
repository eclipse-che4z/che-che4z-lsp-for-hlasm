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

#include <memory>
#include <utility>

#include "diagnosable_ctx.h"
#include "library_info.h"

namespace hlasm_plugin::parser_library::context {
struct code_scope;
class system_variable_map;
const code_scope& get_current_scope(const context::hlasm_context&);
} // namespace hlasm_plugin::parser_library::context
namespace hlasm_plugin::parser_library::expressions {

// structure holding required objects to correctly perform evaluation of expressions
struct evaluation_context
{
    context::hlasm_context& hlasm_ctx;
    const library_info& lib_info;
    diagnostic_op_consumer& diags;
    const context::code_scope* scope = nullptr;
    const context::system_variable_map* sysvars = nullptr;

    evaluation_context(context::hlasm_context& ctx,
        const library_info& lib_info,
        diagnostic_op_consumer& diags,
        const context::code_scope& scope,
        const context::system_variable_map& sysvars)
        : hlasm_ctx(ctx)
        , lib_info(lib_info)
        , diags(diags)
        , scope(&scope)
        , sysvars(&sysvars)
    {}

    evaluation_context(context::hlasm_context& ctx, const library_info& lib_info, diagnostic_op_consumer& diags)
        : hlasm_ctx(ctx)
        , lib_info(lib_info)
        , diags(diags)
    {}

    evaluation_context(const evaluation_context& oth) = delete;

    const context::code_scope& active_scope() const
    {
        if (scope)
            return *scope;
        return context::get_current_scope(hlasm_ctx);
    }
};

} // namespace hlasm_plugin::parser_library::expressions

#endif
