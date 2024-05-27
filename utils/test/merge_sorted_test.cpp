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

#include <array>
#include <vector>

#include "gtest/gtest.h"

#include "utils/merge_sorted.h"
#include "utils/projectors.h"

using namespace hlasm_plugin::utils;

TEST(merge_sorted, simple)
{
    std::vector<int> vec { 1, 3, 5, 7, 9 };
    std::array ar { 0, 2, 4, 6, 8 };

    std::vector<int> expected { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    merge_sorted(vec, ar);

    EXPECT_EQ(vec, expected);
}

TEST(merge_sorted, proxy_reference)
{
    std::vector<bool> vec { true };
    std::array ar { false };

    std::vector<bool> expected { false, true };

    merge_sorted(vec, ar, [](auto&& l, const auto& r) { return (bool)l <=> (bool)r; });

    EXPECT_EQ(vec, expected);
}

TEST(merge_sorted, comparer)
{
    struct value
    {
        int i;
    };
    std::vector<value> vec { value { 1 }, value { 3 }, value { 5 }, value { 7 }, value { 9 } };
    std::array ar { value { 0 }, value { 2 }, value { 4 }, value { 6 }, value { 8 } };

    std::vector<value> expected {
        value { 0 },
        value { 1 },
        value { 2 },
        value { 3 },
        value { 4 },
        value { 5 },
        value { 6 },
        value { 7 },
        value { 8 },
        value { 9 },
    };

    merge_sorted(vec, ar, [](const auto& l, const auto& r) { return l.i <=> r.i; });

    EXPECT_TRUE(std::ranges::equal(vec, expected, {}, &value::i, &value::i));
}

TEST(merge_sorted, complex_merge)
{
    std::vector<std::pair<int, int>> vec { { 1, 1 }, { 3, 1 }, { 5, 1 }, { 7, 1 }, { 9, 1 } };
    std::array<std::pair<int, int>, 10> vec2 {
        { { 0, 1 }, { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 5, 1 }, { 6, 1 }, { 7, 1 }, { 8, 1 }, { 9, 1 } }
    };

    std::vector<std::pair<int, int>> expected {
        { 0, 1 }, { 1, 2 }, { 2, 1 }, { 3, 2 }, { 4, 1 }, { 5, 2 }, { 6, 1 }, { 7, 2 }, { 8, 1 }, { 9, 2 }
    };

    merge_sorted(
        vec,
        vec2,
        [](const auto& l, const auto& r) { return l.first <=> r.first; },
        [](auto& t, const auto& s) { t.second += s.second; });

    EXPECT_EQ(vec, expected);
}

TEST(merge_sorted, move_only)
{
    std::vector<std::unique_ptr<int>> vec;
    for (int i : { 1, 3, 5, 7, 9 })
        vec.emplace_back(std::make_unique<int>(i));
    std::vector<std::unique_ptr<int>> vec2;
    for (int i : { 0, 2, 4, 6, 8 })
        vec2.emplace_back(std::make_unique<int>(i));
    std::vector<int> expected { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    merge_sorted(vec,
        std::make_move_iterator(vec2.begin()),
        std::make_move_iterator(vec2.end()),
        [](const auto& l, const auto& r) { return *l <=> *r; });

    EXPECT_TRUE(std::ranges::equal(vec, expected, {}, hlasm_plugin::utils::dereference));
}

TEST(merge_unsorted, simple)
{
    std::vector<int> vec { 1, 3, 5, 7, 9 };
    std::array ar { 8, 6, 4, 2, 0 };

    std::vector<int> expected { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    merge_unsorted(vec, ar);

    EXPECT_EQ(vec, expected);
}

TEST(merge_unsorted, proxy_reference)
{
    std::vector<bool> vec { true };
    std::array ar { false };

    std::vector<bool> expected { false, true };

    merge_unsorted(vec, ar, [](auto&& l, const auto& r) { return (bool)l <=> (bool)r; });

    EXPECT_EQ(vec, expected);
}

TEST(merge_unsorted, comparer)
{
    struct value
    {
        int i;
    };
    std::vector<value> vec { value { 1 }, value { 3 }, value { 5 }, value { 7 }, value { 9 } };
    std::array ar { value { 8 }, value { 6 }, value { 4 }, value { 2 }, value { 0 } };

    std::vector<value> expected {
        value { 0 },
        value { 1 },
        value { 2 },
        value { 3 },
        value { 4 },
        value { 5 },
        value { 6 },
        value { 7 },
        value { 8 },
        value { 9 },
    };

    merge_unsorted(vec, ar, [](const auto& l, const auto& r) { return l.i <=> r.i; });

    EXPECT_TRUE(std::ranges::equal(vec, expected, {}, &value::i, &value::i));
}

TEST(merge_unsorted, complex_merge)
{
    std::vector<std::pair<int, int>> vec { { 1, 1 }, { 3, 1 }, { 5, 1 }, { 7, 1 }, { 9, 1 } };
    std::array<std::pair<int, int>, 10> vec2 {
        { { 9, 1 }, { 8, 1 }, { 7, 1 }, { 6, 1 }, { 5, 1 }, { 4, 1 }, { 3, 1 }, { 2, 1 }, { 1, 1 }, { 0, 1 } }
    };

    std::vector<std::pair<int, int>> expected {
        { 0, 1 }, { 1, 2 }, { 2, 1 }, { 3, 2 }, { 4, 1 }, { 5, 2 }, { 6, 1 }, { 7, 2 }, { 8, 1 }, { 9, 2 }
    };

    merge_unsorted(
        vec,
        vec2,
        [](const auto& l, const auto& r) { return l.first <=> r.first; },
        [](auto& t, const auto& s) { t.second += s.second; });

    EXPECT_EQ(vec, expected);
}

TEST(merge_unsorted, move_only)
{
    std::vector<std::unique_ptr<int>> vec;
    for (int i : { 1, 3, 5, 7, 9 })
        vec.emplace_back(std::make_unique<int>(i));
    std::vector<std::unique_ptr<int>> vec2;
    for (int i : { 8, 6, 4, 2, 0 })
        vec2.emplace_back(std::make_unique<int>(i));
    std::vector<int> expected { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    merge_unsorted(vec,
        std::make_move_iterator(vec2.begin()),
        std::make_move_iterator(vec2.end()),
        [](const auto& l, const auto& r) { return *l <=> *r; });

    EXPECT_TRUE(std::ranges::equal(vec, expected, {}, hlasm_plugin::utils::dereference));
}
