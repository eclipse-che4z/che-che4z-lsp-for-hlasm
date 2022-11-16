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

#include <span>
#include <string_view>

#include "gtest/gtest.h"

#include "utils/levenshtein_distance.h"

using namespace hlasm_plugin::utils;

namespace {
struct levenshtein_distance_data
{
    std::string_view l;
    std::string_view r;
    size_t result;
};
} // namespace

class levenshtein_distance_fixture : public ::testing::TestWithParam<levenshtein_distance_data>
{};

INSTANTIATE_TEST_SUITE_P(levenshtein_distance_data,
    levenshtein_distance_fixture,
    ::testing::Values(levenshtein_distance_data { "", "", 0 },
        levenshtein_distance_data { "abc", "abc", 0 },
        levenshtein_distance_data { "abc", "", 3 },
        levenshtein_distance_data { "abc", "ab", 1 },
        levenshtein_distance_data { "aaa", "aaba", 1 },
        levenshtein_distance_data { "abc", "acb", 2 },
        levenshtein_distance_data { "abcd", "abc", 1 },
        levenshtein_distance_data { "abdc", "abc", 1 },
        levenshtein_distance_data { "aabc", "abc", 1 },
        levenshtein_distance_data { "aabc", "abbc", 1 }));

TEST_P(levenshtein_distance_fixture, verify)
{
    const auto& [l, r, result] = GetParam();
    EXPECT_EQ(levenshtein_distance(l, r), result);
    EXPECT_EQ(levenshtein_distance(r, l), result);
    EXPECT_EQ(levenshtein_distance_n<16>(l, r), result);
    EXPECT_EQ(levenshtein_distance_n<16>(r, l), result);
}

TEST(levenshtein_distance, mixed_types)
{
    EXPECT_EQ(levenshtein_distance(std::span("abc"), "abc"), 0);
    EXPECT_EQ(levenshtein_distance_n<4>(std::span("abc"), "abc"), 0);
}
