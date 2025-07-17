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

#include "symbol_attributes.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <stdexcept>

#include "../../ebcdic_encoding.h"

namespace hlasm_plugin::parser_library::context {

constexpr char assembler_type_values[][5] = { "", "AR", "CR", "CR32", "CR64", "FPR", "GR", "GR32", "GR64", "VR" };

assembler_type assembler_type_from_string(std::string_view s) noexcept
{
    if (s.size() > sizeof(assembler_type_values[0]))
        return {};
    const auto it = std::ranges::find(assembler_type_values, s);
    if (it == std::ranges::end(assembler_type_values))
        return {};
    return (symbol_attributes::assembler_type)std::ranges::distance(std::ranges::begin(assembler_type_values), it);
}

std::string_view assembler_type_to_string(assembler_type t) noexcept
{
    const auto value = (std::underlying_type_t<assembler_type>)t;
    assert(value < std::size(assembler_type_values));
    return assembler_type_values[value];
}

static_assert(symbol_attributes::undef_type == 'U'_ebcdic);
static_assert(symbol_attributes::undef_length == static_cast<symbol_attributes::len_attr>(-1));
static_assert(symbol_attributes::undef_scale == std::numeric_limits<symbol_attributes::scale_attr>::max());

symbol_attributes symbol_attributes::make_section_attrs()
{
    return symbol_attributes(symbol_origin::SECT, 'J'_ebcdic, 1);
}
symbol_attributes symbol_attributes::make_machine_attrs(symbol_attributes::len_attr length)
{
    return symbol_attributes(symbol_origin::MACH, 'I'_ebcdic, length);
}
symbol_attributes symbol_attributes::make_extrn_attrs()
{
    return symbol_attributes(symbol_origin::SECT, 'T'_ebcdic, 1);
}
symbol_attributes symbol_attributes::make_wxtrn_attrs()
{
    return symbol_attributes(symbol_origin::SECT, '$'_ebcdic, 1);
}
symbol_attributes symbol_attributes::make_org_attrs() { return symbol_attributes(symbol_origin::SECT, 'U'_ebcdic); }

symbol_attributes symbol_attributes::make_ccw_attrs() { return symbol_attributes(symbol_origin::ASM, 'W'_ebcdic, 8); }

symbol_attributes hlasm_plugin::parser_library::context::symbol_attributes::make_cnop_attrs()
{
    return symbol_attributes(symbol_origin::ASM, 'I'_ebcdic);
}

data_attr_kind symbol_attributes::transform_attr(unsigned char c)
{
    c = (char)std::toupper(c);
    switch (c)
    {
        case 'D':
            return data_attr_kind::D;
        case 'O':
            return data_attr_kind::O;
        case 'N':
            return data_attr_kind::N;
        case 'S':
            return data_attr_kind::S;
        case 'K':
            return data_attr_kind::K;
        case 'I':
            return data_attr_kind::I;
        case 'L':
            return data_attr_kind::L;
        case 'T':
            return data_attr_kind::T;
        default:
            return data_attr_kind::UNKNOWN;
    }
}

bool symbol_attributes::requires_ordinary_symbol(data_attr_kind attribute)
{
    return attribute == data_attr_kind::D || attribute == data_attr_kind::L || attribute == data_attr_kind::O
        || attribute == data_attr_kind::S || attribute == data_attr_kind::I;
}

bool symbol_attributes::is_ordinary_attribute(data_attr_kind attribute)
{
    return attribute == data_attr_kind::L || attribute == data_attr_kind::I || attribute == data_attr_kind::S
        || attribute == data_attr_kind::T;
}

symbol_attributes::value_t symbol_attributes::default_value(data_attr_kind attribute)
{
    switch (attribute)
    {
        case data_attr_kind::T:
            return undef_type;
        case data_attr_kind::L:
            return 1;
        default:
            return 0;
    }
}

SET_t symbol_attributes::default_ca_value(data_attr_kind attribute)
{
    switch (attribute)
    {
        case data_attr_kind::T:
        case data_attr_kind::O:
            return std::string("U");
        case data_attr_kind::L:
            return 1;
        default:
            return 0;
    }
}

void symbol_attributes::length(len_attr value)
{
    length_ == undef_length ? length_ = value : throw std::runtime_error("value can be assigned only once");
}

void symbol_attributes::scale(scale_attr value)
{
    scale_ == undef_scale ? scale_ = value : throw std::runtime_error("value can be assigned only once");
}

bool symbol_attributes::is_defined(data_attr_kind attribute) const
{
    switch (attribute)
    {
        case data_attr_kind::T:
            return true;
        case data_attr_kind::L:
            return length_ != undef_length;
        case data_attr_kind::S:
            return scale_ != undef_scale;
        case data_attr_kind::I:
            return scale_ != undef_scale && length_ != undef_length;
        default:
            return false;
    }
}

bool symbol_attributes::can_have_SI_attr() const
{
    return origin_ == symbol_origin::DAT
        && (type_ == 'D'_ebcdic || type_ == 'E'_ebcdic || type_ == 'F'_ebcdic || type_ == 'G'_ebcdic
            || type_ == 'H'_ebcdic || type_ == 'K'_ebcdic || type_ == 'L'_ebcdic || type_ == 'P'_ebcdic
            || type_ == 'Z'_ebcdic);
}

symbol_attributes::value_t symbol_attributes::get_attribute_value(data_attr_kind attribute) const
{
    switch (attribute)
    {
        case data_attr_kind::T:
            return type_;
        case data_attr_kind::L:
            return length_ == undef_length ? 1 : length_;
        case data_attr_kind::S:
            return scale_ == undef_scale ? 0 : scale_;
        case data_attr_kind::I:
            return integer();
        default:
            return 0;
    }
}
} // namespace hlasm_plugin::parser_library::context
