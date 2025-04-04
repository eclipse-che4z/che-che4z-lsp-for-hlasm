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
#include "context/instruction.h"
#include "processing/op_code.h"

TEST(mach_instr_processing, reloc_imm_expected)
{
    std::string input(
        R"( 
          EXRL 1,0(1)
)");
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "M113" }));
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

    EXPECT_TRUE(matches_message_codes(a.diags(), { "M113" }));
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

    EXPECT_TRUE(a.diags().empty());
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

    EXPECT_TRUE(matches_message_codes(a.diags(), { "ME003" }));
}
TEST(mach_instr_processing, vec_reg_expected)
{
    std::string input(
        R"( 
  VLC 12(2,2),4,2
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M114" }));
}
TEST(mach_instr_processing, reloc_symbol_expected)
{
    std::string input(
        R"(
 EXRL 1,12
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "D032" }));
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
    EXPECT_TRUE(a.diags().empty());
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
    EXPECT_TRUE(a.diags().empty());
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
    EXPECT_TRUE(matches_message_codes(a.diags(), { "D032" }));
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
    EXPECT_TRUE(a.diags().empty());
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
    EXPECT_TRUE(matches_message_codes(a.diags(), { "D032" }));
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
    EXPECT_TRUE(matches_message_codes(a.diags(), { "ME003" }));
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
    EXPECT_TRUE(matches_message_codes(a.diags(), { "M120" }));
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
    EXPECT_TRUE(a.diags().empty());
}

TEST(mach_instr_processing, rel_addr_bitmask)
{
    for (const auto& [instr, expected] : std::initializer_list<std::pair<std::string, int>> {
             { "LARL", 0x40 },
             { "LA", 0x00 },
             { "CLIJ", 0x10 },
             { "BPRP", 0x60 },
         })
    {
        EXPECT_EQ(context::instruction::get_machine_instructions(instr).reladdr_mask().mask(), expected) << instr;
    }

    for (const auto& [instr, expected] : std::initializer_list<std::pair<std::string, int>> {
             { "CLIJE", 0x20 },
             { "BNE", 0x00 },
             { "JNE", 0x80 },
         })
    {
        EXPECT_EQ(context::instruction::get_mnemonic_codes(instr).reladdr_mask().mask(), expected) << instr;
    }
}

TEST(mach_instr_processing, instr_size)
{
    for (const auto& [instr, expected] : std::initializer_list<std::pair<std::string, int>> {
             { "LARL", 6 },
             { "LA", 4 },
             { "CLIJ", 6 },
             { "BR", 2 },
             { "DC", 1 },
         })
    {
        EXPECT_EQ(processing::processing_status_cache_key::generate_loctr_len(instr), expected) << instr;
    }
}

TEST(mach_instr_processing, loctr_len_reference)
{
    std::string input = R"(
    LARL  0,A-1+L'*/6
A   DS    0H
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(mach_instr_processing, relimm_qualified)
{
    std::string input = R"(
Q     USING *,12
      J     Q.LABEL
LABEL DS    0H
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(mach_instr_processing, relimm_qualified_bad)
{
    std::string input = R"(
      J     Q.LABEL
LABEL DS    0H
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "ME005" }));
}

TEST(mach_instr_processing, literals_with_index_register)
{
    std::string input(R"(
    USING *,10
    L     0,=A(0)(1)
)");

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(mach_instr_processing, mach_expr_limits)
{
    std::string input(R"(
    LARL   0,-2147483648+*+2147483647+1
    LARL   0,*+-2147483648+2147483647+1
    LARL   0,*+++++2
    LARL   0,*-----2
    LARL   0,*-+-+-2
)");

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(mach_instr_processing, mach_expr_out_of_bounds)
{
    std::string input(R"(
    LARL   0,*-2147483648+2147483647+1
)");

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "CE007" }));
}

TEST(mach_instr_processing, mach_expr_leading_zeros_ok)

{
    std::string input = R"(
A   MVI  0,X'0000000000'
    LARL 0,A+00000000000000
    LARL 0,A+X'00000000000000'
    LARL 0,A+X'00000000000000000000000000000000000000000000000000'
    LARL 0,A+-0000000000
    LARL 0,A+B'0000000000000000000000000000000000000000000000000000000'
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(mach_instr_processing, mach_expr_leading_zeros_nok)

{
    std::string input = R"(
A   LARL 0,A+-00000000000
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "CE007" }));
}

TEST(mach_instr_processing, check_combined_lookup)
{
    for (const auto& orig_mi : context::instruction::all_machine_instructions())
    {
        const auto [mi, mn] = context::instruction::find_machine_instruction_or_mnemonic(orig_mi.name());

        EXPECT_EQ(mi, &orig_mi);
        EXPECT_FALSE(mn);
    }

    for (const auto& orig_mn : context::instruction::all_mnemonic_codes())
    {
        const auto [mi, mn] = context::instruction::find_machine_instruction_or_mnemonic(orig_mn.name());

        EXPECT_EQ(mi, orig_mn.instruction());
        EXPECT_EQ(mn, &orig_mn);
    }
}

TEST(mach_instr_processing, validate_even_odd)

{
    std::string input = R"(
    MVCL 0,3
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "M120" }));
}

TEST(mach_instr_processing, validate_minimal)

{
    std::string input = R"(
    SORTL 0,2
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "M120" }));
}
