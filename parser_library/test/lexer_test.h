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

#ifndef HLASMPLUGIN_HLASMPARSERLIBARY_LEXER_TEST_H
#define HLASMPLUGIN_HLASMPARSERLIBARY_LEXER_TEST_H

#include <string>
#include <sstream>
#include "gmock/gmock.h"
#include "../include/shared/lexer.h"
#include "../include/shared/input_source.h"
#include "hlasmparser.h"
#include "antlr4-runtime.h"
#include <iostream>

//returns contents of source file
std::string get_content(std::string source);

using parser = hlasm_plugin::parser_library::generated::hlasmparser;

TEST(lexer_test, aread)
{
	std::string tcase = "aread";
	std::string in =
R"(        AINSERT 'test string1',FRONT
        AINSERT 'test string2',BACK
&SYMBOL AREAD
This does not go to symbol
     INSTR 1,2,3)";

	std::string out =
R"(AREAD
IGNORED
AREAD
IGNORED
AREAD
IGNORED
AREAD
IGNORED
AREAD
IGNORED
AMPERSAND
ORDSYMBOL
SPACE
ORDSYMBOL
EOLLN
ORDSYMBOL
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
EOLLN
SPACE
ORDSYMBOL
SPACE
NUM
COMMA
NUM
COMMA
NUM
EOLLN
EOF
)";

	hlasm_plugin::parser_library::semantics::lsp_info_processor lsp_proc = { "aread","",std::make_shared<lsp_context>()};
	hlasm_plugin::parser_library::input_source input(in);
	hlasm_plugin::parser_library::lexer l(&input,&lsp_proc);
	antlr4::CommonTokenStream tokens(&l);
	parser parser(&tokens);

	l.ainsert_back("INSERTED BACK 1");
	l.aread();
	l.ainsert_back("INSERTED BACK 2");
	l.ainsert_front("INSERTED FRONT 1");
	l.aread();
	l.aread();
	l.aread();
	l.aread();

	tokens.fill();

	std::stringstream token_stream;
	for (auto token : tokens.getTokens())
		token_stream << parser.getVocabulary().getSymbolicName(token->getType()) << std::endl;
	auto token_string = token_stream.str();

	ASSERT_EQ(token_string, out);
}

TEST(lexer_test, rntest)
{

	std::string out = R"(ORDSYMBOL
SPACE
ORDSYMBOL
SPACE
EOLLN
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
EOLLN
EOF
)";

	hlasm_plugin::parser_library::semantics::lsp_info_processor lsp_proc = { "rntest","",std::make_shared<lsp_context>()};
	hlasm_plugin::parser_library::input_source input("TEST TEST \r\n TEST1 TEST2");
	hlasm_plugin::parser_library::lexer l(&input, &lsp_proc);
	antlr4::CommonTokenStream tokens(&l);
	parser parser(&tokens);

	tokens.fill();

	std::stringstream token_stream;
	for (auto token : tokens.getTokens())
		token_stream << parser.getVocabulary().getSymbolicName(token->getType()) << std::endl;
	auto token_string = token_stream.str();

	ASSERT_EQ(token_string, out);
}

TEST(lexer_test, new_line_in_ignored)
{
	hlasm_plugin::parser_library::semantics::lsp_info_processor lsp_proc = { "new_line_in_ignored","",std::make_shared<lsp_context>()};
	//test case, when a newline is in the first 15 ignored characters after continuation
	hlasm_plugin::parser_library::input_source input(
		R"(NAME1 OP1      OPERAND1,OPERAND2,OPERAND3   This is the normal         X
        
label lr 1,1)");
	hlasm_plugin::parser_library::lexer l(&input, &lsp_proc);
	antlr4::CommonTokenStream tokens(&l);
	parser parser(&tokens);

	tokens.fill();

	std::stringstream token_stream;
	size_t eolln_count = 0;
	for (auto token : tokens.getTokens())
	{
		if (parser.getVocabulary().getSymbolicName(token->getType()) == "EOLLN")
			++eolln_count;
	}
		
	EXPECT_EQ(eolln_count, (size_t)2);

	
}

TEST(lexer_test, unlimited_line)
{
	std::string in =
R"(LABEL INSTR    2,1,                                                                   THIS SHOULD NOT BE IGNORED
      INSTR    2    REMARK
      INSTR    2,1, TOTO JE REMARK)";
	std::string out = R"(ORDSYMBOL
SPACE
ORDSYMBOL
SPACE
NUM
COMMA
NUM
COMMA
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
EOLLN
IGNORED
SPACE
ORDSYMBOL
SPACE
NUM
SPACE
ORDSYMBOL
EOLLN
IGNORED
SPACE
ORDSYMBOL
SPACE
NUM
COMMA
NUM
COMMA
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
EOLLN
EOF
)";

	hlasm_plugin::parser_library::semantics::lsp_info_processor lsp_proc = { "unlimited_line","",std::make_shared<lsp_context>()};
	hlasm_plugin::parser_library::input_source input(in);
	hlasm_plugin::parser_library::lexer l(&input, &lsp_proc);
	antlr4::CommonTokenStream tokens(&l);
	parser parser(&tokens);
	l.set_unlimited_line(true);

	tokens.fill();

	std::stringstream token_stream;
	for (auto token : tokens.getTokens())
		token_stream << parser.getVocabulary().getSymbolicName(token->getType()) << std::endl;
	auto token_string = token_stream.str();

	ASSERT_EQ(token_string, out);
}

TEST(lexer_test, rewind_input)
{
	std::string in =
R"(    REWIND1
REWIND2
    REWIND3)";
	std::string out = R"(SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
EOLLN
ORDSYMBOL
ORDSYMBOL
EOLLN
ORDSYMBOL
EOLLN
SPACE
ORDSYMBOL
ORDSYMBOL
EOLLN
SPACE
ORDSYMBOL
EOLLN
EOF
)";
	hlasm_plugin::parser_library::input_source input(in);
	hlasm_plugin::parser_library::semantics::lsp_info_processor lsp_proc = { "rewind_input","",std::make_shared<lsp_context>()};
	hlasm_plugin::parser_library::lexer l(&input,&lsp_proc);
	antlr4::CommonTokenStream tokens(&l);
	parser parser(&tokens);

	std::stringstream token_stream;
	hlasm_plugin::parser_library::token_ptr token;
	do
	{
		token = l.nextToken();
		token_stream << parser.getVocabulary().getSymbolicName(token->getType()) << std::endl;
		if (token->getText() == "REWIND1")
		{
			l.rewind_input({ 0, 0 });
			break;
		}
	} while (token->getType() != antlr4::Token::EOF);

	do
	{
		token = l.nextToken();
		token_stream << parser.getVocabulary().getSymbolicName(token->getType()) << std::endl;
		if (token->getText() == "REWIND2")
		{
			l.rewind_input({ 0, 4 });
			break;
		}
	} while (token->getType() != antlr4::Token::EOF);

	do
	{
		token = l.nextToken();
		token_stream << parser.getVocabulary().getSymbolicName(token->getType()) << std::endl;
		if (token->getText() == "REWIND3")
		{
			l.rewind_input({ 1, 17 });
			break;
		}
	} while (token->getType() != antlr4::Token::EOF);

	do
	{
		token = l.nextToken();
		token_stream << parser.getVocabulary().getSymbolicName(token->getType()) << std::endl;
	} while (token->getType() != antlr4::Token::EOF);

	auto token_string = token_stream.str();

	ASSERT_EQ(token_string, out);
}

#endif // !HLASMPLUGIN_HLASMPARSERLIBARY_PARSER_TEST_H
