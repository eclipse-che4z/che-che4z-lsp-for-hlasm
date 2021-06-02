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

#ifndef HLASMPLUGIN_PARSER_HLASMLEX_H
#define HLASMPLUGIN_PARSER_HLASMLEX_H

#include <memory>
#include <queue>
#include <set>
#include <string_view>

#include "antlr4-runtime.h"

#include "input_source.h"
#include "parser_library_export.h"
#include "range.h"
#include "semantics/source_info_processor.h"
#include "token.h"
#include "token_factory.h"


namespace hlasm_plugin::parser_library::lexing {

class input_source;
using token_ptr = std::unique_ptr<antlr4::Token>;
using char_t = char32_t;
class lexer final : public antlr4::TokenSource
{
public:
    struct stream_position
    {
        size_t line;
        size_t offset;
    };
    lexer(input_source*, semantics::source_info_processor* lsp_proc);

    lexer(const lexer&) = delete;
    lexer& operator=(const lexer&) = delete;
    lexer& operator=(lexer&&) = delete;
    lexer(lexer&&) = delete;

    // resets lexer's state, goes to the source beginning
    void reset();

    virtual ~lexer() = default;

    // generates next token, main lexer logic
    token_ptr nextToken() override;

    size_t getLine() const override;

    size_t getCharPositionInLine() override;

    antlr4::CharStream* getInputStream() override;

    std::string getSourceName() override;

    Ref<antlr4::TokenFactory<antlr4::CommonToken>> getTokenFactory() override { return dummy_factory; };

    enum Tokens
    {
#include "parsing/grammar/lex.tokens"
    };

    enum Channels
    {
        DEFAULT_CHANNEL = 0,
        HIDDEN_CHANNEL = 1
    };

    // set begin column
    bool set_begin(size_t begin);
    // set end column
    bool set_end(size_t end);
    // set continuation column
    bool set_continue(size_t cont);
    void set_continuation_enabled(bool);
    // enable ictl
    void set_ictl();

    static bool ord_char(char_t c);

    // is next input char an ord char?
    bool is_ord_char() const;
    bool is_space() const;
    bool is_data_attribute() const;
    void set_unlimited_line(bool unlimited_lines);
    // set lexer's input state to file position
    void set_file_offset(position file_offset, bool process_allowed = false);

protected:
    // creates token and inserts to input stream
    void create_token(size_t ttype, size_t channel);
    // consumes char from input & updates lexer state
    void consume();

private:
    bool last_char_utf16_long_ = false;
    bool creating_var_symbol_ = false;
    bool creating_attr_ref_ = false;
    bool process_allowed_ = false;

    size_t last_token_id_ = 0;

    size_t last_line_pos_ = 0;

    std::queue<token_ptr> token_queue_;
    Ref<antlr4::CommonTokenFactory> dummy_factory;

    bool double_byte_enabled_ = false;
    bool continuation_enabled_ = true;
    bool unlimited_line_ = false;
    bool ictl_ = false;
    size_t begin_ = 0;
    size_t end_default_ = 71;
    size_t end_ = 71;
    size_t continue_ = 15;

    std::unique_ptr<token_factory> factory_;
    antlr4::CharStream* input_;
    semantics::source_info_processor* src_proc_;

    struct input_state
    {
        input_source* input = nullptr;
        char_t c = 0;
        size_t line = 0;
        size_t char_position = 0;
        size_t char_position_in_line = 0;
        size_t char_position_in_line_utf16 = 0;
    };

    input_state file_input_state_;
    input_state* input_state_ = &file_input_state_;

    // captures lexer state at the beginning of a token
    input_state token_start_state_;


    bool eof() const;
    bool identifier_divider() const;

    // captures lexer state at the beginning of a token
    void start_token();
    // lex beginning of the line
    void lex_begin();
    // lex last part of line
    void lex_end();
    void lex_comment();
    // lex continuation & everything until the EOL (which is lexed as IGNORED token)
    void lex_continuation();
    // lex whitespace
    void lex_space();
    // check if before end_ and EOL
    bool before_end() const;

    // lexes everything not lexed in lex_tokens()
    void lex_word();

    void check_continuation();

    // lexes everything not lexed in nextToken()
    void lex_tokens();
    // consumes '\r' and/or '\n'
    void consume_new_line();
    // lexes PROCESS instruction
    void lex_process();

    bool is_process() const;
};

} // namespace hlasm_plugin::parser_library::lexing
#endif
