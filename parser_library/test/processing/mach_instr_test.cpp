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
TEST(mach_instr_processing, reloc_imm_expected)
{
    std::string input(
        R"( 
          EXRL 1,0(1)
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M113");
}

TEST(mach_instr_processing, invalid_reloc_operand)
{
    std::string input(
        R"(
SIZE EQU 5
 EXRL 1,LENGTH+LENGTH
LENGTH DS CL(SIZE)
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M113");
}

TEST(mach_instr_processing, valid_reloc_operand)
{
    std::string input(
        R"(
SIZE EQU 5
 EXRL 1,LENGTH+4
LENGTH DS CL(SIZE)
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(mach_instr_processing, reloc_operand_halfword_o_error)
{
    std::string input(
        R"(
 EXRL 1,LEN120
LENGTH DS CL(5)
LEN120 DS CL1
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "ME003");
}
TEST(mach_instr_processing, vec_reg_expected)
{
    std::string input(
        R"( 
  VLC 12(2,2),4,2
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M114");
}
TEST(mach_instr_processing, reloc_symbol_expected)
{
    std::string input(
        R"(
 EXRL 1,12
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "D031");
}
TEST(mach_instr_processing, setc_variable_mnemonic_reloc_operand)
{
    std::string input(
        R"(
&RRR SETC 'NAME'
 J &RRR
&RRR DS 0H
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)0);
}
TEST(mach_instr_processing, setc_variable_reloc_operand)
{
    std::string input(
        R"(
TEST CSECT
&OPS SETC '0,TEST'
     LARL &OPS
     END TEST
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)0);
}
TEST(mach_instr_processing, setc_variable_reloc_symbol_expected_warn)
{
    std::string input(
        R"(
TEST CSECT
&OPS SETC '0,1'
     LARL &OPS
     END TEST
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "D031");
}
TEST(mach_instr_processing, reloc_parsed_in_macro_valid)
{
    std::string input(
        R"(
        MACRO
        CALLRIOPERAND
LABEL   BRAS  0,*+12
        MEND
        CALLRIOPERAND
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)0);
}
TEST(mach_instr_processing, reloc_parsed_in_macro_with_immValue)
{
    std::string input(
        R"(
       MACRO
       CALLRIOPERAND
LABEL  BRAS  0,12
       MEND
       CALLRIOPERAND
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "D031");
}
TEST(mach_instr_processing, reloc_parsed_in_macro_alignment_error)
{
    std::string input(
        R"(
    MACRO
    CALLRIOPERAND
    EXRL 1,LEN120
LENGTH DS CL(5)
LEN120 DS CL1
        MEND
    CALLRIOPERAND
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "ME003");
}

TEST(mach_instr_processing, mach_instr_aligned_assign_to_loctr)
{
    std::string input(
        R"(
SYM  DS   CL15
     LR 1,*-SYM
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), 1U);
    ASSERT_EQ(a.diags()[0].code, "M120");
}

TEST(mach_instr_processing, mach_instr_aligned_assign_to_loctr_reloc_imm)
{
    std::string input(
        R"(
SYM  DS   CL1
     EXRL 1,SYM
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), 0U);
}
