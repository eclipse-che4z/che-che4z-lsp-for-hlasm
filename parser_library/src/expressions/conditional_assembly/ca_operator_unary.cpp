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

#include "ca_operator_unary.h"

#include <algorithm>

#include "ebcdic_encoding.h"
#include "expressions/evaluation_context.h"
#include "terms/ca_function.h"

namespace hlasm_plugin::parser_library::expressions {

ca_unary_operator::ca_unary_operator(ca_expr_ptr expr, context::SET_t_enum expr_kind, range expr_range)
    : ca_expression(expr_kind, std::move(expr_range))
    , expr(std::move(expr))
{}

bool ca_unary_operator::get_undefined_attributed_symbols(
    std::vector<context::id_index>& symbols, const evaluation_context& eval_ctx) const
{
    return expr->get_undefined_attributed_symbols(symbols, eval_ctx);
}

void ca_unary_operator::resolve_expression_tree(ca_expression_ctx expr_ctx, diagnostic_op_consumer& diags)
{
    if (expr_kind != expr_ctx.kind)
        diags.add_diagnostic(diagnostic_op::error_CE004(expr_range));

    expr->resolve_expression_tree(expr_ctx, diags);
}

bool ca_unary_operator::is_character_expression(character_expression_purpose purpose) const
{
    return purpose == character_expression_purpose::assignment && expr_kind == context::SET_t_enum::C_TYPE;
}

void ca_unary_operator::apply(ca_expr_visitor& visitor) const { expr->apply(visitor); }

context::SET_t ca_unary_operator::evaluate(const evaluation_context& eval_ctx) const
{
    return operation(expr->evaluate(eval_ctx), eval_ctx);
}

ca_function_unary_operator::ca_function_unary_operator(ca_expr_ptr expr,
    ca_expr_ops function,
    context::SET_t_enum kind,
    range expr_range,
    context::SET_t_enum parent_expr_kind)
    : ca_unary_operator(std::move(expr), kind, std::move(expr_range))
    , function(function)
    , m_expr_ctx { kind, parent_expr_kind, true }
{}
void ca_function_unary_operator::resolve_expression_tree(ca_expression_ctx expr_ctx, diagnostic_op_consumer& diags)
{
    expr_kind = expr_ctx.kind;
    m_expr_ctx = expr_ctx;
    expr_ctx.kind = ca_common_expr_policy::get_operands_type(function, expr_ctx.kind);
    expr->resolve_expression_tree(expr_ctx, diags);
}

context::SET_t ca_function_unary_operator::operation(context::SET_t operand, const evaluation_context& eval_ctx) const
{
    if (function == ca_expr_ops::NOT)
    {
        if (m_expr_ctx.parent_expr_kind == context::SET_t_enum::A_TYPE)
            return convert_return_types(~operand.access_a(), expr_kind, eval_ctx);
        else if (m_expr_ctx.parent_expr_kind == context::SET_t_enum::B_TYPE)
            return convert_return_types(!operand.access_b(), expr_kind, eval_ctx);
    }
    else if (expr_kind == context::SET_t_enum::C_TYPE)
    {
        diagnostic_adder add_diagnostic(eval_ctx.diags, expr_range);
        switch (function)
        {
            case ca_expr_ops::BYTE:
                return ca_function::BYTE(operand.access_a(), add_diagnostic);
            case ca_expr_ops::DOUBLE:
                return ca_function::DOUBLE(operand.access_c(), add_diagnostic);
            case ca_expr_ops::LOWER:
                return ca_function::LOWER(std::move(operand.access_c()));
            case ca_expr_ops::SIGNED:
                return ca_function::SIGNED(operand.access_a());
            case ca_expr_ops::UPPER:
                return ca_function::UPPER(std::move(operand.access_c()));
            default:
                break;
        }
    }
    return context::SET_t(expr_kind);
}

ca_plus_operator::ca_plus_operator(ca_expr_ptr expr, range expr_range)
    : ca_unary_operator(std::move(expr), context::SET_t_enum::A_TYPE, std::move(expr_range))
{}

context::SET_t ca_plus_operator::operation(context::SET_t operand, const evaluation_context&) const
{
    return operand.access_a();
}

ca_minus_operator::ca_minus_operator(ca_expr_ptr expr, range expr_range)
    : ca_unary_operator(std::move(expr), context::SET_t_enum::A_TYPE, std::move(expr_range))
{}

context::SET_t ca_minus_operator::operation(context::SET_t operand, const evaluation_context&) const
{
    return -operand.access_a();
}

ca_par_operator::ca_par_operator(ca_expr_ptr expr, range expr_range)
    : ca_unary_operator(std::move(expr), context::SET_t_enum::UNDEF_TYPE, std::move(expr_range))
{}

void ca_par_operator::resolve_expression_tree(ca_expression_ctx expr_ctx, diagnostic_op_consumer& diags)
{
    expr->resolve_expression_tree(expr_ctx, diags);
    expr_kind = expr->expr_kind;
}

context::SET_t ca_par_operator::operation(context::SET_t operand, const evaluation_context&) const { return operand; }

} // namespace hlasm_plugin::parser_library::expressions
