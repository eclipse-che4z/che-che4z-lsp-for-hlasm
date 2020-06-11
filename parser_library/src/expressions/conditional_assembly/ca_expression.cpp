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
#include "terms/ca_constant.h"

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
    if (type == context::SET_t_enum::A_TYPE)
    {
        switch (retval.type)
        {
            case context::SET_t_enum::A_TYPE:
                return retval;
            case context::SET_t_enum::B_TYPE:
                return retval.access_b() ? 1 : 0;
            case context::SET_t_enum::C_TYPE:
                return expressions::ca_constant::self_defining_term(
                    retval.access_c(), ranged_diagnostic_collector(&eval_ctx, expr_range));
            default:
                eval_ctx.add_diagnostic(diagnostic_op::error_CE004(expr_range));
                return context::SET_t();
        }
    }
    else if (type != retval.type)
    {
        eval_ctx.add_diagnostic(diagnostic_op::error_CE004(expr_range));
        return context::SET_t();
    }
    return std::move(retval);
}

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin
