/*
 * Copyright (c) 2021 Broadcom.
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

#include "using_label_checker.h"

#include "context/ordinary_assembly/section.h"

namespace hlasm_plugin::parser_library::checking {

void using_label_checker::add_diagnostic(diagnostic_op) const {}

void using_label_checker::visit(const expressions::mach_expr_constant&) {}
void using_label_checker::visit(const expressions::mach_expr_data_attr& attr) {}
void using_label_checker::visit(const expressions::mach_expr_symbol& expr)
{
    if (!expr.qualifier)
        return;
    auto value = expr.evaluate(solver, *this);
    if (value.value_kind() != context::symbol_value_kind::RELOC)
        return;
    const auto& reloc = value.get_reloc();
    if (!reloc.is_simple())
        return;
    const auto* section = reloc.bases().front().first.owner;
    if (!solver.using_active(expr.qualifier, section))
        diags.add_diagnostic(diagnostic_op::error_ME005(*expr.qualifier, *section->name, expr.get_range()));
}
void using_label_checker::visit(const expressions::mach_expr_location_counter&) {}
void using_label_checker::visit(const expressions::mach_expr_default&) {}
void using_label_checker::visit(const expressions::mach_expr_literal& l) { l.get_data_definition().apply(*this); }

} // namespace hlasm_plugin::parser_library::checking
