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

#include "utils/encoding.h"

#include <algorithm>
#include <limits>
#include <optional>

#include "utils/truth_table.h"
#include "utils/unicode_text.h"

namespace hlasm_plugin::utils::encoding {
namespace {
bool has_percents_in_place(std::string_view s, uint8_t cs)
{
    for (size_t i = 0; i < cs; ++i)
    {
        if (s[i * 3] != '%')
            return false;
    }

    return true;
}

std::optional<unsigned char> get_hex(char c)
{
    if (c >= '0' && c <= '9')
        return static_cast<unsigned char>(c - '0');

    if (c >= 'A' && c <= 'F')
        return static_cast<unsigned char>(c - 'A' + 10);

    if (c >= 'a' && c <= 'f')
        return static_cast<unsigned char>(c - 'a' + 10);

    return std::nullopt;
}

std::optional<unsigned char> get_utf_8_byte(char c1, char c2)
{
    auto upper_half = get_hex(c1);
    auto lower_half = get_hex(c2);

    if (!upper_half.has_value() || !lower_half.has_value())
        return std::nullopt;

    return static_cast<unsigned char>((*upper_half << 4) | *lower_half);
}

size_t get_already_encoded_size(std::string_view s)
{
    if (s.empty() || s.front() != '%' || s.size() < 3)
        return 0;

    auto first_byte = get_utf_8_byte(s[1], s[2]);
    if (!first_byte.has_value())
        return 0;

    auto cs = utf8_prefix_sizes[*first_byte].utf8;
    if (cs == 0 || cs == 1)
        return cs;

    if (s.size() < static_cast<size_t>(cs) * 3 || !has_percents_in_place(s, cs))
        return 0;

    if (auto second_byte = get_utf_8_byte(s[4], s[5]);
        !second_byte.has_value() || !utf8_valid_multibyte_prefix(*first_byte, *second_byte))
        return 0;

    for (size_t i = 2; i < cs; ++i)
    {
        if (auto next_byte = get_utf_8_byte(s[i * 3 + 1], s[i * 3 + 2]);
            !next_byte.has_value() || ((*next_byte) & 0xC0) != 0x80)
            return 0;
    }

    return cs;
}

bool try_to_parse_encoded(std::string& out, std::string_view& s)
{
    auto encoded_size = get_already_encoded_size(s);
    if (encoded_size == 0)
        return false;

    while (encoded_size > 0)
    {
        out.push_back(s[0]);
        out.push_back(static_cast<char>(std::toupper(s[1])));
        out.push_back(static_cast<char>(std::toupper(s[2])));
        s.remove_prefix(3);

        encoded_size--;
    }

    return true;
}
constexpr auto unreserved =
    utils::create_truth_table(u8"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._~");
constexpr auto path_tolerable =
    utils::create_truth_table(u8"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._~/*?");
using keep_table_t = decltype(unreserved);
constexpr auto invert_hex = []() {
    std::array<signed char, std::numeric_limits<char8_t>::max() + 1> result {};

    std::ranges::fill(result, -1);

    result[u8'0'] = 0x0;
    result[u8'1'] = 0x1;
    result[u8'2'] = 0x2;
    result[u8'3'] = 0x3;
    result[u8'4'] = 0x4;
    result[u8'5'] = 0x5;
    result[u8'6'] = 0x6;
    result[u8'7'] = 0x7;
    result[u8'8'] = 0x8;
    result[u8'9'] = 0x9;
    result[u8'A'] = 0xA;
    result[u8'B'] = 0xB;
    result[u8'C'] = 0xC;
    result[u8'D'] = 0xD;
    result[u8'E'] = 0xE;
    result[u8'F'] = 0xF;
    result[u8'a'] = 0xa;
    result[u8'b'] = 0xb;
    result[u8'c'] = 0xc;
    result[u8'd'] = 0xd;
    result[u8'e'] = 0xe;
    result[u8'f'] = 0xf;

    return result;
}();
void push_uri_char(std::string& uri, unsigned char c, const keep_table_t& keep)
{
    if (keep[c])
    {
        uri.push_back(c);
    }
    else
    {
        uri.push_back('%');
        uri.push_back("0123456789ABCDEF"[c >> 4 & 0xf]);
        uri.push_back("0123456789ABCDEF"[c & 0xf]);
    }
}
} // namespace

std::string percent_encode_path(std::string_view s)
{
    std::string uri;
    uri.reserve(s.size());

    for (unsigned char c : s)
    {
        if (c == (unsigned char)'\\')
            c = '/';

        push_uri_char(uri, c, path_tolerable);
    }

    return uri;
}

std::string percent_encode_component(std::string_view s)
{
    std::string uri;
    uri.reserve(s.size());

    for (unsigned char c : s)
    {
        push_uri_char(uri, c, unreserved);
    }

    return uri;
}

std::string percent_decode(std::string_view s)
{
    std::string result;
    result.reserve(s.size());

    while (true)
    {
        const size_t n = s.find('%');
        result += s.substr(0, n);

        if (n == std::string_view::npos)
            break;

        s.remove_prefix(n);

        if (s.size() < 3)
            return {};

        const signed char hi = invert_hex[(unsigned char)s[1]];
        const signed char lo = invert_hex[(unsigned char)s[2]];
        s.remove_prefix(3);
        if (hi < 0 || lo < 0)
            return {};
        result.push_back(hi << 4 | lo);
    }

    return result;
}

std::string percent_encode_path_and_ignore_utf8(std::string_view s)
{
    std::string uri;
    uri.reserve(s.size());

    while (!s.empty())
    {
        if (try_to_parse_encoded(uri, s))
            continue;

        auto c = s.front();
        s.remove_prefix(1);
        if (c == (unsigned char)'\\')
            c = '/';

        push_uri_char(uri, c, path_tolerable);
    }
    return uri;
}

} // namespace hlasm_plugin::utils::encoding
