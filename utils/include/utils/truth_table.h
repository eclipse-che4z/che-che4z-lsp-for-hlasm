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

#ifndef HLASMPLUGIN_UTILS_TRUTH_TABLE_H
#define HLASMPLUGIN_UTILS_TRUTH_TABLE_H

#include <array>
#include <assert.h>
#include <limits>
#include <string_view>
#include <type_traits>

namespace hlasm_plugin::utils {

template<typename T = bool>
constexpr auto create_truth_table(std::string_view true_values, T true_value = (T)1) requires(sizeof(T) == 1)
{
    std::array<T, std::numeric_limits<unsigned char>::max() + 1> result {};

    for (auto c : true_values)
        result[(unsigned char)c] = true_value;

    return result;
}

template<typename T, typename U>
constexpr size_t find_mismatch(
    std::basic_string_view<T> s, const std::array<U, std::numeric_limits<unsigned char>::max() + 1>& table)
    requires(std::numeric_limits<T>::max() <= std::numeric_limits<unsigned char>::max())
{
    size_t result = 0;

    for (auto c : s)
    {
        if (!table[(unsigned char)c])
            return result;
        ++result;
    }

    return decltype(s)::npos;
}

template<typename S, typename T>
constexpr size_t find_mismatch(const S& s, const std::array<T, std::numeric_limits<unsigned char>::max() + 1>& table)
{
    return find_mismatch(std::basic_string_view<typename S::value_type>(s), table);
}

} // namespace hlasm_plugin::utils

#endif
