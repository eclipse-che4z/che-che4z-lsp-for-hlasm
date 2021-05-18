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

#include "checking/instr_operand.h"
#include "data_def_types.h"

using namespace hlasm_plugin::parser_library::checking;
using namespace hlasm_plugin::parser_library;

std::string data_def_type::init_type_str(char type, char extension)
{
    std::string type_str;
    type_str.push_back(type);
    if (extension)
        type_str.push_back(extension);
    return type_str;
}

// constructor for types that have same checking for DS and DC
data_def_type::data_def_type(char type,
    char extension,
    modifier_spec bit_length_spec,
    modifier_spec length_spec,
    modifier_spec scale_spec,
    modifier_spec exponent_spec,
    nominal_value_type nominal_type,
    context::alignment implicit_alignment,
    implicit_length_t implicit_length)
    : type(type)
    , extension(extension)
    , nominal_type(nominal_type)
    , bit_length_spec_(bit_length_spec)
    , length_spec_(length_spec)
    , ds_length_spec_(length_spec)
    , ds_bit_length_spec_(bit_length_spec)
    , scale_spec_(scale_spec)
    , exponent_spec_(exponent_spec)
    , alignment_(implicit_alignment)
    , implicit_length_(implicit_length)
{
    type_str = init_type_str(type, extension);
}

// constructor for types that have different lengths with DS than DC
data_def_type::data_def_type(char type,
    char extension,
    modifier_spec bit_length_spec,
    modifier_spec length_spec,
    int max_ds_length_spec,
    modifier_spec scale_spec,
    modifier_spec exponent_spec,
    nominal_value_type nominal_type,
    context::alignment implicit_alignment,
    implicit_length_t implicit_length)
    : type(type)
    , extension(extension)
    , nominal_type(nominal_type)
    , bit_length_spec_(bit_length_spec)
    , length_spec_(length_spec)
    , ds_length_spec_(modifier_bound { 1, max_ds_length_spec })
    , ds_bit_length_spec_(modifier_bound { 1, max_ds_length_spec * 8 })
    , scale_spec_(scale_spec)
    , exponent_spec_(exponent_spec)
    , alignment_(implicit_alignment)
    , implicit_length_(implicit_length)
{
    type_str = init_type_str(type, extension);
}

// for example type X can have bit length 1 to 2048, byte length 1 to 256 with DC,
// with bit length allowed, for example type X can have bit length 1 to 2048, byte length 1 to 256 with DC,
// and may have byte length 1 to 65535 and bit lenght 1 to 65535*8 with DS. It does not take scale or exponent
// modifiers. It takes nominal value enclosed in apostrophes, has alignment to 1 byte and implicit length is as needed.
// data_def_type('X', '\0', modifier_bound{ 1, 2048 }, modifier_bound{ 1, 256 }, 65535, n_a(), n_a(),
// nominal_value_type::STRING, no_align, as_needed()) {}

template<data_instr_type instr_type>
bool data_def_type::check(const data_definition_operand& op, const diagnostic_collector& add_diagnostic) const
{
    auto [ret, check_nom] = check_base<instr_type>(op, add_diagnostic);
    ret &= check(op, add_diagnostic, check_nom);
    if (!ret)
        return false;
    // if operand is ok, we can call get_length and check if it is not too long
    if (get_length(op) >= (1ll << 31) * 8)
    {
        add_diagnostic(diagnostic_op::error_D028(op.operand_range));
        return false;
    }

    return true;
}

bool data_def_type::check_DC(const data_definition_operand& op, const diagnostic_collector& add_diagnostic) const
{
    return check<data_instr_type::DC>(op, add_diagnostic);
}

bool data_def_type::check_DS(const data_definition_operand& op, const diagnostic_collector& add_diagnostic) const
{
    return check<data_instr_type::DS>(op, add_diagnostic);
}

bool data_def_type::expects_single_symbol() const { return false; }


template<typename field_val_T>
bool check_modifier(const data_def_field<field_val_T>& modifier,
    const std::string& type_str,
    const std::string& modifier_name,
    modifier_spec bound,
    const diagnostic_collector& add_diagnostic)
{
    if (!modifier.present || std::holds_alternative<no_check>(bound))
        return true;
    if (std::holds_alternative<ignored>(bound))
    {
        add_diagnostic(diagnostic_op::warn_D025(modifier.rng, type_str, modifier_name));
        return true;
    }
    if (std::holds_alternative<n_a>(bound))
    {
        // modifier not allowed with this type
        add_diagnostic(diagnostic_op::error_D009(modifier.rng, type_str, modifier_name));
        return false;
    }
    if (modifier.value < std::get<modifier_bound>(bound).min || modifier.value > std::get<modifier_bound>(bound).max)
    {
        // modifier out of bounds
        add_diagnostic(diagnostic_op::error_D008(modifier.rng,
            type_str,
            modifier_name,
            std::get<modifier_bound>(bound).min,
            std::get<modifier_bound>(bound).max));
        return false;
    }
    return true;
}

uint64_t data_def_type::get_nominal_length(const nominal_value_t&) const
{
    // all types that have implicit length as needed must override this function
    assert(false);
    return uint64_t();
}

uint32_t data_def_type::get_nominal_length_attribute(const nominal_value_t&) const
{
    // all types that have implicit length as needed must override this function
    assert(false);
    return uint32_t();
}

int16_t data_def_type::get_implicit_scale(const nominal_value_t&) const
{
    // All types except P and Z have implicit scale 0.
    return 0;
}

int32_t data_def_type::get_integer_attribute_impl(uint32_t, int32_t) const
{
    // Types that do not have integer specifier return 0.
    return 0;
}

template<>
std::pair<bool, bool> data_def_type::check_nominal_present<data_instr_type::DC>(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic) const
{
    // nominal value can be omitted with DC when duplication factor is 0.
    bool ret = true;
    bool check_nom = true;
    if (op.dupl_factor.value == 0 && op.dupl_factor.present)
        check_nom = false;

    if (!op.nominal_value.present && check_nom)
    {
        add_diagnostic(diagnostic_op::error_D016(op.operand_range));
        ret = false;
        check_nom = false;
    }
    else if (op.nominal_value.present) // however if nominal value present, it must be valid
        check_nom = true;

    return { ret, check_nom };
}

template<>
std::pair<bool, bool> data_def_type::check_nominal_present<data_instr_type::DS>(
    const data_definition_operand& op, const diagnostic_collector&) const
{
    // DS does not require nominal value
    // however if nominal value present, it must be valid
    if (op.nominal_value.present)
        return { true, true };
    else
        return { true, false };
}
template<data_instr_type instr_type>
modifier_spec data_def_type::get_length_spec() const
{
    if constexpr (instr_type == data_instr_type::DC)
        return length_spec_;
    else
        return ds_length_spec_;
}

template<data_instr_type instr_type>
modifier_spec data_def_type::get_bit_length_spec() const
{
    if constexpr (instr_type == data_instr_type::DC)
        return bit_length_spec_;
    else
        return ds_bit_length_spec_;
}

bool data_def_type::check_dupl_factor(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic) const
{
    if (op.dupl_factor.present)
        if (op.dupl_factor.value < 0)
        {
            // Duplication factor must be non negative
            add_diagnostic(diagnostic_op::error_D011(op.dupl_factor.rng));
            return false;
        }
    return true;
}

template<data_instr_type instr_type>
bool data_def_type::check_length(const data_definition_operand& op, const diagnostic_collector& add_diagnostic) const
{
    if (std::holds_alternative<n_a>(bit_length_spec_) && op.length.len_type == data_def_length_t::length_type::BIT)
    {
        // bit length not allowed with this type
        add_diagnostic(diagnostic_op::error_D007(op.length.rng, type_str));
        return false;
    }
    else
    {
        if (op.length.len_type == data_def_length_t::length_type::BIT)
            return check_modifier(op.length, type_str, "bit length", get_bit_length_spec<instr_type>(), add_diagnostic);
        else
            return check_modifier(op.length, type_str, "length", get_length_spec<instr_type>(), add_diagnostic);
    }
}

bool data_def_type::check_nominal_type(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic) const
{
    bool ret = true;
    switch (nominal_type)
    {
        case nominal_value_type::STRING:
            if (!std::holds_alternative<std::string>(op.nominal_value.value))
            {
                add_diagnostic(diagnostic_op::error_D018(op.operand_range, type_str));
                return false;
            }
            break;
        case nominal_value_type::EXPRESSIONS:
            if (!std::holds_alternative<nominal_value_expressions>(op.nominal_value.value))
            {
                add_diagnostic(diagnostic_op::error_D017(op.operand_range, type_str));
                return false;
            }
            for (auto& p : std::get<nominal_value_expressions>(op.nominal_value.value))
                if (std::holds_alternative<data_def_address>(p))
                {
                    auto adr = std::get<data_def_address>(p);
                    add_diagnostic(
                        diagnostic_op::error_D020({ adr.displacement.rng.start, adr.base.rng.end }, type_str));
                    ret = false;
                }
            if (!ret)
                return false;
            break;
        case nominal_value_type::ADDRESS_OR_EXPRESSION:
            if (!std::holds_alternative<nominal_value_expressions>(op.nominal_value.value))
            {
                add_diagnostic(diagnostic_op::error_D017(op.operand_range, type_str));
                return false;
            }
            break;
        default:
            assert(false);
            return true;
    }
    return true;
}


template<data_instr_type instr_type>
std::pair<bool, bool> data_def_type::check_base(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic) const
{
    assert(op.type.value == type);
    assert(op.extension.value == extension);
    bool ret = check_dupl_factor(op, add_diagnostic);
    ret &= check_length<instr_type>(op, add_diagnostic);
    ret &= check_modifier(op.scale, type_str, "scale", scale_spec_, add_diagnostic);
    ret &= check_modifier(op.exponent, type_str, "exponent", exponent_spec_, add_diagnostic);

    auto [nom_present_ok, check_nom] = check_nominal_present<instr_type>(op, add_diagnostic);
    ret &= nom_present_ok;
    if (check_nom)
    {
        bool nom_type_ok = check_nominal_type(op, add_diagnostic);
        ret &= nom_type_ok;
        check_nom &= nom_type_ok;
    }

    return { ret, check_nom };
}

bool data_def_type::check(const data_definition_operand&, const diagnostic_collector&, bool) const { return true; }

context::alignment data_def_type::get_alignment(bool length_present) const
{
    if (length_present)
        return context::no_align;
    else
        return alignment_;
}

size_t data_def_type::get_number_of_values_in_nominal(const nominal_value_t& nom) const
{
    if (type == 'C' || type == 'G') // C and G do not support multiple nominal values
        return 1;
    else if (std::holds_alternative<std::string>(nom.value))
    {
        const std::string& s = std::get<std::string>(nom.value);
        return std::count(s.begin(), s.end(), ',') + 1;
    }
    else
        return std::get<nominal_value_expressions>(nom.value).size();
}

// this function assumes, that the operand is already checked and was OK
uint64_t data_def_type::get_length(const data_definition_operand& op) const
{
    uint64_t len_in_bits;
    if (op.length.present)
    {
        uint64_t val_count = get_number_of_values_in_nominal(op.nominal_value);
        len_in_bits = val_count * op.length.value;

        if (op.length.len_type == data_def_length_t::BYTE)
            len_in_bits *= 8;
    }
    else if (std::holds_alternative<as_needed>(implicit_length_))
        len_in_bits = get_nominal_length(op.nominal_value) * 8;
    else if (!op.nominal_value.present)
        len_in_bits = std::get<uint64_t>(implicit_length_) * 8;
    else
    {
        uint64_t val_count = get_number_of_values_in_nominal(op.nominal_value);
        len_in_bits = val_count * std::get<uint64_t>(implicit_length_) * 8;
    }
    if (op.dupl_factor.present)
        len_in_bits *= (uint64_t)op.dupl_factor.value;
    return len_in_bits;
}

uint32_t data_def_type::get_length_attribute(const data_def_length_t& length, const nominal_value_t& nominal) const
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

int16_t data_def_type::get_scale_attribute(const scale_modifier_t& scale, const nominal_value_t& nominal) const
{
    if (scale.present)
        return scale.value;
    else
        return get_implicit_scale(nominal);
}

int32_t data_def_type::get_integer_attribute(
    const data_def_length_t& length, const scale_modifier_t& scale, const nominal_value_t& nominal) const
{
    uint32_t L = get_length_attribute(length, nominal);
    int32_t S = get_scale_attribute(scale, nominal);

    // Types, that do not have integer specified return 0;
    return get_integer_attribute_impl(L, S);
}

const std::map<std::pair<char, char>, std::unique_ptr<const data_def_type>> data_def_type::types_and_extensions = []() {
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

data_def_type::~data_def_type() {}

template bool data_def_type::check<data_instr_type::DC>(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic) const;
template bool data_def_type::check<data_instr_type::DS>(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic) const;
