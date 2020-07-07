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

#include "ca_expression.h"

#include "expressions/evaluation_context.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

ca_expression::ca_expression(context::SET_t_enum expr_kind, range expr_range)
    : expr_range(std::move(expr_range))
    , expr_kind(expr_kind)
{ }

context::SET_t ca_expression::convert_return_types(
    context::SET_t retval, context::SET_t_enum type, evaluation_context& eval_ctx) const
{
    if (type != retval.type)
    {
        if (retval.type == context::SET_t_enum::A_TYPE && type == context::SET_t_enum::B_TYPE
            && (retval.access_a() == 0 || retval.access_a() == 1))
            return retval.access_a() == 1;

        eval_ctx.add_diagnostic(diagnostic_op::error_CE004(expr_range));
        return context::SET_t(expr_kind);
    }
    return std::move(retval);
}

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin
