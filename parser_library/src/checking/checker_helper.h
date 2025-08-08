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

#ifndef HLASMPLUGIN_PARSERLIBRRY_CHECKER_HELPER_H
#define HLASMPLUGIN_PARSERLIBRRY_CHECKER_HELPER_H

// This file contains helper functions for checking that analyze operands,
// check various number and data representations.

#include <algorithm>
#include <cassert>
#include <charconv>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "instr_operand.h"

#pragma warning(push)
#pragma warning(disable : 4505)
namespace hlasm_plugin::parser_library::checking {

inline const one_operand* get_simple_operand(const asm_operand* to_check_operand)
{
    return dynamic_cast<const one_operand*>(to_check_operand);
}

inline const complex_operand* get_complex_operand(const asm_operand* to_check_operand)
{
    return dynamic_cast<const complex_operand*>(to_check_operand);
}

inline bool is_operand_complex(const asm_operand* to_check_operand)
{
    return get_complex_operand(to_check_operand) != nullptr;
}

inline bool is_operand_simple(const asm_operand* to_check_operand)
{
    return get_simple_operand(to_check_operand) != nullptr;
}

inline bool is_operand_empty(const asm_operand* to_check_operand)
{
    return dynamic_cast<const empty_operand*>(to_check_operand) != nullptr;
}

inline bool has_one_comma(std::span<const asm_operand* const> to_check)
{
    return to_check.size() == 2 && is_operand_empty(to_check[0]) && is_operand_empty(to_check[1]);
}

inline bool has_all_digits(std::string_view str)
{
    return std::ranges::all_of(str, [](unsigned char c) { return std::isdigit(c); });
}

// function to convert numbers less than 64000 to hexadecimal
inline std::string dec_to_hexa(int to_convert)
{
    if (to_convert >= 64000 || to_convert < 0)
        return "- 1";
    std::string result;
    do
    {
        result.push_back("0123456789ABCDEF"[to_convert % 16]);
    } while (to_convert /= 16);
    std::ranges::reverse(result);
    return result;
}

inline bool is_value_hexa(std::string_view to_test)
{
    return !to_test.empty() && std::ranges::all_of(to_test, [](unsigned char c) { return std::isxdigit(c); });
}

inline bool is_byte_value(const int to_test) { return (to_test <= 255 && to_test >= 0); }

inline bool is_power_of_two(int to_check)
{
    if (to_check >= 0)
        return (to_check & (to_check - 1)) == 0;
    return false;
}

inline bool is_ord_symbol(std::string_view to_test)
{
    assert(!to_test.empty());
    return !to_test.empty() && to_test.size() <= 63 && isalpha((unsigned char)to_test.front())
        && std::ranges::all_of(to_test, [](unsigned char c) { return std::isalnum(c); });
}

inline bool is_var_symbol(std::string_view to_test)
{
    assert(!to_test.empty());
    return to_test.front() == '&' && is_ord_symbol(to_test.substr(1));
}

inline bool is_character_string(std::string_view to_test)
{
    return to_test.empty() || !(to_test.size() == 1 || to_test.size() > 255);
}

inline std::optional<int> as_int(std::string_view s, int base = 10)
{
    const char* const b = std::to_address(s.cbegin());
    const char* const e = std::to_address(s.cend());
    int result = 0;
    if (auto [p, err] = std::from_chars(b, e, result, base); err != std::errc() || p != e)
        return std::nullopt;
    return result;
}

inline bool is_date(std::string_view to_test)
{
    static constexpr char days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (to_test.size() != 8 || !has_all_digits(to_test))
        return false;
    const auto year_o = as_int(to_test.substr(0, 4));
    const auto month_o = as_int(to_test.substr(4, 2));
    const auto day_o = as_int(to_test.substr(6, 2));
    if (!year_o || !month_o || !day_o)
        return false;
    const auto year = *year_o;
    const auto month = *month_o;
    const auto day = *day_o;
    if (month < 1 || month > 12 || day > 31 || day < 1)
        return false;
    const bool leap_year = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    return (day <= days_in_month[month - 1] || (month == 2 && leap_year && day == 29));
};

inline bool is_sign(char c) { return c == '-' || c == '+'; }

inline bool is_digit(char c) { return (c >= '0' && c <= '9'); }

inline bool is_hexadecimal_digit(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

inline bool is_decimal_number_ch(char c) { return is_digit(c) || is_sign(c) || c == '.'; }

// Checks whether a number starts at position i. Leaves i at the first character behind the number.
// number_spec must implement two static functions:
// bool is_end_char(char c) specifies whether char is valid character that delimits number.
// bool is_sign_char(char c) specifies whether char is valid beginning character
template<class number_spec>
inline bool check_number(std::string_view nominal, size_t& i)
{
    if (nominal[i] == ',')
        return false;

    if (number_spec::is_sign_char(nominal[i]))
        ++i;

    bool found_digit = false;
    bool prev_digit = false;
    bool found_dot = false;
    bool prev_dot = false;

    while (i < nominal.size() && !number_spec::is_end_char(nominal[i]))
    {
        char c = nominal[i];
        if (c == '.')
        {
            if (found_dot)
                return false;
            found_dot = true;
            prev_digit = false;
            prev_dot = true;
        }
        else if (is_digit(c))
        {
            prev_digit = true;
            found_digit = true;
            prev_dot = false;
        }
        else if (c != ' ')
            return false;
        ++i;
    }

    // there must be at least one digit in the number and it must end with digit or dot (3. is also valid number)
    if (!found_digit)
        return false;
    if (!prev_digit && !prev_dot)
        return false;

    return true;
}
// Checks whether there is valid exponent on position i, we assume nominal[i] == 'E'. Leaves i on the first invalid
// character.
inline bool check_exponent(std::string_view nominal, size_t& i)
{
    assert(nominal[i] == 'E' || nominal[i] == 'e');
    ++i;
    if (i < nominal.size() && is_sign(nominal[i]))
        ++i;
    bool found_digit = false;
    while (i < nominal.size() && is_digit(nominal[i]))
    {
        ++i;
        found_digit = true;
    }
    return found_digit;
}
} // namespace hlasm_plugin::parser_library::checking

#pragma warning(pop)

#endif // !HLASMPLUGIN_PARSERLIBRRY_CHECKER_HELPER_H
