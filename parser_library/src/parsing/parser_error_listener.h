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

#ifndef HLASMPLUGIN_PARSERLIBRARY_ERROR_LISTENER_H
#define HLASMPLUGIN_PARSERLIBRARY_ERROR_LISTENER_H

#include "ANTLRErrorListener.h"

#include "diagnostic.h"
#include "diagnostic_consumer.h"
#include "semantics/range_provider.h"

// Override antlr Error listener for more polished output
// parser. Provides more readable syntax errors.

namespace hlasm_plugin::parser_library::parsing {

class parser_error_listener_base : public antlr4::ANTLRErrorListener
{
public:
    void syntaxError(antlr4::Recognizer* recognizer,
        antlr4::Token* offendingSymbol,
        size_t line,
        size_t charPositionInLine,
        const std::string& msg,
        std::exception_ptr e) override;

    void reportAmbiguity(antlr4::Parser* recognizer,
        const antlr4::dfa::DFA& dfa,
        size_t startIndex,
        size_t stopIndex,
        bool exact,
        const antlrcpp::BitSet& ambigAlts,
        antlr4::atn::ATNConfigSet* configs) override;

    void reportAttemptingFullContext(antlr4::Parser* recognizer,
        const antlr4::dfa::DFA& dfa,
        size_t startIndex,
        size_t stopIndex,
        const antlrcpp::BitSet& conflictingAlts,
        antlr4::atn::ATNConfigSet* configs) override;

    void reportContextSensitivity(antlr4::Parser* recognizer,
        const antlr4::dfa::DFA& dfa,
        size_t startIndex,
        size_t stopIndex,
        size_t prediction,
        antlr4::atn::ATNConfigSet* configs) override;

protected:
    virtual void add_parser_diagnostic(diagnostic_op (&diag_op)(const range&), range r) = 0;
};

class parser_error_listener final : public parser_error_listener_base
{
    const semantics::range_provider* provider = nullptr;

public:
    parser_error_listener() = default;
    explicit parser_error_listener(const semantics::range_provider* prov)
        : provider(prov) {};

    diagnostic_op_consumer* diagnoser = nullptr;

protected:
    void add_parser_diagnostic(diagnostic_op (&diag_op)(const range&), range r) override
    {
        if (diagnoser)
            diagnoser->add_diagnostic(diag_op(provider ? provider->adjust_range(std::move(r)) : r));
    }
};

} // namespace hlasm_plugin::parser_library::parsing

#endif // !HLASMPLUGIN_PARSERLIBRARY_ERROR_STRATEGY_H
