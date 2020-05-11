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

var_sym_conc::var_sym_conc(vs_ptr symbol)
    : concatenation_point(concat_type::VAR)
    , symbol(std::move(symbol))
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
