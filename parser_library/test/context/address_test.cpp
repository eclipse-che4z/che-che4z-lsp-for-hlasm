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

#include <array>

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "context/hlasm_context.h"
#include "context/ordinary_assembly/address.h"
#include "context/ordinary_assembly/location_counter.h"
#include "context/ordinary_assembly/section.h"
#include "library_info_transitional.h"

using namespace ::testing;

// test for
// address class

TEST(address, normalized_spaces)
{
    hlasm_context ctx;
    ctx.ord_ctx.set_section(id_index("TEST"), section_kind::COMMON, library_info_transitional::empty);

    auto sp1 = ctx.ord_ctx.current_section()->current_location_counter().set_value_undefined(0, 0);
    auto sp2 = ctx.ord_ctx.current_section()->current_location_counter().register_ordinary_space(halfword);

    auto addr = ctx.ord_ctx.current_section()->current_location_counter().current_address();

    sp1->resolve(sp2);

    auto [normalized, _] = addr.normalized_spaces();

    ASSERT_EQ(normalized.size(), (size_t)1);
    EXPECT_EQ(normalized.front().first, sp2);
    EXPECT_EQ(normalized.front().second, (size_t)2);

    auto [normalized_move, __] = std::move(addr).normalized_spaces();

    ASSERT_EQ(normalized_move.size(), (size_t)1);
    EXPECT_EQ(normalized_move.front().first, sp2);
    EXPECT_EQ(normalized_move.front().second, (size_t)2);
}

TEST(address, has_unresolved_spaces)
{
    hlasm_context ctx;
    ctx.ord_ctx.set_section(id_index("TEST"), section_kind::COMMON, library_info_transitional::empty);

    auto sp1 = ctx.ord_ctx.current_section()->current_location_counter().set_value_undefined(0, 0);
    auto sp2 = ctx.ord_ctx.current_section()->current_location_counter().register_ordinary_space(halfword);

    auto addr = ctx.ord_ctx.current_section()->current_location_counter().current_address();

    sp1->resolve(sp2);

    ASSERT_TRUE(addr.has_unresolved_space());

    sp2->resolve(1, resolve_reason::normal);

    ASSERT_FALSE(addr.has_unresolved_space());
}

TEST(address, constructors)
{
    hlasm_context ctx;
    auto sect = ctx.ord_ctx.set_section(id_index("TEST"), section_kind::COMMON, library_info_transitional::empty);

    auto sp1 = ctx.ord_ctx.current_section()->current_location_counter().set_value_undefined(0, 0);
    auto sp2 = ctx.ord_ctx.current_section()->current_location_counter().register_ordinary_space(halfword);

    space_storage spaces { sp2 };

    address addr1({ id_index(), sect }, 12345, spaces);
    address addr2({ id_index(), sect }, 12345, std::move(spaces));

    auto diff = addr1 - addr2;

    EXPECT_TRUE(diff.bases().empty());
    EXPECT_FALSE(diff.has_spaces());
    EXPECT_EQ(diff.offset(), 0);
}

TEST(address, subtract_optimization)
{
    hlasm_context ctx;
    auto sect = ctx.ord_ctx.set_section(id_index("TEST"), section_kind::COMMON, library_info_transitional::empty);

    address addr2({ id_index(), sect }, 12345, {});

    auto diff = address() - addr2;

    std::array expected_bases { address::base_entry { {}, sect, -1 } };

    EXPECT_THAT(diff.bases(), Pointwise(Eq(), expected_bases));
}
