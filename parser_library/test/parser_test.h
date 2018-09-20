#ifndef HLASMPLUGIN_HLASMPARSERLIBARY_PARSER_TEST_H
#define HLASMPLUGIN_HLASMPARSERLIBARY_PARSER_TEST_H

#include <string>
#include <sstream>
#include "gmock/gmock.h"
#include "../include/shared/parser_library.h"
#include "../include/shared/lexer.h"
#include "antlr4-runtime.h"
#include <iostream>
#include "../generated/hlasmparser.h"
#include "../src/parser_tools.h"

const size_t size_t_zero = static_cast<size_t>(0);

//returns contents of source file
std::string get_content(std::string source)
{
	std::ifstream ifs(source);
	std::string content((std::istreambuf_iterator<char>(ifs)),
		(std::istreambuf_iterator<char>()));
	return content;
}

class library_test : public testing::Test
{
public:
	virtual void SetUp(std::string param)
	{
		//lexer
		//im sorry for the heap
		antlr4::ANTLRInputStream * input = new antlr4::ANTLRInputStream(get_content("test/library/input/" + param + ".in"));
		hlasm_plugin::parser_library::lexer * lexer = new hlasm_plugin::parser_library::lexer(input);
		 tokens = new antlr4::CommonTokenStream(lexer);

		tokens->fill();

		//parser
		parser = new hlasm_plugin::parser_library::generated::hlasmparser(tokens);
		parser->lexer = lexer;

		//turns on SLL
		//correctness test throws error and restarts as Asll
		/*
		parser->getInterpreter<antlr4::atn::ParserATNSimulator>()->setPredictionMode(antlr4::atn::PredictionMode::SLL); // try with simpler/faster SLL(*)
		// we don't want error messages or recovery during first try
		parser->removeErrorListeners();
		auto error_handler = std::make_shared<antlr4::BailErrorStrategy>();
		parser->setErrorHandler(error_handler);
		antlr4::tree::ParseTree * tree;
		try {
			parser->lexer = lexer;
		tree = parser->program();
		// if we get here, there was no syntax error and SLL(*) was enough;
		// there is no need to try full LL(*)
		}
		catch (antlr4::RuntimeException ex) {
			std::cout << "SLL FAILURE" << std::endl;
		std::cout << tokens->get(tokens->index())->getLine() << std::endl;
		// The BailErrorStrategy wraps the RecognitionExceptions in
		// RuntimeExceptions so we have to make sure we're detecting
		// a true RecognitionException not some other kind
		tokens->reset(); // rewind input stream
		// back to standard listeners/handlers
		parser->addErrorListener(&antlr4::ConsoleErrorListener::INSTANCE);
		auto another_error_handler = std::make_shared<antlr4::DefaultErrorStrategy>();
		parser->setErrorHandler(another_error_handler);
		parser->getInterpreter<antlr4::atn::ParserATNSimulator>()->setPredictionMode(antlr4::atn::PredictionMode::LL); // try full LL(*)
		tree = parser->program();
		}
		*/
		auto vocab = parser->getVocabulary();
		std::stringstream token_stream;
		for (auto token : tokens->getTokens())
		{
			token_stream << vocab.getSymbolicName(token->getType()) << std::endl;
		}
		token_string = token_stream.str();
		rules = parser->getRuleNames();
		rules_map = parser->getRuleIndexMap();
	}
	virtual void TearDown() {}
	antlr4::CommonTokenStream * tokens;
protected:
	std::string token_string;
	hlasm_plugin::parser_library::generated::hlasmparser * parser;
	std::vector<std::string> rules;
	std::map<std::string, size_t> rules_map;
	std::vector<antlr4::tree::ParseTree *> sub_trees;
};


TEST_F(library_test, expression_test)
{
	std::string tcase = "expression_test";

	SetUp(tcase);
	auto t = parser->expr_test()->test_str;
	ASSERT_EQ(t, get_content("test/library/output/" + tcase + ".output"));

	//no errors found while parsing
	ASSERT_EQ(parser->getNumberOfSyntaxErrors(), size_t_zero);
}

//3 instruction statements and 3 EOLLN
TEST_F(library_test, simple)
{
	std::string tcase = "simple";

	SetUp(tcase);
	
	ASSERT_EQ(token_string, get_content("test/library/output/tokens/" + tcase + ".output"));
	//compare tokens with output file

	parser->program();
	//no errors found while parsing
	ASSERT_EQ(parser->getNumberOfSyntaxErrors(), size_t_zero);
}

//5 instruction statements that have 1,1,1,2 and 4 operands respectively
TEST_F(library_test, operand)
{
	std::string tcase = "operand";

	SetUp(tcase);

	//compare tokens with output file

	parser->program();
	//no errors found while parsing
	ASSERT_EQ(parser->getNumberOfSyntaxErrors(), size_t_zero);

}

//3 alternative forms of instructions
TEST_F(library_test, continuation)
{
	std::string tcase = "continuation";

	SetUp(tcase);


	parser->program();
	//no errors found while parsing
	ASSERT_EQ(parser->getNumberOfSyntaxErrors(), size_t_zero);
}

//#define CORRECTNESS_TEST
#ifdef CORRECTNESS_TEST
//simply parse correctly
TEST_F(library_test, correctness)
{
	std::string tcase = "correctness";

	SetUp(tcase);

	//compare tokens with output file
	//ASSERT_EQ(token_string, get_content("test/library/output/tokens/" + tcase + ".output"));

	parser->program();
	//no errors found while parsing
	ASSERT_EQ(parser->getNumberOfSyntaxErrors(), size_t_zero);
}
#endif

//finding 3 variable symbols in model statement
TEST_F(library_test, model_statement)
{
	std::string tcase = "model_statement";

	SetUp(tcase);


	parser->program();
	//no errors found while parsing
	ASSERT_EQ(parser->getNumberOfSyntaxErrors(), size_t_zero);
}

//simply parse correctly
TEST_F(library_test, comment)
{
	std::string tcase = "comment";

	SetUp(tcase);

	//compare tokens with output file

	parser->program();
	//no errors found while parsing
	ASSERT_EQ(parser->getNumberOfSyntaxErrors(), size_t_zero);
}

#endif // !HLASMPLUGIN_HLASMPARSERLIBARY_PARSER_TEST_H
