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

#include "lexing/logical_line.h"

using namespace hlasm_plugin::parser_library::lexing;

TEST(utf8, substr)
{
    for (const auto& [str, off, len, expected] :
        std::initializer_list<std::tuple<std::string_view, size_t, size_t, utf8_substr_result>> {
            { "abcdef", 0, 0, { "", 0, 0 } },
            { "abcdef", 0, 6, { "abcdef", 6, 6 } },
            { "abcdef", 2, 3, { "cde", 3, 3 } },
            { "abcdef", 2, 3, { "cde", 3, 3 } },
            { (const char*)u8"abc\U0001f34cdef", 3, 1, { (const char*)u8"\U0001f34c", 1, 2 } },
            { (const char*)u8"abc\U0001f34cdef", 2, 3, { (const char*)u8"c\U0001f34cd", 3, 4 } },
            { "abcdef", 10000, 0, { "", 0, 0 } },
            { "abcdef", 0, 10000, { "abcdef", 6, 6 } },
            { (const char*)u8"abc\U0001f34cdef", 0, (size_t)-1, { (const char*)u8"abc\U0001f34cdef", 7, 8 } },
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
