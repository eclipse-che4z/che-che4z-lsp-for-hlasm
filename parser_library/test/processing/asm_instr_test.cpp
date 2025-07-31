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
#include "context/hlasm_context.h"
#include "context/ordinary_assembly/symbol.h"
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
    EXPECT_TRUE(a.diags().empty());

    auto& ctx = *a.context().hlasm_ctx;

    auto symbol = get_symbol(ctx, "CCWSYM");
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
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M122" }));
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
    EXPECT_TRUE(a.diags().empty());

    auto& ctx = *a.context().hlasm_ctx;

    auto symbol = get_symbol(ctx, "CNOPSYM");
    ASSERT_NE(symbol, nullptr);
    EXPECT_EQ(symbol->value().get_reloc().offset(), 2);

    EXPECT_EQ(symbol->attributes().get_attribute_value(context::data_attr_kind::T), 'I'_ebcdic);

    auto symbol_after = get_symbol_reloc(ctx, "A");
    ASSERT_TRUE(symbol_after.has_value());
    EXPECT_EQ(symbol_after->offset(), 8);
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
    EXPECT_TRUE(a.diags().empty());

    auto& ctx = *a.context().hlasm_ctx;

    auto symbol = get_symbol(ctx, "CNOPSYM");
    ASSERT_NE(symbol, nullptr);
    EXPECT_EQ(symbol->value().get_reloc().offset(), 4);

    auto symbol_after = get_symbol_reloc(ctx, "A");
    ASSERT_TRUE(symbol_after.has_value());
    EXPECT_EQ(symbol_after->offset(), 14);
}

TEST(asm_instr_processing, CNOP_non_absolute_expr)
{
    std::string input = R"(
ADDR DC CL4''

CNOPSYM CNOP ADDR,16
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    // Should emit a diagnostic, but we dont check relocatable symbols at all right now.
    // EXPECT_EQ(a.diags().size(), 1U);
    // EXPECT_EQ(a.diags()[0].code, "A143");

    auto& ctx = *a.context().hlasm_ctx;

    auto symbol = get_symbol(ctx, "CNOPSYM");
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

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A163", "A163" }));
}

TEST(asm_instr_processing, ALIAS_external_missing)
{
    /* TODO: label must be an external symbol
    std::string input = R"(
A ALIAS C'SOMESTRING'
)";

    analyzer a(input);
    a.analyze();

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

    EXPECT_TRUE(a.diags().empty());
}

TEST(asm_instr_processing, ALIAS_with_opsyn)
{
    std::string input = R"(
X OPSYN ALIAS
L X     C'SOMESTRING'
  EXTRN L
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(asm_instr_processing, CXD)
{
    std::string input = R"(
    DS   C
C   CXD
L   EQU  *-C
&T  SETC T'C
&O  SETC O'C
&L  SETA L'C
&S  SETA S'C
&I  SETA I'C
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W011", "W011" })); // S, I attributes

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "L"), 4);
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "T"), "A");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "O"), "O");
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "L"), 4);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "S"), 0);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "I"), 0);

    auto C = get_symbol_reloc(a.hlasm_ctx(), "C");
    ASSERT_TRUE(C);
    EXPECT_EQ(C->offset(), 4);
}

TEST(asm_instr_processing, CXD_lookahead)
{
    std::string input = R"(
&T  SETC T'C
&O  SETC O'C
&L  SETA L'C
&S  SETA S'C
&I  SETA I'C
C   CXD
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W011", "W011" })); // S, I attributes

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "T"), "A");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "O"), "O");
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "L"), 4);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "S"), 0);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "I"), 0);
}

TEST(asm_instr_processing, DXD_lookahead)
{
    std::string input = R"(
&T  SETC T'D
&O  SETC O'D
&L  SETA L'D
&S  SETA S'D
&I  SETA I'D
D   DXD  F
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W011", "W011" })); // S, I attributes

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "T"), "J");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "O"), "O");
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "L"), 1);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "S"), 0);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "I"), 0);
}

TEST(asm_instr_processing, DXD_name_conflict_1)
{
    std::string input = R"(
D   DXD  F
D   DSECT
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E031" }));
}

TEST(asm_instr_processing, DXD_name_conflict_2)
{
    std::string input = R"(
D   DSECT
D   DXD  F
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E031" }));
}

TEST(asm_instr_processing, valid_q_ref)
{
    std::string input = R"(
D   DXD  F
    DC   Q(D)
    LARL 0,=Q(D)
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(asm_instr_processing, invalid_q_ref)
{
    std::string input = R"(
D   DXD  F
    DC   Q(D+1)
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D030" }));
}

TEST(asm_instr_processing, invalid_q_ref_literal)
{
    std::string input = R"(
D   DXD  F
    LARL 0,=Q(D+1)
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D030" }));
}

TEST(asm_instr_processing, nogoff_valid_q_nominals)
{
    std::string input = R"(
S        CSECT
         LARL  0,=Q(D,X)
         DC    Q(D)
         DC    Q(X)
D        DSECT
X        DXD   X
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(asm_instr_processing, goff_valid_q_nominals)
{
    std::string input = R"(
S        CSECT
C        CATTR PART(P)
D        DSECT
         EXTRN E
X        DXD   X
         WXTRN W
S        CSECT
         LARL  0,=Q(D,E,P,X,W,S)
         DC    Q(D)
         DC    Q(E)
         DC    Q(P)
         DC    Q(X)
         DC    Q(W)
         DC    Q(S)
)";

    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(asm_instr_processing, invalid_q_nominals)
{
    std::string input = R"(
S        CSECT
         EXTRN E
X        DS    X
         WXTRN W
         LARL  0,=Q(E,X,W,S)
         DC    Q(E)
         DC    Q(X)
         DC    Q(W)
         DC    Q(S)
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D035", "D035", "D035", "D035", "D035", "D035", "D035", "D035" }));
}

TEST(asm_instr_processing, dxd_without_name)
{
    std::string input = R"(
    DXD F
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E053" }));
}

TEST(asm_instr_processing, TITLE_text_label)
{
    std::string input = R"(
0a0 TITLE 'aaa'
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(a.hlasm_ctx().get_title_name(), "0a0");
}

TEST(asm_instr_processing, TITLE_multiple_labels)
{
    std::string input = R"(
000 TITLE 'aaa'
0b0 TITLE 'aaa'
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W016" }));

    EXPECT_EQ(a.hlasm_ctx().get_title_name(), "000");
}

TEST(asm_instr_processing, TITLE_sequence_does_not_count)
{
    std::string input = R"(
0a0 TITLE 'aaa'
.A  TITLE 'aaa'
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(a.hlasm_ctx().get_title_name(), "0a0");
}
