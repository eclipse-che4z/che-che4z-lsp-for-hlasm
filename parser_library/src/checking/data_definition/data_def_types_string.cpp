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

// This file contains implementation of the data_def_type for
// these types: B, C, G, X

#include <concepts>

#include "checking/checker_helper.h"
#include "checking/diagnostic_collector.h"
#include "context/ordinary_assembly/symbol_attributes.h"
#include "data_def_types.h"
#include "utils/unicode_text.h"

using namespace hlasm_plugin::parser_library::checking;
using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library;

uint64_t get_X_B_length(std::string_view s, uint64_t frac)
{
    uint64_t length = 0;
    uint64_t one_length = 0;
    for (char c : s)
    {
        if (c == ',')
        {
            // each value is padded separately
            length += (one_length + frac - 1) / frac;
            one_length = 0;
        }
        else if (c != ' ')
            ++one_length;
    }
    length += (one_length + frac - 1) / frac;
    return length;
}

uint32_t get_X_B_length_attr(std::string_view s, uint64_t frac)
{
    size_t first_value_len = s.find(',');
    if (first_value_len == std::string::npos)
        first_value_len = s.size();
    first_value_len = (first_value_len + frac - 1) / frac;
    return (uint32_t)first_value_len;
}

// Checks comma separated values. is_valid_digit specifies whether the char is valid character of value.
template<std::predicate<char> F>
bool check_comma_separated(std::string_view nom, F is_valid_digit)
{
    bool last_valid = false;
    for (char c : nom)
    {
        if (c == ' ')
            continue;
        if (c == ',')
        {
            if (!last_valid)
                return false;
            last_valid = false;
        }
        else if (is_valid_digit(c))
            last_valid = true;
        else
            return false;
    }
    if (!last_valid)
        return false;
    return true;
}

//******************************   type B   ********************************//
data_def_type_B::data_def_type_B()
    : data_def_type('B',
          '\0',
          modifier_bound { 1, 2048 },
          modifier_bound { 1, 256 },
          n_a(),
          n_a(),
          nominal_value_type::STRING,
          no_align,
          as_needed(),
          integer_type::undefined)
{}

bool data_def_type_B::check_impl(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic, bool check_nominal) const
{
    if (!check_nominal)
        return true;

    if (!check_comma_separated(
            std::get<std::string>(op.nominal_value.value), [](char c) { return c == '0' || c == '1'; }))
    {
        add_diagnostic(diagnostic_op::error_D010(op.nominal_value.rng, type_str));
        return false;
    }

    return true;
}

uint64_t data_def_type_B::get_nominal_length(const reduced_nominal_value_t& op) const
{
    if (!op.present)
        return 1;
    else if (!std::holds_alternative<std::string_view>(op.value))
        return 0;
    else
        return get_X_B_length(std::get<std::string_view>(op.value), 8);
}

uint32_t data_def_type_B::get_nominal_length_attribute(const reduced_nominal_value_t& nom) const
{
    if (!nom.present)
        return 1;
    else if (!std::holds_alternative<std::string_view>(nom.value))
        return 0;
    else
        return get_X_B_length_attr(std::get<std::string_view>(nom.value), 8);
}

//******************************   type C   ********************************//
data_def_type_CA_CE::data_def_type_CA_CE(char extension)
    : data_def_type('C',
          extension,
          modifier_bound { 1, 2048 },
          modifier_bound { 1, 256 },
          65535,
          n_a(),
          n_a(),
          nominal_value_type::STRING,
          no_align,
          as_needed(),
          integer_type::undefined)
{}

uint64_t data_def_type_CA_CE::get_nominal_length(const reduced_nominal_value_t& op) const
{
    if (!op.present)
        return 1;
    else if (!std::holds_alternative<std::string_view>(op.value))
        return 0;
    else
        return utils::length_utf32_no_validation(std::get<std::string_view>(op.value));
}

uint32_t data_def_type_CA_CE::get_nominal_length_attribute(const reduced_nominal_value_t& nom) const
{
    if (!nom.present)
        return 1;
    else if (!std::holds_alternative<std::string_view>(nom.value))
        return 0;
    else
        return (uint32_t)utils::length_utf32_no_validation(std::get<std::string_view>(nom.value));
}

data_def_type_C::data_def_type_C()
    : data_def_type_CA_CE('\0')
{}

data_def_type_CA::data_def_type_CA()
    : data_def_type_CA_CE('A')
{}

data_def_type_CE::data_def_type_CE()
    : data_def_type_CA_CE('E')
{}

data_def_type_CU::data_def_type_CU()
    : data_def_type('C',
          'U',
          n_a(),
          modifier_bound { 1, 256 },
          n_a(),
          n_a(),
          nominal_value_type::STRING,
          no_align,
          as_needed(),
          integer_type::undefined)
{}

uint64_t data_def_type_CU::get_nominal_length(const reduced_nominal_value_t& op) const
{
    if (!op.present)
        return 2;
    else if (!std::holds_alternative<std::string_view>(op.value))
        return 0;
    else
        return 2 * (uint64_t)utils::length_utf16_no_validation(std::get<std::string_view>(op.value));
}

uint32_t data_def_type_CU::get_nominal_length_attribute(const reduced_nominal_value_t& nom) const
{
    if (!nom.present)
        return 2;
    else if (!std::holds_alternative<std::string_view>(nom.value))
        return 0;
    else
        return 2 * (uint32_t)utils::length_utf16_no_validation(std::get<std::string_view>(nom.value));
}

bool data_def_type_CU::check_impl(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic, bool) const
{
    if (op.length.present && op.length.value % 2 == 1)
    {
        add_diagnostic(diagnostic_op::error_D014(op.length.rng, type_str));
        return false;
    }
    return true;
}

//******************************   type G   ********************************//

data_def_type_G::data_def_type_G()
    : data_def_type('G',
          '\0',
          n_a(),
          modifier_bound { 1, 256 },
          65534,
          n_a(),
          n_a(),
          nominal_value_type::STRING,
          no_align,
          as_needed(),
          integer_type::undefined)
{}

bool data_def_type_G::check_impl(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic, bool) const
{
    if (op.length.present && op.length.value % 2 == 1)
    {
        add_diagnostic(diagnostic_op::error_D014(op.length.rng, type_str));
        return false;
    }

    return true;
}

uint64_t data_def_type_G::get_nominal_length(const reduced_nominal_value_t& op) const
{
    if (!op.present)
        return 2;
    else if (!std::holds_alternative<std::string_view>(op.value))
        return 0;
    else
    {
        const auto s = std::get<std::string_view>(op.value);
        return utils::length_utf32_no_validation(s)
            - std::ranges::count_if(s, [](char c) { return c == '<' || c == '>'; });
    }
}

uint32_t data_def_type_G::get_nominal_length_attribute(const reduced_nominal_value_t& nom) const
{
    if (!nom.present)
        return 2;
    else if (!std::holds_alternative<std::string_view>(nom.value))
        return 0;
    else
    {
        const auto s = std::get<std::string_view>(nom.value);
        return (uint32_t)(utils::length_utf32_no_validation(s)
            - std::ranges::count_if(s, [](char c) { return c == '<' || c == '>'; }));
    }
}

//******************************   type X   ********************************//


data_def_type_X::data_def_type_X()
    : data_def_type('X',
          '\0',
          modifier_bound { 1, 2048 },
          modifier_bound { 1, 256 },
          65535,
          n_a(),
          n_a(),
          nominal_value_type::STRING,
          no_align,
          as_needed(),
          integer_type::undefined)
{}

bool data_def_type_X::check_impl(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic, bool check_nominal) const
{
    if (!check_nominal)
        return true;

    if (!check_comma_separated(std::get<std::string>(op.nominal_value.value), &is_hexadecimal_digit))
    {
        add_diagnostic(diagnostic_op::error_D010(op.nominal_value.rng, type_str));
        return false;
    }
    return true;
}

uint64_t data_def_type_X::get_nominal_length(const reduced_nominal_value_t& op) const
{
    if (!op.present)
        return 1;
    else if (!std::holds_alternative<std::string_view>(op.value))
        return 0;
    else
        return get_X_B_length(std::get<std::string_view>(op.value), 2);
}

uint32_t data_def_type_X::get_nominal_length_attribute(const reduced_nominal_value_t& nom) const
{
    if (!nom.present)
        return 1;
    else if (!std::holds_alternative<std::string_view>(nom.value))
        return 0;
    else
        return get_X_B_length_attr(std::get<std::string_view>(nom.value), 2);
}
