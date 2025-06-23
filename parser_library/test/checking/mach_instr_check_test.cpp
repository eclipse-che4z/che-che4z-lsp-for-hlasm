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
#include "checking/diagnostic_collector.h"
#include "checking/instr_operand.h"
#include "checking/instruction_checker.h"
#include "instructions/instruction.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::checking;

namespace {
const auto& get_mi(std::string_view arg)
{
    return hlasm_plugin::parser_library::instructions::get_machine_instructions(arg);
}
one_operand op_val_0 { 0 };
one_operand op_val_1 { 1 };
one_operand op_val_2 { 2 };
one_operand op_val_3 { 3 };
one_operand op_val_4 { 4 };
one_operand op_val_11 { 11 };
one_operand op_val_12 { 12 };
one_operand op_val_13 { 13 };
one_operand op_val_14 { 14 };
one_operand op_val_15 { 15 };
} // namespace

namespace hlasm_plugin::parser_library::processing {
bool check(const instructions::machine_instruction& mi,
    std::string_view name_of_instruction,
    std::span<const checking::machine_operand* const> to_check,
    const range& stmt_range,
    const diagnostic_collector& add_diagnostic);
}

TEST(machine_instr_check_test, BALR_test)
{
    diagnostic_collector collector;
    std::string balr_name = "BALR";
    std::vector<const checking::machine_operand*> operands { &op_val_14, &op_val_15 };
    EXPECT_TRUE(check(get_mi(balr_name), balr_name, operands, range(), collector));
}

TEST(machine_instr_check_test, BAL_test)
{
    diagnostic_collector collector;
    std::string bal_name = "BAL";
    address_operand valid_op = address_operand(address_state::RES_VALID, 1, 1, 111);
    std::vector<const checking::machine_operand*> operands { &op_val_2, &valid_op };
    EXPECT_TRUE(check(get_mi(bal_name), bal_name, operands, range(), collector));
}

TEST(machine_instr_check_test, LR_test)
{
    diagnostic_collector collector;
    std::string lr_name = "LR";
    std::vector<const checking::machine_operand*> operands { &op_val_0, &op_val_2 };
    EXPECT_TRUE(check(get_mi(lr_name), lr_name, operands, range(), collector));
}

TEST(machine_instr_check_test, MVCL_test)
{
    diagnostic_collector collector;
    std::string mvcl_name = "MVCL";
    std::vector<const checking::machine_operand*> operands { &op_val_4, &op_val_0 };
    EXPECT_TRUE(check(get_mi(mvcl_name), mvcl_name, operands, range(), collector));
}

TEST(machine_instr_check_test, XR_test)
{
    diagnostic_collector collector;
    std::string xr_name = "XR";
    std::vector<const checking::machine_operand*> operands { &op_val_15, &op_val_15 };
    EXPECT_TRUE(check(get_mi(xr_name), xr_name, operands, range(), collector));
}
TEST(machine_instr_check_test, CLC_test)
{
    diagnostic_collector collector;
    std::string clc_name = "CLC";
    address_operand length_one = address_operand(address_state::UNRES, 1, 2, 4);
    std::vector<const checking::machine_operand*> operands { &length_one, &op_val_2 };
    EXPECT_TRUE(check(get_mi(clc_name), clc_name, operands, range(), collector));
}
TEST(machine_instr_check_test, a_test_invalid)
{
    diagnostic_collector collector;
    std::string a_name = "A";

    address_operand invalid_op = address_operand(address_state::RES_INVALID, 1, 1, 1);
    std::vector<const checking::machine_operand*> operands { &op_val_4, &invalid_op };
    EXPECT_FALSE(check(get_mi(a_name), a_name, operands, range(), collector));
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
