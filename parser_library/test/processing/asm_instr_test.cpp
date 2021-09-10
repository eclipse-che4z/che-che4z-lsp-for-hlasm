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
#include "analyzer.h"
#include "ebcdic_encoding.h"

using namespace hlasm_plugin::parser_library;

TEST(asm_instr_processing, CCW)
{
    std::string input = R"(
       LR 1,1   random instruction to test CCW alignment
LEN    EQU 4
CCWSYM    CCW X'04',ADDR,0,LEN
ADDR   DS CL4
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_EQ(a.diags().size(), 0U);

    auto& ctx = *a.context().hlasm_ctx;

    auto symbol = ctx.ord_ctx.get_symbol(ctx.ids().add("CCWSYM"));
    ASSERT_NE(symbol, nullptr);
    EXPECT_EQ(symbol->value().get_reloc().offset(), 8);

    EXPECT_EQ(symbol->attributes().get_attribute_value(context::data_attr_kind::T), 'W'_ebcdic);
    EXPECT_EQ(symbol->attributes().get_attribute_value(context::data_attr_kind::L), 8);
}

TEST(asm_instr_processing, CCW_loctr_correct_assign)
{
    std::string input = R"(
SYM  DS   CL249
     CCW  *-SYM,0,0,0
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), 1U);
    EXPECT_EQ(a.diags()[0].code, "M122");
}

TEST(asm_instr_processing, CNOP)
{
    std::string input = R"(
 DC CL1''

CNOPSYM CNOP 0,8

A LR 1,1
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_EQ(a.diags().size(), 0U);

    auto& ctx = *a.context().hlasm_ctx;

    auto symbol = ctx.ord_ctx.get_symbol(ctx.ids().add("CNOPSYM"));
    ASSERT_NE(symbol, nullptr);
    EXPECT_EQ(symbol->value().get_reloc().offset(), 2);

    EXPECT_EQ(symbol->attributes().get_attribute_value(context::data_attr_kind::T), 'I'_ebcdic);

    auto symbol_after = ctx.ord_ctx.get_symbol(ctx.ids().add("A"));
    ASSERT_NE(symbol_after, nullptr);
    EXPECT_EQ(symbol_after->value().get_reloc().offset(), 8);
}

TEST(asm_instr_processing, CNOP_byte_expr)
{
    std::string input = R"(
         DC CL4''
N14      EQU 14
CNOPSYM  CNOP N14,16

A        LR 1,1
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_EQ(a.diags().size(), 0U);

    auto& ctx = *a.context().hlasm_ctx;

    auto symbol = ctx.ord_ctx.get_symbol(ctx.ids().add("CNOPSYM"));
    ASSERT_NE(symbol, nullptr);
    EXPECT_EQ(symbol->value().get_reloc().offset(), 4);

    auto symbol_after = ctx.ord_ctx.get_symbol(ctx.ids().add("A"));
    ASSERT_NE(symbol_after, nullptr);
    EXPECT_EQ(symbol_after->value().get_reloc().offset(), 14);
}

TEST(asm_instr_processing, CNOP_non_absolute_expr)
{
    std::string input = R"(
ADDR DC CL4''

CNOPSYM CNOP ADDR,16
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), 0U);

    // Should emit a diagnostic, but we dont check relocatable symbols at all right now.
    // EXPECT_EQ(a.diags().size(), 1U);
    // EXPECT_EQ(a.diags()[0].code, "A143");

    auto& ctx = *a.context().hlasm_ctx;

    auto symbol = ctx.ord_ctx.get_symbol(ctx.ids().add("CNOPSYM"));
    ASSERT_NE(symbol, nullptr);
    EXPECT_EQ(symbol->value().get_reloc().offset(), 4);
}

TEST(asm_instr_processing, ALIAS_mandatory_label)
{
    std::string input = R"(
  ALIAS C'SOMESTRING'
  ALIAS X'434343434343'
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A163", "A163" }));
}

TEST(asm_instr_processing, ALIAS_external_missing)
{
    /* TODO: lable must be an external symbol
    std::string input = R"(
A ALIAS C'SOMESTRING'
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "????" }));
    */
}

TEST(asm_instr_processing, ALIAS_external_present)
{
    std::string input = R"(
A DSECT
B START
C CSECT
D DXD F
  DC Q(A)
  ENTRY E
E DS 0H
  DC V(F)
G RSECT
H COM
  EXTRN I
  WXTRN J
A ALIAS C'STRING'
B ALIAS C'STRING'
C ALIAS C'STRING'
D ALIAS C'STRING'
E ALIAS C'STRING'
F ALIAS C'STRING'
G ALIAS C'STRING'
H ALIAS C'STRING'
I ALIAS C'STRING'
J ALIAS C'STRING'
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
}
