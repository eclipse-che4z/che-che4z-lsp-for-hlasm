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

#include <cassert>
#include <format>
#include <iterator>
#include <optional>

#include "checking/diagnostic_collector.h"
#include "checking/instr_operand.h"
#include "data_def_types.h"

namespace hlasm_plugin::parser_library::checking {

// constructor for types that have same checking for DS and DC
data_def_type::data_def_type(data_definition_type type,
    char extension,
    modifier_spec bit_length_spec,
    modifier_spec length_spec,
    modifier_spec scale_spec,
    modifier_spec exponent_spec,
    nominal_value_type nominal_type,
    context::alignment implicit_alignment,
    implicit_length_t implicit_length,
    context::integer_type int_type,
    expects_single_symbol_t single_symbol,
    bool ignores_scale)
    : type_ext { (char)type, extension }
    , nominal_type(nominal_type)
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
    , single_symbol(single_symbol)
{
    assert(!std::holds_alternative<ignored>(bit_length_spec));
    assert(!std::holds_alternative<ignored>(length_spec));
}

// constructor for types that have different lengths with DS than DC
data_def_type::data_def_type(data_definition_type type,
    char extension,
    modifier_spec bit_length_spec,
    modifier_spec length_spec,
    int max_ds_length_spec,
    modifier_spec scale_spec,
    modifier_spec exponent_spec,
    nominal_value_type nominal_type,
    context::alignment implicit_alignment,
    implicit_length_t implicit_length,
    context::integer_type int_type)
    : type_ext { (char)type, extension }
    , nominal_type(nominal_type)
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
    assert(!std::holds_alternative<ignored>(bit_length_spec));
    assert(!std::holds_alternative<ignored>(length_spec));
}

// for example type X can have bit length 1 to 2048, byte length 1 to 256 with DC,
// with bit length allowed, for example type X can have bit length 1 to 2048, byte length 1 to 256 with DC,
// and may have byte length 1 to 65535 and bit lenght 1 to 65535*8 with DS. It does not take scale or exponent
// modifiers. It takes nominal value enclosed in apostrophes, has alignment to 1 byte and implicit length is as needed.
// data_def_type('X', '\0', modifier_bound{ 1, 2048 }, modifier_bound{ 1, 256 }, 65535, n_a(), n_a(),
// nominal_value_type::STRING, no_align, as_needed()) {}

uint64_t data_def_type::get_nominal_length(const reduced_nominal_value_t&) const
{
    // all types that have implicit length as needed must override this function
    assert(false);
    return uint64_t();
}

uint32_t data_def_type::get_nominal_length_attribute(const reduced_nominal_value_t&) const
{
    // all types that have implicit length as needed must override this function
    assert(false);
    return uint32_t();
}

int16_t data_def_type::get_implicit_scale(const reduced_nominal_value_t&) const
{
    // All types except P and Z have implicit scale 0.
    return 0;
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

namespace {
struct
{
    std::optional<range> operator()(const data_def_expr& e) const
    {
        if (e.ignored)
            return std::nullopt;
        if (e.ex_kind != expr_type::ABS)
            return e.rng;
        return std::nullopt;
    }
    std::optional<range> operator()(const data_def_address& a) const
    {
        if (a.ignored)
            return std::nullopt;
        if (!a.displacement.present || !a.base.present)
            return a.total;
        return std::nullopt;
    }
} const abs_or_addr;
} // namespace

bool data_def_type::check_nominal_type(
    const nominal_value_t& nominal, const diagnostic_collector& add_diagnostic, const range& r) const
{
    bool ret = true;
    switch (nominal_type)
    {
        case nominal_value_type::STRING:
            if (!std::holds_alternative<std::string>(nominal.value))
            {
                add_diagnostic(diagnostic_op::error_D018(r, type_str()));
                return false;
            }
            break;
        case nominal_value_type::EXPRESSIONS:
            if (!std::holds_alternative<nominal_value_expressions>(nominal.value))
            {
                add_diagnostic(diagnostic_op::error_D017(r, type_str()));
                return false;
            }
            for (auto& p : std::get<nominal_value_expressions>(nominal.value))
                if (std::holds_alternative<data_def_address>(p))
                {
                    const auto& adr = std::get<data_def_address>(p);
                    add_diagnostic(
                        diagnostic_op::error_D020({ adr.displacement.rng.start, adr.base.rng.end }, type_str()));
                    ret = false;
                }
            if (!ret)
                return false;
            break;
        case nominal_value_type::ADDRESS_OR_EXPRESSION:
            if (!std::holds_alternative<nominal_value_expressions>(nominal.value))
            {
                add_diagnostic(diagnostic_op::error_D017(r, type_str()));
                return false;
            }
            for (const auto& p : std::get<nominal_value_expressions>(nominal.value))
            {
                if (auto range_o = std::visit(abs_or_addr, p); range_o)
                {
                    add_diagnostic(diagnostic_op::error_D033(*range_o));
                    ret = false;
                }
            }
            if (!ret)
                return false;

            break;
        default:
            assert(false);
            return true;
    }
    return true;
}

bool data_def_type::check_impl(
    const data_definition_common&, const nominal_value_t&, const diagnostic_collector&, bool) const
{
    return true;
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
    else if (std::holds_alternative<as_needed>(implicit_length_))
        len_in_bits = get_nominal_length(rnv) * 8;
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

uint32_t data_def_type::get_length_attribute(
    const data_def_length_t& length, const reduced_nominal_value_t& nominal) const
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
    else if (std::holds_alternative<as_needed>(implicit_length_))
        return get_nominal_length_attribute(nominal);
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

const std::map<std::pair<char, char>, std::unique_ptr<const data_def_type>> types_and_extensions = []() {
    std::map<std::pair<char, char>, std::unique_ptr<const data_def_type>> ret;
    ret.emplace(std::make_pair('B', '\0'), std::make_unique<data_def_type_B>());
    ret.emplace(std::make_pair('C', '\0'), std::make_unique<data_def_type_C>());
    ret.emplace(std::make_pair('C', 'A'), std::make_unique<data_def_type_CA>());
    ret.emplace(std::make_pair('C', 'E'), std::make_unique<data_def_type_CE>());
    ret.emplace(std::make_pair('C', 'U'), std::make_unique<data_def_type_CU>());
    ret.emplace(std::make_pair('G', '\0'), std::make_unique<data_def_type_G>());
    ret.emplace(std::make_pair('X', '\0'), std::make_unique<data_def_type_X>());
    ret.emplace(std::make_pair('H', '\0'), std::make_unique<data_def_type_H>());
    ret.emplace(std::make_pair('F', '\0'), std::make_unique<data_def_type_F>());
    ret.emplace(std::make_pair('F', 'D'), std::make_unique<data_def_type_FD>());
    ret.emplace(std::make_pair('P', '\0'), std::make_unique<data_def_type_P>());
    ret.emplace(std::make_pair('Z', '\0'), std::make_unique<data_def_type_Z>());
    ret.emplace(std::make_pair('A', '\0'), std::make_unique<data_def_type_A>());
    ret.emplace(std::make_pair('A', 'D'), std::make_unique<data_def_type_AD>());
    ret.emplace(std::make_pair('Y', '\0'), std::make_unique<data_def_type_Y>());
    ret.emplace(std::make_pair('R', '\0'), std::make_unique<data_def_type_R>());
    ret.emplace(std::make_pair('R', 'D'), std::make_unique<data_def_type_RD>());
    ret.emplace(std::make_pair('S', '\0'), std::make_unique<data_def_type_S>());
    ret.emplace(std::make_pair('S', 'Y'), std::make_unique<data_def_type_SY>());
    ret.emplace(std::make_pair('V', '\0'), std::make_unique<data_def_type_V>());
    ret.emplace(std::make_pair('V', 'D'), std::make_unique<data_def_type_VD>());
    ret.emplace(std::make_pair('Q', '\0'), std::make_unique<data_def_type_Q>());
    ret.emplace(std::make_pair('Q', 'D'), std::make_unique<data_def_type_QD>());
    ret.emplace(std::make_pair('Q', 'Y'), std::make_unique<data_def_type_QY>());
    ret.emplace(std::make_pair('J', '\0'), std::make_unique<data_def_type_J>());
    ret.emplace(std::make_pair('J', 'D'), std::make_unique<data_def_type_JD>());
    ret.emplace(std::make_pair('E', '\0'), std::make_unique<data_def_type_E>());
    ret.emplace(std::make_pair('E', 'H'), std::make_unique<data_def_type_EH>());
    ret.emplace(std::make_pair('E', 'D'), std::make_unique<data_def_type_ED>());
    ret.emplace(std::make_pair('E', 'B'), std::make_unique<data_def_type_EB>());
    ret.emplace(std::make_pair('D', '\0'), std::make_unique<data_def_type_D>());
    ret.emplace(std::make_pair('D', 'H'), std::make_unique<data_def_type_DH>());
    ret.emplace(std::make_pair('D', 'D'), std::make_unique<data_def_type_DD>());
    ret.emplace(std::make_pair('D', 'B'), std::make_unique<data_def_type_DB>());
    ret.emplace(std::make_pair('L', '\0'), std::make_unique<data_def_type_L>());
    ret.emplace(std::make_pair('L', 'H'), std::make_unique<data_def_type_LH>());
    ret.emplace(std::make_pair('L', 'Q'), std::make_unique<data_def_type_LQ>());
    ret.emplace(std::make_pair('L', 'D'), std::make_unique<data_def_type_LD>());
    ret.emplace(std::make_pair('L', 'B'), std::make_unique<data_def_type_LB>());
    return ret;
}();

const data_def_type* data_def_type::access_data_def_type(char type, char extension)
{
    auto found = types_and_extensions.find({ type, extension });
    return found == types_and_extensions.end() ? nullptr : found->second.get();
}

data_def_type::~data_def_type() = default;

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
