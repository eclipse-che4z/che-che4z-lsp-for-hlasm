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

#include "mach_expression.h"

#include <typeinfo>

#include "mach_expr_term.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::expressions;

mach_expression::mach_expression(range rng)
    : expr_range_(rng)
{}

mach_expr_ptr hlasm_plugin::parser_library::expressions::mach_expression::assign_expr(
    mach_expr_ptr expr, range expr_range)
{
    return expr ? std::move(expr) : std::make_unique<mach_expr_default>(expr_range);
}

bool mach_expression::is_similar(const mach_expression& expr) const
{
    return typeid(*this) == typeid(expr) && do_is_similar(expr);
}

range mach_expression::get_range() const { return expr_range_; }

context::symbol_value hlasm_plugin::parser_library::expressions::mach_expression::resolve(
    context::dependency_solver& solver) const
{
    diagnostic_consumer_transform dummy([](diagnostic_op) {});
    auto tmp_val = evaluate(solver, dummy);
    if (tmp_val.value_kind() == context::symbol_value_kind::UNDEF)
        return 0;
    else
        return tmp_val;
}
