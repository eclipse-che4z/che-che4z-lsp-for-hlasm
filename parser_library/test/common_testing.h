#pragma once
#include "gmock/gmock.h"
#include <utility>
#include "antlr4-runtime.h"
#include "../include/shared/lexer.h"
#include "../src/generated/hlasmparser.h"
#include "shared/token_stream.h"
#include "shared/input_source.h"


using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library::semantics;

const size_t size_t_zero = static_cast<size_t>(0);


//returns contents of source file
std::string get_content(std::string source)
{
	std::ifstream ifs(source);
	std::string content((std::istreambuf_iterator<char>(ifs)),
		(std::istreambuf_iterator<char>()));
	return content;
}


struct parser_holder
{
	std::unique_ptr<hlasm_plugin::parser_library::input_source> input;
	std::unique_ptr<hlasm_plugin::parser_library::lexer> lexer;
	std::unique_ptr<hlasm_plugin::parser_library::token_stream> tokens;
	std::shared_ptr<hlasm_plugin::parser_library::context::hlasm_context> ctx;
	std::unique_ptr<hlasm_plugin::parser_library::generated::hlasmparser> parser;

	parser_holder(std::string text)
	{
		input = std::make_unique<hlasm_plugin::parser_library::input_source>(std::move(text));
		lexer = std::make_unique<hlasm_plugin::parser_library::lexer>(input.get());
		tokens = std::make_unique<hlasm_plugin::parser_library::token_stream>(lexer.get());
		parser = std::make_unique<hlasm_plugin::parser_library::generated::hlasmparser>(tokens.get());
		ctx = std::make_shared<hlasm_plugin::parser_library::context::hlasm_context>();

		parser->initialize(ctx, lexer.get());
	}

	std::pair<bool, antlr4::ParserRuleContext*> try_parse_sll()
	{
		parser->getInterpreter<antlr4::atn::ParserATNSimulator>()->setPredictionMode(antlr4::atn::PredictionMode::SLL); // try with simpler/faster SLL(*)
		// we don't want error messages or recovery during first try
		parser->removeErrorListeners();
		parser->setErrorHandler(std::make_shared<antlr4::BailErrorStrategy>());
		try {
			auto tree = parser->program();
			// if we get here, there was no syntax error and SLL(*) was enough;
			// there is no need to try full LL(*)
			return { true,tree };
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
			parser->setErrorHandler(std::make_shared<antlr4::DefaultErrorStrategy>());
			parser->getInterpreter<antlr4::atn::ParserATNSimulator>()->setPredictionMode(antlr4::atn::PredictionMode::LL); // try full LL(*)
			auto tree = parser->program();
			return { false,tree };
		}
	}
};
