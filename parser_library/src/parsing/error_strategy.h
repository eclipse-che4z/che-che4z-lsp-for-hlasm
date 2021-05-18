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

#ifndef HLASMPLUGIN_PARSERLIBRARY_ERROR_STRATEGY_H
#define HLASMPLUGIN_PARSERLIBRARY_ERROR_STRATEGY_H

#include "DefaultErrorStrategy.h"

namespace hlasm_plugin::parser_library::parsing {

enum tokens
{
#include "grammar/lex.tokens"
};

// Overrides default ANTLR error strategy, so it returns our specialized
// tokens instead of ANTLR abstract tokens. The rest of implementation is
// copied from antlr4::DefaultErrorStrategy.
class error_strategy : public antlr4::DefaultErrorStrategy
{
    void reportError(antlr4::Parser* recognizer, const antlr4::RecognitionException& e) override
    {
        if (inErrorRecoveryMode(recognizer))
        {
            return; // don't report spurious errors
        }

        // recovery strategy
        antlr4::misc::IntervalSet endTokens;

        endTokens.addItems(tokens::EOLLN);
        consumeUntil(recognizer, endTokens);

        beginErrorCondition(recognizer);
        if (antlrcpp::is<const antlr4::NoViableAltException*>(&e))
        {
            reportNoViableAlternative(recognizer, static_cast<const antlr4::NoViableAltException&>(e));
        }
        else if (antlrcpp::is<const antlr4::InputMismatchException*>(&e))
        {
            reportInputMismatch(recognizer, static_cast<const antlr4::InputMismatchException&>(e));
        }
        else if (antlrcpp::is<const antlr4::FailedPredicateException*>(&e))
        {
            reportFailedPredicate(recognizer, static_cast<const antlr4::FailedPredicateException&>(e));
        }
        else if (antlrcpp::is<const antlr4::RecognitionException*>(&e))
        {
            recognizer->notifyErrorListeners(e.getOffendingToken(), e.what(), std::current_exception());
        }
    }

    antlr4::Token* getMissingSymbol(antlr4::Parser* recognizer) override
    {
        using namespace antlr4;

        auto* currentSymbol = recognizer->getCurrentToken();
        assert(currentSymbol);

        misc::IntervalSet expecting = getExpectedTokens(recognizer);
        size_t expectedTokenType = expecting.getMinElement(); // get any element
        std::string tokenText;
        if (expectedTokenType == Token::EOF)
        {
            tokenText = "Unexpected end of file";
        }
        else
        {
            tokenText = "<missing " + recognizer->getVocabulary().getDisplayName(expectedTokenType) + ">";
        }
        auto* current = currentSymbol;
        // Get parser_library::token instead of antlr4::Token
        auto* lookback = recognizer->getTokenStream()->LT(-1);
        if (current->getType() == Token::EOF && lookback != nullptr)
        {
            current = lookback;
        }

        auto& lex = dynamic_cast<lexing::lexer&>(*recognizer->getTokenStream()->getTokenSource());
        auto& lexing_token = dynamic_cast<lexing::token&>(*current);
        // return specialized tokens
        _errorSymbols.push_back(lex.get_token_factory()->create(current->getTokenSource(),
            lexing_token.getInputStream(),
            expectedTokenType,
            Token::DEFAULT_CHANNEL,
            INVALID_INDEX,
            INVALID_INDEX,
            lexing_token.getLine(),
            lexing_token.getCharPositionInLine(),
            (size_t)-1,
            lexing_token.get_char_position_in_line_16(),
            lexing_token.get_end_of_token_in_line_utf16()));

        return _errorSymbols.back().get();
    }

    antlr4::Token* singleTokenDeletion(antlr4::Parser*) override { return nullptr; }

    bool singleTokenInsertion(antlr4::Parser*) override { return false; }

    std::vector<std::unique_ptr<antlr4::Token>> _errorSymbols;
};

} // namespace hlasm_plugin::parser_library::parsing


#endif