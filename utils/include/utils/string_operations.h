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

#ifndef HLASMPLUGIN_UTILS_STRING_OPERATIONS_H
#define HLASMPLUGIN_UTILS_STRING_OPERATIONS_H

#include <cctype>
#include <string_view>
#include <utility>

namespace hlasm_plugin::utils {

size_t trim_left(std::string_view& s);
size_t trim_right(std::string_view& s);

size_t consume(std::string_view& s, std::string_view lit);
std::string_view next_nonblank_sequence(std::string_view s);

inline bool isblank32(char32_t c) { return c <= 255 && std::isblank(static_cast<unsigned char>(c)); }

template<typename T>
bool consume(T& b, const T& e, std::string_view lit)
{
    auto work = b;
    for (auto c : lit)
    {
        if (work == e)
            return false;
        if (*work != c)
            return false;
        ++work;
    }
    b = work;
    return true;
}

template<typename T>
size_t trim_left(T& b, const T& e, std::initializer_list<std::string_view> to_trim = { " " })
{
    size_t result = 0;
    while (b != e)
    {
        bool found = false;
        for (const auto& s : to_trim)
        {
            if (consume(b, e, s))
            {
                ++result;
                found = true;
                break;
            }
        }
        if (!found)
            break;
    }
    return result;
}

} // namespace hlasm_plugin::utils

#endif
