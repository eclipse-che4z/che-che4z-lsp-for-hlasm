#ifndef HLASMPLUGIN_HLASMPARSERLIBARY_LEXER_TEST_H
#define HLASMPLUGIN_HLASMPARSERLIBARY_LEXER_TEST_H

#include <string>
#include <sstream>
#include "gmock/gmock.h"
#include "../include/shared/lexer.h"
#include "../src/generated/hlasmparser.h"
#include "antlr4-runtime.h"
#include <iostream>

//returns contents of source file
std::string get_content(std::string source);

using parser = hlasm_plugin::parser_library::generated::hlasmparser;

class lexer_test : public testing::Test
{
};


TEST_F(lexer_test, aread)
{
	std::string tcase = "aread";

	antlr4::ANTLRInputStream input(get_content("test/library/input/" + tcase + ".in"));
	hlasm_plugin::parser_library::lexer l(&input);
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

	ASSERT_EQ(token_string, get_content("test/library/output/tokens/" + tcase + ".output"));
}

TEST_F(lexer_test, rntest)
{
	std::string tcase = "rntest";

	antlr4::ANTLRInputStream input("TEST TEST \r\n TEST1 TEST2");
	hlasm_plugin::parser_library::lexer l(&input);
	antlr4::CommonTokenStream tokens(&l);
	parser parser(&tokens);

	tokens.fill();

	std::stringstream token_stream;
	for (auto token : tokens.getTokens())
		token_stream << parser.getVocabulary().getSymbolicName(token->getType()) << std::endl;
	auto token_string = token_stream.str();

	ASSERT_EQ(token_string, get_content("test/library/output/tokens/" + tcase + ".output"));
}

TEST_F(lexer_test, unlimited_line)
{
	std::string tcase = "unlimited_line";

	antlr4::ANTLRInputStream input(get_content("test/library/input/" + tcase + ".in"));
	hlasm_plugin::parser_library::lexer l(&input);
	antlr4::CommonTokenStream tokens(&l);
	parser parser(&tokens);
	l.set_unlimited_line(true);

	tokens.fill();

	std::stringstream token_stream;
	for (auto token : tokens.getTokens())
		token_stream << parser.getVocabulary().getSymbolicName(token->getType()) << std::endl;
	auto token_string = token_stream.str();

	ASSERT_EQ(token_string, get_content("test/library/output/tokens/" + tcase + ".output"));
}


#endif // !HLASMPLUGIN_HLASMPARSERLIBARY_PARSER_TEST_H
