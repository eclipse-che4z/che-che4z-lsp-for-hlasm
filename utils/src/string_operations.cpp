/*
 * Copyright (c) 2022 Broadcom.
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

#include "utils/string_operations.h"

#include <algorithm>

namespace hlasm_plugin::utils {

size_t trim_left(std::string_view& s)
{
    const auto to_trim = s.find_first_not_of(' ');
    if (to_trim == std::string_view::npos)
    {
        auto s_length = s.length();
        s = {};
        return s_length;
    }

    s.remove_prefix(to_trim);
    return to_trim;
}

size_t trim_left(std::string_view& s, std::string_view to_trim)
{
    const auto to_trim_idx = s.find_first_not_of(to_trim);
    if (to_trim_idx == std::string_view::npos)
    {
        auto s_length = s.length();
        s = {};
        return s_length;
    }

    s.remove_prefix(to_trim_idx);
    return to_trim_idx;
}

size_t trim_right(std::string_view& s)
{
    const auto to_trim = s.find_last_not_of(' ');
    if (to_trim == std::string_view::npos)
    {
        auto s_length = s.length();
        s = {};
        return s_length;
    }

    s = s.substr(0, to_trim + 1);
    return to_trim;
}

size_t trim_right(std::string_view& s, std::string_view to_trim)
{
    const auto to_trim_idx = s.find_last_not_of(to_trim);
    if (to_trim_idx == std::string_view::npos)
    {
        auto s_length = s.length();
        s = {};
        return s_length;
    }

    s = s.substr(0, to_trim_idx + 1);
    return to_trim_idx;
}

size_t consume(std::string_view& s, std::string_view lit)
{
    // case sensitive
    if (!s.starts_with(lit))
        return 0;
    s.remove_prefix(lit.size());
    return lit.size();
}

std::string_view next_nonblank_sequence(std::string_view s)
{
    if (s.empty() || s.front() == ' ')
        return {};

    auto space = s.find(' ');
    if (space == std::string_view::npos)
        space = s.size();

    return s.substr(0, space);
}

std::string& to_upper(std::string& s)
{
    std::ranges::transform(s, s.begin(), [](unsigned char c) { return upper_cased[c]; });
    return s;
}

std::string to_upper_copy(std::string s)
{
    to_upper(s);
    return s;
}

std::string to_upper_copy(std::string_view s) { return to_upper_copy(std::string(s)); }

} // namespace hlasm_plugin::utils
