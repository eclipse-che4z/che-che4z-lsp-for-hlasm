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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CA_OPERATOR_UNARY_H
#define HLASMPLUGIN_PARSERLIBRARY_CA_OPERATOR_UNARY_H

#include "ca_expr_policy.h"
#include "ca_expression.h"

namespace hlasm_plugin::parser_library::expressions {

class ca_unary_operator : public ca_expression
{
public:
    const ca_expr_ptr expr;

    ca_unary_operator(ca_expr_ptr expr, context::SET_t_enum expr_kind, range expr_range);

    undef_sym_set get_undefined_attributed_symbols(const evaluation_context& eval_ctx) const override;

    void resolve_expression_tree(context::SET_t_enum kind) override;

    void collect_diags() const override;

    bool is_character_expression(character_expression_purpose purpose) const override;

    void apply(ca_expr_visitor& visitor) const override;

    context::SET_t evaluate(const evaluation_context& eval_ctx) const override;

    virtual context::SET_t operation(context::SET_t operand, const evaluation_context& eval_ctx) const = 0;
};

class ca_plus_operator : public ca_unary_operator
{
public:
    ca_plus_operator(ca_expr_ptr expr, range expr_range);

    context::SET_t operation(context::SET_t operand, const evaluation_context& eval_ctx) const override;
};

class ca_minus_operator : public ca_unary_operator
{
public:
    ca_minus_operator(ca_expr_ptr expr, range expr_range);

    context::SET_t operation(context::SET_t operand, const evaluation_context& eval_ctx) const override;
};

class ca_par_operator : public ca_unary_operator
{
public:
    ca_par_operator(ca_expr_ptr expr, range expr_range);

    void resolve_expression_tree(context::SET_t_enum kind) override;

    context::SET_t operation(context::SET_t operand, const evaluation_context& eval_ctx) const override;
};

// NOT, BYTE, ...
class ca_function_unary_operator : public ca_unary_operator
{
public:
    ca_expr_ops function;

    ca_function_unary_operator(ca_expr_ptr expr, ca_expr_ops function, context::SET_t_enum expr_kind, range expr_range);

    void resolve_expression_tree(context::SET_t_enum kind) override;

    context::SET_t operation(context::SET_t operand, const evaluation_context& eval_ctx) const override;
};

} // namespace hlasm_plugin::parser_library::expressions


#endif
