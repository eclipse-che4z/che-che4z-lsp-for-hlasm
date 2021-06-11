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

#include "parser_error_listener.h"

#include "lexing/token_stream.h"

enum Tokens
{
#include "grammar/lex.tokens"
};

using namespace hlasm_plugin::parser_library::lexing;

namespace hlasm_plugin::parser_library::parsing {

bool is_comparative_sign(size_t input)
{
    return (input == LT || input == GT || input == EQUALS || input == EQ || input == OR || input == AND || input == LE
        || input == LTx || input == GTx || input == GE || input == NE);
}

bool is_sign(size_t input)
{
    return (input == ASTERISK || input == MINUS || input == PLUS || is_comparative_sign(input) || input == SLASH);
}

// return last symbol before eolln in line
int get_end_index(antlr4::TokenStream* input_stream, int start)
{
    if (start < (int)input_stream->size())
    {
        return (int)input_stream->size() - 1;
    }
    return -1;
}

bool can_follow_sign(size_t input)
{
    return (input == IDENTIFIER || input == ORDSYMBOL || input == AMPERSAND || input == LPAR || input == CONTINUATION
        || input == COMMENT);
}

bool can_be_before_sign(size_t input)
{
    return (input == IDENTIFIER || input == ORDSYMBOL || input == AMPERSAND || input == RPAR || input == CONTINUATION
        || input == COMMENT);
}

void iterate_error_stream(antlr4::TokenStream* input_stream,
    int start,
    int end,
    bool& right_prec,
    bool& only_par,
    bool& left_prec,
    bool& sign_followed,
    bool& sign_preceding,
    bool& unexpected_sign,
    bool& odd_apostrophes,
    bool& ampersand_followed)
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
    return expectedTokens.contains(static_cast<size_t>(exp_token));
}

void parser_error_listener_base::syntaxError(
    antlr4::Recognizer*, antlr4::Token*, size_t line, size_t char_pos_in_line, const std::string&, std::exception_ptr e)
{
    try
    {
        if (e)
            std::rethrow_exception(e);
    }
    catch (antlr4::NoViableAltException& excp)
    {
        auto input_stream = dynamic_cast<token_stream*>(excp.getInputStream());

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
            // add_parser_diagnostic(range(position(line,char_pos_in_line)), diagnostic_severity::error, "S0004", "HLASM
            // plugin", "NO EOLLN - TO DO"); return;
        }

        bool sign_followed = true;
        bool sign_preceding = true;
        bool only_par = true;
        bool right_prec = false;
        bool left_prec = false;
        bool unexpected_sign = false;
        bool odd_apostrophes = false;
        bool ampersand_followed = true;

        iterate_error_stream(input_stream,
            start_index,
            end_index,
            right_prec,
            only_par,
            left_prec,
            sign_followed,
            sign_preceding,
            unexpected_sign,
            odd_apostrophes,
            ampersand_followed);

        // right paranthesis has no left match
        if (right_prec)
            add_parser_diagnostic(range(position(line, char_pos_in_line)),
                diagnostic_severity::error,
                "S0012",
                "Right parenthesis has no left match");
        // left paranthesis has no right match
        else if (left_prec)
            add_parser_diagnostic(range(position(line, char_pos_in_line)),
                diagnostic_severity::error,
                "S0011",
                "Left parenthesis has no right match");
        // nothing else but left and right parenthesis is present
        else if (only_par)
            add_parser_diagnostic(range(position(line, char_pos_in_line)),
                diagnostic_severity::error,
                "S0010",
                "Only left and right paranthesis present");
        // sign followed by a wrong token
        else if (!sign_followed)
            add_parser_diagnostic(range(position(line, char_pos_in_line)),
                diagnostic_severity::error,
                "S0009",
                "A sign has to be followed by an expression");
        // ampersand not followed with a name of a variable symbol
        else if (!ampersand_followed)
            add_parser_diagnostic(range(position(line, char_pos_in_line)),
                diagnostic_severity::error,
                "S0008",
                "Ampersand has to be followed by a name of a variable");
        // expression starting with a sign
        else if (!sign_preceding)
            add_parser_diagnostic(range(position(line, char_pos_in_line)),
                diagnostic_severity::error,
                "S0007",
                "A sign needs to be preceded by an expression");
        // unexpected sign in an expression - GT, LT etc
        else if (unexpected_sign)
            add_parser_diagnostic(range(position(line, char_pos_in_line)),
                diagnostic_severity::error,
                "S0006",
                "Unexpected sign in an expression");
        // apostrophe expected
        else if (odd_apostrophes && is_expected(APOSTROPHE, expected_tokens))
            add_parser_diagnostic(
                range(position(line, char_pos_in_line)), diagnostic_severity::error, "S0005", "Expected an apostrophe");
        // unfinished statement - solo label on line
        else if (start_token->getCharPositionInLine() == 0)
            add_parser_diagnostic(range(position(line, char_pos_in_line)),
                diagnostic_severity::error,
                "S0004",
                "Unfinished statement, the label cannot be alone on a line");
        // other undeclared errors
        else
            add_parser_diagnostic(
                range(position(line, char_pos_in_line)), diagnostic_severity::error, "S0002", "Syntax error");
    }
    catch (antlr4::InputMismatchException& excp)
    {
        auto offender = excp.getOffendingToken();

        if (offender->getType() == antlr4::Token::EOF)
            add_parser_diagnostic(range(position(line, char_pos_in_line)),
                diagnostic_severity::error,
                "S0003",
                "Unexpected end of statement");
        else
            add_parser_diagnostic(
                range(position(line, char_pos_in_line)), diagnostic_severity::error, "S0002", "Syntax error");
    }
    catch (...)
    {
        add_parser_diagnostic(
            range(position(line, char_pos_in_line)), diagnostic_severity::error, "S0001", "C++ error");
    }
}

void parser_error_listener_base::reportAmbiguity(
    antlr4::Parser*, const antlr4::dfa::DFA&, size_t, size_t, bool, const antlrcpp::BitSet&, antlr4::atn::ATNConfigSet*)
{}

void parser_error_listener_base::reportAttemptingFullContext(
    antlr4::Parser*, const antlr4::dfa::DFA&, size_t, size_t, const antlrcpp::BitSet&, antlr4::atn::ATNConfigSet*)
{}

void parser_error_listener_base::reportContextSensitivity(
    antlr4::Parser*, const antlr4::dfa::DFA&, size_t, size_t, size_t, antlr4::atn::ATNConfigSet*)
{}


parser_error_listener::parser_error_listener(std::string file_name)
    : file_name_(std::move(file_name))
{}

void parser_error_listener::collect_diags() const {}

void parser_error_listener::add_parser_diagnostic(
    range diagnostic_range, diagnostic_severity severity, std::string code, std::string message)
{
    add_diagnostic(diagnostic_s(file_name_, diagnostic_range, severity, std::move(code), std::move(message), {}));
}

} // namespace hlasm_plugin::parser_library::parsing
