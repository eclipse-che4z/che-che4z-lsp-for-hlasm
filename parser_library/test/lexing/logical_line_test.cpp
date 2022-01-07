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

#include <algorithm>
#include <array>
#include <functional>
#include <numeric>
#include <vector>

#include "gtest/gtest.h"

#include "lexing/logical_line.h"

using namespace hlasm_plugin::parser_library::lexing;

TEST(logical_line, empty)
{
    std::string_view input = "";
    logical_line line;

    EXPECT_FALSE(extract_logical_line(line, input, default_ictl));
    EXPECT_FALSE(extract_logical_line(line, input, default_ictl_dbcs));
}

TEST(logical_line, empty_line)
{
    std::string_view input = "\n";
    logical_line line;

    EXPECT_TRUE(extract_logical_line(line, input, default_ictl));

    EXPECT_FALSE(line.continuation_error);
    EXPECT_FALSE(line.so_si_continuation);
    EXPECT_FALSE(line.missing_next_line);

    ASSERT_EQ(line.segments.size(), 1);
    EXPECT_EQ(line.segments[0].code.size(), 0);
    EXPECT_EQ(line.segments[0].continuation.size(), 0);
    EXPECT_EQ(line.segments[0].ignore.size(), 0);

    EXPECT_FALSE(extract_logical_line(line, input, default_ictl));
}

TEST(logical_line, single_line)
{
    std::string_view input =
        "12345678901234567890123456789012345678901234567890123456789012345678901 345678901234567890";
    logical_line line;

    EXPECT_TRUE(extract_logical_line(line, input, default_ictl));

    EXPECT_FALSE(line.continuation_error);
    EXPECT_FALSE(line.so_si_continuation);
    EXPECT_FALSE(line.missing_next_line);

    ASSERT_EQ(line.segments.size(), 1);
    EXPECT_EQ(line.segments[0].code.size(), 71);
    EXPECT_EQ(line.segments[0].continuation.size(), 0);
    EXPECT_EQ(line.segments[0].ignore.size(), 19);
    EXPECT_FALSE(line.segments[0].continuation_error);
    EXPECT_FALSE(line.segments[0].so_si_continuation);

    EXPECT_FALSE(extract_logical_line(line, input, default_ictl));
}

TEST(logical_line, continued_line)
{
    std::string_view input =
        "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\n"
        "               67890123456789012345678901234567890123456789012345678901 34567890\n";
    logical_line line;

    EXPECT_TRUE(extract_logical_line(line, input, default_ictl));

    EXPECT_FALSE(line.continuation_error);
    EXPECT_FALSE(line.so_si_continuation);
    EXPECT_FALSE(line.missing_next_line);

    ASSERT_EQ(line.segments.size(), 2);
    EXPECT_EQ(line.segments[0].code.size(), 71);
    EXPECT_EQ(line.segments[0].continuation.size(), 1);
    EXPECT_EQ(line.segments[0].ignore.size(), 18);
    EXPECT_FALSE(line.segments[0].continuation_error);
    EXPECT_FALSE(line.segments[0].so_si_continuation);

    EXPECT_EQ(line.segments[1].code.size(), 56);
    EXPECT_EQ(line.segments[1].continuation.size(), 0);
    EXPECT_EQ(line.segments[1].ignore.size(), 9);
    EXPECT_FALSE(line.segments[1].continuation_error);
    EXPECT_FALSE(line.segments[1].so_si_continuation);

    EXPECT_FALSE(extract_logical_line(line, input, default_ictl));
}

TEST(logical_line, bad_continuation)
{
    std::string_view input =
        "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\n"
        "              X67890123456789012345678901234567890123456789012345678901 3456789012\n";
    logical_line line;

    EXPECT_TRUE(extract_logical_line(line, input, default_ictl));

    EXPECT_TRUE(line.continuation_error);
    EXPECT_FALSE(line.so_si_continuation);
    EXPECT_FALSE(line.missing_next_line);

    ASSERT_EQ(line.segments.size(), 2);
    EXPECT_EQ(line.segments[0].code.size(), 71);
    EXPECT_EQ(line.segments[0].continuation.size(), 1);
    EXPECT_EQ(line.segments[0].ignore.size(), 18);
    EXPECT_FALSE(line.segments[0].continuation_error);
    EXPECT_FALSE(line.segments[0].so_si_continuation);

    EXPECT_EQ(line.segments[1].code.size(), 56);
    EXPECT_EQ(line.segments[1].continuation.size(), 0);
    EXPECT_EQ(line.segments[1].ignore.size(), 11);
    EXPECT_TRUE(line.segments[1].continuation_error);
    EXPECT_FALSE(line.segments[1].so_si_continuation);

    EXPECT_FALSE(extract_logical_line(line, input, default_ictl));
}

TEST(logical_line, dbcs_continued_line)
{
    std::string_view input =
        "1234567890123456789012345678901234567890123456789012345678901234567890XX345678901234567890\n"
        "               67890123456789012345678901234567890123456789012345678901 34567890\n";
    logical_line line;

    EXPECT_TRUE(extract_logical_line(line, input, default_ictl_dbcs));

    EXPECT_FALSE(line.continuation_error);
    EXPECT_FALSE(line.so_si_continuation);
    EXPECT_FALSE(line.missing_next_line);

    ASSERT_EQ(line.segments.size(), 2);
    EXPECT_EQ(line.segments[0].code.size(), 70);
    EXPECT_EQ(line.segments[0].continuation.size(), 2);
    EXPECT_EQ(line.segments[0].ignore.size(), 18);
    EXPECT_FALSE(line.segments[0].continuation_error);
    EXPECT_FALSE(line.segments[0].so_si_continuation);

    EXPECT_EQ(line.segments[1].code.size(), 56);
    EXPECT_EQ(line.segments[1].continuation.size(), 0);
    EXPECT_EQ(line.segments[1].ignore.size(), 9);
    EXPECT_FALSE(line.segments[1].continuation_error);
    EXPECT_FALSE(line.segments[1].so_si_continuation);

    EXPECT_FALSE(extract_logical_line(line, input, default_ictl_dbcs));
}

TEST(logical_line, dbcs_so_si_detect)
{
    std::string_view input =
        "1234567890123456789012345678901234567890123456789012345678901234567890>>345678901234567890\n"
        "               67890123456789012345678901234567890123456789012345678901 34567890\n";
    logical_line line;

    EXPECT_TRUE(extract_logical_line(line, input, default_ictl_dbcs));

    EXPECT_FALSE(line.continuation_error);
    EXPECT_TRUE(line.so_si_continuation);
    EXPECT_FALSE(line.missing_next_line);

    ASSERT_EQ(line.segments.size(), 2);
    EXPECT_EQ(line.segments[0].code.size(), 70);
    EXPECT_EQ(line.segments[0].continuation.size(), 2);
    EXPECT_EQ(line.segments[0].ignore.size(), 18);
    EXPECT_FALSE(line.segments[0].continuation_error);
    EXPECT_TRUE(line.segments[0].so_si_continuation);

    EXPECT_EQ(line.segments[1].code.size(), 56);
    EXPECT_EQ(line.segments[1].continuation.size(), 0);
    EXPECT_EQ(line.segments[1].ignore.size(), 9);
    EXPECT_FALSE(line.segments[1].continuation_error);
    EXPECT_FALSE(line.segments[1].so_si_continuation);

    EXPECT_FALSE(extract_logical_line(line, input, default_ictl_dbcs));
}

TEST(logical_line, missing_next_line)
{
    std::string_view input =
        "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\n";
    logical_line line;

    EXPECT_TRUE(extract_logical_line(line, input, default_ictl_dbcs));

    EXPECT_FALSE(line.continuation_error);
    EXPECT_FALSE(line.so_si_continuation);
    EXPECT_TRUE(line.missing_next_line);

    ASSERT_EQ(line.segments.size(), 1);
    EXPECT_EQ(line.segments[0].code.size(), 71);
    EXPECT_EQ(line.segments[0].continuation.size(), 1);
    EXPECT_EQ(line.segments[0].ignore.size(), 18);
    EXPECT_FALSE(line.segments[0].continuation_error);
    EXPECT_FALSE(line.segments[0].so_si_continuation);

    EXPECT_FALSE(extract_logical_line(line, input, default_ictl));
}

TEST(logical_line, eol)
{
    using namespace std::literals;
    std::array tests = {
        std::pair { " "sv, logical_line_segment_eol::none },
        std::pair { " \n"sv, logical_line_segment_eol::lf },
        std::pair { " \r"sv, logical_line_segment_eol::cr },
        std::pair { " \r\n"sv, logical_line_segment_eol::crlf },
    };

    for (auto& t : tests)
    {
        logical_line ll;
        ASSERT_TRUE(extract_logical_line(ll, t.first, default_ictl));
        EXPECT_EQ(ll.segments.front().eol, t.second);
        EXPECT_EQ(t.first.size(), 0);
    }
}

class logical_line_iterator_fixture : public ::testing::TestWithParam<std::vector<std::string_view>>
{};

TEST_P(logical_line_iterator_fixture, iterator)
{
    logical_line line;
    const auto& parm = GetParam();
    std::transform(parm.begin(), parm.end(), std::back_inserter(line.segments), [](const auto& c) {
        logical_line_segment lls;
        lls.code = c;
        return lls;
    });

    std::string concat_parm;
    for (auto p : parm)
        concat_parm.append(p);

    EXPECT_TRUE(std::equal(line.begin(), line.end(), concat_parm.begin(), concat_parm.end()));
    EXPECT_TRUE(std::equal(std::make_reverse_iterator(line.end()),
        std::make_reverse_iterator(line.begin()),
        concat_parm.rbegin(),
        concat_parm.rend()));
}

INSTANTIATE_TEST_SUITE_P(logical_line,
    logical_line_iterator_fixture,
    ::testing::ValuesIn(std::vector {
        std::vector<std::string_view> {},
        std::vector<std::string_view> { "" },
        std::vector<std::string_view> { "", "" },
        std::vector<std::string_view> { "a", "b" },
        std::vector<std::string_view> { "", "b" },
        std::vector<std::string_view> { "a", "" },
        std::vector<std::string_view> { "a", "", "c" },
        std::vector<std::string_view> { "a", "", "c", "" },
        std::vector<std::string_view> { "a", "", "c", "", "e" },
        std::vector<std::string_view> { "", "", "abc", "", "", "def", "", "", "ghi", "", "" },
    }));
