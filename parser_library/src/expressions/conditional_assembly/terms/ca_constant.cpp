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

#include "ca_constant.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

ca_constant::ca_constant(context::A_t value, range expr_range)
    : ca_expression(context::SET_t_enum::A_TYPE, std::move(expr_range))
    , value(value)
{ }

undef_sym_set ca_constant::get_undefined_attributed_symbols(const context::dependency_solver&) const
{
    return undef_sym_set();
}

void ca_constant::resolve_expression_tree(context::SET_t_enum kind)
{
    if (kind == context::SET_t_enum::C_TYPE)
        add_diagnostic(diagnostic_op::error_CE004(expr_range));
}

void ca_constant::collect_diags() const { }

bool ca_constant::is_character_expression() const { return false; }

context::SET_t ca_constant::evaluate(evaluation_context&) const { return value; }

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin
