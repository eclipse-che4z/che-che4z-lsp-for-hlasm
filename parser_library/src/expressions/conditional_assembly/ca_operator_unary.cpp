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

#include "ca_expr_term.h"
#include "ebcdic_encoding.h"
#include "expressions/evaluation_context.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

ca_unary_operator::ca_unary_operator(ca_expr_ptr expr, context::SET_t_enum expr_kind, range expr_range)
    : ca_expression(expr_kind, std::move(expr_range))
    , expr(std::move(expr))
{ }

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
{ }

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
        switch (function)
        {
            case ca_expr_ops::BYTE:
                return BYTE(std::move(operand), expr_range, eval_ctx);
            case ca_expr_ops::DOUBLE:
                return DOUBLE(std::move(operand), expr_range, eval_ctx);
            case ca_expr_ops::LOWER:
                return LOWER(std::move(operand), expr_range, eval_ctx);
            case ca_expr_ops::SIGNED:
                return SIGNED(std::move(operand), expr_range, eval_ctx);
            case ca_expr_ops::UPPER:
                return UPPER(std::move(operand), expr_range, eval_ctx);
            default:
                break;
        }
    }
    return context::SET_t();
}

context::SET_t ca_function_unary_operator::BYTE(context::SET_t param, range param_range, evaluation_context& eval_ctx)
{
    auto value = param.access_a();
    if (value > 255 || value < 0)
    {
        eval_ctx.add_diagnostic(diagnostic_op::error_CE007(param_range));
        return context::SET_t();
    }
    else
        return ebcdic_encoding::to_ascii(static_cast<unsigned char>(value));
}

context::SET_t ca_function_unary_operator::DOUBLE(context::SET_t param, range param_range, evaluation_context& eval_ctx)
{
    std::string ret;
    for (char c : param.access_c())
    {
        ret.push_back(c);
        if (c == '\'' || c == '&')
            ret.push_back(c);
    }

    if (ret.size() > ca_string::MAX_STR_SIZE)
    {
        eval_ctx.add_diagnostic(diagnostic_op::error_CE011(param_range));
        return context::SET_t();
    }

    return ret;
}

context::SET_t ca_function_unary_operator::LOWER(context::SET_t param, range param_range, evaluation_context& eval_ctx)
{
    auto value = param.access_c();
    std::transform(value.begin(), value.end(), value.begin(), [](char c) { return static_cast<char>(tolower(c)); });
    return std::move(value);
}

context::SET_t ca_function_unary_operator::SIGNED(context::SET_t param, range param_range, evaluation_context& eval_ctx)
{
    return std::to_string(param.access_a());
}

context::SET_t ca_function_unary_operator::UPPER(context::SET_t param, range param_range, evaluation_context& eval_ctx)
{
    auto value = param.access_c();
    std::transform(value.begin(), value.end(), value.begin(), [](char c) { return static_cast<char>(toupper(c)); });
    return std::move(value);
}

ca_plus_operator::ca_plus_operator(ca_expr_ptr expr, range expr_range)
    : ca_unary_operator(std::move(expr), context::SET_t_enum::A_TYPE, std::move(expr_range))
{ }

context::SET_t ca_plus_operator::operation(context::SET_t operand, evaluation_context&) const
{
    return operand.access_a();
}

ca_minus_operator::ca_minus_operator(ca_expr_ptr expr, range expr_range)
    : ca_unary_operator(std::move(expr), context::SET_t_enum::A_TYPE, std::move(expr_range))
{ }

context::SET_t ca_minus_operator::operation(context::SET_t operand, evaluation_context&) const
{
    return -operand.access_a();
}

ca_par_operator::ca_par_operator(ca_expr_ptr expr, range expr_range)
    : ca_unary_operator(std::move(expr), context::SET_t_enum::UNDEF_TYPE, std::move(expr_range))
{ }

void ca_par_operator::resolve_expression_tree(context::SET_t_enum kind)
{
    expr->resolve_expression_tree(kind);
    expr_kind = expr->expr_kind;
    if (expr_kind != kind)
        add_diagnostic(diagnostic_op::error_CE004(expr_range));
}

context::SET_t ca_par_operator::operation(context::SET_t operand, evaluation_context&) const
{
    return std::move(operand);
}

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin
