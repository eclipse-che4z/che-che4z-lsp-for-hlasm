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

#include "utils/time.h"

#include <string_view>
#ifdef __EMSCRIPTEN__
#    include <emscripten.h>

#    include <emscripten/bind.h>
#else
#    define __STDC_WANT_LIB_EXT1__
#    include <chrono>
#    include <time.h>
#endif

namespace {
// template has lower overload priority than the original function
struct tm* localtime_r(const auto* timer, struct tm* buf)
{
    if (auto ret = localtime(timer))
    {
        *buf = *ret;
        return buf;
    }
    return nullptr;
}
} // namespace

namespace hlasm_plugin::utils {

std::string timestamp::to_string() const
{
    constexpr auto padded_append = [](auto& where, const auto& what, size_t pad) {
        auto s = std::to_string(what);
        if (s.size() < pad)
            where.append(pad - s.size(), '0');
        where.append(s);
    };

    std::string curr_time;
    curr_time.reserve(std::string_view("yyyy-MM-dd HH:mm:ss.SSSSSS").size());

    padded_append(curr_time, year(), 4);
    curr_time.append(1, '-');
    padded_append(curr_time, month(), 2);
    curr_time.append(1, '-');
    padded_append(curr_time, day(), 2);
    curr_time.append(1, ' ');
    padded_append(curr_time, hour(), 2);
    curr_time.append(1, ':');
    padded_append(curr_time, minute(), 2);
    curr_time.append(1, ':');
    padded_append(curr_time, second(), 2);
    curr_time.append(1, '.');
    padded_append(curr_time, microsecond(), 6);

    return curr_time;
}

std::optional<timestamp> timestamp::now()
{
#ifdef __EMSCRIPTEN__
    // clang-format off
    double compressed = EM_ASM_DOUBLE({
        try
        {
            const x = new Date();
            let r = x.getFullYear();
            r = r * 16 + x.getMonth();
            r = r * 32 + x.getDate();
            r = r * 32 + x.getHours();
            r = r * 64 + x.getMinutes();
            r = r * 64 + x.getSeconds();
            r = r * 1024 + x.getMilliseconds();

            return r;
        }
        catch (e)
        {
            return -1;
        }
    });
    // clang-format on
    if (compressed < 0)
        return std::nullopt;

    constexpr auto shift_out = [](auto& value, int bits) {
        auto result = value & (1 << bits) - 1;
        value >>= bits;
        return result;
    };
    auto v = (unsigned long long)compressed;

    auto microseconds = 1000 * shift_out(v, 10);
    auto second = shift_out(v, 6);
    auto minute = shift_out(v, 6);
    auto hour = shift_out(v, 5);
    auto day = shift_out(v, 5);
    auto month = shift_out(v, 4) + 1;
    auto year = v;

    return timestamp(year, month, day, hour, minute, second, microseconds);
#else
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto now_t = system_clock::to_time_t(now);

    struct tm tm_buf;
#    ifdef __STDC_LIB_EXT1__
    if (!localtime_s(&now_t, &tm_buf))
        return std::nullopt;
#    elif defined _MSC_VER
    if (localtime_s(&tm_buf, &now_t))
        return std::nullopt;
#    else
    if (!localtime_r(&now_t, &tm_buf))
        return std::nullopt;
#    endif

    const auto subsecond = now - floor<seconds>(now);

    return timestamp(tm_buf.tm_year + 1900,
        tm_buf.tm_mon + 1,
        tm_buf.tm_mday,
        tm_buf.tm_hour,
        tm_buf.tm_min,
        tm_buf.tm_sec,
        static_cast<unsigned>(std::chrono::nanoseconds(subsecond).count() / 1000));
#endif
}

} // namespace hlasm_plugin::utils
