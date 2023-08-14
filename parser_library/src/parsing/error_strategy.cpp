/*
 * Copyright (c) 2023 Broadcom.
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

#include "error_strategy.h"

#include <cstdlib>

#include "antlr4-runtime.h"

namespace hlasm_plugin::parser_library::parsing {

void error_strategy::reset(antlr4::Parser* recognizer)
{
    m_error_reported = false;
    antlr4::DefaultErrorStrategy::reset(recognizer);
}

void error_strategy::reportError(antlr4::Parser* recognizer, const antlr4::RecognitionException& e)
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

antlr4::Token* error_strategy::getMissingSymbol(antlr4::Parser*)
{
    assert(false);
    std::abort();
}

} // namespace hlasm_plugin::parser_library::parsing
