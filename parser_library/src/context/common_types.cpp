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

#include <cassert>
#include <cctype>

namespace hlasm_plugin::parser_library::context {

SET_t::SET_t(context::A_t value)
    : a_value(value)
    , b_value(value)
    , c_value(object_traits<C_t>::default_v())
    , type(SET_t_enum::A_TYPE)
{}

SET_t::SET_t(context::B_t value)
    : a_value(value)
    , b_value(value)
    , c_value(object_traits<C_t>::default_v())
    , type(SET_t_enum::B_TYPE)
{}

SET_t::SET_t(context::C_t value)
    : a_value(object_traits<A_t>::default_v())
    , b_value(object_traits<B_t>::default_v())
    , c_value(std::move(value))
    , type(SET_t_enum::C_TYPE)
{}

SET_t::SET_t(const char* value)
    : a_value(object_traits<A_t>::default_v())
    , b_value(object_traits<B_t>::default_v())
    , c_value(value)
    , type(SET_t_enum::C_TYPE)
{}

SET_t::SET_t(SET_t_enum type)
    : a_value(object_traits<A_t>::default_v())
    , b_value(object_traits<B_t>::default_v())
    , c_value(object_traits<C_t>::default_v())
    , type(type)
{}

A_t& SET_t::access_a() { return a_value; }

B_t& SET_t::access_b() { return b_value; }

C_t& SET_t::access_c() { return c_value; }

const A_t& SET_t::access_a() const { return a_value; }

const B_t& SET_t::access_b() const { return b_value; }

const C_t& SET_t::access_c() const { return c_value; }

bool SET_t::operator==(const SET_t& r) const noexcept
{
    if (type != r.type)
        return false;

    switch (type)
    {
        case SET_t_enum::A_TYPE:
            return a_value == r.a_value;
        case SET_t_enum::B_TYPE:
            return b_value == r.b_value;
        case SET_t_enum::C_TYPE:
            return c_value == r.c_value;
        case SET_t_enum::UNDEF_TYPE:
            return true;
        default:
            assert(false);
            return false;
    }
}

} // namespace hlasm_plugin::parser_library::context
