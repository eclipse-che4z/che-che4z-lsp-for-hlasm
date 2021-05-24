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


/* UTF8 <-> UTF32 */
#ifdef __GNUG__
thread_local std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
#elif _MSC_VER
thread_local std::wstring_convert<std::codecvt_utf8<int32_t>, int32_t> converter;
#endif

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

void lexer::rewind_input(stream_position pos)
{
    auto inp = file_input_state_.input;

    if (pos.offset >= input_->size()) // we are rewinding from the end, so limit offset by input size
        pos.offset = input_->size() - 1;

    inp->rewind_input(pos.offset);
    eof_generated_ = false;

    if (!token_queue_.empty())
        delete_token(token_queue_.front()->getTokenIndex());

    file_input_state_.char_position = pos.offset;
    file_input_state_.line = pos.line;
    file_input_state_.char_position_in_line = 0;
    file_input_state_.char_position_in_line_utf16 = 0;

    if (pos.line == 0)
        last_lln_end_pos_ = { static_cast<size_t>(-1), static_cast<size_t>(-1) };
    else
        last_lln_end_pos_ = { pos.line - 1, pos.offset };

    last_lln_begin_pos_ = { pos.line, pos.offset };

    file_input_state_.c = static_cast<char_t>(inp->LA(1));
}

bool lexer::is_last_line() const
{
    for (size_t i = 1; input_->LA(i) != antlr4::CharStream::EOF && i < 100; i++)
    {
        if (input_->LA(i) == '\n' || input_->LA(i) == '\r')
            return false;
    }
    return true;
}

lexer::stream_position lexer::last_lln_begin_position() const { return last_lln_begin_pos_; }

lexer::stream_position lexer::last_lln_end_position() const { return last_lln_end_pos_; }

bool lexer::eof_generated() const { return eof_generated_; }

void lexer::set_unlimited_line(bool unlimited_lines) { unlimited_line_ = unlimited_lines; }
void lexer::set_file_offset(position file_offset)
{
    input_state_->line = (size_t)file_offset.line;
    input_state_->char_position_in_line = (size_t)file_offset.column;
    input_state_->char_position_in_line_utf16 = (size_t)file_offset.column;
}

void lexer::reset()
{
    token_queue_ = {};
    last_token_id_ = 0;
    input_state_->char_position = 0;
    file_input_state_.c = static_cast<char_t>(input_->LA(1));
    eof_generated_ = false;
}

void lexer::append()
{
    file_input_state_.c = static_cast<char_t>(input_->LA(1));
    eof_generated_ = false;
}


size_t lexer::getLine() const { return input_state_->line; }

size_t lexer::getCharPositionInLine() { return input_state_->char_position_in_line; }

antlr4::CharStream* lexer::getInputStream() { return input_; }

std::string lexer::getSourceName() { return input_->getSourceName(); }

bool lexer::double_byte_enabled() const { return double_byte_enabled_; }

void lexer::set_double_byte_enabled(bool dbe) { double_byte_enabled_ = dbe; }

/*
 * check if token is after continuation
 * token is unmarked after the call
 */
bool lexer::continuation_before_token(size_t token_index)
{
    if (tokens_after_continuation_.find(token_index) != tokens_after_continuation_.end())
    {
        tokens_after_continuation_.erase(token_index);
        return true;
    }
    return false;
}

void lexer::create_token(size_t ttype, size_t channel = Channels::DEFAULT_CHANNEL)
{
    if (ttype == Token::EOF)
    {
        assert(!eof_generated_);
        eof_generated_ = true;
    }
    /* do not generate empty tokens (except EOF) */
    if (token_start_state_.char_position == token_start_state_.input->index() && ttype != Token::EOF)
        return;

    /* mark first token after continuation */
    if (channel == DEFAULT_CHANNEL && last_continuation_ != static_cast<size_t>(-1))
    {
        last_continuation_ = static_cast<size_t>(-1);
        tokens_after_continuation_.emplace(last_token_id_);
    }

    /* record last continuation */
    if (ttype == CONTINUATION)
        last_continuation_ = last_token_id_;

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

        else if (!unlimited_line_ && input_state_->char_position_in_line == end_ && !isspace(input_state_->c)
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

void lexer::delete_token(ssize_t index)
{
    eof_generated_ = false;
    last_token_id_ = index;
    token_queue_ = {};
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
                apostrophes_++;
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

        if (!isspace(input_state_->c) && !eof() && continuation_enabled_)
            lex_continuation();
        else
        {
            lex_end();
            set_last_line_pos(input_state_->char_position, token_start_state_.line);

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
    if (cc != CharStream::EOF && !isspace(static_cast<int>(cc)))
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
    return ((ictl_ && input_state_->line <= 11) || (!ictl_ && input_state_->line <= 10))
        && toupper(static_cast<int>(input_state_->input->LA(2))) == 'P'
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

    apostrophes_ = 0;
    end_++; /* including END column */
    while (!eof() && before_end() && input_state_->c != '\n' && input_state_->c != '\r'
        && (apostrophes_ % 2 == 1 || (apostrophes_ % 2 == 0 && input_state_->c != ' ')))
    {
        start_token();
        lex_tokens();
    }
    end_--;
    lex_end();
}


bool lexer::char_start_utf8(unsigned c)
{
    /* https://en.wikipedia.org/wiki/UTF-8#Description */
    return !(c & (1 << 7))

        || (!(c & (1 << 5)) && (c & (3 << 6)))

        || (!(c & (1 << 4)) && (c & (7 << 5)))

        || (!(c & (1 << 3)) && (c & (15 << 4)));
}

size_t lexer::length_utf16(const std::string& str)
{
    if (str.length() == 0)
        return 0;

    size_t l = 0;
    size_t ll = 1;
    for (auto i = str.cbegin() + 1; i != str.cend(); ++i)
    {
        if (char_start_utf8(*i))
        {
            /* 4B in UTF8 = 2x UTF16 otherwise 1x UTF16 */
            l += (ll == 4) ? 2 : 1;
            ll = 0;
        }
        ++ll;
    }
    l += (ll == 4) ? 2 : 1;
    return l;
}

void lexer::set_last_line_pos(size_t idx, size_t line)
{
    if (!(last_lln_end_pos_.line == static_cast<size_t>(-1) && last_lln_end_pos_.offset == static_cast<size_t>(-1)))
    {
        last_lln_begin_pos_ = { last_lln_end_pos_.line + 1, last_lln_end_pos_.offset };
    }
    last_lln_end_pos_ = { line, idx };
}
