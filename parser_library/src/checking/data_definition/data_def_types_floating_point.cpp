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
// floating point types: E, D, L

#include <array>
#include <string_view>

#include "checking/checker_helper.h"
#include "checking/data_definition/data_def_type_base.h"
#include "diagnostic_op.h"
#include "utils/insist.h"

namespace hlasm_plugin::parser_library::checking {

namespace {
struct round_mode
{
    static constexpr size_t max_length = 3;

    std::array<char, max_length + 1> data = {};

    constexpr round_mode(char type, std::string_view len) noexcept
        : data { type }
    {
        utils::insist(len.size() <= max_length);
        std::ranges::copy(len, data.begin() + 1);
    }

    constexpr bool operator==(const round_mode&) const noexcept = default;
};
constexpr round_mode allowed_round_modes[] = {
    { 'B', "1" },
    { 'B', "4" },
    { 'B', "5" },
    { 'B', "6" },
    { 'B', "7" },
    { 'H', "1" },
    { 'H', "4" },
    { 'H', "5" },
    { 'H', "6" },
    { 'H', "7" },
    { 'D', "8" },
    { 'D', "9" },
    { 'D', "10" },
    { 'D', "11" },
    { 'D', "12" },
    { 'D', "13" },
    { 'D', "14" },
    { 'D', "15" },
};
} // namespace

class E_D_L_number_spec
{
public:
    static bool is_end_char(char c) { return c == ',' || c == 'E' || c == 'e' || c == 'R' || c == 'r'; }

    static bool is_sign_char(char c) { return c == '+' || c == '-'; }
};

std::span<const std::string_view> floating_point_special_values(char subtype)
{
    static constexpr std::string_view bd_type[] = {
        "(SNAN)",
        "(QNAN)",
        "(NAN)",
        "(INF)",
        "(MAX)",
        "(MIN)",
        "(DMIN)",
    };
    static constexpr std::string_view h_type[] = {
        "(MAX)",
        "(MIN)",
        "(DMIN)",
    };
    if (subtype == 'B' || subtype == 'D')
        return bd_type;
    else if (subtype == 'H' || subtype == 'Q')
        return h_type;
    else
        return {};
}

enum class matched_special_value
{
    no,
    yes,
    error,
};

matched_special_value try_matching_special_value(
    std::string_view num, size_t& i, std::span<const std::string_view> list)
{
    if (list.empty())
        return matched_special_value::no;

    auto start = num.find_first_not_of(' ', i);
    if (start == std::string_view::npos)
        return matched_special_value::no;
    if (auto wo_spaces = num.substr(start);
        !wo_spaces.starts_with("(") && !wo_spaces.starts_with("+(") && !wo_spaces.starts_with("-("))
        return matched_special_value::no;
    else
        start += wo_spaces.front() != '(';

    for (auto s : list)
    {
        if (!std::ranges::equal(num.substr(start, s.size()), s, [](unsigned char l, unsigned char r) {
                return toupper(l) == toupper(r);
            }))
            continue;
        i = start + s.size();
        if (i >= num.size())
            return matched_special_value::yes;
        if (num[i] == ',')
        {
            ++i;
            return matched_special_value::yes;
        }
        return matched_special_value::error;
    }
    return matched_special_value::error;
}

nominal_diag_func check_nominal_E_D_L(std::string_view nom, char extension) noexcept
{
    size_t i = 0;
    if (nom.empty())
        return diagnostic_op::error_D010;
    else if (nom.back() == ',')
        return diagnostic_op::error_D010;

    const auto special_values = floating_point_special_values(extension);
    while (i < nom.size())
    {
        switch (try_matching_special_value(nom, i, special_values))
        {
            case matched_special_value::no:
                break;
            case matched_special_value::yes:
                continue;
            case matched_special_value::error:
                return diagnostic_op::error_D010;
        }
        // the number may end with E, R or ',' and begin with + or -.
        if (!check_number<E_D_L_number_spec>(nom, i))
            return diagnostic_op::error_D010;

        if (i >= nom.size())
            break;

        // After E comes exponent
        if (nom[i] == 'E' || nom[i] == 'e')
        {
            if (!check_exponent(nom, i))
                return diagnostic_op::error_D010;
            if (i >= nom.size())
                break;
        }

        // After R comes rounding mode
        if (nom[i] == 'R' || nom[i] == 'r')
        {
            ++i;
            std::string round_mode_s;
            while (i < nom.size() && is_digit(nom[i]))
            {
                round_mode_s += nom[i];
                ++i;
            }

            if (round_mode_s.size() > round_mode::max_length
                || std::ranges::find(allowed_round_modes, round_mode(extension, round_mode_s))
                    == std::ranges::end(allowed_round_modes))
                return [](const range& r, std::string_view) { return diagnostic_op::error_D026(r); };
        }
        if (i < nom.size() && nom[i] != ',')
            return diagnostic_op::error_D010;
        ++i;
    }

    return nullptr;
}

} // namespace hlasm_plugin::parser_library::checking
