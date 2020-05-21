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

#include "ca_operator_binary.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

ca_binary_operator::ca_binary_operator(
    ca_expr_ptr left_expr, ca_expr_ptr right_expr, context::SET_t_enum expr_kind, range expr_range)
    : ca_expression(expr_kind, std::move(expr_range))
    , left_expr(std::move(left_expr))
    , right_expr(std::move(right_expr))
{}

undef_sym_set ca_binary_operator::get_undefined_attributed_symbols(const context::dependency_solver& solver) const
{
    auto tmp = left_expr->get_undefined_attributed_symbols(solver);
    tmp.merge(right_expr->get_undefined_attributed_symbols(solver));
    return tmp;
}

void ca_binary_operator::resolve_expression_tree(context::SET_t_enum kind)
{
    if (expr_kind != kind)
        add_diagnostic(diagnostic_op::error_CE004(expr_range));
    else
    {
        left_expr->resolve_expression_tree(kind);
        right_expr->resolve_expression_tree(kind);
    }
}

void ca_binary_operator::collect_diags() const
{
    collect_diags_from_child(*left_expr);
    collect_diags_from_child(*right_expr);
}

bool ca_binary_operator::is_character_expression() const { return left_expr->is_character_expression(); }

ca_function_binary_operator::ca_function_binary_operator(ca_expr_ptr left_expr,
    ca_expr_ptr right_expr,
    ca_expr_ops function,
    context::SET_t_enum expr_kind,
    range expr_range)
    : ca_binary_operator(std::move(left_expr), std::move(right_expr), expr_kind, std::move(expr_range))
    , function(function)
{}

void ca_function_binary_operator::resolve_expression_tree(context::SET_t_enum kind)
{
    if (expr_kind != kind)
        add_diagnostic(diagnostic_op::error_CE004(expr_range));
    else if ((function == ca_expr_ops::FIND || function == ca_expr_ops::INDEX) && !left_expr->is_character_expression())
        add_diagnostic(diagnostic_op::error_CE004(left_expr->expr_range));
    else
    {
        context::SET_t_enum operands_kind;

        if (is_relational())
        {
            operands_kind =
                left_expr->is_character_expression() ? context::SET_t_enum::C_TYPE : context::SET_t_enum::A_TYPE;
        }
        else
            operands_kind = ca_common_expr_policy::get_operands_type(function, kind);

        left_expr->resolve_expression_tree(operands_kind);
        right_expr->resolve_expression_tree(operands_kind);
    }
}

bool ca_function_binary_operator::is_relational() const
{
    switch (function)
    {
        case ca_expr_ops::EQ:
        case ca_expr_ops::NE:
        case ca_expr_ops::LE:
        case ca_expr_ops::LT:
        case ca_expr_ops::GE:
        case ca_expr_ops::GT:
            return true;
        default:
            return false;
    }
}

ca_add_operator::ca_add_operator(ca_expr_ptr left_expr, ca_expr_ptr right_expr, range expr_range)
    : ca_binary_operator(
        std::move(left_expr), std::move(right_expr), context::SET_t_enum::A_TYPE, std::move(expr_range))
{}

ca_sub_operator::ca_sub_operator(ca_expr_ptr left_expr, ca_expr_ptr right_expr, range expr_range)
    : ca_binary_operator(
        std::move(left_expr), std::move(right_expr), context::SET_t_enum::A_TYPE, std::move(expr_range))
{}

ca_div_operator::ca_div_operator(ca_expr_ptr left_expr, ca_expr_ptr right_expr, range expr_range)
    : ca_binary_operator(
        std::move(left_expr), std::move(right_expr), context::SET_t_enum::A_TYPE, std::move(expr_range))
{}

ca_mul_operator::ca_mul_operator(ca_expr_ptr left_expr, ca_expr_ptr right_expr, range expr_range)
    : ca_binary_operator(
        std::move(left_expr), std::move(right_expr), context::SET_t_enum::A_TYPE, std::move(expr_range))
{}

ca_conc_operator::ca_conc_operator(ca_expr_ptr left_expr, ca_expr_ptr right_expr, range expr_range)
    : ca_binary_operator(
        std::move(left_expr), std::move(right_expr), context::SET_t_enum::C_TYPE, std::move(expr_range))
{}

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin
