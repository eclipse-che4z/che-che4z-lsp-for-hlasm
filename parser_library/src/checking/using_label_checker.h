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


#ifndef HLASMPLUGIN_PARSERLIBRARY_USING_LABEL_CHECKER_H
#define HLASMPLUGIN_PARSERLIBRARY_USING_LABEL_CHECKER_H

#include "diagnostic_consumer.h"
#include "expressions/mach_expr_visitor.h"

namespace hlasm_plugin::parser_library::context {
class dependency_solver;
} // namespace hlasm_plugin::parser_library::context

namespace hlasm_plugin::parser_library::checking {

class using_label_checker final : public expressions::mach_expr_visitor
{
    context::dependency_solver& solver;
    diagnostic_consumer_t<diagnostic_op>& diags;

public:
    using_label_checker(context::dependency_solver& solver, diagnostic_consumer_t<diagnostic_op>& diags)
        : solver(solver)
        , diags(diags)
    {}

    // Inherited via mach_expr_visitor
    void visit(const expressions::mach_expr_constant&) override;
    void visit(const expressions::mach_expr_data_attr& attr) override;
    void visit(const expressions::mach_expr_data_attr_literal& attr) override;
    void visit(const expressions::mach_expr_symbol& expr) override;
    void visit(const expressions::mach_expr_location_counter&) override;
    void visit(const expressions::mach_expr_literal& l) override;
};

} // namespace hlasm_plugin::parser_library::checking

#endif
