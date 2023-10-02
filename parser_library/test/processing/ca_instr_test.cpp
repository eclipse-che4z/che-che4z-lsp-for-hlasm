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

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library::semantics;

// Tests for variable substitution for model statements concatenation of multiple substitutions CA instructions
// Some inputs are deliberately not written in uppercase to also verify case-insensitivity

TEST(var_subs, gbl_instr_only)
{
    std::string input("   gbla var");
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "var"), 0);
}

TEST(var_subs, lcl_instr_only)
{
    std::string input("   lcla var");
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "var"), 0);
}

TEST(var_subs, gbl_instr_more)
{
    std::string input("   gbla var,var2,var3");
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "var"), 0);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "var2"), 0);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "var3"), 0);
}

TEST(var_subs, lcl_instr_more)
{
    std::string input("   lcla var,var2,var3");
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "var"), 0);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "var2"), 0);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "var3"), 0);
}

TEST(var_subs, big_arrays)
{
    std::string input = R"(
    lclc &larr(100000000)
    gblc &garr(100000000)
)";
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_vector<C_t>(a.hlasm_ctx(), "larr")->size(), 0);
    EXPECT_EQ(get_var_vector<C_t>(a.hlasm_ctx(), "garr")->size(), 0);
}

TEST(var_subs, set_to_var)
{
    std::string input("&var seta 3");
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "var"), 3);
}

TEST(var_subs, set_to_var_idx)
{
    std::string input("&var(2) seta 3");
    analyzer a(input);
    a.analyze();

    std::unordered_map<size_t, A_t> expected = { { 1, 3 } };
    EXPECT_EQ(get_var_vector_map<A_t>(a.hlasm_ctx(), "var"), expected);
}

TEST(var_subs, set_to_var_idx_many)
{
    std::string input("&var(2) seta 3,4,5");
    analyzer a(input);
    a.analyze();

    std::unordered_map<size_t, A_t> expected = { { 1, 3 }, { 2, 4 }, { 3, 5 } };
    EXPECT_EQ(get_var_vector_map<A_t>(a.hlasm_ctx(), "var"), expected);
}

TEST(var_subs, var_sym_reset)
{
    std::string input = R"(
&var setc 'avc'   
&var setc 'XXX'
)";
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "var"), "XXX");
}

TEST(var_subs, created_set_sym)
{
    std::string input = R"(
&var setc 'avc'   
&var2 setb 0  
&(ab&var.cd&var2) seta 11
)";
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "abavccd0"), 11);
}

TEST(var_subs, instruction_substitution_space_at_end)
{
    std::string input = R"(
&var setc 'lr '   
     &var 1,1
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(var_subs, instruction_substitution_space_in_middle)
{
    std::string input = R"(
&var setc 'lr 1,1'   
     &var 
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E075" }));
}

TEST(var_concatenation, concatenated_string_dot_last)
{
    std::string input = R"(
&var setc 'avc'   
&var2 setc '&var.'
)";
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "var2"), "avc");
}

TEST(var_concatenation, concatenated_string_dot)
{
    std::string input = R"(
&var setc 'avc'   
&var2 setc '&var.-get'
)";
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "var2"), "avc-get");
}

TEST(var_concatenation, concatenated_string_double_dot)
{
    std::string input = R"(
&var setc 'avc'   
&var2 setc '&var..'
)";
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "var2"), "avc.");
}

TEST(AGO, extended)
{
    std::string input(R"(
 AGO (2).a,.b,.c
.a anop   
&var1 setb 0
.b anop
&var2 setb 0
.c anop
&var3 setb 0
)");
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "var1"), std::nullopt);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "var2"), false);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "var3"), false);
}

TEST(AGO, extended_fail)
{
    std::string input(R"(
 AGO (8).a,.b,.c
.a anop   
&var1 setb 0
.b anop
&var2 setb 0
.c anop
&var3 setb 0
)");
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "var1"), false);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "var2"), false);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "var3"), false);
}

TEST(AIF, extended)
{
    std::string input(R"(
 AIF (0).a,(1).b,(1).c
.a anop   
&var1 setb 0
.b anop
&var2 setb 0
.c anop
&var3 setb 0
)");
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "var1"), std::nullopt);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "var2"), false);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "var3"), false);
}

TEST(AIF, missing_comma)
{
    std::string input(R"(
         AIF ('&SYSPARM' EQ '').A('&SYSPARM' EQ '').A
.A       ANOP
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002" }));
}

TEST(AIF, extended_fail)
{
    std::string input(R"(
 AIF (0).a,(0).b,(0).c
.a anop   
&var1 setb 0
.b anop
&var2 setb 0
.c anop
&var3 setb 0
)");
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "var1"), false);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "var2"), false);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "var3"), false);
}

TEST(ACTR, exceeded)
{
    std::string input(R"(
.A ANOP
 LR 1,1
 AGO .A
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E056" }));
}

TEST(ACTR, infinite_ACTR)
{
    std::string input(R"(
.A ANOP
 ACTR 1024
 LR 1,1
 AGO .A
)");
    analyzer a(input, analyzer_options(asm_option { .statement_count_limit = 10000 }));
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W063", "E077" }));
}

TEST(ACTR, negative)
{
    std::string input(R"(
&A SETA -2147483648
   ACTR &A
   AGO .A
.A ANOP
&B SETA 1
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E056" }));
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "B"), std::nullopt);
}

TEST(MHELP, SYSNDX_limit)
{
    std::string input = R"(
         GBLC &LASTNDX
         MACRO
         MAC
         GBLC &LASTNDX
&LASTNDX SETC '&SYSNDX'
         MEND

         MHELP 256
&I       SETA  0
.NEXT    AIF   (&I GT 256).DONE
&I       SETA  &I+1
         MAC
         AGO   .NEXT
.DONE    ANOP  ,
 )";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E072" }));
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "LASTNDX"), "0256");
}

TEST(MHELP, invalid_operands)
{
    std::string input = R"(
 MHELP
 MHELP 1,1
 MHELP ,
 MHELP ABC
 MHELP (1).ABC
ABC EQU 1
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E021", "E020", "E020", "CE012", "CE004", "CE012" }));
}

TEST(MHELP, valid_operands)
{
    std::string input = R"(
ABC EQU 1
&VAR SETA 1
 MHELP 1
 MHELP X'1'
 MHELP B'1'
 MHELP ABC
 MHELP ABC+ABC
 MHELP ABC*5
 MHELP &VAR+1
 MHELP &VAR*&VAR
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
}

TEST(SET, conversions_valid)
{
    std::string input(R"(
&A SETA 1
&B SETB 0
&C SETC '2'

&A SETA &B
&A SETA &C

&C SETC '&A'
&C SETC '&B'
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(SET, conversions_invalid)
{
    std::string input(R"(
&A SETA 1
&B SETB 0
&C SETC '2'

&A SETA '1'
&B SETB ('1')

&C SETC &A
&C SETC &B
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "CE004", "CE004", "CE017", "CE017" }));
}

TEST(CA_instructions, undefined_relocatable)
{
    std::string input(R"(
A EQU B
L1 LR 1,1
L2 LR 1,1

&V1 SETA L2-L1
&V2 SETA A

B EQU 1
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "CE012", "CE012", "CE012" }));
}

TEST(var_subs, defined_by_self_ref)
{
    std::string input("&VAR(N'&VAR+1) SETA N'&VAR+1");
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_vector<A_t>(a.hlasm_ctx(), "VAR"), std::vector<A_t> { 2 });
}

TEST(SET, empty_args_in_arrays)
{
    std::string input = R"(
        MACRO
        MAC
        GBLA &RES
&A(1)   SETC 'A',,,,'B'
&RES    SETA N'&A
        MEND

        GBLA &RES
        MAC
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "RES"), 5);
}

TEST(SET, single_comma)
{
    std::string input = R"(
        MACRO
        MAC
        GBLA &RES
&A(1)   SETC ,
&RES    SETA N'&A
        MEND

        GBLA &RES
        MAC
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "RES"), 2);
}

TEST(SET, empty_args_in_arrays_no_overwrite)
{
    std::string input = R"(
        MACRO
        MAC
        GBLC &A(5)
&A(1)   SETC 'A',,,,'B'
        MEND

        GBLC &A(5)
&A(1)   SETC 'A','B','C','D','E'
        MAC
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    std::vector<C_t> expected { "A", "B", "C", "D", "B" };
    EXPECT_EQ(get_var_vector<C_t>(a.hlasm_ctx(), "A"), expected);
}


TEST(var_subs, expr_list_as_index)
{
    std::string input = R"(
&var(1) seta &var((1 and 1))
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_vector<A_t>(a.hlasm_ctx(), "var"), std::vector<A_t>({ 0 }));
}

TEST(SET, missing_var_name)
{
    std::string input = "&A SETA --&";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0008", "E010" }));
}
