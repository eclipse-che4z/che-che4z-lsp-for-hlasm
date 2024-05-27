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

#include <algorithm>
#include <ranges>
#include <string_view>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "lexing/logical_line.h"

using namespace hlasm_plugin::parser_library::lexing;

namespace {
class logical_line_iterator_fixture : public ::testing::TestWithParam<std::vector<std::string_view>>
{};
} // namespace

using test_logical_line = logical_line<std::string_view::iterator>;
using test_logical_line_segment = logical_line_segment<std::string_view::iterator>;

TEST_P(logical_line_iterator_fixture, general_behavior)
{
    test_logical_line line;
    const auto& parm = GetParam();
    std::ranges::transform(parm, std::back_inserter(line.segments), [](const auto& c) {
        return test_logical_line_segment { c.begin(), c.begin(), c.end(), c.end(), c.end() };
    });

    std::string concat_parm;
    for (auto p : parm)
        concat_parm.append(p);

    EXPECT_TRUE(std::ranges::equal(line, concat_parm));
    EXPECT_TRUE(std::ranges::equal(std::views::reverse(line), std::views::reverse(concat_parm)));
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

namespace {
class logical_line_iterator_coordinates_test : public testing::Test
{
public:
    logical_line_iterator_coordinates_test(std::string_view input)
        : m_input(std::move(input))
    {}

    void SetUp() override { ASSERT_TRUE(extract_logical_line(m_line, m_input, default_ictl).first); }

protected:
    test_logical_line m_line;
    std::string_view m_input;
};

class logical_line_iterator_coordinates_singleline : public logical_line_iterator_coordinates_test
{
public:
    logical_line_iterator_coordinates_singleline()
        : logical_line_iterator_coordinates_test("123456")
    {}
};
} // namespace

TEST_F(logical_line_iterator_coordinates_singleline, unchanged_code_part)
{
    auto expected = std::pair<size_t, size_t>(0, 0);
    EXPECT_EQ(m_line.begin().get_coordinates(), expected);
    EXPECT_EQ(m_line.end().get_coordinates(), expected);

    expected = std::pair<size_t, size_t>(3, 0);
    EXPECT_EQ(std::next(m_line.begin(), 3).get_coordinates(), expected);

    expected = std::pair<size_t, size_t>(5, 0);
    EXPECT_EQ(std::prev(m_line.end()).get_coordinates(), expected);
}

TEST_F(logical_line_iterator_coordinates_singleline, removed_code_suffix)
{
    std::advance(m_line.segments.front().continuation, -3);

    auto expected = std::pair<size_t, size_t>(0, 0);
    EXPECT_EQ(m_line.begin().get_coordinates(), expected);

    expected = std::pair<size_t, size_t>(2, 0);
    EXPECT_EQ(std::prev(m_line.end()).get_coordinates(), expected);
}

namespace {
class logical_line_iterator_coordinates_multiline : public logical_line_iterator_coordinates_test
{
public:
    logical_line_iterator_coordinates_multiline()
        : logical_line_iterator_coordinates_test(m_input)
    {}

private:
    inline static const std::string_view m_input =
        R"(                  EXEC      SQL                                        X00004000
               --comment                                               X
               SELECT                                                  X
               1       --rem                                           X00050000
                   INTO :B                                             X
               FROM                                                    X
               SYSIBM.SYSDUMMY1)";
};

} // namespace

TEST_F(logical_line_iterator_coordinates_multiline, unchanged_code_part)
{
    auto expected = std::pair<size_t, size_t>(0, 0);
    EXPECT_EQ(m_line.begin().get_coordinates(), expected);

    expected = std::pair<size_t, size_t>(18, 2);
    EXPECT_EQ(std::next(m_line.begin(), 130).get_coordinates(), expected);

    expected = std::pair<size_t, size_t>(30, 6);
    EXPECT_EQ(std::prev(m_line.end()).get_coordinates(), expected);
}

TEST_F(logical_line_iterator_coordinates_multiline, empty_all_lines)
{
    std::ranges::fill(m_line.segments, logical_line_segment<std::string_view::iterator>());

    auto expected = std::pair<size_t, size_t>(0, 0);
    EXPECT_EQ(m_line.begin().get_coordinates(), expected);
    EXPECT_EQ(m_line.end().get_coordinates(), expected);
}

TEST_F(logical_line_iterator_coordinates_multiline, empty_last_line)
{
    m_line.segments.back().code = m_line.segments.back().continuation;

    auto expected = std::pair<size_t, size_t>(0, 0);
    EXPECT_EQ(m_line.begin().get_coordinates(), expected);

    expected = std::pair<size_t, size_t>(70, 5);
    EXPECT_EQ(std::prev(m_line.end()).get_coordinates(), expected);
}

TEST_F(logical_line_iterator_coordinates_multiline, empty_some_lines)
{
    m_line.segments[1].code = m_line.segments[1].continuation;
    std::advance(m_line.segments[3].continuation, -46);

    auto expected = std::pair<size_t, size_t>(0, 0);
    EXPECT_EQ(m_line.begin().get_coordinates(), expected);

    expected = std::pair<size_t, size_t>(70, 0);
    EXPECT_EQ(std::next(m_line.begin(), 70).get_coordinates(), expected);

    expected = std::pair<size_t, size_t>(15, 2);
    EXPECT_EQ(std::next(m_line.begin(), 71).get_coordinates(), expected);

    expected = std::pair<size_t, size_t>(70, 4);
    EXPECT_EQ(std::next(m_line.begin(), 192).get_coordinates(), expected);

    expected = std::pair<size_t, size_t>(15, 5);
    EXPECT_EQ(std::next(m_line.begin(), 193).get_coordinates(), expected);

    expected = std::pair<size_t, size_t>(30, 6);
    EXPECT_EQ(std::prev(m_line.end()).get_coordinates(), expected);
}
