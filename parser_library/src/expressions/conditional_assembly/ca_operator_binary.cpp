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

context::SET_t ca_binary_operator::evaluate(evaluation_context& eval_ctx) const
{
    return operation(left_expr->evaluate(eval_ctx), right_expr->evaluate(eval_ctx));
}

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

context::SET_t ca_function_binary_operator::operation(context::SET_t lhs, context::SET_t rhs) const
{
    return context::SET_t();
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

context::SET_t ca_add::operation(context::SET_t lhs, context::SET_t rhs) { return lhs.access_a() + rhs.access_a(); }

context::SET_t ca_sub::operation(context::SET_t lhs, context::SET_t rhs) { return lhs.access_a() - rhs.access_a(); }

context::SET_t ca_mul::operation(context::SET_t lhs, context::SET_t rhs) { return lhs.access_a() * rhs.access_a(); }

context::SET_t ca_div::operation(context::SET_t lhs, context::SET_t rhs) { return lhs.access_a() / rhs.access_a(); }

context::SET_t ca_conc::operation(context::SET_t lhs, context::SET_t rhs)
{
    auto ret = lhs.access_c();
    ret.reserve(ret.size() + rhs.access_c().size());
    ret.append(rhs.access_c().begin(), rhs.access_c().end());
    return ret;
}

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin
