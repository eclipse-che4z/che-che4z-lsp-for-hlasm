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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CA_OPERATOR_BINARY_H
#define HLASMPLUGIN_PARSERLIBRARY_CA_OPERATOR_BINARY_H

#include <compare>

#include "ca_expr_policy.h"
#include "ca_expression.h"

namespace hlasm_plugin::parser_library::expressions {

// abstract class for binary CA operators
class ca_binary_operator : public ca_expression
{
public:
    const ca_expr_ptr left_expr;
    const ca_expr_ptr right_expr;

    ca_binary_operator(ca_expr_ptr left_expr, ca_expr_ptr right_expr, context::SET_t_enum expr_kind, range expr_range);

    bool get_undefined_attributed_symbols(
        std::vector<context::id_index>& symbols, const evaluation_context& eval_ctx) const override;

    void resolve_expression_tree(ca_expression_ctx expr_ctx, diagnostic_op_consumer& diags) override;

    bool is_character_expression(character_expression_purpose purpose) const override;

    void apply(ca_expr_visitor& visitor) const override;

    context::SET_t evaluate(const evaluation_context& eval_ctx) const override;

    virtual context::SET_t operation(
        context::SET_t lhs, context::SET_t rhs, const evaluation_context& eval_ctx) const = 0;
};

// binary CA operators - + - * / .
template<typename OP>
class ca_basic_binary_operator final : public ca_binary_operator
{
public:
    ca_basic_binary_operator(ca_expr_ptr left_expr, ca_expr_ptr right_expr, range expr_range)
        : ca_binary_operator(std::move(left_expr), std::move(right_expr), OP::type, std::move(expr_range))
    {}

    context::SET_t operation(context::SET_t lhs, context::SET_t rhs, const evaluation_context& eval_ctx) const override
    {
        return OP::operation(std::move(lhs), std::move(rhs), expr_range, eval_ctx);
    }
};

// function binary CA operators - AND, SLL, OR, ...
class ca_function_binary_operator final : public ca_binary_operator
{
public:
    ca_function_binary_operator(ca_expr_ptr left_expr,
        ca_expr_ptr right_expr,
        ca_expr_ops function,
        context::SET_t_enum expr_kind,
        range expr_range,
        context::SET_t_enum parent_expr_kind = context::SET_t_enum::UNDEF_TYPE);

    bool get_undefined_attributed_symbols(
        std::vector<context::id_index>& symbols, const evaluation_context& eval_ctx) const override;

    void resolve_expression_tree(ca_expression_ctx expr_ctx, diagnostic_op_consumer& diags) override;

    context::SET_t operation(context::SET_t lhs, context::SET_t rhs, const evaluation_context& eval_ctx) const override;

    static std::strong_ordering compare_string(const context::C_t& lhs, const context::C_t& rhs) noexcept;
    static bool equal_string(const context::C_t& lhs, const context::C_t& rhs) noexcept;
    static std::strong_ordering compare_relational(
        const context::SET_t& lhs, const context::SET_t& rhs, context::SET_t_enum type) noexcept;

private:
    bool is_relational() const noexcept;
    bool is_string_equality(context::SET_t_enum type) const noexcept;
    ca_expr_ops function;
    ca_expression_ctx m_expr_ctx;
};

struct ca_add
{
    static constexpr context::SET_t_enum type = context::SET_t_enum::A_TYPE;

    static context::SET_t operation(
        const context::SET_t& lhs, const context::SET_t& rhs, range expr_range, const evaluation_context& eval_ctx);
};

struct ca_sub
{
    static constexpr context::SET_t_enum type = context::SET_t_enum::A_TYPE;

    static context::SET_t operation(
        const context::SET_t& lhs, const context::SET_t& rhs, range expr_range, const evaluation_context& eval_ctx);
};

struct ca_mul
{
    static constexpr context::SET_t_enum type = context::SET_t_enum::A_TYPE;

    static context::SET_t operation(
        const context::SET_t& lhs, const context::SET_t& rhs, range expr_range, const evaluation_context& eval_ctx);
};

struct ca_div
{
    static constexpr context::SET_t_enum type = context::SET_t_enum::A_TYPE;

    static context::SET_t operation(
        const context::SET_t& lhs, const context::SET_t& rhs, range expr_range, const evaluation_context& eval_ctx);
};

struct ca_conc
{
    static constexpr context::SET_t_enum type = context::SET_t_enum::C_TYPE;

    static context::SET_t operation(
        context::SET_t lhs, context::SET_t rhs, range expr_range, const evaluation_context& eval_ctx);
};

} // namespace hlasm_plugin::parser_library::expressions


#endif
