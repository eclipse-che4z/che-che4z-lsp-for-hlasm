#ifndef HLASMPLUGIN_PARSERLIBRARY_ERROR_STRATEGY_H
#define HLASMPLUGIN_PARSERLIBRARY_ERROR_STRATEGY_H

#include "DefaultErrorStrategy.h"
#include "shared/lexer.h"

namespace hlasm_plugin::parser_library
{


class error_strategy : public antlr4::DefaultErrorStrategy
{
	virtual antlr4::Token* getMissingSymbol(antlr4::Parser *recognizer) override
	{
		using namespace antlr4;

		token * currentSymbol = dynamic_cast<token *>(recognizer->getCurrentToken());
		assert(currentSymbol);
		
		misc::IntervalSet expecting = getExpectedTokens(recognizer);
		size_t expectedTokenType = expecting.getMinElement(); // get any element
		std::string tokenText;
		if (expectedTokenType == Token::EOF) {
			tokenText = "Unexpected end of file";
		}
		else {
			tokenText = "<missing " + recognizer->getVocabulary().getDisplayName(expectedTokenType) + ">";
		}
		token *current = currentSymbol;
		token *lookback = dynamic_cast<token *>(recognizer->getTokenStream()->LT(-1));
		if (current->getType() == Token::EOF && lookback != nullptr) {
			current = lookback;
		}

		lexer * lex = dynamic_cast<lexer *>(recognizer->getTokenStream()->getTokenSource());

		

		_errorSymbols.push_back(lex->get_token_factory()->create(current->getTokenSource(), current->getInputStream(),
			expectedTokenType, Token::DEFAULT_CHANNEL, INVALID_INDEX, INVALID_INDEX,
			current->getLine(), current->getCharPositionInLine(), (size_t) -1, current->get_char_position_in_line_16(), current->get_end_of_token_in_line_utf16()));

		return _errorSymbols.back().get();
	}

	std::vector<std::unique_ptr<antlr4::Token>> _errorSymbols;
};

}


#endif