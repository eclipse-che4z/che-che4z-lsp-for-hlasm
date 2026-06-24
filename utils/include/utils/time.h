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

#ifndef HLASMPLUGIN_UTILS_TIME_H
#define HLASMPLUGIN_UTILS_TIME_H

#include <compare>
#include <optional>
#include <string>

namespace hlasm_plugin::utils {

struct timestamp
{
    unsigned long long year : 18 = 0;
    unsigned long long month : 4 = 0;
    unsigned long long day : 5 = 0;
    unsigned long long hour : 5 = 0;
    unsigned long long minute : 6 = 0;
    unsigned long long second : 6 = 0;
    unsigned long long microsecond : 20 = 0;

#if !defined(_MSC_VER) || defined(__clang__)
    unsigned long long as_ull() const noexcept
    {
        auto v = (unsigned long long)year;
        v <<= 4;
        v |= (unsigned long long)month;
        v <<= 5;
        v |= (unsigned long long)day;
        v <<= 5;
        v |= (unsigned long long)hour;
        v <<= 6;
        v |= (unsigned long long)minute;
        v <<= 6;
        v |= (unsigned long long)second;
        v <<= 20;
        v |= (unsigned long long)microsecond;

        return v;
    }
#endif

#if defined(_MSC_VER) && !defined(__clang__)
    auto operator<=>(const timestamp&) const noexcept = default;
#else
    // both gcc and clang have issues with this???
    auto operator<=>(const timestamp& o) const noexcept { return as_ull() <=> o.as_ull(); }
    bool operator==(const timestamp& o) const noexcept { return as_ull() == o.as_ull(); }
#endif // _MSC_VER

    std::string to_string() const;

    static std::optional<timestamp> now();
};


} // namespace hlasm_plugin::utils

#endif
