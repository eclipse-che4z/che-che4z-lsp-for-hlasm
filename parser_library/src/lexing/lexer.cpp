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

#include "lexer.h"

#include <algorithm>
#include <assert.h>
#include <cctype>
#include <string>
#include <utility>

using namespace antlr4;
using namespace std;

using namespace hlasm_plugin;
using namespace parser_library;
using namespace lexing;

lexer::lexer(input_source* input, semantics::source_info_processor* lsp_proc)
    : input_(input)
    , src_proc_(lsp_proc)
{
    factory_ = std::make_unique<token_factory>();

    file_input_state_.input = input;

    // initialize IS with first character from input
    input_state_->c = static_cast<char_t>(input_->LA(1));

    dummy_factory = make_shared<antlr4::CommonTokenFactory>();
}

void lexer::set_unlimited_line(bool unlimited_lines) { unlimited_line_ = unlimited_lines; }
void lexer::set_file_offset(position file_offset, bool process_allowed)
{
    input_state_->line = (size_t)file_offset.line;
    input_state_->char_position_in_line = (size_t)file_offset.column;
    input_state_->char_position_in_line_utf16 = (size_t)file_offset.column;
    process_allowed_ = process_allowed;
}

void lexer::reset()
{
    token_queue_ = {};
    last_token_id_ = 0;
    input_state_->char_position = 0;
    file_input_state_.c = static_cast<char_t>(input_->LA(1));
}


size_t lexer::getLine() const { return input_state_->line; }

size_t lexer::getCharPositionInLine() { return input_state_->char_position_in_line; }

antlr4::CharStream* lexer::getInputStream() { return input_; }

std::string lexer::getSourceName() { return input_->getSourceName(); }

void lexer::create_token(size_t ttype, size_t channel = Channels::DEFAULT_CHANNEL)
{
    /* do not generate empty tokens (except EOF) */
    if (token_start_state_.char_position == token_start_state_.input->index() && ttype != Token::EOF)
        return;

    creating_var_symbol_ = ttype == AMPERSAND;
    if (creating_attr_ref_)
        creating_attr_ref_ = ttype == IGNORED || ttype == CONTINUATION;

    last_token_id_++;

    token_queue_.push(factory_->create(this,
        token_start_state_.input,
        ttype,
        channel,
        token_start_state_.char_position,
        input_state_->char_position - 1,
        token_start_state_.line,
        token_start_state_.char_position_in_line,
        last_token_id_ - 1,
        token_start_state_.char_position_in_line_utf16,
        input_state_->char_position_in_line_utf16));

    auto stop_position_in_line = last_char_utf16_long_ ? input_state_->char_position_in_line_utf16 - 1
                                                       : input_state_->char_position_in_line_utf16;

    if (src_proc_)
        switch (ttype)
        {
            case CONTINUATION:
                src_proc_->add_hl_symbol(
                    token_info(range(position(token_start_state_.line, token_start_state_.char_position_in_line_utf16),
                                   position(input_state_->line, stop_position_in_line)),
                        semantics::hl_scopes::continuation));
                break;
            case IGNORED: {
                auto line_pos =
                    (token_start_state_.line != input_state_->line) ? last_line_pos_ : stop_position_in_line;
                src_proc_->add_hl_symbol(
                    token_info(range(position(token_start_state_.line, token_start_state_.char_position_in_line_utf16),
                                   position(token_start_state_.line, line_pos)),
                        semantics::hl_scopes::ignored));
            }
            break;
            case COMMENT:
                src_proc_->add_hl_symbol(
                    token_info(range(position(token_start_state_.line, token_start_state_.char_position_in_line_utf16),
                                   position(input_state_->line, stop_position_in_line)),
                        semantics::hl_scopes::comment));
                break;
        }
}

void lexer::consume()
{
    if (input_state_->c == '\n')
    {
        input_state_->line++;
        input_state_->char_position_in_line = static_cast<size_t>(-1);
        input_state_->char_position_in_line_utf16 = static_cast<size_t>(-1);
    }

    if (input_state_->c != static_cast<char_t>(-1))
    {
        input_state_->input->consume();
        input_state_->char_position++;
        input_state_->c = static_cast<char_t>(input_state_->input->LA(1));

        input_state_->char_position_in_line++;
        if (input_state_->c == static_cast<char32_t>(-1))
        {
            last_char_utf16_long_ = false;
            input_state_->char_position_in_line_utf16 += 1;
        }
        else if (input_state_->c > 0xFFFF)
        {
            last_char_utf16_long_ = true;
            input_state_->char_position_in_line_utf16 += 2;
        }
        else
        {
            last_char_utf16_long_ = false;
            input_state_->char_position_in_line_utf16++;
        }
    }
}

bool lexer::eof() const { return input_->LA(1) == CharStream::EOF; }

/* set start token info */
void lexer::start_token() { token_start_state_ = *input_state_; }

bool isspace32(char_t c) { return c <= 255 && isspace((unsigned char)c); }

/*
main logic
returns already lexed token from token queue
or if empty, tries to lex next token from input
*/
token_ptr lexer::nextToken()
{
    while (true)
    {
        if (!token_queue_.empty())
        {
            auto t = move(token_queue_.front());
            token_queue_.pop();
            return t;
        }

        // capture lexer state befor the start of token lexing
        // so that we know where the currently lexed token begins
        start_token();

        if (eof())
        {
            create_token(Token::EOF);
        }

        else if (double_byte_enabled_)
            check_continuation();

        else if (!unlimited_line_ && input_state_->char_position_in_line == end_ && !isspace32(input_state_->c)
            && continuation_enabled_)
            lex_continuation();

        else if ((unlimited_line_ && (input_state_->c == '\r' || input_state_->c == '\n'))
            || (!unlimited_line_ && input_state_->char_position_in_line >= end_))
            lex_end();

        else if (input_state_->char_position_in_line < begin_)
            lex_begin();

        else
            // lex non-special tokens
            lex_tokens();
    }
}

void lexer::lex_tokens()
{
    switch (input_state_->c)
    {
        case '*':
            if (input_state_->char_position_in_line == begin_)
            {
                if (is_process())
                {
                    lex_process();
                    break;
                }
                lex_comment();
            }
            else
            {
                consume();
                create_token(ASTERISK);
            }
            break;

        case '.':
            /* macro comment */
            if (input_state_->char_position_in_line == begin_ && input_->LA(2) == '*')
            {
                lex_comment();
            }
            else
            {
                consume();
                create_token(DOT);
            }
            break;

        case ' ':
            lex_space();
            break;

        case '-':
            consume();
            create_token(MINUS);
            break;

        case '+':
            consume();
            create_token(PLUS);
            break;

        case '=':
            consume();
            create_token(EQUALS);
            break;

        case '<':
            consume();
            create_token(LT);
            break;

        case '>':
            consume();
            create_token(GT);
            break;

        case ',':
            consume();
            create_token(COMMA);
            break;

        case '(':
            consume();
            create_token(LPAR);
            break;

        case ')':
            consume();
            create_token(RPAR);
            break;

        case '\'':
            consume();
            if (!creating_attr_ref_)
            {
                create_token(APOSTROPHE);
            }
            else
                create_token(ATTR);
            break;

        case '/':
            consume();
            create_token(SLASH);
            break;

        case '&':
            consume();
            create_token(AMPERSAND);
            break;

        case '\r':
            consume();
            if (input_state_->c == '\n')
                consume();
            break;
        case '\n':
            consume();
            break;

        case '|':
            consume();
            create_token(VERTICAL);
            break;

        default:
            lex_word();
            break;
    }
}

bool lexer::identifier_divider() const
{
    switch (input_state_->c)
    {
        case '*':
        case '.':
        case '-':
        case '+':
        case '=':
        case '<':
        case '>':
        case ',':
        case '(':
        case ')':
        case '\'':
        case '/':
        case '&':
        case '|':
            return true;
        default:
            return false;
    }
}

void lexer::lex_begin()
{
    start_token();
    while (input_state_->char_position_in_line < begin_ && !eof() && input_state_->c != '\n')
        consume();
    create_token(IGNORED, HIDDEN_CHANNEL);
}

void lexer::lex_end()
{
    start_token();
    while (input_state_->c != '\n' && !eof() && input_state_->c != (char_t)-1)
        consume();

    if (!eof())
    {
        last_line_pos_ = input_state_->char_position_in_line;
        consume();
    }
    if (double_byte_enabled_)
        check_continuation();
    create_token(IGNORED, HIDDEN_CHANNEL);
}

void lexer::lex_comment()
{
    while (true)
    {
        start_token();
        while (input_state_->char_position_in_line < end_ && !eof() && input_state_->c != '\n')
            consume();
        create_token(COMMENT, HIDDEN_CHANNEL);

        if (!isspace32(input_state_->c) && !eof() && continuation_enabled_)
            lex_continuation();
        else
        {
            lex_end();

            break;
        }
    }
}

void lexer::consume_new_line()
{
    // we accept both separately and combine
    if (input_state_->c == '\r')
        consume();
    if (input_state_->c == '\n')
        consume();
}

/* lex continuation and ignores */
void lexer::lex_continuation()
{
    start_token();

    /* lex continuation */
    while (input_state_->char_position_in_line <= end_default_ && !eof())
        consume();

    /* reset END */
    end_ = end_default_;

    create_token(CONTINUATION, HIDDEN_CHANNEL);

    lex_end();
    lex_begin();

    /* lex continuation */
    start_token();
    while (input_state_->char_position_in_line < continue_ && !eof() && input_state_->c != '\n')
        consume();
    create_token(IGNORED, HIDDEN_CHANNEL);
}

/* if DOUBLE_BYTE_ENABLED check start of continuation for current line */
void lexer::check_continuation()
{
    end_ = end_default_;

    auto cc = input_->LA(end_default_ + 1);
    if (cc != CharStream::EOF && !isspace32(static_cast<char_t>(cc)))
    {
        do
        {
            if (input_->LA(end_) != cc)
                break;
            end_--;
        } while (end_ > begin_);
    }
}

void lexer::lex_space()
{
    while (input_state_->c == ' ' && before_end() && !eof())
        consume();
    create_token(SPACE, DEFAULT_CHANNEL);
}

bool lexer::before_end() const
{
    return input_state_->char_position_in_line < end_
        || (unlimited_line_ && input_state_->c != '\r' && input_state_->c != '\n');
}

bool lexer::ord_char(char_t c)
{
    return (c <= 255) && (isalnum(c) || isalpha(c) || c == '_' || c == '@' || c == '$' || c == '#');
}

bool lexer::is_ord_char() const { return ord_char(input_state_->c); }

bool lexer::is_space() const { return input_state_->c == ' ' || input_state_->c == '\n' || input_state_->c == '\r'; }

bool lexer::is_data_attribute() const
{
    auto tmp = std::toupper(input_state_->c);
    return tmp == 'D' || tmp == 'O' || tmp == 'N' || tmp == 'S' || tmp == 'K' || tmp == 'I' || tmp == 'L' || tmp == 'T';
}

void lexer::lex_word()
{
    bool last_char_data_attr = false;
    bool ord = is_ord_char() && (input_state_->c < '0' || input_state_->c > '9');
    bool num = (input_state_->c >= '0' && input_state_->c <= '9');

    size_t w_len = 0;
    while (!is_space() && !eof() && !identifier_divider() && before_end())
    {
        ord &= is_ord_char();
        num &= (input_state_->c >= '0' && input_state_->c <= '9');
        last_char_data_attr = is_data_attribute();

        if (creating_var_symbol_ && !ord && w_len > 0 && w_len <= 63)
        {
            create_token(ORDSYMBOL);
            return;
        }

        consume();
        ++w_len;
    }

    bool var_sym_tmp = creating_var_symbol_;

    if (ord && w_len <= 63)
        create_token(ORDSYMBOL);
    else if (num)
        create_token(NUM);
    else
        create_token(IDENTIFIER);

    if (input_state_->c == '\'' && last_char_data_attr && !var_sym_tmp && w_len == 1
        && (unlimited_line_ || input_state_->char_position_in_line != end_))
    {
        start_token();
        consume();
        create_token(ATTR);
    }

    creating_attr_ref_ = !unlimited_line_ && input_state_->char_position_in_line == end_ && last_char_data_attr
        && !var_sym_tmp && w_len == 1;
}

bool lexer::set_begin(size_t begin)
{
    if (begin >= 1 && begin <= 40)
    {
        begin_ = begin;
        return true;
    }
    return false;
}

bool lexer::set_end(size_t end)
{
    if (end == 80)
        continuation_enabled_ = false;
    if (end >= 41 && end <= 80)
    {
        end_default_ = end;
        end_ = end_default_;
        return true;
    }
    return false;
}

bool lexer::set_continue(size_t cont)
{
    if (cont >= 2 && cont <= 40 && begin_ < cont)
    {
        continue_ = cont;
        return true;
    }
    return false;
}

void lexer::set_continuation_enabled(bool enabled) { continuation_enabled_ = enabled; }

bool lexer::is_process() const
{
    return process_allowed_ && toupper(static_cast<int>(input_state_->input->LA(2))) == 'P'
        && toupper(static_cast<int>(input_state_->input->LA(3))) == 'R'
        && toupper(static_cast<int>(input_state_->input->LA(4))) == 'O'
        && toupper(static_cast<int>(input_state_->input->LA(5))) == 'C'
        && toupper(static_cast<int>(input_state_->input->LA(6))) == 'E'
        && toupper(static_cast<int>(input_state_->input->LA(7))) == 'S'
        && toupper(static_cast<int>(input_state_->input->LA(8))) == 'S';
}

void lexer::set_ictl() { ictl_ = true; }

void lexer::lex_process()
{
    /* lex *PROCESS */
    start_token();
    for (size_t i = 0; i < 8; i++)
        consume();
    create_token(PROCESS);

    start_token();
    lex_space();

    size_t apostrophes = 0;
    end_++; /* including END column */
    while (!eof() && before_end() && input_state_->c != '\n' && input_state_->c != '\r'
        && (apostrophes % 2 == 1 || (apostrophes % 2 == 0 && input_state_->c != ' ')))
    {
        start_token();
        lex_tokens();
    }
    end_--;
    lex_end();
}
