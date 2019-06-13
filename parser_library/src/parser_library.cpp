#include <iostream>

#include "../include/shared/parser_library.h"
#include "../include/shared/token_stream.h"
#include "parser_tools.h"
#include "context/hlasm_context.h"
#include "generated/hlasmparser.h"
#include "analyzer.h"



namespace hlasm_plugin {
namespace parser_library {

//testing output
void parser_library::parse(const std::string & src)
{
	analyzer a(src);
	/*
	auto input(std::make_unique<input_source>(src));
	auto lexer(std::make_unique<lexer>(input.get()));
	auto tokens(std::make_unique<token_stream>(lexer.get()));

	generated::hlasmparser parser(tokens.get());
	//parser.initialize(std::make_shared<hlasm_plugin::parser_library::context::hlasm_context>(), lexer.get());
	

	*/
	auto vocab = a.parser().getVocabulary();


	auto l = new antlr4::DiagnosticErrorListener();
	a.parser().addErrorListener(l);
	a.parser().getInterpreter<antlr4::atn::ParserATNSimulator>()->setPredictionMode(antlr4::atn::PredictionMode::LL_EXACT_AMBIG_DETECTION);

	auto tree = a.parser().program();
	/*
	for (auto && token : dynamic_cast<antlr4::BufferedTokenStream*>(a.parser().getTokenStream())->getTokens())
	{
		auto type = token->getType();
		std::cout << vocab.getSymbolicName(type);
		if (type != lexer::Tokens::EOLLN && type != lexer::Tokens::SPACE && type != lexer::Tokens::CONTINUATION && type != lexer::Tokens::IGNORED) std::cout << "---" << "\"" << token->getText() << "\"";
		std::cout << std::endl;
	}
	std::cout << l << std::endl;

	hlasm_plugin::parser_tools::useful_tree mytree(tree, vocab, a.parser().getRuleNames());
	//mytree.out_tree(std::cout);*/
	
}
}
}
