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

#include <functional>
#include <iterator>
#include <limits>
#include <optional>

#include "network/uri/uri.hpp"

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

bool try_to_parse_encoded(std::back_insert_iterator<std::string>& out, std::string_view& s)
{
    auto encoded_size = get_already_encoded_size(s);
    if (encoded_size == 0)
        return false;

    while (encoded_size > 0)
    {
        out++ = s[0];
        out++ = static_cast<char>(std::toupper(s[1]));
        out++ = static_cast<char>(std::toupper(s[2]));
        s.remove_prefix(3);

        encoded_size--;
    }

    return true;
}
} // namespace

std::string percent_encode(std::string_view s)
{
    std::string uri;
    uri.reserve(s.size());
    auto out = std::back_inserter(uri);

    while (!s.empty())
    {
        auto c = s.front();
        if (c == '\\')
            c = '/';

        network::detail::encode_char(c, out, "/.*?");
        s.remove_prefix(1);
    }

    return uri;
}

std::string percent_encode_component(std::string_view s)
{
    std::string uri;
    uri.reserve(s.size());
    auto out = std::back_inserter(uri);

    for (auto c : s)
    {
        network::detail::encode_char(c, out);
    }

    return uri;
}

std::string percent_decode(std::string_view s)
{
    std::string result;
    result.reserve(s.size());

    try
    {
        network::detail::decode(s.begin(), s.end(), std::back_inserter(result));
    }
    catch (const network::percent_decoding_error&)
    {
        return {};
    }

    return result;
}

std::string percent_encode_and_ignore_utf8(std::string_view s)
{
    std::string uri;
    uri.reserve(s.size());
    auto out = std::back_inserter(uri);

    while (!s.empty())
    {
        if (try_to_parse_encoded(out, s))
            continue;

        auto c = s.front();
        if (c == '\\')
            c = '/';

        network::detail::encode_char(c, out, "/.*?");
        s.remove_prefix(1);
    }
    return uri;
}

constexpr std::string_view uri_friendly_base16 = "abcdefghijklmnop";
constexpr std::string_view uri_friendly_base16uc = "ABCDEFGHIJKLMNOP";

static_assert(uri_friendly_base16.size() == 16);
static_assert(uri_friendly_base16uc.size() == 16);

std::string uri_friendly_base16_encode(std::string_view s)
{
    std::string result;
    result.reserve(s.size() * 2);


    for (unsigned char c : s)
    {
        result.push_back(uri_friendly_base16[c >> 4]);
        result.push_back(uri_friendly_base16[c & 15]);
    }

    return result;
}

std::string uri_friendly_base16_decode(std::string_view s)
{
    if (s.size() & 1)
        return {};

    static constexpr const auto inverse = []() {
        std::array<signed char, 256> result {};
        for (auto& c : result)
            c = -1;

        for (size_t i = 0; i < uri_friendly_base16.size(); ++i)
        {
            result[(unsigned char)uri_friendly_base16[i]] = i;
            result[(unsigned char)uri_friendly_base16uc[i]] = i;
        }

        return result;
    }();

    std::string result;
    result.reserve(s.size() / 2);
    for (size_t i = 0; i < s.size(); i += 2)
    {
        auto c0 = inverse[(unsigned char)s[i]];
        auto c1 = inverse[(unsigned char)s[i + 1]];
        if (c0 < 0 || c1 < 0)
        {
            result.clear();
            break;
        }
        result.push_back(c0 << 4 | c1);
    }

    return result;
}

} // namespace hlasm_plugin::utils::encoding
