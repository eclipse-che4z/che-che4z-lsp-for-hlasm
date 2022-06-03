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

#include "utils/utf8text.h"

#include <algorithm>

namespace hlasm_plugin::utils {
constinit const std::array<char_size, 256> utf8_prefix_sizes = []() {
    std::array<char_size, 256> sizes = {};
    static_assert(std::numeric_limits<unsigned char>::max() < sizes.size());
    for (int i = 0b0000'0000; i <= 0b0111'1111; ++i)
        sizes[i] = { 1, 1 };
    for (int i = 0b1100'0000; i <= 0b1101'1111; ++i)
        sizes[i] = { 2, 1 };
    for (int i = 0b1110'0000; i <= 0b1110'1111; ++i)
        sizes[i] = { 3, 1 };
    for (int i = 0b1111'0000; i <= 0b1111'0111; ++i)
        sizes[i] = { 4, 2 };
    return sizes;
}();

constinit const std::array<unsigned char, 128> utf8_valid_multibyte_prefix_table = []() {
    std::array<unsigned char, 128> result {};
    const auto update = [&result](unsigned char f, unsigned char s) {
        int bitid = (f - 0xC0) << 4 | s >> 4;
        result[bitid / 8] |= (0x80 >> bitid % 8);
    };
    const auto update_range = [update](unsigned char fl, unsigned char fh, unsigned char sl, unsigned char sh) {
        for (unsigned char f = fl; f <= fh; ++f)
            for (unsigned char s = sl; s <= sh; ++s)
                update(f, s);
    };

    update_range(0xc2, 0xdf, 0x80, 0xbf);
    update_range(0xe0, 0xe0, 0xa0, 0xbf);
    update_range(0xe1, 0xec, 0x80, 0xbf);
    update_range(0xed, 0xed, 0x80, 0x9f);
    update_range(0xee, 0xef, 0x80, 0xbf);
    update_range(0xf0, 0xf0, 0x90, 0xbf);
    update_range(0xf1, 0xf3, 0x80, 0xbf);
    update_range(0xf4, 0xf4, 0x80, 0x8f);

    return result;
}();

void append_utf8_sanitized(std::string& result, std::string_view str)
{
    auto it = str.begin();
    auto end = str.end();
    while (true)
    {
        // handle ascii printable characters
        auto first_complex = std::find_if(it, end, [](unsigned char c) { return c < 0x20 || c >= 0x7f; });
        result.append(it, first_complex);
        it = first_complex;
        if (it == end)
            break;


        unsigned char c = *it;
        auto cs = utf8_prefix_sizes[c];
        if (cs.utf8 > 1 && (end - it) >= cs.utf8 && utf8_valid_multibyte_prefix(c, *std::next(it))
            && std::all_of(it + 2, it + cs.utf8, [](unsigned char c) { return (c & 0xC0) == 0x80; }))
        {
            char32_t combined = c & ~(0xffu << (8 - cs.utf8));
            for (auto p = it + 1; p != it + cs.utf8; ++p)
                combined = combined << 6 | *p & 0x3fu;

            if (combined < 0x8d
                || combined > 0x9f && (0xfffe & combined) != 0xfffe && (combined < 0xfdd0 || combined > 0xfdef))
            {
                result.append(it, it + cs.utf8);
                it += cs.utf8;
                continue;
            }
        }

        static constexpr char hex_digits[] = "0123456789ABCDEF";

        // 0x00-0x1F, 0x7F, 0x8D-0x9F, not characters and invalid sequences
        result.push_back('<');
        result.push_back(hex_digits[(c >> 4) & 0xf]);
        result.push_back(hex_digits[(c >> 0) & 0xf]);
        result.push_back('>');

        ++it;
    }
}

bool utf8_one_byte_begin(char ch)
{
    return (ch & 0x80) == 0; // 0xxxxxxx
}

bool utf8_continue_byte(char ch)
{
    return (ch & 0xC0) == 0x80; // 10xxxxxx
}


bool utf8_two_byte_begin(char ch)
{
    return (ch & 0xE0) == 0xC0; // 110xxxxx
}

bool utf8_three_byte_begin(char ch)
{
    return (ch & 0xF0) == 0xE0; // 1110xxxx
}

bool utf8_four_byte_begin(char ch)
{
    return (ch & 0xF8) == 0xF0; // 11110xxx
}

std::string replace_non_utf8_chars(std::string_view text)
{
    std::string ret;
    ret.reserve(text.size());
    while (!text.empty())
    {
        if (utf8_one_byte_begin(text.front()))
        {
            ret.push_back(text.front());
            text.remove_prefix(1);
            continue;
        }

        const auto cs = utils::utf8_prefix_sizes[(unsigned char)text.front()];
        if (cs.utf8 != 0 && cs.utf8 <= text.size() && utils::utf8_valid_multibyte_prefix(text[0], text[1])
            && std::all_of(text.begin() + 2, text.begin() + cs.utf8, utf8_continue_byte))
        {
            // copy the character to output
            ret.append(text.substr(0, cs.utf8));
            text.remove_prefix(cs.utf8);
        }
        else
        {
            // UTF8 replacement for unknown character
            ret.push_back((uint8_t)0xEF);
            ret.push_back((uint8_t)0xBF);
            ret.push_back((uint8_t)0xBD);
            text.remove_prefix(1);
        }
    }
    return ret;
}

} // namespace hlasm_plugin::utils
