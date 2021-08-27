/*
 * Copyright (c) 2021 Broadcom.
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

TEST(START, defines_section)
{
    std::string input(R"(
S START
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());

    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.section_defined(a.hlasm_ctx().ids().add("S"), section_kind::EXECUTABLE));
}

TEST(START, section_with_offset)
{
    std::string input(R"(
S START 7
E EQU *
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());

    const auto* s = a.hlasm_ctx().ord_ctx.get_section(a.hlasm_ctx().ids().add("S"));
    ASSERT_TRUE(s);
    const auto* e = a.hlasm_ctx().ord_ctx.get_symbol(a.hlasm_ctx().ids().add("E"));
    ASSERT_TRUE(e);

    const auto& e_value = e->value();
    ASSERT_EQ(e_value.value_kind(), symbol_value_kind::RELOC);

    const auto& reloc = e_value.get_reloc();
    ASSERT_EQ(reloc.bases().size(), 1);
    EXPECT_EQ(reloc.bases().front().first.owner, s);
    EXPECT_EQ(reloc.offset(), 8);
}

TEST(START, generates_csect)
{
    std::string input(R"(
S START 
S CSECT
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());
}

TEST(START, already_started)
{
    std::string input(R"(
S CSECT
S START
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E073" }));
}

TEST(START, already_started_private_csect)
{
    std::string input(R"(
E EQU 0
S START
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E073" }));
}

TEST(START, dsect_not_csect)
{
    std::string input(R"(
D DSECT
E EQU 0
S CSECT
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());
}
