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
#include "context/literal_pool.h"

// test for
// handling of literals

TEST(literals, duplicate_when_loctr_references)
{
    std::string input = R"(
      MACRO
      MAC
      LARL 0,=A(*)
      MEND
      MAC
      MAC
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    auto* sect = get_section(a.hlasm_ctx(), "");
    ASSERT_TRUE(sect);
    EXPECT_EQ(sect->location_counters().back()->current_address().offset(), 24);
}

TEST(literals, unique_when_no_loctr_references)
{
    std::string input = R"(
      MACRO
      MAC
      LARL 0,=A(0)
      MEND
      MAC
      MAC
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    auto* sect = get_section(a.hlasm_ctx(), "");
    ASSERT_TRUE(sect);
    EXPECT_EQ(sect->location_counters().back()->current_address().offset(), 20);
}

TEST(literals, no_nested_literals)
{
    std::string input = R"(
    LARL 0,=A(=A(0))
    DC   A(=A(0))
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0013", "S0013" }));
}

TEST(literals, attribute_references_to_literals)
{
    std::string input = R"(
    DC   A(L'=A(0))
A   EQU  L'=A(0)
    LARL 0,=A(L'=A(0))
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    const auto* symbol = get_symbol(a.hlasm_ctx(), "A");
    ASSERT_TRUE(symbol);
    auto symbol_value = symbol->value();
    ASSERT_EQ(symbol_value.value_kind(), context::symbol_value_kind::ABS);
    EXPECT_EQ(symbol_value.get_abs(), 4);

    auto* sect = get_section(a.hlasm_ctx(), "");
    ASSERT_TRUE(sect);
    EXPECT_EQ(sect->location_counters().back()->current_address().offset(), 24);
}

TEST(literals, attribute_references_to_literals_in_ca)
{
    std::string input = R"(
&L SETA L'=FS1'0'
&D SETA D'=FS1'0'
&S SETA S'=FS1'0'
&I SETA I'=FS1'0'
&T SETC T'=FS1'0'
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<context::A_t>(a.hlasm_ctx(), "L"), 4);
    EXPECT_EQ(get_var_value<context::A_t>(a.hlasm_ctx(), "D"), 1);
    EXPECT_EQ(get_var_value<context::A_t>(a.hlasm_ctx(), "S"), 1);
    EXPECT_EQ(get_var_value<context::A_t>(a.hlasm_ctx(), "I"), 30);
    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "T"), "F");

    auto* sect = get_section(a.hlasm_ctx(), "");
    ASSERT_TRUE(sect);
    EXPECT_EQ(sect->location_counters().back()->current_address().offset(), 4);
}
