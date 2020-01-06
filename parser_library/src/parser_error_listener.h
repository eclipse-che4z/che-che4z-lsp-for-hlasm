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

#include "antlr4-runtime.h"
#include "diagnosable_ctx.h"

namespace hlasm_plugin::parser_library {

class parser_error_listener_base : public antlr4::ANTLRErrorListener
{
public:
	virtual void syntaxError(antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol, size_t line,
		size_t charPositionInLine, const std::string &msg, std::exception_ptr e) override;

	virtual void reportAmbiguity(antlr4::Parser *recognizer, const antlr4::dfa::DFA &dfa, size_t startIndex, size_t stopIndex, bool exact,
		const antlrcpp::BitSet &ambigAlts, antlr4::atn::ATNConfigSet *configs) override;

	virtual void reportAttemptingFullContext(antlr4::Parser *recognizer, const antlr4::dfa::DFA &dfa, size_t startIndex, size_t stopIndex,
		const antlrcpp::BitSet &conflictingAlts, antlr4::atn::ATNConfigSet *configs) override;

	virtual void reportContextSensitivity(antlr4::Parser *recognizer, const antlr4::dfa::DFA &dfa, size_t startIndex, size_t stopIndex,
		size_t prediction, antlr4::atn::ATNConfigSet *configs) override;

protected:
	virtual void add_parser_diagnostic(range diagnostic_range, diagnostic_severity severity, std::string code, std::string message) = 0;

};

class parser_error_listener : public parser_error_listener_base, public diagnosable_impl
{
public:
	parser_error_listener(std::string file_name);

	virtual void collect_diags() const override;

protected:
	virtual void add_parser_diagnostic(range diagnostic_range, diagnostic_severity severity, std::string code, std::string message) override;

private:
	std::string file_name_;
};

}

#endif // !HLASMPLUGIN_PARSERLIBRARY_ERROR_STRATEGY_H
