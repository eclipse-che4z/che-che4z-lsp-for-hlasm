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

#ifndef CONTEXT_SYMBOL_ATTRIBUTES_H
#define CONTEXT_SYMBOL_ATTRIBUTES_H

#include <cstdint>
#include <string_view>

#include "context/common_types.h"

namespace hlasm_plugin::parser_library::context {

// enumeration of all data attributes
enum class data_attr_kind : unsigned char
{
    UNKNOWN,
    T,
    L,
    S,
    I,
    K,
    N,
    D,
    O,

    max = O,
};

// tells how symbol is created
// whether it is section definition, machine label, equated, data definition symbol, or created by some other ASM
// instruction
enum class symbol_origin : unsigned char
{
    SECT,
    MACH,
    EQU,
    DAT,
    ASM,
    UNKNOWN
};

struct program_type
{
    char ebcdic_value[4] = {};
    bool valid = false;

    program_type() = default;
    explicit constexpr program_type(std::uint32_t v) noexcept
        : valid(true)
    {
        for (int i = 3; i != -1; --i)
        {
            ebcdic_value[i] = v & 0b1111'1111U;
            v >>= 8;
        }
    }

    constexpr bool operator==(const program_type&) const noexcept = default;
};

enum class assembler_type : unsigned char
{
    NONE,
    AR,
    CR,
    CR32,
    CR64,
    FPR,
    GR,
    GR32,
    GR64,
    VR,
};

assembler_type assembler_type_from_string(std::string_view s) noexcept;
std::string_view assembler_type_to_string(assembler_type t) noexcept;

// structure wrapping attributes of the symbol
// the structure fields are to be constant except undefined fields, their value can be defined later
struct symbol_attributes
{
    using value_t = int32_t;
    using type_attr = uint16_t;
    using len_attr = uint32_t;
    using scale_attr = int16_t;
    using program_type = program_type;
    using assembler_type = assembler_type;

    // static field describing undefined states of attributes
    static constexpr type_attr undef_type = 0xe4;
    static constexpr len_attr undef_length = static_cast<len_attr>(-1);
    static constexpr scale_attr undef_scale = 32767;

    // predefined symbol_attributes classes
    static symbol_attributes make_section_attrs();
    static symbol_attributes make_machine_attrs(len_attr);
    static symbol_attributes make_extrn_attrs();
    static symbol_attributes make_wxtrn_attrs();
    static symbol_attributes make_org_attrs();
    static symbol_attributes make_ccw_attrs();
    static symbol_attributes make_cnop_attrs();

    // helper function to transform char to enum
    static data_attr_kind transform_attr(unsigned char c);

    static bool requires_ordinary_symbol(data_attr_kind attribute);
    static bool is_ordinary_attribute(data_attr_kind attribute);
    static value_t default_value(data_attr_kind attribute);
    static SET_t default_ca_value(data_attr_kind attribute);

    constexpr symbol_attributes(symbol_origin origin,
        type_attr type,
        len_attr length = undef_length,
        scale_attr scale = undef_scale,
        len_attr integer = undef_length,
        program_type prog_type = {},
        assembler_type asm_type = assembler_type::NONE) noexcept
        : length_(length)
        , integer_(integer)
        , type_(type)
        , scale_(scale)
        , origin_(origin)
        , prog_type_(prog_type)
        , asm_type_(asm_type)
    {}

    explicit constexpr symbol_attributes(symbol_origin origin) noexcept
        : length_(undef_length)
        , integer_(undef_length)
        , type_(undef_type)
        , scale_(undef_scale)
        , origin_(origin)
        , prog_type_()
        , asm_type_(assembler_type::NONE)
    {}

    symbol_origin origin() const noexcept { return origin_; }
    type_attr type() const noexcept { return type_; }
    len_attr length() const noexcept { return length_; }
    scale_attr scale() const noexcept { return scale_; }
    len_attr integer() const noexcept { return integer_; }
    assembler_type asm_type() const noexcept { return asm_type_; }
    program_type prog_type() const noexcept { return prog_type_; }

    bool is_defined(data_attr_kind attribute) const;

    bool can_have_SI_attr() const;

    value_t get_attribute_value(data_attr_kind attribute) const;

    // sets length if undefined
    void length(len_attr value);
    // sets scale if undefined
    void scale(scale_attr value);

private:
    len_attr length_;
    len_attr integer_;
    type_attr type_;
    scale_attr scale_;
    symbol_origin origin_;
    program_type prog_type_;
    assembler_type asm_type_;
};

} // namespace hlasm_plugin::parser_library::context
#endif
