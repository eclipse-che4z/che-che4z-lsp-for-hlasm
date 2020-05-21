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
#include "ca_expresssion.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

class ca_binary_operator : public ca_expression
{
public:
    const ca_expr_ptr left_expr, right_expr;

    ca_binary_operator(ca_expr_ptr left_expr, ca_expr_ptr right_expr, context::SET_t_enum expr_kind, range expr_range);

    virtual undef_sym_set get_undefined_attributed_symbols(const context::dependency_solver& solver) const override;

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual void collect_diags() const override;

    virtual bool is_character_expression() const override;
};

// +, -, *, /, .
template<typename OP> class ca_arithmetic_binary_operator : public ca_binary_operator
{
public:
    ca_arithmetic_binary_operator(ca_expr_ptr left_expr, ca_expr_ptr right_expr, range expr_range)
        : ca_binary_operator(std::move(left_expr), std::move(right_expr), OP::expr_kind, std::move(expr_range))
    {}
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
        range expr_range)
        : ca_binary_operator(std::move(left_expr), std::move(right_expr), expr_kind, std::move(expr_range))
        , function(function)
    {}

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

private:
    bool is_relational() const;
};


} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin


#endif
