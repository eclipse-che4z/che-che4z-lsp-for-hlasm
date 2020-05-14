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

#include "ca_operator.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

ca_unary_operator::ca_unary_operator(ca_expr_ptr expr)
    : expr(std::move(expr))
{}

undef_sym_set ca_unary_operator::get_undefined_attributed_symbols(const context::dependency_solver& solver) const
{
    return expr->get_undefined_attributed_symbols(solver);
}

void ca_unary_operator::resolve_expression_tree(context::SET_t_enum kind) { expr->resolve_expression_tree(kind); }

ca_binary_operator::ca_binary_operator(ca_expr_ptr left_expr, ca_expr_ptr right_expr)
    : left_expr(std::move(left_expr))
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
    left_expr->resolve_expression_tree(kind);
    right_expr->resolve_expression_tree(kind);
}

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin
