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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CA_EXPRESSION_VISITOR_H
#define HLASMPLUGIN_PARSERLIBRARY_CA_EXPRESSION_VISITOR_H

#include "terms/ca_constant.h"
#include "terms/ca_expr_list.h"
#include "terms/ca_function.h"
#include "terms/ca_string.h"
#include "terms/ca_symbol.h"
#include "terms/ca_symbol_attribute.h"
#include "terms/ca_var_sym.h"

namespace hlasm_plugin::parser_library::expressions {

class ca_expr_visitor
{
public:
    virtual void visit(const ca_constant& expr) = 0;
    virtual void visit(const ca_expr_list& expr) = 0;
    virtual void visit(const ca_function& expr) = 0;
    virtual void visit(const ca_string& expr) = 0;
    virtual void visit(const ca_symbol& expr) = 0;
    virtual void visit(const ca_symbol_attribute& expr) = 0;
    virtual void visit(const ca_var_sym& expr) = 0;

    virtual ~ca_expr_visitor() = default;
};


} // namespace hlasm_plugin::parser_library::expressions

#endif
