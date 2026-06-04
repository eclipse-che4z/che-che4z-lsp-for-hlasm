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

#ifndef HLASMPLUGIN_PARSERLIBRARY_MACH_EXPRESSION_VISITOR_H
#define HLASMPLUGIN_PARSERLIBRARY_MACH_EXPRESSION_VISITOR_H

#include "mach_expr_term.h"
#include "mach_operator.h"


namespace hlasm_plugin::parser_library::expressions {

class mach_expr_visitor
{
public:
    virtual void visit(const mach_expr_constant& expr) = 0;
    virtual void visit(const mach_expr_data_attr& expr) = 0;
    virtual void visit(const mach_expr_data_attr_literal& expr) = 0;
    virtual void visit(const mach_expr_symbol& expr) = 0;
    virtual void visit(const mach_expr_location_counter& expr) = 0;
    virtual void visit(const mach_expr_literal& expr) = 0;

protected:
    ~mach_expr_visitor() = default;
};

} // namespace hlasm_plugin::parser_library::expressions

#endif
