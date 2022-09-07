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
#include "../mock_parse_lib_provider.h"

// tests for OPSYN instruction

TEST(OPSYN, simple)
{
    std::string input(R"(
OP1 OPSYN LR
OP2 OPSYN OP1
  OP2 1,1
  OP1 1,1
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(OPSYN, undefined_operand)
{
    std::string input(R"(
OP2 OPSYN OP1
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(OPSYN, undefined_name)
{
    std::string input(R"(
OP2 OPSYN
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(OPSYN, missing_name)
{
    std::string input(R"(
&VAR SETC ''
&VAR OPSYN &VAR
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(OPSYN, incorrect_operands)
{
    std::string input(R"(
LR OPSYN A,B
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(OPSYN, delete_opcode)
{
    std::string input(R"(
LR OPSYN 
  LR 1,1
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(OPSYN, preserve_opcode)
{
    std::string input(R"(
OP1 OPSYN LR
LR OPSYN 
  OP1 1,1
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(OPSYN, non_CA_instruction_before_macro_def)
{
    std::string input(R"(
OPSYN_THIS OPSYN SAM31 
 MACRO
 M
 OPSYN_THIS
 MEND
 M
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(OPSYN, non_CA_instruction_before_macro_call)
{
    std::string input(R"(
 MACRO
 M
 OPSYN_THIS
 MEND
OPSYN_THIS OPSYN SAM31 
 M
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(OPSYN, non_CA_instruction_after_macro_call)
{
    std::string input(R"(
 MACRO
 M
 OPSYN_THIS
 MEND
 M
OPSYN_THIS OPSYN SAM31 
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(OPSYN, CA_instruction)
{
    std::string input(R"(
 MACRO
 M1
 AGO .A
.A MEND
AGO OPSYN ACTR
 MACRO
 M2
 AGO 13
 MEND
 M1
 M2
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(OPSYN, macro_definition)
{
    std::string input(R"(
MACROX OPSYN MACRO
MENDX OPSYN MEND

 MACROX
 M
 LR 1,1
 MENDX

 M
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(OPSYN, macro_redefinition)
{
    std::string input(R"(
 MACRO
 M
 LR 1,1
 MEND

M_ OPSYN M

 MACRO
 M
 M_
 MEND

 M
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(OPSYN, macro_mach_redefinition)
{
    std::string input(R"(
LR_ OPSYN LR

 MACRO
 LR &VAR
 LR_ 1,&VAR
 MEND

 LR 1
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(OPSYN, late_macro_definition)
{
    std::string input(R"(
LRX OPSYN LR
 MACRO
 LR
 LR 1,1
 MEND
 
 LRX  

)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(OPSYN, removed_machine_instruction)
{
    std::string input(R"(
LR OPSYN
X OPSYN LR
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(OPSYN, macro_after_delete)
{
    std::string input(R"(
LR OPSYN
   LR
)");

    std::string LIB =
        R"( MACRO
 LR
 MEND)";
    mock_parse_lib_provider mock { { "LR", LIB } };
    analyzer a(input, analyzer_options { &mock });
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(OPSYN, tolerate_comma_argument)
{
    std::string input(R"(
LR OPSYN ,
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());
}

TEST(OPSYN, full_circle)
{
    std::string input(R"(
         GBLA  &CNT
XDS      OPSYN DS
DS       OPSYN ,

         MACRO ,
&Label   DS
         GBLA  &CNT
&CNT     SETA  &CNT+1
         MEND  ,

A        DS    C

DS       OPSYN XDS
XDS      OPSYN ,

B        DS    C
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "CNT"), 1);
}

TEST(OPSYN, reladdr_caching)
{
    std::string input(R"(
      MACRO
      MAC
      INSTR 0,A
      MEND

INSTR OPSYN LA
      USING *,12
      MAC
      DROP  ,
INSTR OPSYN LARL
      MAC
A     DS    0H
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
}

TEST(OPSYN, macro_replacement_1)
{
    std::string input = R"(
BCT  OPSYN BCT
BCT_ OPSYN BCT

     MACRO
     BCT   &A,&B
     BRCT  &A,&B
     MEND

     BCT   0,*
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
}

TEST(OPSYN, macro_replacement_2)
{
    std::string input = R"(
JCT  OPSYN JCT
JCT_ OPSYN JCT

     MACRO
     JCT   &A,&B
     BCT   &A,&B
     MEND
     JCT   0,*
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "ME007" }));
}

TEST(OPSYN, macro_replacement_3)
{
    std::string input = R"(
     MACRO
     BCT   &A,&B
     BRCT  &A,&B
     MEND

     BCT   0,*
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
}

TEST(OPSYN, macro_replacement_4)
{
    std::string input = R"(
     MACRO
     JCT   &A,&B
     BCT   &A,&B
     MEND
     JCT   0,*
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "ME007" }));
}

TEST(OPSYN, SETA_SETB_expressions)
{
    std::string input = R"(
           MACRO
           MAC &IDX
           GBLA &VAR(2)
&VAR(&IDX) SETA (10 AND 2)
           MEND

           GBLA &VAR(2)
           MAC 1
SETA       OPSYN SETB
           MAC 2
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_vector<A_t>(a.hlasm_ctx(), "VAR"), std::vector<A_t>(2, 2));
}

TEST(OPSYN, SETA_SETB_expressions_ainsert)
{
    std::string input = R"(
           MACRO
           MAC &IDX
           AINSERT '&&VAR(&IDX) SETA (10 AND 2)',BACK
           MEND

           GBLA &VAR(2)
           MAC 1
SETA       OPSYN SETB
           MAC 2
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E013" }));
}