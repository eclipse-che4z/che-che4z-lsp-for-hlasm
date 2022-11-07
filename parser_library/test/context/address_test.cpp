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

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "library_info_transitional.h"

// test for
// address class

TEST(address, normalized_spaces)
{
    hlasm_context ctx;
    ctx.ord_ctx.set_section(id_index("TEST"), section_kind::COMMON, location(), library_info_transitional::empty);

    auto sp1 = ctx.ord_ctx.current_section()->current_location_counter().set_value(
        ctx.ord_ctx.current_section()->current_location_counter().current_address(), 0, 0, true);
    auto sp2 = ctx.ord_ctx.current_section()->current_location_counter().register_ordinary_space(halfword);

    auto addr = ctx.ord_ctx.current_section()->current_location_counter().current_address();

    space::resolve(sp1, sp2);

    auto normalized = addr.normalized_spaces();

    ASSERT_EQ(normalized.size(), (size_t)1);
    EXPECT_EQ(normalized.front().first, sp2);
    EXPECT_EQ(normalized.front().second, (size_t)2);
}

TEST(address, has_unresolved_spaces)
{
    hlasm_context ctx;
    ctx.ord_ctx.set_section(id_index("TEST"), section_kind::COMMON, location(), library_info_transitional::empty);

    auto sp1 = ctx.ord_ctx.current_section()->current_location_counter().set_value(
        ctx.ord_ctx.current_section()->current_location_counter().current_address(), 0, 0, true);
    auto sp2 = ctx.ord_ctx.current_section()->current_location_counter().register_ordinary_space(halfword);

    auto addr = ctx.ord_ctx.current_section()->current_location_counter().current_address();

    space::resolve(sp1, sp2);

    ASSERT_TRUE(addr.has_unresolved_space());

    space::resolve(sp2, 1);

    ASSERT_FALSE(addr.has_unresolved_space());
}
