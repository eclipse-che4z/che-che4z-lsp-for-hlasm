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

#include "variable_symbol.h"

#include "concatenation.h"
#include "expressions/conditional_assembly/ca_expression.h"

using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library;

basic_variable_symbol::basic_variable_symbol(
    id_index name, std::vector<expressions::ca_expr_ptr> subscript, range symbol_range)
    : variable_symbol(false, std::move(subscript), std::move(symbol_range))
    , name(name)
{}


created_variable_symbol::created_variable_symbol(
    concat_chain created_name, std::vector<expressions::ca_expr_ptr> subscript, range symbol_range)
    : variable_symbol(true, std::move(subscript), std::move(symbol_range))
    , created_name(std::move(created_name))
{}


basic_variable_symbol* variable_symbol::access_basic()
{
    return created ? nullptr : static_cast<basic_variable_symbol*>(this);
}

const basic_variable_symbol* variable_symbol::access_basic() const
{
    return created ? nullptr : static_cast<const basic_variable_symbol*>(this);
}

created_variable_symbol* variable_symbol::access_created()
{
    return created ? static_cast<created_variable_symbol*>(this) : nullptr;
}

const created_variable_symbol* variable_symbol::access_created() const
{
    return created ? static_cast<const created_variable_symbol*>(this) : nullptr;
}

variable_symbol::variable_symbol(
    const bool created, std::vector<expressions::ca_expr_ptr> subscript, const range symbol_range)
    : concatenation_point(concat_type::VAR)
    , created(created)
    , subscript(std::move(subscript))
    , symbol_range(std::move(symbol_range))
{}
