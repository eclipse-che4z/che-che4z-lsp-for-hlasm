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

#include "concatenation_term.h"

using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library;

char_str_conc::char_str_conc(std::string value)
    : concatenation_point(concat_type::STR)
    , value(std::move(value))
{}

basic_var_sym_conc::basic_var_sym_conc(
    id_index name, std::vector<antlr4::ParserRuleContext*> subscript, hlasm_plugin::parser_library::range symbol_range)
    : var_sym_conc(false, std::move(subscript), std::move(symbol_range))
    , name(name)
{}


created_var_sym_conc::created_var_sym_conc(concat_chain created_name,
    std::vector<antlr4::ParserRuleContext*> subscript,
    hlasm_plugin::parser_library::range symbol_range)
    : var_sym_conc(true, std::move(subscript), std::move(symbol_range))
    , created_name(std::move(created_name))
{}

dot_conc::dot_conc()
    : concatenation_point(concat_type::DOT)
{}

equals_conc::equals_conc()
    : concatenation_point(concat_type::EQU)
{}

sublist_conc::sublist_conc(std::vector<concat_chain> list)
    : concatenation_point(concat_type::SUB)
    , list(std::move(list))
{}

basic_var_sym_conc* var_sym_conc::access_basic() { return created ? nullptr : static_cast<basic_var_sym_conc*>(this); }

const basic_var_sym_conc* var_sym_conc::access_basic() const
{
    return created ? nullptr : static_cast<const basic_var_sym_conc*>(this);
}

created_var_sym_conc* var_sym_conc::access_created() { return created ? static_cast<created_var_sym_conc*>(this) : nullptr; }

const created_var_sym_conc* var_sym_conc::access_created() const
{
    return created ? static_cast<const created_var_sym_conc*>(this) : nullptr;
}

var_sym_conc::var_sym_conc(const bool created, std::vector<antlr4::ParserRuleContext*> subscript, const range symbol_range)
    : concatenation_point(concat_type::VAR)
    , created(created)
    , subscript(std::move(subscript))
    , symbol_range(std::move(symbol_range))
{}