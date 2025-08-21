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

#include "data_def_type_base.h"

#include <algorithm>
#include <cassert>
#include <format>
#include <iterator>

#include "checking/instr_operand.h"
#include "context/ordinary_assembly/symbol_attributes.h"
#include "utils/insist.h"

namespace hlasm_plugin::parser_library::checking {

// constructor for types that have same checking for DS and DC
consteval data_def_type::data_def_type(data_definition_type type,
    char extension,
    modifier_spec bit_length_spec,
    modifier_spec length_spec,
    modifier_spec scale_spec,
    modifier_spec exponent_spec,
    context::alignment implicit_alignment,
    implicit_length_t implicit_length,
    context::integer_type int_type,
    bool ignores_scale) noexcept
    : type_ext { (char)type, extension }
    , bit_length_spec_(bit_length_spec)
    , length_spec_(length_spec)
    , ds_length_spec_(length_spec)
    , ds_bit_length_spec_(bit_length_spec)
    , scale_spec_(scale_spec)
    , exponent_spec_(exponent_spec)
    , alignment_(implicit_alignment)
    , implicit_length_(implicit_length)
    , int_type_(int_type)
    , ignores_scale_(ignores_scale)
{
    utils::insist(!std::holds_alternative<ignored>(bit_length_spec));
    utils::insist(!std::holds_alternative<ignored>(length_spec));
}

// constructor for types that have different lengths with DS than DC
consteval data_def_type::data_def_type(data_definition_type type,
    char extension,
    modifier_spec bit_length_spec,
    modifier_spec length_spec,
    int max_ds_length_spec,
    modifier_spec scale_spec,
    modifier_spec exponent_spec,
    context::alignment implicit_alignment,
    implicit_length_t implicit_length,
    context::integer_type int_type) noexcept
    : type_ext { (char)type, extension }
    , bit_length_spec_(bit_length_spec)
    , length_spec_(length_spec)
    , ds_length_spec_(modifier_bound { 1, max_ds_length_spec })
    , ds_bit_length_spec_(modifier_bound { 1, max_ds_length_spec * 8 })
    , scale_spec_(scale_spec)
    , exponent_spec_(exponent_spec)
    , alignment_(implicit_alignment)
    , implicit_length_(implicit_length)
    , int_type_(int_type)
{
    utils::insist(!std::holds_alternative<ignored>(bit_length_spec));
    utils::insist(!std::holds_alternative<ignored>(length_spec));
}

// for example type X can have bit length 1 to 2048, byte length 1 to 256 with DC,
// with bit length allowed, for example type X can have bit length 1 to 2048, byte length 1 to 256 with DC,
// and may have byte length 1 to 65535 and bit lenght 1 to 65535*8 with DS. It does not take scale or exponent
// modifiers. It takes nominal value enclosed in apostrophes, has alignment to 1 byte and implicit length is as needed.
// data_def_type('X', '\0', modifier_bound{ 1, 2048 }, modifier_bound{ 1, 256 }, 65535, n_a(), n_a(),
// nominal_value_type::STRING, no_align, as_needed()) {}

int16_t data_def_type::get_implicit_scale(const reduced_nominal_value_t& op) const
{
    switch (type())
    {
        case data_definition_type::P:
        case data_definition_type::Z: {
            if (!std::holds_alternative<std::string_view>(op))
                return 0;
            // Count number of characters between the first . and first ,

            uint16_t count = 0;
            bool do_count = false;
            for (char c : std::get<std::string_view>(op))
            {
                if (c == ',')
                    break;

                if (c == '.')
                    do_count = true;
                else if (do_count)
                    ++count;
            }
            return count;
        }

        default:
            // All types except P and Z have implicit scale 0.
            return 0;
    }
}

modifier_spec data_def_type::get_length_spec(data_instr_type instr_type) const
{
    if (instr_type == data_instr_type::DC)
        return length_spec_;
    else
        return ds_length_spec_;
}

modifier_spec data_def_type::get_bit_length_spec(data_instr_type instr_type) const
{
    if (instr_type == data_instr_type::DC)
        return bit_length_spec_;
    else
        return ds_bit_length_spec_;
}

size_t data_def_type::get_number_of_values_in_nominal(const reduced_nominal_value_t& nom) const
{
    if (type() == data_definition_type::C || type() == data_definition_type::G)
        return 1; // C and G do not support multiple nominal values
    else if (std::holds_alternative<std::string_view>(nom))
        return std::ranges::count(std::get<std::string_view>(nom), ',') + 1;
    else if (std::holds_alternative<size_t>(nom))
        return std::get<size_t>(nom);
    else
        return 1;
}

// this function assumes, that the operand is already checked and was OK
uint64_t data_def_type::get_length(
    int32_t dupl_factor, int32_t length, bool length_in_bits, const reduced_nominal_value_t& rnv) const
{
    uint64_t len_in_bits;
    if (length >= 0)
    {
        uint64_t val_count = get_number_of_values_in_nominal(rnv);
        len_in_bits = val_count * length;

        if (!length_in_bits)
            len_in_bits *= 8;
    }
    else if (const auto* an = std::get_if<as_needed>(&implicit_length_))
    {
        if (std::holds_alternative<std::monostate>(rnv))
            len_in_bits = an->get_empty_length();
        else if (!std::holds_alternative<std::string_view>(rnv))
            len_in_bits = an->get_error_length();
        else
            len_in_bits = an->get_nominal_length(std::get<std::string_view>(rnv));
        len_in_bits *= 8;
    }
    else if (std::holds_alternative<std::monostate>(rnv))
        len_in_bits = std::get<uint64_t>(implicit_length_) * 8;
    else
    {
        uint64_t val_count = get_number_of_values_in_nominal(rnv);
        len_in_bits = val_count * std::get<uint64_t>(implicit_length_) * 8;
    }
    if (dupl_factor >= 0)
        len_in_bits *= (uint64_t)dupl_factor;
    return len_in_bits;
}

uint32_t data_def_type::get_length_attribute(const data_def_length_t& length, const reduced_nominal_value_t& rnv) const
{
    if (length.present)
    {
        uint32_t len_attr;
        len_attr = length.value;
        if (length.len_type == data_def_length_t::BIT)
        {
            len_attr = round_up(length.value, 8);
            len_attr /= 8;
        }
        return len_attr;
    }
    else if (const auto* an = std::get_if<as_needed>(&implicit_length_))
    {
        if (std::holds_alternative<std::monostate>(rnv))
            return an->get_empty_length();
        else if (!std::holds_alternative<std::string_view>(rnv))
            return an->get_error_length();
        else
            return an->get_nominal_length_attribute(std::get<std::string_view>(rnv));
    }
    else
        return (uint32_t)std::get<uint64_t>(implicit_length_);
}

int16_t data_def_type::get_scale_attribute(const scale_modifier_t& scale, const reduced_nominal_value_t& nominal) const
{
    if (ignores_scale())
        return 0;
    else if (scale.present)
        return scale.value;
    else
        return get_implicit_scale(nominal);
}

consteval data_def_type data_def_type_B()
{
    return data_def_type(data_definition_type::B,
        '\0',
        modifier_bound { 1, 2048 },
        modifier_bound { 1, 256 },
        n_a(),
        n_a(),
        context::no_align,
        as_needed(B_nominal_extras),
        context::integer_type::undefined);
}

consteval data_def_type data_def_type_CA_CE(char extension)
{
    return data_def_type(data_definition_type::C,
        extension,
        modifier_bound { 1, 2048 },
        modifier_bound { 1, 256 },
        65535,
        n_a(),
        n_a(),
        context::no_align,
        as_needed(CA_CE_nominal_extras),
        context::integer_type::undefined);
}

consteval data_def_type data_def_type_C() { return data_def_type_CA_CE('\0'); }

consteval data_def_type data_def_type_CA() { return data_def_type_CA_CE('A'); }

consteval data_def_type data_def_type_CE() { return data_def_type_CA_CE('E'); }

consteval data_def_type data_def_type_CU()
{
    return data_def_type(data_definition_type::C,
        'U',
        n_a(),
        modifier_bound { 1, 256, true },
        n_a(),
        n_a(),
        context::no_align,
        as_needed(CU_nominal_extras),
        context::integer_type::undefined);
}

consteval data_def_type data_def_type_G()
{
    return data_def_type(data_definition_type::G,
        '\0',
        n_a(),
        modifier_bound { 1, 256, true },
        65534,
        n_a(),
        n_a(),
        context::no_align,
        as_needed(G_nominal_extras),
        context::integer_type::undefined);
}

consteval data_def_type data_def_type_X()
{
    return data_def_type(data_definition_type::X,
        '\0',
        modifier_bound { 1, 2048 },
        modifier_bound { 1, 256 },
        65535,
        n_a(),
        n_a(),
        context::no_align,
        as_needed(X_nominal_extras),
        context::integer_type::undefined);
}

consteval data_def_type data_def_type_H_F_FD(data_definition_type type, char extension, uint8_t word_length)
{
    return data_def_type(type,
        extension,
        modifier_bound { 1, 64 },
        modifier_bound { 1, 8 },
        modifier_bound { -187, 346 },
        modifier_bound { -85, 75 },
        { 0, word_length },
        word_length,
        context::integer_type::fixed);
}

consteval data_def_type data_def_type_H() { return data_def_type_H_F_FD(data_definition_type::H, '\0', 2); }

consteval data_def_type data_def_type_F() { return data_def_type_H_F_FD(data_definition_type::F, '\0', 4); }

consteval data_def_type data_def_type_FD() { return data_def_type_H_F_FD(data_definition_type::F, 'D', 8); }

consteval data_def_type data_def_type_P_Z(data_definition_type type, context::integer_type int_type, as_needed extras)
{
    return data_def_type(type,
        '\0',
        modifier_bound { 1, 128 },
        modifier_bound { 1, 16 },
        n_a(),
        n_a(),
        context::no_align,
        extras,
        int_type);
}

consteval data_def_type data_def_type_P()
{
    return data_def_type_P_Z(data_definition_type::P, context::integer_type::packed, as_needed(P_nominal_extras));
}

consteval data_def_type data_def_type_Z()
{
    return data_def_type_P_Z(data_definition_type::Z, context::integer_type::zoned, as_needed(Z_nominal_extras));
}

consteval data_def_type data_def_type_A_AD_Y(
    data_definition_type type, char extension, context::alignment align, unsigned char implicit_length)
{
    return data_def_type(type,
        extension,
        modifier_bound { 1, implicit_length * 8 },
        modifier_bound { 1, implicit_length },
        n_a(),
        n_a(),
        align,
        implicit_length,
        context::integer_type::undefined);
}

consteval data_def_type data_def_type_A()
{
    return data_def_type_A_AD_Y(data_definition_type::A, '\0', context::fullword, 4);
}

consteval data_def_type data_def_type_AD()
{
    return data_def_type_A_AD_Y(data_definition_type::A, 'D', context::doubleword, 8);
}

consteval data_def_type data_def_type_Y()
{
    return data_def_type_A_AD_Y(data_definition_type::Y, '\0', context::halfword, 2);
}

consteval data_def_type data_def_type_S_SY(char extension, int size)
{
    return data_def_type(data_definition_type::S,
        extension,
        n_a(),
        modifier_bound { size, size },
        n_a(),
        n_a(),
        context::halfword,
        (unsigned long long)size,
        context::integer_type::undefined);
}

consteval data_def_type data_def_type_S() { return data_def_type_S_SY('\0', 2); }

consteval data_def_type data_def_type_SY() { return data_def_type_S_SY('Y', 3); }

consteval data_def_type data_def_type_single_symbol(data_definition_type type,
    char extension,
    modifier_spec length_bound,
    context::alignment align,
    uint64_t implicit_length)
{
    return data_def_type(
        type, extension, n_a(), length_bound, n_a(), n_a(), align, implicit_length, context::integer_type::undefined);
}

consteval data_def_type data_def_type_R()
{
    return data_def_type_single_symbol(data_definition_type::R, '\0', modifier_bound { 3, 4 }, context::fullword, 4);
}

consteval data_def_type data_def_type_RD()
{
    return data_def_type_single_symbol(data_definition_type::R, 'D', bound_list { 3, 4, 8 }, context::doubleword, 8);
}

consteval data_def_type data_def_type_V()
{
    return data_def_type_single_symbol(data_definition_type::V, '\0', modifier_bound { 3, 4 }, context::fullword, 4);
}

consteval data_def_type data_def_type_VD()
{
    return data_def_type_single_symbol(data_definition_type::V, 'D', bound_list { 3, 4, 8 }, context::doubleword, 8);
}

consteval data_def_type data_def_type_Q()
{
    return data_def_type_single_symbol(data_definition_type::Q, '\0', modifier_bound { 1, 4 }, context::fullword, 4);
}

consteval data_def_type data_def_type_QD()
{
    return data_def_type_single_symbol(data_definition_type::Q, 'D', modifier_bound { 1, 8 }, context::quadword, 8);
}

consteval data_def_type data_def_type_QY()
{
    return data_def_type_single_symbol(data_definition_type::Q, 'Y', modifier_bound { 3, 3 }, context::halfword, 3);
}

consteval data_def_type data_def_type_J()
{
    return data_def_type_single_symbol(data_definition_type::J, '\0', bound_list { 2, 3, 4 }, context::fullword, 4);
}

consteval data_def_type data_def_type_JD()
{
    return data_def_type_single_symbol(data_definition_type::J, 'D', bound_list { 2, 3, 4, 8 }, context::doubleword, 8);
}

consteval data_def_type data_def_type_E_D_L(data_definition_type type,
    char extension,
    modifier_spec bit_length_spec,
    modifier_spec length_spec,
    modifier_spec scale_spec,
    context::alignment align,
    uint64_t implicit_length)
{
    return data_def_type(type,
        extension,
        bit_length_spec,
        length_spec,
        scale_spec,
        modifier_bound { -85, 75 },
        align,
        implicit_length,
        context::integer_type::hexfloat,
        extension == 'D' || extension == 'B');
}

consteval data_def_type data_def_type_E()
{
    return data_def_type_E_D_L(data_definition_type::E,
        '\0',
        modifier_bound { 1, 64 },
        modifier_bound { 1, 8 },
        modifier_bound { 0, 5 },
        context::fullword,
        4);
}

consteval data_def_type data_def_type_EH()
{
    return data_def_type_E_D_L(data_definition_type::E,
        'H',
        modifier_bound { 12, 64 },
        modifier_bound { 1, 8 },
        modifier_bound { 0, 5 },
        context::fullword,
        4);
}

consteval data_def_type data_def_type_ED()
{
    return data_def_type_E_D_L(data_definition_type::E,
        'D',
        modifier_bound { 32, 32 },
        modifier_bound { 4, 4 },
        ignored(),
        context::fullword,
        4);
}

consteval data_def_type data_def_type_EB()
{
    return data_def_type_E_D_L(data_definition_type::E,
        'B',
        modifier_bound { 32, 32 },
        modifier_bound { 4, 4 },
        ignored(),
        context::fullword,
        4);
}

consteval data_def_type data_def_type_D()
{
    return data_def_type_E_D_L(data_definition_type::D,
        '\0',
        modifier_bound { 1, 64 },
        modifier_bound { 1, 8 },
        modifier_bound { 0, 13 },
        context::doubleword,
        8);
}

consteval data_def_type data_def_type_DH()
{
    return data_def_type_E_D_L(data_definition_type::D,
        'H',
        modifier_bound { 12, 64 },
        modifier_bound { 1, 8 },
        modifier_bound { 0, 13 },
        context::doubleword,
        8);
}

consteval data_def_type data_def_type_DB()
{
    return data_def_type_E_D_L(data_definition_type::D,
        'B',
        modifier_bound { 64, 64 },
        modifier_bound { 8, 8 },
        ignored(),
        context::doubleword,
        8);
}

consteval data_def_type data_def_type_DD()
{
    return data_def_type_E_D_L(data_definition_type::D,
        'D',
        modifier_bound { 64, 64 },
        modifier_bound { 8, 8 },
        ignored(),
        context::doubleword,
        8);
}

consteval data_def_type data_def_type_L()
{
    return data_def_type_E_D_L(data_definition_type::L,
        '\0',
        modifier_bound { 1, 128 },
        modifier_bound { 1, 16 },
        modifier_bound { 0, 27 },
        context::doubleword,
        16);
}

consteval data_def_type data_def_type_LH()
{
    return data_def_type_E_D_L(data_definition_type::L,
        'H',
        modifier_bound { 12, 128 },
        modifier_bound { 1, 16 },
        modifier_bound { 0, 27 },
        context::doubleword,
        16);
}

consteval data_def_type data_def_type_LQ()
{
    return data_def_type_E_D_L(data_definition_type::L,
        'Q',
        modifier_bound { 12, 128 },
        modifier_bound { 1, 16 },
        modifier_bound { 0, 27 },
        context::quadword,
        16);
}

consteval data_def_type data_def_type_LD()
{
    return data_def_type_E_D_L(data_definition_type::L,
        'D',
        modifier_bound { 128, 128 },
        modifier_bound { 16, 16 },
        ignored(),
        context::doubleword,
        16);
}

consteval data_def_type data_def_type_LB()
{
    return data_def_type_E_D_L(data_definition_type::L,
        'B',
        modifier_bound { 128, 128 },
        modifier_bound { 16, 16 },
        ignored(),
        context::doubleword,
        16);
}

constexpr data_def_type types_and_extensions[] = {
    data_def_type_A(),
    data_def_type_AD(),
    data_def_type_B(),
    data_def_type_C(),
    data_def_type_CA(),
    data_def_type_CE(),
    data_def_type_CU(),
    data_def_type_D(),
    data_def_type_DB(),
    data_def_type_DD(),
    data_def_type_DH(),
    data_def_type_E(),
    data_def_type_EB(),
    data_def_type_ED(),
    data_def_type_EH(),
    data_def_type_F(),
    data_def_type_FD(),
    data_def_type_G(),
    data_def_type_H(),
    data_def_type_J(),
    data_def_type_JD(),
    data_def_type_L(),
    data_def_type_LB(),
    data_def_type_LD(),
    data_def_type_LH(),
    data_def_type_LQ(),
    data_def_type_P(),
    data_def_type_Q(),
    data_def_type_QD(),
    data_def_type_QY(),
    data_def_type_R(),
    data_def_type_RD(),
    data_def_type_S(),
    data_def_type_SY(),
    data_def_type_V(),
    data_def_type_VD(),
    data_def_type_X(),
    data_def_type_Y(),
    data_def_type_Z(),
};
static_assert(std::ranges::is_sorted(types_and_extensions, {}, &data_def_type::type_ext));

constexpr auto types_and_extensions_search = []() {
    std::array<decltype(data_def_type::type_ext), std::size(types_and_extensions)> result {};

    std::ranges::transform(types_and_extensions, result.data(), &data_def_type::type_ext);

    return result;
}();

static_assert(std::ranges::equal(types_and_extensions, types_and_extensions_search, {}, &data_def_type::type_ext, {}));

const data_def_type* data_def_type::access_data_def_type(char type, char extension)
{
    const auto found = std::ranges::find(types_and_extensions_search, std::array { type, extension });
    if (found == std::ranges::end(types_and_extensions_search))
        return nullptr;
    return &types_and_extensions[std::ranges::distance(std::ranges::begin(types_and_extensions_search), found)];
}

std::string bound_list::to_diag_list() const
{
    auto remaining = m_allowed.count();
    std::string s;
    for (size_t i = 0; i < m_allowed.size(); ++i)
    {
        if (!m_allowed.test(i))
            continue;
        --remaining;
        if (s.empty())
            std::format_to(std::back_inserter(s), "{}", i);
        else if (remaining > 0)
            std::format_to(std::back_inserter(s), ", {}", i);
        else
            std::format_to(std::back_inserter(s), " or {}", i);
    }
    return s;
}

} // namespace hlasm_plugin::parser_library::checking
