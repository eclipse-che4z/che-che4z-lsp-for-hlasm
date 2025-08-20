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

#include <algorithm>
#include <concepts>

#include "checking/data_definition/data_def_fields.h"
#include "checking/data_definition/data_def_type_base.h"
#include "utils/unicode_text.h"

namespace hlasm_plugin::parser_library::checking {

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
uint64_t get_B_nominal_length(const reduced_nominal_value_t& op)
{
    if (std::holds_alternative<std::monostate>(op))
        return 1;
    else if (!std::holds_alternative<std::string_view>(op))
        return 0;
    else
        return get_X_B_length(std::get<std::string_view>(op), 8);
}
uint32_t get_B_nominal_length_attribute(const reduced_nominal_value_t& nom)
{
    if (std::holds_alternative<std::monostate>(nom))
        return 1;
    else if (!std::holds_alternative<std::string_view>(nom))
        return 0;
    else
        return get_X_B_length_attr(std::get<std::string_view>(nom), 8);
}
constinit const as_needed::impl_t B_nominal_extras { get_B_nominal_length, get_B_nominal_length_attribute };

//******************************   type C   ********************************//
uint64_t get_CA_CE_nominal_length(const reduced_nominal_value_t& op)
{
    if (std::holds_alternative<std::monostate>(op))
        return 1;
    else if (!std::holds_alternative<std::string_view>(op))
        return 0;
    else
        return utils::length_utf32_no_validation(std::get<std::string_view>(op));
}

uint32_t get_CA_CE_nominal_length_attribute(const reduced_nominal_value_t& nom)
{
    if (std::holds_alternative<std::monostate>(nom))
        return 1;
    else if (!std::holds_alternative<std::string_view>(nom))
        return 0;
    else
        return (uint32_t)utils::length_utf32_no_validation(std::get<std::string_view>(nom));
}

constinit const as_needed::impl_t CA_CE_nominal_extras { get_CA_CE_nominal_length, get_CA_CE_nominal_length_attribute };

uint64_t get_CU_nominal_length(const reduced_nominal_value_t& op)
{
    if (std::holds_alternative<std::monostate>(op))
        return 2;
    else if (!std::holds_alternative<std::string_view>(op))
        return 0;
    else
        return 2 * (uint64_t)utils::length_utf16_no_validation(std::get<std::string_view>(op));
}

uint32_t get_CU_nominal_length_attribute(const reduced_nominal_value_t& nom)
{
    if (std::holds_alternative<std::monostate>(nom))
        return 2;
    else if (!std::holds_alternative<std::string_view>(nom))
        return 0;
    else
        return 2 * (uint32_t)utils::length_utf16_no_validation(std::get<std::string_view>(nom));
}

constinit const as_needed::impl_t CU_nominal_extras { get_CU_nominal_length, get_CU_nominal_length_attribute };

//******************************   type G   ********************************//
uint64_t get_G_nominal_length(const reduced_nominal_value_t& op)
{
    if (std::holds_alternative<std::monostate>(op))
        return 2;
    else if (!std::holds_alternative<std::string_view>(op))
        return 0;
    else
    {
        const auto s = std::get<std::string_view>(op);
        return utils::length_utf32_no_validation(s)
            - std::ranges::count_if(s, [](char c) { return c == '<' || c == '>'; });
    }
}

uint32_t get_G_nominal_length_attribute(const reduced_nominal_value_t& nom)
{
    if (std::holds_alternative<std::monostate>(nom))
        return 2;
    else if (!std::holds_alternative<std::string_view>(nom))
        return 0;
    else
    {
        const auto s = std::get<std::string_view>(nom);
        return (uint32_t)(utils::length_utf32_no_validation(s)
            - std::ranges::count_if(s, [](char c) { return c == '<' || c == '>'; }));
    }
}

constinit const as_needed::impl_t G_nominal_extras { get_G_nominal_length, get_G_nominal_length_attribute };

//******************************   type X   ********************************//
uint64_t get_X_nominal_length(const reduced_nominal_value_t& op)
{
    if (std::holds_alternative<std::monostate>(op))
        return 1;
    else if (!std::holds_alternative<std::string_view>(op))
        return 0;
    else
        return get_X_B_length(std::get<std::string_view>(op), 2);
}
uint32_t get_X_nominal_length_attribute(const reduced_nominal_value_t& nom)
{
    if (std::holds_alternative<std::monostate>(nom))
        return 1;
    else if (!std::holds_alternative<std::string_view>(nom))
        return 0;
    else
        return get_X_B_length_attr(std::get<std::string_view>(nom), 2);
}
constinit const as_needed::impl_t X_nominal_extras { get_X_nominal_length, get_X_nominal_length_attribute };

} // namespace hlasm_plugin::parser_library::checking
