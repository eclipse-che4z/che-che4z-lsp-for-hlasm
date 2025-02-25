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

#include <optional>
#include <tuple>

#include "gtest/gtest.h"

#include "utils/unicode_text.h"

using namespace hlasm_plugin::utils;

TEST(utf8, substr)
{
    for (const auto& [str, off, len, expected] :
        std::initializer_list<std::tuple<std::string_view, size_t, size_t, utf8_substr_result>> {
            { "abcdef", 0, 0, { "", 0, 0, true } },
            { "abcdef", 0, 6, { "abcdef", 6, 6, true } },
            { "abcdef", 2, 3, { "cde", 3, 3, true } },
            { "abcdef", 2, 3, { "cde", 3, 3, true } },
            { (const char*)u8"abc\U0001f34cdef", 3, 1, { (const char*)u8"\U0001f34c", 1, 2, true } },
            { (const char*)u8"abc\U0001f34cdef", 2, 3, { (const char*)u8"c\U0001f34cd", 3, 4, true } },
            { "abcdef", 10000, 0, { "", 0, 0, false } },
            { "abcdef", 0, 10000, { "abcdef", 6, 6, true } },
            { (const char*)u8"abc\U0001f34cdef", 0, (size_t)-1, { (const char*)u8"abc\U0001f34cdef", 7, 8, true } },
        })
    {
        EXPECT_EQ(utf8_substr<false>(str, off, len), expected) << str << ":" << off << ":" << len;
        EXPECT_EQ(utf8_substr<true>(str, off, len), expected) << str << ":" << off << ":" << len;
    }
}

TEST(utf8, substr_with_validate)
{
    for (const auto& [str, off, len] : std::initializer_list<std::tuple<std::string_view, size_t, size_t>> {
             { "\x80", 1, 0 },
             { "\xff\x7f", 1, 0 },
             { "\xc0\x7f", 1, 0 },
             { "\x80", 0, 1 },
             { "\xff\x7f", 0, 1 },
             { "\xc0\x7f", 0, 1 },
         })
    {
        EXPECT_THROW(utf8_substr<true>(str, off, len), utf8_error) << str << ":" << off << ":" << len;
    }
}

TEST(utf8, multibyte_validation)
{
    for (const auto [f, s, e] : std::initializer_list<std::tuple<unsigned char, unsigned char, bool>> {
             { 0, 0, false },
             { 0x7f, 0, false },
             { 0xa0, 0x80, false },
             { 0xc0, 0, false },
             { 0xc0, 0x80, false },
             { 0xc0, 0x90, false },
             { 0xc2, 0x80, true },
             { 0xed, 0xa0, false },
             { 0xed, 0xbf, false },
             { 0xf4, 0x8f, true },
             { 0xf4, 0x90, false },
             { 0xff, 0xff, false },
         })
        EXPECT_EQ(utf8_valid_multibyte_prefix(f, s), e);
}

TEST(utf8, iterator)
{
    std::string_view s = (const char*)u8"a\U000000b6\U00001D01\U0001f34c";
    utf8_iterator it(s.begin(), utf8_multicounter<utf8_byte_counter, utf8_utf16_counter, utf8_utf32_counter>());

    EXPECT_EQ(it.counter(), it.counter(counter_index<0>));
    EXPECT_EQ(it.counter<0>(), 0);
    EXPECT_EQ(it.counter<1>(), 0);
    EXPECT_EQ(it.counter<2>(), 0);

    utf8_next(it, 0, s.end());

    EXPECT_EQ(it.counter(), it.counter(counter_index<0>));
    EXPECT_EQ(it.counter<0>(), 0);
    EXPECT_EQ(it.counter<1>(), 0);
    EXPECT_EQ(it.counter<2>(), 0);

    utf8_next(it, 1, s.end());

    EXPECT_EQ(it.counter(), it.counter(counter_index<0>));
    EXPECT_EQ(it.counter<0>(), 1);
    EXPECT_EQ(it.counter<1>(), 1);
    EXPECT_EQ(it.counter<2>(), 1);

    utf8_next(it, 1, s.end());

    EXPECT_EQ(it.counter(), it.counter<0>());
    EXPECT_EQ(it.counter<0>(), 3);
    EXPECT_EQ(it.counter<1>(), 2);
    EXPECT_EQ(it.counter<2>(), 2);

    utf8_next(it, 1, s.end());

    EXPECT_EQ(it.counter(), it.counter<0>());
    EXPECT_EQ(it.counter<0>(), 6);
    EXPECT_EQ(it.counter<1>(), 3);
    EXPECT_EQ(it.counter<2>(), 3);

    utf8_next(it, 1, s.end());

    EXPECT_EQ(it.counter(), it.counter<0>());
    EXPECT_EQ(it.counter<0>(), 10);
    EXPECT_EQ(it.counter<1>(), 5);
    EXPECT_EQ(it.counter<2>(), 4);

    // try going past the end
    utf8_next(it, 1, s.end());

    EXPECT_EQ(it.counter(), it.counter<0>());
    EXPECT_EQ(it.counter<0>(), 10);
    EXPECT_EQ(it.counter<1>(), 5);
    EXPECT_EQ(it.counter<2>(), 4);

    utf8_prev(it, 1, s.begin());

    EXPECT_EQ(it.counter(), it.counter<0>());
    EXPECT_EQ(it.counter<0>(), 6);
    EXPECT_EQ(it.counter<1>(), 3);
    EXPECT_EQ(it.counter<2>(), 3);

    utf8_prev(it, 1, s.begin());

    EXPECT_EQ(it.counter(), it.counter<0>());
    EXPECT_EQ(it.counter<0>(), 3);
    EXPECT_EQ(it.counter<1>(), 2);
    EXPECT_EQ(it.counter<2>(), 2);

    utf8_prev(it, 1, s.begin());

    EXPECT_EQ(it.counter(), it.counter<0>());
    EXPECT_EQ(it.counter<0>(), 1);
    EXPECT_EQ(it.counter<1>(), 1);
    EXPECT_EQ(it.counter<2>(), 1);

    utf8_prev(it, 1, s.begin());

    EXPECT_EQ(it.counter(), it.counter<0>());
    EXPECT_EQ(it.counter<0>(), 0);
    EXPECT_EQ(it.counter<1>(), 0);
    EXPECT_EQ(it.counter<2>(), 0);

    // try going past the beginning
    utf8_prev(it, 1, s.begin());

    EXPECT_EQ(it.counter(), it.counter<0>());
    EXPECT_EQ(it.counter<0>(), 0);
    EXPECT_EQ(it.counter<1>(), 0);
    EXPECT_EQ(it.counter<2>(), 0);
}

const std::string_view unicode_utf8 = (const char*)u8"\U00010000\U0000a123\U00000140\U00000041";
const std::u32string_view unicode_utf32 = U"\U00010000\U0000a123\U00000140\U00000041";

TEST(replace_non_utf8_chars, no_change)
{
    EXPECT_EQ(unicode_utf8, hlasm_plugin::utils::replace_non_utf8_chars(unicode_utf8));
}

TEST(replace_non_utf8_chars, last_char)
{
    std::string common_str = "this is some common string";
    std::string u8 = common_str;
    u8.push_back((uint8_t)0xAF);

    std::string res = hlasm_plugin::utils::replace_non_utf8_chars(u8);

    EXPECT_NE(u8, res);
    EXPECT_EQ(res.size(), u8.size() + 2);
    EXPECT_EQ(res[res.size() - 3], '\xEF');
    EXPECT_EQ(res[res.size() - 2], '\xBF');
    EXPECT_EQ(res[res.size() - 1], '\xBD');
    EXPECT_EQ(res.substr(0, common_str.size()), common_str);
}

TEST(replace_non_utf8_chars, middle_char)
{
    std::string begin = "begin";
    std::string end = "end";
    std::string u8 = begin + '\xF0' + end;

    std::string res = hlasm_plugin::utils::replace_non_utf8_chars(u8);

    EXPECT_NE(u8, res);
    EXPECT_EQ(res.size(), u8.size() + 2);
    EXPECT_EQ(res[begin.size()], '\xEF');
    EXPECT_EQ(res[begin.size() + 1], '\xBF');
    EXPECT_EQ(res[begin.size() + 2], '\xBD');
    EXPECT_EQ(res.substr(0, begin.size()), begin);
    EXPECT_EQ(res.substr(begin.size() + 3), end);
}
