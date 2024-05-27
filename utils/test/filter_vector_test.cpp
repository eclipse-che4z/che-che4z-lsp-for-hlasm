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

#include "gtest/gtest.h"

#include "utils/filter_vector.h"

using namespace hlasm_plugin::utils;

TEST(filter_vector, simple)
{
    filter_vector<uint32_t> f;

    f.emplace_back();
    f.emplace_back();
    f.emplace_back();

    EXPECT_EQ(f.size(), 3);

    f.pop_back();
    f.pop_back();
    f.pop_back();

    EXPECT_EQ(f.size(), 0);
}

TEST(filter_vector, set_reset)
{
    filter_vector<uint32_t> f;

    f.emplace_back();
    f.emplace_back();
    f.emplace_back();

    f.set(0, 0);
    f.set(filter_vector<uint32_t>::effective_bit_count - 1, 2);

    EXPECT_TRUE(f.any(0));
    EXPECT_FALSE(f.any(1));
    EXPECT_TRUE(f.any(2));

    f.reset(0, 1);

    EXPECT_TRUE(f.any(0));
    EXPECT_FALSE(f.any(1));
    EXPECT_TRUE(f.any(2));
}

TEST(filter_vector, clear)
{
    filter_vector<uint32_t> f;

    f.emplace_back();

    EXPECT_GT(f.size(), 0);

    f.clear();

    EXPECT_EQ(f.size(), 0);
}

TEST(filter_vector, reset)
{
    filter_vector<uint32_t> f;

    f.emplace_back();

    f.set(0, 0);
    f.set(filter_vector<uint32_t>::effective_bit_count - 1, 0);

    EXPECT_TRUE(f.any(0));

    f.reset(0);

    EXPECT_FALSE(f.any(0));
}

TEST(filter_vector, reset_global)
{
    filter_vector<uint32_t> f;

    f.emplace_back();
    f.emplace_back();
    f.emplace_back();

    f.set(0, 0);
    f.set(0, 1);
    f.set(filter_vector<uint32_t>::effective_bit_count - 1, 2);

    EXPECT_TRUE(f.any(0));
    EXPECT_TRUE(f.any(1));
    EXPECT_TRUE(f.any(2));

    f.reset_global(0);

    EXPECT_FALSE(f.any(0));
    EXPECT_FALSE(f.any(1));
    EXPECT_TRUE(f.any(2));
}

TEST(filter_vector, reset_global_accum)
{
    filter_vector<uint32_t> f;

    f.emplace_back();
    f.emplace_back();
    f.emplace_back();
    f.emplace_back();

    f.set(0, 0);
    f.set(0, 1);
    f.set(filter_vector<uint32_t>::effective_bit_count - 1, 2);
    f.set(filter_vector<uint32_t>::effective_bit_count - 1, 3);

    EXPECT_TRUE(f.any(0));
    EXPECT_TRUE(f.any(1));
    EXPECT_TRUE(f.any(2));
    EXPECT_TRUE(f.any(3));

    auto accum = f.get_global_reset_accumulator();
    accum.reset(0);
    accum.reset(filter_vector<uint32_t>::effective_bit_count - 1);

    EXPECT_TRUE(f.any(0));
    EXPECT_TRUE(f.any(1));
    EXPECT_TRUE(f.any(2));
    EXPECT_TRUE(f.any(3));

    f.reset_global(accum);

    EXPECT_FALSE(f.any(0));
    EXPECT_FALSE(f.any(1));
    EXPECT_FALSE(f.any(2));
    EXPECT_FALSE(f.any(3));
}

TEST(filter_vector, swap)
{
    filter_vector<uint32_t> f;

    f.emplace_back();
    f.emplace_back();

    f.set(0, 0);

    EXPECT_TRUE(f.any(0));
    EXPECT_FALSE(f.any(1));

    f.swap(0, 1);

    EXPECT_FALSE(f.any(0));
    EXPECT_TRUE(f.any(1));
}

TEST(filter_vector, get_set_bitset)
{
    filter_vector<uint32_t> f;

    f.emplace_back();

    f.set(0, 0);
    f.set(filter_vector<uint32_t>::effective_bit_count - 1, 0);

    EXPECT_TRUE(f.any(0));

    auto bitset = f.get(0);
    bitset.front() ^= 0x00000001u;
    bitset.back() ^= 0x80000000u;

    EXPECT_TRUE(std::ranges::all_of(bitset, [](auto n) { return n == 0; }));

    f.set(bitset, 0);

    EXPECT_FALSE(f.any(0));

    bitset.front() ^= 0x00000001u;
    bitset.back() ^= 0x80000000u;
    f.set(bitset, 0);

    EXPECT_TRUE(f.any(0));
    EXPECT_EQ(bitset, f.get(0));
}

TEST(filter_vector, assign)
{
    filter_vector<uint32_t> f;

    f.emplace_back();
    f.emplace_back();

    f.set(0, 0);
    f.set(filter_vector<uint32_t>::effective_bit_count - 1, 0);

    EXPECT_TRUE(f.any(0));
    EXPECT_FALSE(f.any(1));

    f.assign(1, 0);

    EXPECT_TRUE(f.any(0));
    EXPECT_TRUE(f.any(1));
}
