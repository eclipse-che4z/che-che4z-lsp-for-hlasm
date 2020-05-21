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

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

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

ca_plus_operator::ca_plus_operator(ca_expr_ptr expr, range expr_range)
    : ca_unary_operator(std::move(expr), context::SET_t_enum::A_TYPE, std::move(expr_range))
{}

ca_minus_operator::ca_minus_operator(ca_expr_ptr expr, range expr_range)
    : ca_unary_operator(std::move(expr), context::SET_t_enum::A_TYPE, std::move(expr_range))
{}

ca_par_operator::ca_par_operator(ca_expr_ptr expr, range expr_range)
    : ca_unary_operator(std::move(expr), context::SET_t_enum::UNDEF_TYPE, std::move(expr_range))
{}

void ca_par_operator::resolve_expression_tree(context::SET_t_enum kind)
{
    expr->resolve_expression_tree(kind);
    expr_kind = expr->expr_kind;
    if (expr_kind != kind)
        add_diagnostic(diagnostic_op::error_CE004(expr_range));
}

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin
