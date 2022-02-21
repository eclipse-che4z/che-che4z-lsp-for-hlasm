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

#include "common_testing.h"


TEST(diagnostics, overall_correctness)
{
    std::string input(
        R"( 
 J LABEL
 ACONTROL COMPAT(CASE)
 CATTR DEFLOAD,FILL(3)
 CATTR FILL(3)
 AINSERT ' sam64',BACK
&x setc ' sam64'
 AINSERT '&x',BACK
LABEL EQU *+2
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(diagnostics, string_substitution)
{
    std::string input(
        R"( 
&x setc '* 10'
 AINSERT '&x',BACK

&a seta 31
&b setc 'ANY'
 AMODE &b&a

&z setc 'string'
 EXTRN A,PART(B),PART(C,D),E,F,PART(&z,H)

)");

    analyzer a(input);
    a.analyze();

    a.collect_diags();

    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(diagnostics, division_by_zero) // test ok
{
    std::string input(
        R"( 
 ADATA 3,4,5,6/0,'test'

&a seta 0
&b seta 4/&a

&c seta 0/0
 
 L 1,2(2,3/0)

 CLCL 10/0,3

)");

    analyzer a(input);
    a.analyze();

    a.collect_diags();

    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(diagnostics, instr_zero_op) // test ok
{
    std::string input(
        R"( 
 SPACE
 EJECT
 ORG
 ISEQ
 END
)");

    analyzer a(input);
    a.analyze();

    a.collect_diags();

    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

    ASSERT_EQ(a.diags().size(), (size_t)0);
}

/*
TEST(diagnostics, unkown_symbols) // to do? number of errors?
{
        std::string input(
                R"(
 ADATA 1,2,3,10,’&a’
 Ĵ
 ˷
 ͺ
 ̆
)"
);

        analyzer a(input);
        a.analyze();

        a.collect_diags();

        ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

        ASSERT_EQ(a.diags().size(), (size_t)0);
}*/


TEST(diagnostics, case_insensitivity)
{
    std::string input(
        R"( 
 AcOnTROL NoAfPR,compat(CaSe,cASE),FLAG(USING0),OPTABLE(zs5,LIsT)
 ADATA -300,2*100,2,3,'TEST'
 AINSERT ' sAm31 this needs to be valid code',bacK
 AMODE any31
 CATTR rMODE(31),ALIgn(2)
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();

    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(diagnostics, machine)
{
    std::string input(
        R"( 
 L 0,2222
 AHI 0,2
 ST 0,2(2,2)
 LR  12,15                  SET BASE REGISTER
 ST  15,16(,7)
 LA  1,255(,1) 
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();

    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

    ASSERT_EQ(a.diags().size(), (size_t)0);
}


TEST(diagnostics, mnemonics)
{
    // 4 - 4bit
    // 10(2,2) - D(4bit, base)
    // 30000 - 16b
    // 80000 - 32bit
    std::string input(
        R"( 
  B 10(2,2)
  BR 4 
  J LABEL1
  NOP 10(2,2)
  NOPR 4
  JNOP LABEL1
  BH 10(2,2)
  BHR 4
  JH LABEL1
  BL 10(2,2)
  BLR 4
  JL LABEL1
  BE 10(2,2)
  BER 4
  JE LABEL1
  BNH 10(2,2)
  BNHR 4
  JNH LABEL1
  BNL 10(2,2)
  BNLR 4
  JNL LABEL1
  BNE 10(2,2)
  BNER 4
  JNE LABEL1
  BO 10(2,2)
  BOR 4
  JO LABEL1
  BNO 10(2,2)
  BNOR 4 
  JNO LABEL1
  BRUL LABEL2
  BRHL LABEL2
  BRLL LABEL2
  BREL LABEL2
  BRNHL LABEL2
  BRNLL LABEL2
  BRNEL LABEL2
  BROL LABEL2
  BRNOL LABEL2
  JLNOP LABEL2
LABEL1 EQU *+19000
LABEL2 equ *+79000
)");

    analyzer a(input);
    a.analyze();

    a.collect_diags();

    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(diagnostics,
    complex_operands) // to do - add machine, check CCW, EQU, OPSYN, other instructions with labels - org etc
{
    std::string input(
        R"( 
S START 32
 ACONTROL NOAFPR,COMPAT(CASE,NOCASE),FLAG(USING0,AL),OPTABLE(ZS5,LIST)
 ACONTROL NOTYPECHECK,TYPECHECK(MAGNITUDE,NOREG),OPTABLE(DOS)
 ADATA -300,2*100,2,3,'test'
 AINSERT ' sam24 this must be valid code',BACK
 AMODE ANY31
 CATTR RMODE(31),ALIGN(2)
 CATTR ALIGN(1),DEFLOAD,EXECUTABLE,FILL(5),RENT,NOTREUS,PRIORITY(2)
 CEJECT 10/2
 CNOP 6,8
 COM    
S CSECT 
 EXITCTL LISTING,256,*+128,,-2
 EXITCTL SOURCE,,,
 EXTRN A,PART(B),PART(C,D),E
 ICTL 1,71,16 
 ICTL 9,80
 DROP ,
 ISEQ 10,50-4
label LOCTR
 LTORG
 MNOTE 120,'message'
lr OPSYN   
 ORG *+500   remark
 ORG *+1,,4 
 ORG ,
 PRINT ON,OFF,ON,DATA,MCALL,NOPRINT 
 PUNCH 'string'
 PUSH PRINT,NOPRINT
 REPRO
 RMODE 24
label1 RSECT
 SPACE 4
 TITLE 'string'   remark
 USING (3,4),12
 USING 1,3,15,0,1/1
 WXTRN AW,PART(BW),PART(CW,DW),EW
 XATTR ATTR(lab),REFERENCE(DIRECT,DATA),LINK(XPLINK),SCOPE(SECTION)
 END ,(MYCOMPIlER,0101,00273)
)");

    analyzer a(input);
    a.analyze();

    a.collect_diags();

    ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(diagnostics, parser_diagnostics_passing)
{
    std::string input(
        R"( 
 MACRO
 M 
 MEND
 M (ABC,(DEF,GHI),JKL
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(diagnostics, previously_defined_enum_operand)
{
    std::string input(
        R"( 
PRINT EQU *
      POP PRINT
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(diagnostics, system_variable_without_subscript_invalid_opcode)
{
    std::string input(
        R"( 
 MACRO
 MAC
 &SYSNDX
 MEND 
 
 MAC
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)1);
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E049" }));
}

TEST(diagnostics, system_variable_with_subscript_1_invalid_opcode)
{
    std::string input(
        R"( 
 MACRO
 MAC
 &SYSNDX(1)
 MEND 
 
 MAC
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)1);
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E049" }));
}

TEST(diagnostics, system_variable_with_subscripts_1_1_invalid_opcode)
{
    std::string input(
        R"( 
 MACRO
 MAC
 &SYSNDX(1,1)
 MEND 
 
 MAC
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)0);
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E049" }));
}

TEST(diagnostics, system_variable_with_subscripts_1_1_1_invalid_opcode)
{
    std::string input(
        R"( 
 MACRO
 MAC
 &SYSNDX(1,1,1)
 MEND 
 
 MAC
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)0);
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E049" }));
}

TEST(diagnostics, system_variable_with_subscript_0_null_opcode)
{
    std::string input(
        R"( 
 MACRO
 MAC
 &SYSNDX(0)
 MEND 
 
 MAC
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)1);
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E012" }));
}

TEST(diagnostics, system_variable_with_subscripts_1_0_null_opcode)
{
    std::string input(
        R"( 
 MACRO
 MAC
 &SYSNDX(1,0)
 MEND 
 
 MAC
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)0);
     EXPECT_TRUE(matches_message_codes(a.diags(), { "E012" }));
}

TEST(diagnostics, system_variable_with_subscripts_1_1_0_null_opcode)
{
    std::string input(
        R"( 
 MACRO
 MAC
 &SYSNDX(1,1,0)
 MEND 
 
 MAC
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)0);
     EXPECT_TRUE(matches_message_codes(a.diags(), { "E012" }));
}

TEST(diagnostics, system_variable_with_subscript_2_null_opcode)
{
    std::string input(
        R"( 
 MACRO
 MAC
 &SYSNDX(2)
 MEND 
 
 MAC
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)1);
    // EXPECT_TRUE(matches_message_codes(a.diags(), { "E049" })); // todo - Find/add diag for "ASMA087S"
}

TEST(diagnostics, system_variable_with_subscripts_2_0_null_opcode)
{
    std::string input(
        R"( 
 MACRO
 MAC
 &SYSNDX(2,0)
 MEND 
 
 MAC
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)0);
    // EXPECT_TRUE(matches_message_codes(a.diags(), { "E049" })); // todo - Find/add diag for "ASMA087S"
}

TEST(diagnostics, system_variable_with_subscripts_2_1_null_opcode)
{
    std::string input(
        R"( 
 MACRO
 MAC
 &SYSNDX(2,1)
 MEND 
 
 MAC
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)0);
    // EXPECT_TRUE(matches_message_codes(a.diags(), { "E049" })); // todo - Find/add diag for "ASMA087S"
}

TEST(diagnostics, system_variable_with_subscripts_2_2_2_null_opcode)
{
    std::string input(
        R"( 
 MACRO
 MAC
 &SYSNDX(2,2,2)
 MEND 
 
 MAC
)");

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)0);
    // EXPECT_TRUE(matches_message_codes(a.diags(), { "E049" })); // todo - Find/add diag for "ASMA087S"
}