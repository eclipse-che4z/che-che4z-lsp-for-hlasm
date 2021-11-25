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
    EXPECT_EQ(get_var_value<context::A_t>(a.hlasm_ctx(), "D"), 0);
    EXPECT_EQ(get_var_value<context::A_t>(a.hlasm_ctx(), "S"), 1);
    EXPECT_EQ(get_var_value<context::A_t>(a.hlasm_ctx(), "I"), 30);
    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "T"), "U");

    auto* sect = get_section(a.hlasm_ctx(), "");
    ASSERT_TRUE(sect);
    EXPECT_EQ(sect->location_counters().back()->current_address().offset(), 4);
}

TEST(literals, attribute_references_to_literals_in_ca_future)
{
    // IBM HLASM currently fails to parse the expression with parenthesis (ASMA128S),
    // but the support claims it may be accepted in the future (TS007628335)
    std::string input = R"(
A DC A(0)
B DC A(0)
&L SETA L'=FL(B-A)S1'0'
&D SETA D'=FL(B-A)S1'0'
&S SETA S'=FL(B-A)S1'0'
&I SETA I'=FL(B-A)S1'0'
&T SETC T'=FL(B-A)S1'0'
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<context::A_t>(a.hlasm_ctx(), "L"), 4);
    EXPECT_EQ(get_var_value<context::A_t>(a.hlasm_ctx(), "D"), 0);
    EXPECT_EQ(get_var_value<context::A_t>(a.hlasm_ctx(), "S"), 1);
    EXPECT_EQ(get_var_value<context::A_t>(a.hlasm_ctx(), "I"), 30);
    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "T"), "U");

    auto* sect = get_section(a.hlasm_ctx(), "");
    ASSERT_TRUE(sect);
    EXPECT_EQ(sect->location_counters().back()->current_address().offset(), 12);
}

TEST(literals, ltorg)
{
    std::string input = R"(
    LARL 0,=A(0)
A   LTORG
B   EQU   *-A
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    const auto* symbol = get_symbol(a.hlasm_ctx(), "B");
    ASSERT_TRUE(symbol);
    auto symbol_value = symbol->value();
    ASSERT_EQ(symbol_value.value_kind(), context::symbol_value_kind::ABS);
    EXPECT_EQ(symbol_value.get_abs(), 4);
}

TEST(literals, ltorg_repeating_literals)
{
    std::string input = R"(
    LARL 0,=A(0)
    LARL 0,=A(0)
    LTORG
    LARL 0,=A(0)
    LARL 0,=A(0)
    LTORG
    LARL 0,=A(0)
    LARL 0,=A(0)
    LTORG
    LARL 0,=A(0)
    LARL 0,=A(0)
    LTORG
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    auto* sect = get_section(a.hlasm_ctx(), "");
    ASSERT_TRUE(sect);
    EXPECT_EQ(sect->location_counters().back()->current_address().offset(), 68);
}

TEST(literals, similar)
{
    struct test
    {
        std::string l;
        std::string r;
        size_t size;

        std::string operator()() const { return "L:" + l + '\n' + "R:" + r + '\n' + "S:" + std::to_string(size); }
    };
    for (const auto& t : {
             test { "=A(0)", "=a(0)", 4 },
             test { "=A(0)", "=AL4(0)", 8 },
             test { "=A(a)", "=a(A)", 4 },
             test { "=A(A)", "=A(B)", 8 },
             test { "=A(A,B)", "=a(B,A)", 16 },
             test { "=A(A,b)", "=a(a,B)", 8 },
             test { "=F'1 2'", "=F'1     2'", 8 },
             test { "=A(0)", "=1A(0)", 8 },
             test { "=F'0'", "=A(0)", 8 },
             test { "=FD'0'", "=F'0'", 12 },
             test { "=(B-A)A(0)", "=(1)A(0)", 8 },
             test { "=A(*)", "=A(*)", 8 },
             test { "=Al4(0)", "=aL4(0)", 4 },
         })
    {
        std::string input = "A EQU 1\nB EQU 2\nT1   EQU  L'" + t.l + '\n' + "T2   EQU  L'" + t.r;
        analyzer a(input);
        a.analyze();
        a.collect_diags();

        EXPECT_TRUE(a.diags().empty());

        auto* sect = get_section(a.hlasm_ctx(), "");
        ASSERT_TRUE(sect);
        EXPECT_EQ(sect->location_counters().back()->current_address().offset(), t.size) << t();
    }
}

TEST(literals, in_machine_instructions)
{
    std::string input = R"(
    MVC =A(0),=A(0)
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    auto* sect = get_section(a.hlasm_ctx(), "");
    ASSERT_TRUE(sect);
    EXPECT_EQ(sect->location_counters().back()->current_address().offset(), 12);
}
