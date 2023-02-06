/*
 * Copyright (c) 2023 Broadcom.
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

#include <string_view>

#include "gtest/gtest.h"

#include "lexing/logical_line.h"
#include "range.h"
#include "semantics/range_provider.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::semantics;

TEST(text_range_test, empty_line)
{
    std::string_view input = "123456";
    lexing::logical_line<std::string_view::iterator> ll;

    ASSERT_TRUE(extract_logical_line(ll, input, lexing::default_ictl).first);
    ll.segments.front().code = ll.segments.front().continuation;

    EXPECT_EQ(text_range(ll.begin(), ll.end(), 1), range(position(1, 0)));
}

TEST(text_range_test, single_line)
{
    std::string_view input = "123456";
    lexing::logical_line<std::string_view::iterator> ll;

    ASSERT_TRUE(extract_logical_line(ll, input, lexing::default_ictl).first);

    EXPECT_EQ(text_range(ll.begin(), ll.end(), 2), range(position(2, 0), position(2, 6)));
    EXPECT_EQ(text_range(std::next(ll.begin()), ll.end(), 0), range(position(0, 1), position(0, 6)));
    EXPECT_EQ(text_range(std::next(ll.begin(), 3), std::prev(ll.end(), 1), 0), range(position(0, 3), position(0, 5)));
    EXPECT_EQ(text_range(std::next(ll.begin(), 3), std::prev(ll.end(), 3), 0), range(position(0, 3)));
}

TEST(text_range_test, multi_line)
{
    std::string_view input = R"(                  SOME                                                 X
               TEXT                                                    X
               GOES                                                    X
               HERE)";
    lexing::logical_line<std::string_view::iterator> ll;

    ASSERT_TRUE(extract_logical_line(ll, input, lexing::default_ictl).first);

    EXPECT_EQ(text_range(ll.begin(), ll.end(), 2), range(position(2, 0), position(5, 19)));
    EXPECT_EQ(
        text_range(std::next(ll.begin(), 71), std::next(ll.begin(), 127), 0), range(position(1, 15), position(1, 71)));
    EXPECT_EQ(text_range(ll.begin(), std::prev(ll.end(), 4), 0), range(position(0, 0), position(2, 71)));
}
