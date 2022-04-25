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
class error_strategy final : public antlr4::DefaultErrorStrategy
{
    bool m_error_reported = false;
    void reset(antlr4::Parser* recognizer) override
    {
        m_error_reported = false;
        antlr4::DefaultErrorStrategy::reset(recognizer);
    }
    void reportError(antlr4::Parser* recognizer, const antlr4::RecognitionException& e) override
    {
        if (m_error_reported && inErrorRecoveryMode(recognizer))
        {
            return; // don't report spurious errors
        }
        m_error_reported = true;

        // recovery strategy
        antlr4::misc::IntervalSet endTokens;

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

    antlr4::Token* getMissingSymbol(antlr4::Parser*) override
    {
        assert(false);
        throw std::logic_error("unreachable - singleTokenInsertion returns false");
    }

    antlr4::Token* singleTokenDeletion(antlr4::Parser*) override { return nullptr; }

    bool singleTokenInsertion(antlr4::Parser*) override { return false; }

public:
    bool error_reported() const { return m_error_reported; }
};

} // namespace hlasm_plugin::parser_library::parsing


#endif