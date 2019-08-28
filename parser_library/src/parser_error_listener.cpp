#include "parser_error_listener.h"
#include "../include/shared/token_stream.h"

enum Tokens {
#include "grammar/lex.tokens"
};

namespace hlasm_plugin::parser_library {

parser_error_listener::parser_error_listener(std::string file_name) : file_name_(std::move(file_name)) {}
	



void parser_error_listener::collect_diags() const
{

}

bool is_comparative_sign(size_t input)
{
	return (input == LT || input == GT || input == EQUALS || input == EQ || input == OR
		|| input == AND || input == LE || input == LTx || input == GTx || input == GE || input == NE
		);
}

bool is_sign(size_t input)
{
	return (input == ASTERISK || input == MINUS || input == PLUS
		|| is_comparative_sign(input) || input == SLASH
		);
}

// return last symbol before eolln in line
int get_end_index(antlr4::TokenStream * input_stream, int start)
{
	while (start < (int) input_stream->size())
	{
		if (input_stream->get(start)->getType() == EOLLN)
			return start - 1;
		start++;
	}
	return -1;
}

bool can_follow_sign(size_t input)
{
	return (input == IDENTIFIER || input == ORDSYMBOL || input == AMPERSAND || input == LPAR
		|| input == CONTINUATION || input == COMMENT);
}

bool can_be_before_sign(size_t input)
{
	return (input == IDENTIFIER || input == ORDSYMBOL || input == AMPERSAND || input == RPAR
		|| input == CONTINUATION || input == COMMENT);
}

void iterate_error_stream(antlr4::TokenStream * input_stream, int start, int end,
	bool & right_prec, bool & only_par, bool & left_prec, bool & sign_followed, bool &sign_preceding,
	bool & unexpected_sign, bool & odd_apostrophes, bool & ampersand_followed)
{
	int parenthesis = 0;
	int apostrophes = 0;
	for (int i = start; i <= end; i++)
	{
		auto type = input_stream->get(i)->getType();
		if (type == LPAR)
			parenthesis--;
		else if (type == RPAR)
			parenthesis++;
		else
		{
			only_par = false;
			if ((is_sign(type) || type == AMPERSAND)
				&& (i == end || (i < end && !can_follow_sign(input_stream->get(i + 1)->getType()))))
			{
				if (is_sign(type))
					sign_followed = false;
				if (type == AMPERSAND)
					ampersand_followed = false;
			}
			if ((is_sign(type) && type != PLUS && type != MINUS)
				&& (i == start || (i != start && !can_be_before_sign(input_stream->get(i - 1)->getType()))))
				sign_preceding = false;
			if (is_comparative_sign(type))
				unexpected_sign = true;
			if (type == APOSTROPHE)
				apostrophes++;
		}
		// if there is right bracket preceding left bracket
		if (parenthesis > 0)
			right_prec = true;
	}
	if (apostrophes % 2 == 1)
		odd_apostrophes = true;
	if (parenthesis < 0)
		left_prec = true;
}

bool is_expected(int exp_token, antlr4::misc::IntervalSet expectedTokens)
{
	return expectedTokens.contains(static_cast<size_t> (exp_token));
}

void parser_error_listener::syntaxError(antlr4::Recognizer *, antlr4::Token * , size_t line, size_t char_pos_in_line, const std::string & , std::exception_ptr e)
{
	try
	{	
		if (e)
			std::rethrow_exception(e);
	}
	catch (antlr4::NoViableAltException & excp)
	{
		auto input_stream = dynamic_cast<token_stream *> (excp.getInputStream());

		auto expected_tokens = excp.getExpectedTokens();

		auto start_token = excp.getStartToken();
		int start_index = (int)start_token->getTokenIndex();

		// find first eoln

		auto first_symbol_type = input_stream->get(start_index)->getType();
		
		// while it's a space, skip spaces
		while (first_symbol_type == SPACE)
		{
			start_index++;
			first_symbol_type = input_stream->get(start_index)->getType();
		}

		auto end_index = get_end_index(input_stream, start_index);

		// no eolln, end index at last index of the stream
		if (end_index == -1)
		{
			end_index = (int)input_stream->size() - 1;
			//add_diagnostic(diagnostic_s(file_name_, range(position(line,char_pos_in_line)), diagnostic_severity::error, "S0004", "HLASM plugin", "NO EOLLN - TO DO", {}));
			//return;
		}

		bool sign_followed = true;
		bool sign_preceding = true;
		bool only_par = true; 
		bool right_prec = false;
		bool left_prec = false;
		bool unexpected_sign = false;
		bool odd_apostrophes = false;
		bool ampersand_followed = true;

		iterate_error_stream(input_stream, start_index, end_index,
			right_prec, only_par, left_prec, sign_followed, sign_preceding, unexpected_sign, odd_apostrophes, ampersand_followed);

		// right paranthesis has no left match
		if (right_prec)
			add_diagnostic(diagnostic_s(file_name_, range(position(line,char_pos_in_line)), diagnostic_severity::error, "S0012", "HLASM plugin", "Right parenthesis has no left match", {}));
		// left paranthesis has no right match
		else if (left_prec)
			add_diagnostic(diagnostic_s(file_name_, range(position(line,char_pos_in_line)), diagnostic_severity::error, "S0011", "HLASM plugin", "Left parenthesis has no right match", {}));
		// nothing else but left and right parenthesis is present
		else if (only_par)
			add_diagnostic(diagnostic_s(file_name_, range(position(line,char_pos_in_line)), diagnostic_severity::error, "S0010", "HLASM plugin", "Only left and right paranthesis present", {}));
		// sign followed by a wrong token
		else if (!sign_followed)
			add_diagnostic(diagnostic_s(file_name_, range(position(line,char_pos_in_line)), diagnostic_severity::error, "S0009", "HLASM plugin", "A sign has to be followed by an expression", {}));
		// ampersand not followed with a name of a variable symbol
		else if (!ampersand_followed)
			add_diagnostic(diagnostic_s(file_name_, range(position(line,char_pos_in_line)), diagnostic_severity::error, "S0008", "HLASM plugin", "Ampersand has to be followed by a name of a variable", {}));
		// expression starting with a sign
		else if (!sign_preceding)
			add_diagnostic(diagnostic_s(file_name_, range(position(line,char_pos_in_line)), diagnostic_severity::error, "S0007", "HLASM plugin", "A sign needs to be preceded by an expression", {}));
		// unexpected sign in an expression - GT, LT etc
		else if (unexpected_sign)
			add_diagnostic(diagnostic_s(file_name_, range(position(line,char_pos_in_line)), diagnostic_severity::error, "S0006", "HLASM plugin", "Unexpected sign in an epxression", {}));
		// apostrophe expected  
		else if (odd_apostrophes && is_expected(APOSTROPHE, expected_tokens))
			add_diagnostic(diagnostic_s(file_name_, range(position(line,char_pos_in_line)), diagnostic_severity::error, "S0005", "HLASM plugin", "Expected an apostrophe", {}));
		// unfinished statement - solo label on line
		else if (start_token->getCharPositionInLine() == 0)
			add_diagnostic(diagnostic_s(file_name_, range(position(line,char_pos_in_line)), diagnostic_severity::error, "S0004", "HLASM plugin", "Unfinished statement, the label cannot be alone on a line", {}));
		// other undeclared errors
		else
			add_diagnostic(diagnostic_s(file_name_, range(position(line,char_pos_in_line)), diagnostic_severity::error, "S0002", "HLASM plugin", "Syntax error", {}));
	}
	catch (antlr4::InputMismatchException & excp)
	{
		auto offender = excp.getOffendingToken();

		if (offender->getType() == EOLLN)
			add_diagnostic(diagnostic_s(file_name_, range(position(line,char_pos_in_line)), diagnostic_severity::error, "S0003", "HLASM plugin", "Unexpected end of statement", {}));
		else
			add_diagnostic(diagnostic_s(file_name_, range(position(line,char_pos_in_line)), diagnostic_severity::error, "S0002", "HLASM plugin", "Syntax error", {}));
	}
	catch (...)
	{
		add_diagnostic(diagnostic_s(file_name_, range(position(line,char_pos_in_line)), diagnostic_severity::error, "S0001", "HLASM plugin", "C++ error", {}));
	}
}

void parser_error_listener::reportAmbiguity(antlr4::Parser *, const antlr4::dfa::DFA &, size_t, size_t, bool, const antlrcpp::BitSet &, antlr4::atn::ATNConfigSet *)
{
}

void parser_error_listener::reportAttemptingFullContext(antlr4::Parser *, const antlr4::dfa::DFA &, size_t, size_t, const antlrcpp::BitSet &, antlr4::atn::ATNConfigSet *)
{
}

void parser_error_listener::reportContextSensitivity(antlr4::Parser *, const antlr4::dfa::DFA &, size_t, size_t, size_t, antlr4::atn::ATNConfigSet *)
{
}

}
