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

#include "context/ordinary_assembly/symbol_value.h"
#include "mach_expr_term.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::expressions;

mach_expression::mach_expression(range rng)
    : expr_range_(rng)
{}

bool mach_expression::is_similar(const mach_expression& expr) const
{
    return typeid(*this) == typeid(expr) && do_is_similar(expr);
}

bool mach_expression::has_dependencies(
    context::dependency_solver& info, std::vector<context::id_index>* missing_symbols) const
{
    auto d = get_dependencies(info);
    if (missing_symbols)
        d.collect_unique_symbolic_dependencies(*missing_symbols);

    return d.contains_dependencies();
}

context::symbol_value hlasm_plugin::parser_library::expressions::mach_expression::resolve(
    context::dependency_solver& solver) const
{
    auto tmp_val = evaluate(solver, drop_diagnostic_op);
    if (tmp_val.value_kind() == context::symbol_value_kind::UNDEF)
        return 0;
    else
        return tmp_val;
}
