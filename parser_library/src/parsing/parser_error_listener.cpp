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

#include "antlr4-runtime.h"

#include "lexing/token_stream.h"
#include "parser_impl.h"

enum Tokens
{
#include "grammar/lex.tokens"
};

using namespace hlasm_plugin::parser_library::lexing;

namespace hlasm_plugin::parser_library::parsing {
namespace {
bool is_comparative_sign(size_t input) { return input == LT || input == GT || input == EQUALS; }

bool is_sign(size_t input)
{
    return input == ASTERISK || input == MINUS || input == PLUS || is_comparative_sign(input) || input == SLASH;
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
    return input == IDENTIFIER || input == ORDSYMBOL || input == AMPERSAND || input == LPAR || input == CONTINUATION;
}

bool can_be_before_sign(size_t input)
{
    return input == IDENTIFIER || input == ORDSYMBOL || input == AMPERSAND || input == RPAR || input == CONTINUATION;
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
            if (type == AMPERSAND && i < end && input_stream->get(i + 1)->getType() == AMPERSAND)
            {
                i += 1;
                continue;
            }
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
            if (type == APOSTROPHE
                || (type == ATTR && (i + 1 <= end && input_stream->get(i + 1)->getType() == AMPERSAND))
                || (type == ATTR
                    && (!parser_impl::is_attribute_consuming(input_stream->get(i - 1))
                        || (i + 1 <= end && input_stream->get(i + 1)->getType() != antlr4::Token::EOF
                            && !parser_impl::can_attribute_consume(input_stream->get(i + 1))))))
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

int get_alternative_start_index(const antlr4::ParserRuleContext* ctx)
{
    while (ctx)
    {
        auto first = ctx->getStart();
        if (!first)
            break;

        if (first->getType() == antlr4::Token::EOF)
        {
            ctx = dynamic_cast<const antlr4::ParserRuleContext*>(ctx->parent);
            continue;
        }
        return (int)first->getTokenIndex();
    }
    return -1;
}

std::pair<int, int> get_start_end_indices(
    token_stream* input_stream, const antlr4::RuleContext* ctx, const antlr4::Token* start_token)
{
    int start_index = (int)start_token->getTokenIndex();

    auto alternate_start_index = get_alternative_start_index(dynamic_cast<const antlr4::ParserRuleContext*>(ctx));

    // find first eoln

    if (alternate_start_index != -1 && alternate_start_index < start_index)
    {
        start_index = alternate_start_index;
    }

    auto first_symbol_type = input_stream->get(start_index)->getType();

    // while it's a space, skip spaces
    while (first_symbol_type == SPACE)
    {
        start_index++;
        first_symbol_type = input_stream->get(start_index)->getType();
    }

    auto end_index = get_end_index(input_stream, start_index);

    // end index at last index of the stream
    if (end_index == -1)
    {
        end_index = (int)input_stream->size() - 1;
    }

    return { start_index, end_index };
}

} // namespace


void parser_error_listener_base::handle_error(token_stream* input_stream,
    int start_index,
    int end_index,
    size_t line,
    size_t char_pos_in_line,
    const antlr4::Token* start_token,
    const antlr4::misc::IntervalSet& expected_tokens)
{
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

    // ampersand not followed with a name of a variable symbol
    if (!ampersand_followed)
        add_parser_diagnostic(diagnostic_op::error_S0008, range(position(line, char_pos_in_line)));
    // apostrophe expected
    else if (odd_apostrophes && expected_tokens.contains((size_t)APOSTROPHE))
        add_parser_diagnostic(diagnostic_op::error_S0005, range(position(line, char_pos_in_line)));
    // right parenthesis has no left match
    else if (right_prec)
        add_parser_diagnostic(diagnostic_op::error_S0012, range(position(line, char_pos_in_line)));
    // left parenthesis has no right match
    else if (left_prec)
        add_parser_diagnostic(diagnostic_op::error_S0011, range(position(line, char_pos_in_line)));
    // nothing else but left and right parenthesis is present
    else if (only_par)
        add_parser_diagnostic(diagnostic_op::error_S0010, range(position(line, char_pos_in_line)));
    // sign followed by a wrong token
    else if (!sign_followed)
        add_parser_diagnostic(diagnostic_op::error_S0009, range(position(line, char_pos_in_line)));
    // expression starting with a sign
    else if (!sign_preceding)
        add_parser_diagnostic(diagnostic_op::error_S0007, range(position(line, char_pos_in_line)));
    // unexpected sign in an expression - GT, LT etc
    else if (unexpected_sign)
        add_parser_diagnostic(diagnostic_op::error_S0006, range(position(line, char_pos_in_line)));
    // unfinished statement - solo label on line
    else if (start_token->getCharPositionInLine() == 0)
        add_parser_diagnostic(diagnostic_op::error_S0004, range(position(line, char_pos_in_line)));
    // EOF errors
    else if (start_token->getType() == antlr4::Token::EOF)
        add_parser_diagnostic(diagnostic_op::error_S0003, range(position(line, char_pos_in_line)));
    // other undeclared errors
    else
        add_parser_diagnostic(diagnostic_op::error_S0002, range(position(line, char_pos_in_line)));
}

void parser_error_listener_base::syntaxError(antlr4::Recognizer*,
    antlr4::Token* t,
    size_t line,
    size_t char_pos_in_line,
    const std::string&,
    std::exception_ptr e)
{
    try
    {
        if (e)
            std::rethrow_exception(std::move(e));
    }
    catch (antlr4::NoViableAltException& excp)
    {
        auto input_stream = dynamic_cast<token_stream*>(excp.getInputStream());

        auto start_token = excp.getStartToken();

        auto [start_index, end_index] = get_start_end_indices(input_stream, excp.getCtx(), start_token);

        handle_error(
            input_stream, start_index, end_index, line, char_pos_in_line, start_token, excp.getExpectedTokens());
    }
    catch (antlr4::RecognitionException& excp)
    {
        auto input_stream = dynamic_cast<token_stream*>(excp.getInputStream());

        auto [start_index, end_index] = get_start_end_indices(input_stream, excp.getCtx(), t);

        handle_error(input_stream, start_index, end_index, line, char_pos_in_line, t, excp.getExpectedTokens());
    }
    catch (...)
    {
        add_parser_diagnostic(diagnostic_op::error_S0001, range(position(line, char_pos_in_line)));
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

} // namespace hlasm_plugin::parser_library::parsing
