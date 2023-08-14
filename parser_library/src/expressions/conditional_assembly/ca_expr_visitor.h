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

namespace hlasm_plugin::parser_library::expressions {

class ca_constant;
class ca_expr_list;
class ca_function;
class ca_string;
class ca_symbol;
class ca_symbol_attribute;
class ca_var_sym;

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

protected:
    ~ca_expr_visitor() = default;
};


} // namespace hlasm_plugin::parser_library::expressions

#endif
