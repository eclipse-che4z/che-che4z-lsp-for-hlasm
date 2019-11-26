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

#ifndef HLASMPLUGIN_PARSERLIBRARY_TEST_DIAGNOSTICS_CHECK_TEST_H
#define HLASMPLUGIN_PARSERLIBRARY_TEST_DIAGNOSTICS_CHECK_TEST_H

#include "common_testing.h"


TEST(diagnostics, overall_correctness)
{
	std::string input(
		R"( 
 J 5
 ACONTROL COMPAT(CASE)
 CATTR DEFLOAD,FILL(3)
 CATTR FILL(3)
 AINSERT 'abc',BACK
&x setc 'abc'
 AINSERT '&x',BACK
)"
);
	analyzer a(input);
	a.analyze();

	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();

	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)0);
}

TEST(diagnostics, string_substitution)
{
	std::string input(
		R"( 
&x setc '10'
 AINSERT '&x',BACK

&a seta 31
&b setc 'ANY'
 AMODE &b&a

&z setc 'string'
 EXTRN 2,PART(2),PART(2,2),1000,'3',PART(&z,4) 

)"
);

	analyzer a(input);
	a.analyze();

	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();

	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)0);
}

TEST(diagnostics, division_by_zero) //test ok
{
	std::string input(
		R"( 
 ADATA 3,4,5,6/0,'test'

&a seta 0
&b seta 4/&a

&c seta 0/0
 
 L 1,2(2,3/0)

 CLCL 10/0,3

)"
);

	analyzer a(input);
	a.analyze();

	a.collect_diags();

	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)0);
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
)"
);

	analyzer a(input);
	a.analyze();

	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();

	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)0);
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

	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();

	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)0);
}*/


TEST(diagnostics, case_insensitivity)
{
	std::string input(
		R"( 
 AcOnTROL NoAfPR,compat(CaSe,cASE),FLAG(USING0),OPTABLE(zs5,LIsT)
 ADATA -300,2*100,2,3,'TEST'
 AINSERT 'test',bacK
 AMODE any31
 CATTR rMODE(31),ALIgn(2)
)"
);
	analyzer a(input);
	a.analyze();

	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();

	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)0);
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
)"
);
	analyzer a(input);
	a.analyze();

	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();

	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)0);
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
  J 30000
  NOP 10(2,2)
  NOPR 4
  JNOP 30000
  BH 10(2,2)
  BHR 4
  JH 30000
  BL 10(2,2)
  BLR 4
  JL 30000
  BE 10(2,2)
  BER 4
  JE 30000
  BNH 10(2,2)
  BNHR 4
  JNH 30000
  BNL 10(2,2)
  BNLR 4
  JNL 30000
  BNE 10(2,2)
  BNER 4
  JNE 30000
  BO 10(2,2)
  BOR 4
  JO 30000
  BNO 10(2,2)
  BNOR 4 
  JNO 30000
  BRUL 80000
  BRHL 80000
  BRLL 80000
  BREL 80000
  BRNHL 80000
  BRNLL 80000
  BRNEL 80000
  BROL 80000
  BRNOL 80000
  JLNOP 80000
)"
);

	analyzer a(input);
	a.analyze();

	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();

	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)0);
}

TEST(diagnostics, complex_operands) // to do - add machine, check CCW, EQU, OPSYN, other instructions with labels - org etc
{
	std::string input(
		R"( 
 ACONTROL NOAFPR,COMPAT(CASE,NOCASE),FLAG(USING0,AL),OPTABLE(ZS5,LIST)
 ACONTROL NOTYPECHECK,TYPECHECK(MAGNITUDE,NOREG),OPTABLE(DOS)
 ADATA -300,2*100,2,3,'test'
 AINSERT 'test',BACK
 AMODE ANY31
 CATTR RMODE(31),ALIGN(2)
 CATTR ALIGN(1),DEFLOAD,EXECUTABLE,FILL(5),RENT,NOTREUS,PRIORITY(2)
 CEJECT 10/2
 CNOP 6,8
 COM    
 CSECT
 END ,(MYCOMPIlER,0101,00273)
 EXITCTL LISTING,256,*+128,,-2
 EXITCTL SOURCE,,,
 EXTRN 2,PART(2),PART(2,2),1000,'3',PART('string',4)
 ICTL 1,71,16 
 ICTL 9,80
 DROP ,
 ISEQ 10,50-4
label LOCTR
 LTORG
 MNOTE 120,'message'
 OPSYN   
 ORG *-500   remark
 ORG 2+1,,4 
 ORG ,
 PRINT ON,OFF,ON,DATA,MCALL,NOPRINT 
 PUNCH 'string'
 PUSH PRINT,NOPRINT
 REPRO
 END ,
 RMODE 24
label1 RSECT
 SPACE 4
 START 34
 TITLE 'string'   remark
 USING (3,3),12
 USING 1,3,15,0,0/0
 WXTRN 2,PART(2),PART(2,2),1000,'3',PART('string',4)
 XATTR ATTR(lab),REFERENCE(DIRECT,DATA),LINK(XPLINK),SCOPE(SECTION)
)"
);

	analyzer a(input);
	a.analyze();

	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();

	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)0);
}

#endif
