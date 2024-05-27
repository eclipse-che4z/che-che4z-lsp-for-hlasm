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

#include "input_source.h"

#include <exception>
#include <iterator>

#include "logical_line.h"

namespace hlasm_plugin::parser_library::lexing {
namespace {
input_source::char_substitution append_utf8_to_utf32(std::u32string& t, std::string_view s)
{
    input_source::char_substitution subs {};

    while (!s.empty())
    {
        unsigned char c = s.front();
        if (c < 0x80)
        {
            t.append(1, c);
            s.remove_prefix(1);
            continue;
        }
        const auto cs = utils::utf8_prefix_sizes[c];
        if (cs.utf8 && cs.utf8 <= s.size())
        {
            char32_t v = c & 0b0111'1111u >> cs.utf8;
            for (int i = 1; i < cs.utf8; ++i)
                v = v << 6 | (s[i] & 0b0011'1111u);

            if (v == utils::substitute_character)
                subs.client = true;

            t.append(1, v);
            s.remove_prefix(cs.utf8);
        }
        else
        {
            subs.server = true;
            t.append(1, utils::substitute_character);
            s.remove_prefix(1);
        }
    }

    return subs;
}
} // namespace

input_source::input_source(std::string_view input)
    : ANTLRInputStream()
{
    append(input);
}

input_source::char_substitution input_source::append(std::u32string_view str)
{
    _data.append(str);
    return {};
}

input_source::char_substitution input_source::append(std::string_view str)
{
    p = _data.size();
    return append_utf8_to_utf32(_data, str);
}

input_source::char_substitution input_source::new_input(std::string_view str)
{
    p = 0;
    _data.clear();
    return append_utf8_to_utf32(_data, str);
}


input_source::char_substitution input_source::new_input(
    const logical_line<utils::utf8_iterator<std::string_view::iterator, utils::utf8_utf16_counter>>& l)
{
    input_source::char_substitution subs {};
    new_input("");

    for (size_t i = 0; i < l.segments.size(); ++i)
    {
        const auto& s = l.segments[i];
        if (i > 0)
            _data.append(std::ranges::distance(s.begin, s.code), s.continuation_error ? 'X' : ' ');

        subs |= append(std::string_view(s.code.base(), s.end.base()));

        if (i + 1 < l.segments.size())
        {
            // do not add the last EOL
            switch (s.eol)
            {
                case hlasm_plugin::parser_library::lexing::logical_line_segment_eol::none:
                    break;
                case hlasm_plugin::parser_library::lexing::logical_line_segment_eol::lf:
                    append("\n");
                    break;
                case hlasm_plugin::parser_library::lexing::logical_line_segment_eol::crlf:
                    append("\r\n");
                    break;
                case hlasm_plugin::parser_library::lexing::logical_line_segment_eol::cr:
                    append("\r");
                    break;
            }
        }
    }

    reset();
    return subs;
}

std::string input_source::getText(const antlr4::misc::Interval& interval)
{
    std::string n;
    for (auto i = interval.a; i <= interval.b; ++i)
    {
        uint32_t ch = _data[i];
        constexpr unsigned int last6 = 0x3F;
        constexpr unsigned int last5 = 0x1F;
        constexpr unsigned int last4 = 0x0F;
        constexpr unsigned int last3 = 0x07;
        if (ch <= 0x007F) // U+0000 - U+007F
            n.insert(n.end(), (char)ch);
        else if (ch <= 0x7FF) // U+0080 - U+07FF
        {
            char utf8[2];
            utf8[1] = '\x80'; // 10xxxxxx
            utf8[1] += ch & last6;

            ch = ch >> 6;
            utf8[0] = '\xc0'; // 110xxxxx
            utf8[0] += ch & last5;

            n.append(utf8, 2);
        }
        else if (ch <= 0xFFFF) // U+0800 - U+FFFF
        {
            char utf8[3];
            // third
            utf8[2] = '\x80';
            utf8[2] += ch & last6;

            ch = ch >> 6;
            // second
            utf8[1] = '\x80';
            utf8[1] += ch & last6;

            ch = ch >> 6;
            utf8[0] = '\xe0';
            utf8[0] += ch & last4;

            n.append(utf8, 3);
        }
        else if (ch <= 0x10FFFF) // U+10000 - U+10FFFF
        {
            char utf8[4];
            for (int j = 3; j > 0; --j)
            {
                utf8[j] = '\x80';
                utf8[j] += ch & last6;
                ch = ch >> 6;
            }

            utf8[0] = '\xf0'; // 11110xxx
            utf8[0] += ch & last3;

            n.append(utf8, 4);
        }
        else
            throw std::runtime_error("not valid unicode character");
    }
    return n;
}

} // namespace hlasm_plugin::parser_library::lexing
