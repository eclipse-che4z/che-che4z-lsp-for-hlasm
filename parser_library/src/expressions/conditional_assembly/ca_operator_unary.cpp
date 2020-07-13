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

undef_sym_set ca_unary_operator::get_undefined_attributed_symbols(const context::dependency_solver& solver) const
{
    return expr->get_undefined_attributed_symbols(solver);
}

void ca_unary_operator::resolve_expression_tree(context::SET_t_enum kind)
{
    if (expr_kind != kind)
        add_diagnostic(diagnostic_op::error_CE004(expr_range));
    else
        expr->resolve_expression_tree(kind);
}

void ca_unary_operator::collect_diags() const { collect_diags_from_child(*expr); }

bool ca_unary_operator::is_character_expression() const { return false; }

context::SET_t ca_unary_operator::evaluate(evaluation_context& eval_ctx) const
{
    return operation(expr->evaluate(eval_ctx), eval_ctx);
}

ca_function_unary_operator::ca_function_unary_operator(
    ca_expr_ptr expr, ca_expr_ops function, context::SET_t_enum kind, range expr_range)
    : ca_unary_operator(std::move(expr), kind, std::move(expr_range))
    , function(function)
{}

void ca_function_unary_operator::resolve_expression_tree(context::SET_t_enum kind)
{
    if (expr_kind != kind)
        add_diagnostic(diagnostic_op::error_CE004(expr_range));
    else
        expr->resolve_expression_tree(ca_common_expr_policy::get_operands_type(function, kind));
}

context::SET_t ca_function_unary_operator::operation(context::SET_t operand, evaluation_context& eval_ctx) const
{
    if (expr_kind == context::SET_t_enum::A_TYPE)
    {
        if (function == ca_expr_ops::NOT)
            return ~operand.access_a();
    }
    else if (expr_kind == context::SET_t_enum::B_TYPE)
    {
        if (function == ca_expr_ops::NOT)
            return !operand.access_b();
    }
    else if (expr_kind == context::SET_t_enum::C_TYPE)
    {
        ranged_diagnostic_collector add_diagnostic(&eval_ctx, expr_range);
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

context::SET_t ca_plus_operator::operation(context::SET_t operand, evaluation_context&) const
{
    return operand.access_a();
}

ca_minus_operator::ca_minus_operator(ca_expr_ptr expr, range expr_range)
    : ca_unary_operator(std::move(expr), context::SET_t_enum::A_TYPE, std::move(expr_range))
{}

context::SET_t ca_minus_operator::operation(context::SET_t operand, evaluation_context&) const
{
    return -operand.access_a();
}

ca_par_operator::ca_par_operator(ca_expr_ptr expr, range expr_range)
    : ca_unary_operator(std::move(expr), context::SET_t_enum::UNDEF_TYPE, std::move(expr_range))
{}

void ca_par_operator::resolve_expression_tree(context::SET_t_enum kind)
{
    expr->resolve_expression_tree(kind);
    expr_kind = expr->expr_kind;
}

context::SET_t ca_par_operator::operation(context::SET_t operand, evaluation_context&) const { return operand; }

} // namespace hlasm_plugin::parser_library::expressions
