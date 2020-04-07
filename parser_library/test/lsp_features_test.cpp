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

#include "../src/parse_lib_provider.h"
#include "../src/analyzer.h"
#include "../src/context/instruction.h"
#include "mock_parse_lib_provider.h"


using namespace hlasm_plugin::parser_library;

class lsp_features_test : public testing::Test
{
public:
	lsp_features_test() : a(contents, SOURCE_FILE, lib_provider),
		instruction_count(context::instruction::machine_instructions.size() +
			context::instruction::assembler_instructions.size() +
			context::instruction::ca_instructions.size() +
			context::instruction::mnemonic_codes.size())
	{};

	virtual void SetUp()
	{
		a.analyze();
	}
	virtual void TearDown() {}
protected:
	const std::string contents = 
R"(   MAC  1
&VAR   SETA 5
       LR   &VAR,&VAR REMARK
       AGO  .HERE
       L    1,0(5)
.HERE  ANOP
* line 6
       MACRO
&LABEL MAC   &VAR
*THIS IS DOCUMENTATION
* line 10
       LR     &VAR,&LABEL
       AGO    .HERE
       AGO    .HERE2
.HERE2 ANOP
.HERE  ANOP 
       MEND
* line 17
1      MAC   2
* line 19
      COPY COPYFILE
      LR R1,R2
R1 EQU 1
R1      MAC   R2
   LCLC &SETC
* line 25
 
)";
	std::string content;
	mock_parse_lib_provider lib_provider;
	analyzer a;
	const size_t instruction_count;
};

TEST_F(lsp_features_test, go_to)
{
	// jump from source to macro, MAC instruction
	EXPECT_EQ(semantics::position_uri_s(MACRO_FILE,position(1,7)), a.lsp_processor().go_to_definition(position(0, 4)));
	// no jump
	EXPECT_EQ(semantics::position_uri_s(SOURCE_FILE, position(0, 8)), a.lsp_processor().go_to_definition(position(0, 8)));
	// jump in source, open code, var symbol &VAR
	EXPECT_EQ(semantics::position_uri_s(SOURCE_FILE, position(1, 0)), a.lsp_processor().go_to_definition(position(2, 13)));
	// jump in source, open code, seq symbol .HERE
	EXPECT_EQ(semantics::position_uri_s(SOURCE_FILE, position(5, 0)), a.lsp_processor().go_to_definition(position(3, 13)));
	// jump in source, macro, seq symbol .HERE
	EXPECT_EQ(semantics::position_uri_s(SOURCE_FILE, position(15, 0)), a.lsp_processor().go_to_definition(position(12, 15)));
	// jump in source, macro, var symbol &LABEL
	EXPECT_EQ(semantics::position_uri_s(SOURCE_FILE, position(8, 0)), a.lsp_processor().go_to_definition(position(11, 20)));
	// jump in source, macro, var symbol &VAR
	EXPECT_EQ(semantics::position_uri_s(SOURCE_FILE, position(8, 13)), a.lsp_processor().go_to_definition(position(11, 15)));
	// forward jump in source, open code, ord symbol R1
	EXPECT_EQ(semantics::position_uri_s(SOURCE_FILE, position(22, 0)), a.lsp_processor().go_to_definition(position(21, 10)));
	// jump from source to copy file, ord symbol R2 on machine instrution
	EXPECT_EQ(semantics::position_uri_s(COPY_FILE, position(0, 0)), a.lsp_processor().go_to_definition(position(21, 13)));
	// jump from source to copy file, ord symbol R2 on macro MAC
	EXPECT_EQ(semantics::position_uri_s(COPY_FILE, position(0, 0)), a.lsp_processor().go_to_definition(position(23, 14)));
	// jump from source to first instruction in copy file, COPY COPYFILE
	EXPECT_EQ(semantics::position_uri_s(COPY_FILE, position(0, 3)), a.lsp_processor().go_to_definition(position(20, 14)));
}

TEST_F(lsp_features_test, refs)
{
	// the same as go_to test but with references
	// reference MAC, should appear once in source and once in macro file
	EXPECT_EQ(a.lsp_processor().references(position(0, 4)).size(), (size_t)2);
	// no reference, returns the same line where the references command was called
	EXPECT_EQ((size_t)1, a.lsp_processor().references(position(0, 8)).size());
	// source code references for &VAR, appeared three times
	EXPECT_EQ((size_t)3, a.lsp_processor().references(position(2, 13)).size());
	// source code references for .HERE, appeared twice
	EXPECT_EQ((size_t)2, a.lsp_processor().references(position(3, 13)).size());
	// references inside macro def, seq symbol .HERE
	EXPECT_EQ((size_t)2, a.lsp_processor().references(position(12, 15)).size());
	//  references inside macro def, var symbol &LABEL
	EXPECT_EQ((size_t)2, a.lsp_processor().references(position(11, 20)).size());
	//  references inside macro def, var symbol &VAR
	EXPECT_EQ((size_t)2, a.lsp_processor().references(position(11, 15)).size());
}

// 4 cases, instruction, sequence, variable and none
TEST_F(lsp_features_test, hover)
{
	// hover for macro MAC, contains description and user documentation
	auto result = a.lsp_processor().hover(position(18, 8));
	ASSERT_EQ((size_t)4, result.size());
	EXPECT_EQ("LABEL VAR", result[0]);
	EXPECT_EQ("version 1", result[1]);
	EXPECT_EQ("THIS IS DOCUMENTATION", result[2]);
	EXPECT_EQ(" line 10", result[3]);

	// hover for sequence symbol, defined even though it is skipped because of the macro parsing (wanted behaviour)
	result = a.lsp_processor().hover(position(13, 15));
	ASSERT_EQ((size_t)1, result.size());
	EXPECT_EQ("Defined at line 14", result[0]);

	// hover for variable symbol, name and type number
	result = a.lsp_processor().hover(position(2, 13));
	ASSERT_EQ((size_t)1, result.size());
	EXPECT_EQ("number", result[0]);

	// hover for variable symbol, name and type string
	result = a.lsp_processor().hover(position(24, 10));
	ASSERT_EQ((size_t)1, result.size());
	EXPECT_EQ("string", result[0]);

	// hover on ord symbol R1, its value
	result = a.lsp_processor().hover(position(21, 10));
	ASSERT_EQ((size_t)4, result.size());
	EXPECT_EQ("1", result[0]);
	EXPECT_EQ("Absolute Symbol", result[1]);
	EXPECT_EQ("L: 1", result[2]);
	EXPECT_EQ("T: U", result[3]);

	// hover on ord symbol R1 definition
	result = a.lsp_processor().hover(position(22, 1));
	ASSERT_EQ((size_t)4, result.size());
	EXPECT_EQ("1", result[0]);
	EXPECT_EQ("Absolute Symbol", result[1]);
	EXPECT_EQ("L: 1", result[2]);
	EXPECT_EQ("T: U", result[3]);

	// hover on COPYFILE, definition file
	result = a.lsp_processor().hover(position(20, 14));
	ASSERT_EQ((size_t)1, result.size());
	EXPECT_EQ("Defined in file: " + std::string(COPY_FILE), result[0]);

	// no hover on remarks
	result = a.lsp_processor().hover(position(2, 24));
	ASSERT_EQ((size_t)0, result.size());
}

// completion for variable symbols (&), sequence syms (.) and instructions ((S*)(s+)(S*))
TEST_F(lsp_features_test, completion)
{
	// all instructions + 2 newly defined macros
	EXPECT_EQ(instruction_count + 2, a.lsp_processor().completion(position(26, 1), '\0', 1).items.size());
	// current scope detection missing !
	// seq symbols
	EXPECT_EQ((size_t)1, a.lsp_processor().completion(position(6, 0), '.', 2).items.size());
	// var symbols
	EXPECT_EQ((size_t)3, a.lsp_processor().completion(position(10, 0), '&', 2).items.size());
}
