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

TEST(diagnostics, second_par_omitted)
{
    std::string input(
        R"( 
 C 1,1(2,)
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M004");
}

TEST(diagnostics, displ_unsigned_size)
{
    std::string input(
        R"( 
  AL 1,12331(2,2)
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M130");
}

TEST(diagnostics, displ_signed_size)
{
    std::string input(
        R"( 
  ALG 1,2222(2,2) 
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(diagnostics, displ_signed_err)
{
    std::string input(
        R"( 
  ALG 1,524288(2,2) 
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M130");
}

TEST(diagnostics, db_incorrect_format)
{
    std::string input(
        R"( 
 NI 1(,2),2
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M104");
}

TEST(diagnostics, db_not_corresponding)
{
    std::string input(
        R"( 
 CHHSI 1(222),2
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M131");
}

TEST(diagnostics, dxb_second_par_incorrect)
{
    std::string input(
        R"( 
  CGH 2,2(2,22)
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M131");
}

TEST(diagnostics, length_not_corresponding)
{
    std::string input(
        R"( 
 MVC 1(2222,2),2(2) 
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M132");
}

TEST(diagnostics, dis_reg_not_corresponding)
{
    std::string input(
        R"( 
 VLLEZ 1,2(18,2),3
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M135");
}

TEST(diagnostics, reg_not_corresponding)
{
    std::string input(
        R"( 
 MVCP 1(22,2),2(2),3 
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M133");
}

TEST(diagnostics, vec_reg_not_corresponding)
{
    std::string input(
        R"( 
 VGEF 1,1(17,1),1 
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M134");
}

TEST(diagnostics, displ_as_simple_unsigned)
{
    std::string input(
        R"( 
  VLEF 1,11111,3  
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M130");
}

TEST(diagnostics, displ_as_simple_signed_correct)
{
    std::string input(
        R"( 
 BCTG 1,11111 
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(diagnostics, displ_as_simple_signed_err)
{
    std::string input(
        R"( 
 BCTG 1,524288 
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M130");
}

TEST(diagnostics, immS_out_of_range)
{
    std::string input(
        R"( 
 VFTCI 1,2,3323,4,4
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M122");
}

TEST(diagnostics, reloc_ImmS_out_of_range)
{
    std::string input(
        R"( 
 BRAS 1,DISP
LEN123   DS CL(64444)
DISP     MVC 0(1),1  
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M123");
}

TEST(diagnostics, mask_out_of_range)
{
    std::string input(
        R"( 
 CGIT 1,2,31 
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M121");
}

TEST(diagnostics, immU_out_of_range)
{
    std::string input(
        R"( 
 MVI 1,1000 
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M122");
}

TEST(diagnostics, vecReg_out_of_range)
{
    std::string input(
        R"( 
 VBPERM 1,99,3
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M124");
}

TEST(diagnostics, mask_expected)
{
    std::string input(
        R"( 
  VLC 12,4,2(1,6)
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M111");
}

TEST(diagnostics, imm_expected)
{
    std::string input(
        R"( 
 IILH 1,1(8,9)
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().at(0).code, "M112");
}
TEST(diagnostics, relocImm_expected)
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
TEST(diagnostics, vecReg_expected)
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