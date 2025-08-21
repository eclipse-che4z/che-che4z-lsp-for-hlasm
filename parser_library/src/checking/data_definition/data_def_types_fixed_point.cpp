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
// these types: P, Z, H, F

#include "checking/checker_helper.h"
#include "checking/data_definition/data_def_type_base.h"
#include "diagnostic_op.h"

namespace hlasm_plugin::parser_library::checking {

//***************************   types H, F, FD   *****************************//

class H_F_FD_number_spec
{
public:
    static bool is_end_char(char c) { return c == ',' || c == 'E' || c == 'e'; }

    static bool is_sign_char(char c) { return c == 'U' || c == 'u' || c == '+' || c == '-'; }
};

nominal_diag_func check_nominal_H_F_FD(std::string_view nom) noexcept
{
    size_t i = 0;
    if (nom.empty())
        return nullptr;
    while (i < nom.size())
    {
        // checks number, may begin with +,- or U, ends with exponent or comma
        if (!check_number<H_F_FD_number_spec>(nom, i))
            return diagnostic_op::error_D010;
        if (i >= nom.size())
            return nullptr;
        // check exponent
        if (nom[i] == 'E' || nom[i] == 'e')
        {
            if (!check_exponent(nom, i))
                return diagnostic_op::error_D010;
            if (i >= nom.size())
                return nullptr;
        }
        if (nom[i] != ',')
            return diagnostic_op::error_D010;
        ++i;
    }
    if (nom.back() == ',')
        return diagnostic_op::error_D010;

    return nullptr;
    // TODO truncation is also an error
}

//***************************   types P, Z   *****************************//

class P_Z_number_spec
{
public:
    static bool is_end_char(char c) { return c == ','; }

    static bool is_sign_char(char c) { return c == '+' || c == '-'; }
};

nominal_diag_func check_nominal_P_Z(std::string_view nom) noexcept
{
    // TO DO truncation is also an error
    size_t i = 0;
    if (nom.empty())
        return diagnostic_op::error_D010;

    while (i < nom.size())
    {
        if (!check_number<P_Z_number_spec>(nom, i))
            return diagnostic_op::error_D010;
        ++i;
    }
    if (nom.back() == ',')
        return diagnostic_op::error_D010;

    return nullptr;
}

uint64_t get_P_nominal_length(std::string_view op) noexcept
{
    uint64_t bytes_count = 0;
    // 4 sign bits are added to each assembled number
    uint64_t halfbytes_count = 1;
    for (char c : op)
    {
        if (c == ',')
        {
            bytes_count += (halfbytes_count + 1) / 2;
            halfbytes_count = 1;
        }
        else if (is_digit(c))
            ++halfbytes_count;
    }
    bytes_count += (halfbytes_count + 1) / 2;
    // each digit is assembled as 4 bits, 4 more sign bits are assembled per each number

    return bytes_count;
}

uint32_t get_P_nominal_length_attribute(std::string_view op) noexcept
{
    // 4 sign bits are added to each assembled number
    uint32_t halfbytes_count = 1;
    for (char c : op)
    {
        if (c == ',')
            break;
        else if (is_digit(c))
            ++halfbytes_count;
    }

    return (halfbytes_count + 1) / 2;
    // each digit is assembled as 4 bits, 4 more sign bits are assembled per each number
}

constinit const as_needed::impl_t P_nominal_extras { get_P_nominal_length, get_P_nominal_length_attribute, 1, 1 };

uint64_t get_Z_nominal_length(std::string_view op) noexcept
{
    // each digit is assembled as one byte
    return std::ranges::count_if(op, &is_digit);
}

uint32_t get_Z_nominal_length_attribute(std::string_view op) noexcept
{
    uint32_t first_value_len = 0;
    for (char c : op)
    {
        if (c == ',')
            break;
        if (is_digit(c))
            ++first_value_len;
    }

    // each digit is assembled as one byte

    return first_value_len;
}

constinit const as_needed::impl_t Z_nominal_extras { get_Z_nominal_length, get_Z_nominal_length_attribute, 1, 1 };

} // namespace hlasm_plugin::parser_library::checking
