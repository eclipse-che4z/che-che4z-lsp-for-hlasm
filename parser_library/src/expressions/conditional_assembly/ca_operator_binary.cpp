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

#include <limits>

#include "ebcdic_encoding.h"
#include "expressions/evaluation_context.h"
#include "terms/ca_function.h"
#include "terms/ca_string.h"

namespace hlasm_plugin::parser_library::expressions {

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

context::SET_t ca_binary_operator::evaluate(const evaluation_context& eval_ctx) const
{
    return operation(left_expr->evaluate(eval_ctx), right_expr->evaluate(eval_ctx), eval_ctx);
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

context::A_t shift_operands(context::A_t lhs, context::A_t rhs, ca_expr_ops shift)
{
    auto shift_part = rhs & 0x3f; // first 6 bits
    if (shift_part == 0)
        return rhs;

    std::uint32_t unsigned_lhs = lhs;
    auto sign_bit = unsigned_lhs & (1U << 31);

    unsigned int result;

    if (shift_part >= 32)
    {
        switch (shift)
        {
            case ca_expr_ops::SLA:
                result = sign_bit;
                break;
            case ca_expr_ops::SRA:
                result = ~0U;
                break;
            default:
                result = 0;
                break;
        }
        return result;
    }

    switch (shift)
    {
        case ca_expr_ops::SLA:
            result = (unsigned_lhs << shift_part) | sign_bit;
            break;
        case ca_expr_ops::SLL:
            result = unsigned_lhs << shift_part;
            break;
        case ca_expr_ops::SRA:
            if (sign_bit)
                result = (unsigned_lhs >> shift_part) | (~0U << (32 - shift_part));
            else
                result = unsigned_lhs >> shift_part;
            break;
        case ca_expr_ops::SRL:
            result = unsigned_lhs >> shift_part;
            break;
        default:
            result = 0;
            break;
    }

    return result;
}

context::SET_t ca_function_binary_operator::operation(
    context::SET_t lhs, context::SET_t rhs, const evaluation_context&) const
{
    if (expr_kind == context::SET_t_enum::A_TYPE)
    {
        switch (function)
        {
            case ca_expr_ops::SLA:
            case ca_expr_ops::SLL:
            case ca_expr_ops::SRA:
            case ca_expr_ops::SRL:
                return shift_operands(lhs.access_a(), rhs.access_a(), function);
            case ca_expr_ops::FIND:
                return ca_function::FIND(lhs.access_c(), rhs.access_c());
            case ca_expr_ops::INDEX:
                return ca_function::INDEX(lhs.access_c(), rhs.access_c());
            case ca_expr_ops::AND:
                return lhs.access_a() & rhs.access_a();
            case ca_expr_ops::OR:
                return lhs.access_a() | rhs.access_a();
            case ca_expr_ops::XOR:
                return lhs.access_a() ^ rhs.access_a();
            default:
                break;
        }
    }
    else if (expr_kind == context::SET_t_enum::B_TYPE)
    {
        int comp = 0;
        if (is_relational())
            comp = compare_relational(lhs, rhs, left_expr->expr_kind);

        switch (function)
        {
            case ca_expr_ops::EQ:
                return comp == 0;
            case ca_expr_ops::NE:
                return comp != 0;
            case ca_expr_ops::LE:
                return comp <= 0;
            case ca_expr_ops::LT:
                return comp < 0;
            case ca_expr_ops::GE:
                return comp >= 0;
            case ca_expr_ops::GT:
                return comp > 0;
            case ca_expr_ops::AND:
                return lhs.access_b() && rhs.access_b();
            case ca_expr_ops::OR:
                return lhs.access_b() || rhs.access_b();
            case ca_expr_ops::XOR:
                return lhs.access_b() != rhs.access_b();
            case ca_expr_ops::AND_NOT:
                return lhs.access_b() && !rhs.access_b();
            case ca_expr_ops::OR_NOT:
                return lhs.access_b() || !rhs.access_b();
            case ca_expr_ops::XOR_NOT:
                return lhs.access_b() != !rhs.access_b();
            default:
                break;
        }
    }
    return context::SET_t(expr_kind);
}

int ca_function_binary_operator::compare_string(const context::C_t& lhs, const context::C_t& rhs)
{
    int diff = (int)lhs.size() - (int)rhs.size();

    if (diff != 0)
        return diff;

    return ebcdic_encoding::to_ebcdic(lhs).compare(ebcdic_encoding::to_ebcdic(rhs));
}

int ca_function_binary_operator::compare_relational(
    const context::SET_t& lhs, const context::SET_t& rhs, context::SET_t_enum type)
{
    switch (type)
    {
        case context::SET_t_enum::A_TYPE:
            return lhs.access_a() - rhs.access_a();
        case context::SET_t_enum::C_TYPE:
            return compare_string(lhs.access_c(), rhs.access_c());
        default:
            return 0;
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

context::A_t overflow_transform(std::int64_t val, range expr_range, const evaluation_context& eval_ctx)
{
    if (val > std::numeric_limits<context::A_t>::max())
    {
        eval_ctx.add_diagnostic(diagnostic_op::error_CE013(expr_range));
        return 0;
    }
    else if (val < std::numeric_limits<context::A_t>::min())
    {
        eval_ctx.add_diagnostic(diagnostic_op::error_CE014(expr_range));
        return 0;
    }
    else
        return (context::A_t)val;
}

context::SET_t ca_add::operation(
    const context::SET_t& lhs, const context::SET_t& rhs, range expr_range, const evaluation_context& eval_ctx)
{
    return overflow_transform((std::int64_t)lhs.access_a() + (std::int64_t)rhs.access_a(), expr_range, eval_ctx);
}

context::SET_t ca_sub::operation(
    const context::SET_t& lhs, const context::SET_t& rhs, range expr_range, const evaluation_context& eval_ctx)
{
    return overflow_transform((std::int64_t)lhs.access_a() - (std::int64_t)rhs.access_a(), expr_range, eval_ctx);
}

context::SET_t ca_mul::operation(
    const context::SET_t& lhs, const context::SET_t& rhs, range expr_range, const evaluation_context& eval_ctx)
{
    return overflow_transform((std::int64_t)lhs.access_a() * (std::int64_t)rhs.access_a(), expr_range, eval_ctx);
}

context::SET_t ca_div::operation(
    const context::SET_t& lhs, const context::SET_t& rhs, range expr_range, const evaluation_context& eval_ctx)
{
    if (rhs.access_a() == 0)
        return 0;
    return overflow_transform((std::int64_t)lhs.access_a() / (std::int64_t)rhs.access_a(), expr_range, eval_ctx);
}

context::SET_t ca_conc::operation(
    context::SET_t lhs, context::SET_t rhs, range expr_range, const evaluation_context& eval_ctx)
{
    if (lhs.access_c().size() + rhs.access_c().size() > ca_string::MAX_STR_SIZE)
    {
        eval_ctx.add_diagnostic(diagnostic_op::error_CE011(expr_range));
        return context::object_traits<context::C_t>::default_v();
    }
    auto& ret = lhs.access_c();
    ret.reserve(ret.size() + rhs.access_c().size());
    ret.append(rhs.access_c().begin(), rhs.access_c().end());
    return ret;
}

} // namespace hlasm_plugin::parser_library::expressions
