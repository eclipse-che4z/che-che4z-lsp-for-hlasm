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
#include "ca_expresssion.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

class ca_unary_operator : public ca_expression
{
public:
    const ca_expr_ptr expr;

    ca_unary_operator(ca_expr_ptr expr, context::SET_t_enum expr_kind, range expr_range);

    virtual undef_sym_set get_undefined_attributed_symbols(const context::dependency_solver& solver) const override;

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual void collect_diags() const override;

    virtual bool is_character_expression() const override;

    virtual context::SET_t evaluate(evaluation_context& eval_ctx) const override;

    virtual context::SET_t operation(context::SET_t operand, evaluation_context& eval_ctx) const = 0;
};

class ca_plus_operator : public ca_unary_operator
{
public:
    ca_plus_operator(ca_expr_ptr expr, range expr_range);

    virtual context::SET_t operation(context::SET_t operand, evaluation_context& eval_ctx) const override;
};

class ca_minus_operator : public ca_unary_operator
{
public:
    ca_minus_operator(ca_expr_ptr expr, range expr_range);

    virtual context::SET_t operation(context::SET_t operand, evaluation_context& eval_ctx) const override;
};

class ca_par_operator : public ca_unary_operator
{
public:
    ca_par_operator(ca_expr_ptr expr, range expr_range);

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual context::SET_t operation(context::SET_t operand, evaluation_context& eval_ctx) const override;
};

// NOT, BYTE, ...
class ca_function_unary_operator : public ca_unary_operator
{
public:
    ca_expr_ops function;

    ca_function_unary_operator(ca_expr_ptr expr, ca_expr_ops function, context::SET_t_enum expr_kind, range expr_range);

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual context::SET_t operation(context::SET_t operand, evaluation_context& eval_ctx) const override;

    static context::SET_t BYTE(context::SET_t param, ca_expr_ptr owner);
    static context::SET_t DOUBLE(context::SET_t param);
    static context::SET_t LOWER(context::SET_t param);
    static context::SET_t SIGNED(context::SET_t param);
    static context::SET_t UPPER(context::SET_t param);
};


} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin


#endif
