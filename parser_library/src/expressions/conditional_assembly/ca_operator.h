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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CA_OPERATOR_H
#define HLASMPLUGIN_PARSERLIBRARY_CA_OPERATOR_H

#include "ca_expr_policy.h"
#include "ca_expresssion.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

class ca_unary_operator : public ca_expression
{
public:
    const ca_expr_ptr expr;

    ca_unary_operator(ca_expr_ptr expr, range expr_range);

    virtual undef_sym_set get_undefined_attributed_symbols(const context::dependency_solver& solver) const override;

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual void collect_diags() const override;
};

// +, -
template<typename OP> class ca_arithmetic_unary_operator : public ca_unary_operator
{
public:
    ca_arithmetic_unary_operator(ca_expr_ptr expr, range expr_range)
        : ca_unary_operator(std::move(expr), std::move(expr_range))
    {
        expr_kind = context::SET_t_enum::A_TYPE;
    }
};

class ca_parentheses_operator : public ca_unary_operator
{
public:
    ca_parentheses_operator(ca_expr_ptr expr, range expr_range)
        : ca_unary_operator(std::move(expr), std::move(expr_range))
    {}
};

class ca_function_unary_operator : public ca_unary_operator
{
public:
    ca_expr_ops function;

    ca_function_unary_operator(ca_expr_ptr expr, ca_expr_ops function, context::SET_t_enum kind, range expr_range)
        : ca_unary_operator(std::move(expr), std::move(expr_range))
        , function(function)
    {
        expr_kind = kind;
    }
};

class ca_binary_operator : public ca_expression
{
public:
    const ca_expr_ptr left_expr, right_expr;

    ca_binary_operator(ca_expr_ptr left_expr, ca_expr_ptr right_expr, range expr_range);

    virtual undef_sym_set get_undefined_attributed_symbols(const context::dependency_solver& solver) const override;

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual void collect_diags() const override;
};

// basic add, sub, div, mul
template<typename OP> class ca_arithmetic_binary_operator : public ca_binary_operator
{
public:
    ca_arithmetic_binary_operator(ca_expr_ptr left_expr, ca_expr_ptr right_expr, range expr_range)
        : ca_binary_operator(std::move(left_expr), std::move(right_expr), std::move(expr_range))
    {
        expr_kind = context::SET_t_enum::A_TYPE;
    }
};

// AND, SLL, OR ...
class ca_function_binary_operator : public ca_binary_operator
{
public:
    ca_expr_ops function;

    ca_function_binary_operator(
        ca_expr_ptr left_expr, ca_expr_ptr right_expr, ca_expr_ops function, context::SET_t_enum kind, range expr_range)
        : ca_binary_operator(std::move(left_expr), std::move(right_expr), std::move(expr_range))
        , function(function)
    {
        expr_kind = kind;
    }
};


} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin


#endif
