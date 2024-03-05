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

class timestamp
{
    unsigned long long m_year : 18 = 0;
    unsigned long long m_month : 4 = 0;
    unsigned long long m_day : 5 = 0;
    unsigned long long m_hour : 5 = 0;
    unsigned long long m_minute : 6 = 0;
    unsigned long long m_second : 6 = 0;
    unsigned long long m_microsecond : 20 = 0;

#if !defined(_MSC_VER) || defined(__clang__)
    unsigned long long as_ull() const noexcept
    {
        auto v = (unsigned long long)m_year;
        v <<= 4;
        v |= (unsigned long long)m_month;
        v <<= 5;
        v |= (unsigned long long)m_day;
        v <<= 5;
        v |= (unsigned long long)m_hour;
        v <<= 6;
        v |= (unsigned long long)m_minute;
        v <<= 6;
        v |= (unsigned long long)m_second;
        v <<= 20;
        v |= (unsigned long long)m_microsecond;

        return v;
    }
#endif

public:
    timestamp() = default;
    timestamp(unsigned year,
        unsigned month,
        unsigned day,
        unsigned hour,
        unsigned minute,
        unsigned second,
        unsigned microsecond) noexcept
        : m_year(year)
        , m_month(month)
        , m_day(day)
        , m_hour(hour)
        , m_minute(minute)
        , m_second(second)
        , m_microsecond(microsecond)
    {}
    timestamp(unsigned year, unsigned month, unsigned day) noexcept
        : timestamp(year, month, day, 0, 0, 0, 0)
    {}
    unsigned year() const noexcept { return m_year; }
    unsigned month() const noexcept { return m_month; }
    unsigned day() const noexcept { return m_day; }
    unsigned hour() const noexcept { return m_hour; }
    unsigned minute() const noexcept { return m_minute; }
    unsigned second() const noexcept { return m_second; }
    unsigned microsecond() const noexcept { return m_microsecond; }

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
