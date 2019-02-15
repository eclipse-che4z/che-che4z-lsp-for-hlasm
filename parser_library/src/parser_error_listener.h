#ifndef HLASMPLUGIN_PARSERLIBRARY_ERROR_LISTENER_H
#define HLASMPLUGIN_PARSERLIBRARY_ERROR_LISTENER_H

#include "antlr4-runtime.h"
#include "diagnosable_impl.h"

namespace hlasm_plugin::parser_library {

class parser_error_listener : public antlr4::ANTLRErrorListener, public diagnosable_impl
{
	virtual void collect_diags() const override;
	
	virtual void syntaxError(antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol, size_t line,
		size_t charPositionInLine, const std::string &msg, std::exception_ptr e) override;

	virtual void reportAmbiguity(antlr4::Parser *recognizer, const antlr4::dfa::DFA &dfa, size_t startIndex, size_t stopIndex, bool exact,
		const antlrcpp::BitSet &ambigAlts, antlr4::atn::ATNConfigSet *configs) override;

	virtual void reportAttemptingFullContext(antlr4::Parser *recognizer, const antlr4::dfa::DFA &dfa, size_t startIndex, size_t stopIndex,
		const antlrcpp::BitSet &conflictingAlts, antlr4::atn::ATNConfigSet *configs) override;

	virtual void reportContextSensitivity(antlr4::Parser *recognizer, const antlr4::dfa::DFA &dfa, size_t startIndex, size_t stopIndex,
		size_t prediction, antlr4::atn::ATNConfigSet *configs) override;


};

}

#endif // !HLASMPLUGIN_PARSERLIBRARY_ERROR_STRATEGY_H
