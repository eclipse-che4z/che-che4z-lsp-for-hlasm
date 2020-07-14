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

#include "ca_expr_policy.h"
#include "ca_expression.h"

namespace hlasm_plugin::parser_library::expressions {

class ca_binary_operator : public ca_expression
{
public:
    const ca_expr_ptr left_expr;
    const ca_expr_ptr right_expr;

    ca_binary_operator(ca_expr_ptr left_expr, ca_expr_ptr right_expr, context::SET_t_enum expr_kind, range expr_range);

    virtual undef_sym_set get_undefined_attributed_symbols(const context::dependency_solver& solver) const override;

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual void collect_diags() const override;

    virtual bool is_character_expression() const override;

    virtual context::SET_t evaluate(evaluation_context& eval_ctx) const override;

    virtual context::SET_t operation(context::SET_t lhs, context::SET_t rhs, evaluation_context& eval_ctx) const = 0;
};

template<typename OP>
class ca_basic_binary_operator : public ca_binary_operator
{
public:
    ca_basic_binary_operator(ca_expr_ptr left_expr, ca_expr_ptr right_expr, range expr_range)
        : ca_binary_operator(std::move(left_expr), std::move(right_expr), OP::type, std::move(expr_range))
    {}

    virtual context::SET_t operation(
        context::SET_t lhs, context::SET_t rhs, evaluation_context& eval_ctx) const override
    {
        return OP::operation(std::move(lhs), std::move(rhs), expr_range, eval_ctx);
    }
};

// AND, SLL, OR, ...
class ca_function_binary_operator : public ca_binary_operator
{
public:
    ca_expr_ops function;

    ca_function_binary_operator(ca_expr_ptr left_expr,
        ca_expr_ptr right_expr,
        ca_expr_ops function,
        context::SET_t_enum expr_kind,
        range expr_range);

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual context::SET_t operation(
        context::SET_t lhs, context::SET_t rhs, evaluation_context& eval_ctx) const override;

    static int compare_string(const context::C_t& lhs, const context::C_t& rhs);
    static int compare_relational(const context::SET_t& lhs, const context::SET_t& rhs, context::SET_t_enum type);

private:
    bool is_relational() const;
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
