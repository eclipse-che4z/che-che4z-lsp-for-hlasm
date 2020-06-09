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

#include "common_types.h"

#include <cctype>

namespace hlasm_plugin::parser_library::context {

std::string& to_upper(std::string& s)
{
    for (auto& c : s)
        c = static_cast<char>(std::toupper(c));
    return s;
}

std::string to_upper_copy(std::string s)
{
    for (auto& c : s)
        c = static_cast<char>(std::toupper(c));
    return std::move(s);
}

SET_t::SET_t(context::A_t value)
    : a_value(value)
    , b_value(value)
    , c_value(object_traits<C_t>::default_v())
    , type(SET_t_enum::A_TYPE)
{ }

SET_t::SET_t(context::B_t value)
    : a_value(object_traits<A_t>::default_v())
    , b_value(value)
    , c_value(object_traits<C_t>::default_v())
    , type(SET_t_enum::B_TYPE)
{ }

SET_t::SET_t(context::C_t value)
    : a_value(object_traits<A_t>::default_v())
    , b_value(object_traits<B_t>::default_v())
    , c_value(value)
    , type(SET_t_enum::C_TYPE)
{ }

SET_t::SET_t()
    : a_value(object_traits<A_t>::default_v())
    , b_value(object_traits<B_t>::default_v())
    , c_value(object_traits<C_t>::default_v())
    , type(SET_t_enum::UNDEF_TYPE)
{ }

A_t& SET_t::access_a() { return a_value; }

B_t& SET_t::access_b() { return b_value; }

C_t& SET_t::access_c() { return c_value; }

const A_t& SET_t::access_a() const { return a_value; }

const B_t& SET_t::access_b() const { return b_value; }

const C_t& SET_t::access_c() const { return c_value; }

} // namespace hlasm_plugin::parser_library::context
