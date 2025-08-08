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
#include "context/hlasm_context.h"
#include "instructions/instruction.h"
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
    using instructions::reladdr_transform_mask;
    for (const auto& [instr, expected] : std::initializer_list<std::pair<std::string, reladdr_transform_mask>> {
             { "LARL", (reladdr_transform_mask)0x40 },
             { "LA", (reladdr_transform_mask)0x00 },
             { "CLIJ", (reladdr_transform_mask)0x10 },
             { "BPRP", (reladdr_transform_mask)0x60 },
         })
    {
        EXPECT_EQ(instructions::get_machine_instructions(instr).reladdr_mask(), expected) << instr;
    }

    for (const auto& [instr, expected] : std::initializer_list<std::pair<std::string, reladdr_transform_mask>> {
             { "CLIJE", (reladdr_transform_mask)0x20 },
             { "BNE", (reladdr_transform_mask)0x00 },
             { "JNE", (reladdr_transform_mask)0x80 },
         })
    {
        EXPECT_EQ(instructions::get_mnemonic_codes(instr).reladdr_mask(), expected) << instr;
    }
}

TEST(mach_instr_processing, instr_size)
{
    for (const auto& [instr, expected] : std::initializer_list<std::pair<op_code, int>> {
             { op_code(id_index("LARL"), &instructions::get_machine_instructions("LARL")), 6 },
             { op_code(id_index("LA"), &instructions::get_machine_instructions("LA")), 4 },
             { op_code(id_index("CLIJ"), &instructions::get_machine_instructions("CLIJ")), 6 },
             { op_code(id_index("BR"), &instructions::get_mnemonic_codes("BR")), 2 },
             { op_code(id_index("DC"), &instructions::get_assembler_instructions("DC")), 1 },
         })
    {
        EXPECT_EQ(processing::processing_status_cache_key::generate_loctr_len(instr), expected)
            << instr.value.to_string_view();
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

TEST(mach_instr_processing, operand_versioning_1)

{
    std::string input = R"(
    KIMD 2,4
    KIMD 2,4,0
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(mach_instr_processing, operand_versioning_2)

{
    std::string input = R"(
    KIMD 2,4
    KIMD 2,4,0
)";

    analyzer a(input, analyzer_options(asm_option { .instr_set = instruction_set_version::Z17 }));
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(mach_instr_processing, operand_versioning_3)

{
    std::string input = R"(
    KIMD 2,4
    KIMD 2,4,0
)";

    analyzer a(input, analyzer_options(asm_option { .instr_set = instruction_set_version::Z16 }));
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "M000" }));
}
