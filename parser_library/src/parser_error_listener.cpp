#include "parser_error_listener.h"

namespace hlasm_plugin::parser_library {

parser_error_listener::parser_error_listener(std::string file_name) : file_name_(std::move(file_name)) {}


void parser_error_listener::collect_diags() const
{

}

void parser_error_listener::syntaxError(antlr4::Recognizer *, antlr4::Token *, size_t line, size_t char_pos_in_line, const std::string & msg, std::exception_ptr )
{
	add_diagnostic(diagnostic_s(file_name_, { {line, char_pos_in_line}, {line, char_pos_in_line} }, diagnostic_severity::error, "S0001", "HLASM plugin", msg, {}));
}

void parser_error_listener::reportAmbiguity(antlr4::Parser * , const antlr4::dfa::DFA & , size_t , size_t , bool , const antlrcpp::BitSet & , antlr4::atn::ATNConfigSet * )
{
}

void parser_error_listener::reportAttemptingFullContext(antlr4::Parser * , const antlr4::dfa::DFA & , size_t , size_t , const antlrcpp::BitSet & , antlr4::atn::ATNConfigSet * )
{
}

void parser_error_listener::reportContextSensitivity(antlr4::Parser * , const antlr4::dfa::DFA & , size_t , size_t , size_t , antlr4::atn::ATNConfigSet * )
{
}

}
