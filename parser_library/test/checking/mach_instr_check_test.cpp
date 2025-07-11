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

using namespace hlasm_plugin::parser_library;

TEST(machine_instr_check_test, BALR_test)
{
    analyzer a(" BALR 14,15");
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(machine_instr_check_test, BAL_test)
{
    analyzer a(" BAL 2,1(1,111)");
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "M131" }));
}

TEST(machine_instr_check_test, LR_test)
{
    analyzer a(" LR 0,2");
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(machine_instr_check_test, MVCL_test)
{
    analyzer a(" MVCL 4,0");
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(machine_instr_check_test, XR_test)
{
    analyzer a(" XR 15,15");
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(machine_instr_check_test, CLC_test)
{
    analyzer a(R"(
A   DS  XL2
    USING A-1,4
    CLC A,2
)");
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(machine_instr_check_test, a_test_invalid)
{
    analyzer a(R"(
    A   4,1(1,B)
B   DS  F
)");
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "ME010" }));
}

TEST(machine_instr_check_test, duplicate_base_specified)
{
    analyzer a(R"(
    USING *,1
    A     0,B(,1)
B   DS    F
)");
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "ME011" }));
}

TEST(machine_instr_check_test, second_par_omitted)
{
    std::string input(
        R"( 
 C 1,1(2,)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M004" }));
}

TEST(machine_instr_check_test, displ_unsigned_size)
{
    std::string input(
        R"( 
  AL 1,12331(2,2)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M130" }));
}

TEST(machine_instr_check_test, displ_signed_size)
{
    std::string input(
        R"( 
  ALG 1,2222(2,2) 
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(a.diags().empty());
}

TEST(machine_instr_check_test, displ_signed_err)
{
    std::string input(
        R"( 
  ALG 1,524288(2,2) 
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M130" }));
}

TEST(machine_instr_check_test, db_incorrect_format)
{
    std::string input(
        R"( 
 NI 1(,2),2
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M104" }));
}

TEST(machine_instr_check_test, db_not_corresponding)
{
    std::string input(
        R"( 
 CHHSI 1(222),2
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M131" }));
}

TEST(machine_instr_check_test, dxb_second_par_incorrect)
{
    std::string input(
        R"( 
  CGH 2,2(2,22)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M131" }));
}

TEST(machine_instr_check_test, length_not_corresponding)
{
    std::string input(
        R"( 
 MVC 1(2222,2),2(2) 
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M132" }));
}

TEST(machine_instr_check_test, dis_reg_not_corresponding)
{
    std::string input(
        R"( 
 VLLEZ 1,2(18,2),3
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M135" }));
}

TEST(machine_instr_check_test, reg_not_corresponding)
{
    std::string input(
        R"( 
 MVCP 1(22,2),2(2),3 
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M133" }));
}

TEST(machine_instr_check_test, vec_reg_not_corresponding)
{
    std::string input(
        R"( 
 VGEF 1,1(32,1),1
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M134" }));
}

TEST(machine_instr_check_test, displ_as_simple_unsigned)
{
    std::string input(
        R"( 
  VLEF 1,11111,3  
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M130" }));
}

TEST(machine_instr_check_test, displ_as_simple_signed_correct)
{
    std::string input(
        R"( 
 BCTG 1,11111 
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(a.diags().empty());
}

TEST(machine_instr_check_test, displ_as_simple_signed_err)
{
    std::string input(
        R"( 
 BCTG 1,524288 
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M130" }));
}

TEST(machine_instr_check_test, immS_out_of_range)
{
    std::string input(
        R"( 
 VLEIF 1,32768,0
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M137" }));
}

TEST(machine_instr_check_test, reloc_ImmS_out_of_range)
{
    std::string input(
        R"(
         BRAS 1,DISP
         DS   CL(65532)
DISP     MVC  0(1),1
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M123" }));
}

TEST(machine_instr_check_test, reloc_ImmS_in_range)
{
    std::string input(
        R"(
         BRAS 1,DISP
         DS   CL(65530)
DISP     MVC 0(1),1
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(a.diags().empty());
}

TEST(machine_instr_check_test, mask_out_of_range)
{
    std::string input(
        R"( 
 CGIT 1,2,31 
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M121" }));
}

TEST(machine_instr_check_test, immU_out_of_range)
{
    std::string input(
        R"( 
 MVI 1,1000 
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M137" }));
}

TEST(machine_instr_check_test, vecReg_out_of_range)
{
    std::string input(
        R"( 
 VBPERM 1,99,3
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M124" }));
}

TEST(machine_instr_check_test, mask_expected)
{
    std::string input(
        R"( 
  VLC 12,4,2(1,6)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M111" }));
}

TEST(machine_instr_check_test, imm_expected)
{
    std::string input(
        R"( 
 IILH 1,1(8,9)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M112" }));
}

TEST(machine_instr_check_test, vec_reg_limits)
{
    std::string input(
        R"(
 VAC 31,31,31,31,15
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(a.diags().empty());
}

TEST(machine_instr_check_test, mnemonics_with_optional_args)
{
    std::string input(
        R"(
        VFAEZHS 0,0
        VFAEZHS 0,0,0
        VFAEZHS 0,0,0,0
        VFAEZHS 0,0,0,0,0
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M001", "M001" }));
}

TEST(machine_instr_check_test, length_limits)
{
    std::string input(
        R"(
        CLC 0(256,1),0(2)
        CLC 0(1,1),0(2)
        CLC 0(0,1),0(2)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(a.diags().empty());
}

TEST(machine_instr_check_test, mnemonic_insert_middle)
{
    std::string input(
        R"(
        CIBE 0,0,0(0)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(a.diags().empty());
}
