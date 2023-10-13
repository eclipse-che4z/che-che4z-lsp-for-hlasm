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

#ifndef HLASMPLUGIN_PARSER_HLASMTOKEN_H
#define HLASMPLUGIN_PARSER_HLASMTOKEN_H

#include <string>

#include "CharStream.h"
#include "TokenSource.h"
#include "WritableToken.h" // "Token.h" on windows conflicts with this file
#include "parser_library_export.h"

namespace hlasm_plugin::parser_library::lexing {

class token final : public antlr4::Token
{
public:
    token(antlr4::CharStream* input,
        size_t type,
        size_t channel,
        size_t start,
        size_t stop,
        size_t line,
        size_t char_position_in_line,
        size_t token_index,
        size_t char_position_in_line_16,
        size_t end_of_token_in_line_utf16) noexcept
        : input_(input)
        , type_(type)
        , channel_(channel)
        , start_(start)
        , stop_(stop)
        , line_(line)
        , char_position_in_line_(char_position_in_line)
        , token_index_(token_index)
        , char_position_in_line_16_(char_position_in_line_16)
        , end_of_token_in_line_utf16_(end_of_token_in_line_utf16)
    {}

    std::string getText() const override;

    size_t getType() const override { return type_; }

    size_t getLine() const override { return line_; }

    size_t getCharPositionInLine() const override { return get_char_position_in_line_16(); }

    size_t getChannel() const override { return channel_; }

    size_t getTokenIndex() const override { return token_index_; }

    size_t getStartIndex() const override { return start_; }

    size_t getStopIndex() const override { return stop_; }

    antlr4::TokenSource* getTokenSource() const override { return nullptr; }

    antlr4::CharStream* getInputStream() const override { return input_; }

    std::string toString() const override;

    size_t get_char_position_in_line_16() const { return char_position_in_line_16_; }

    size_t get_end_of_token_in_line_utf16() const { return end_of_token_in_line_utf16_; }

    size_t get_logical_column() const { return char_position_in_line_; }

private:
    antlr4::CharStream* input_ {};
    size_t type_;
    size_t channel_;
    size_t start_;
    size_t stop_;
    size_t line_;
    size_t char_position_in_line_;
    size_t token_index_;
    size_t char_position_in_line_16_;
    size_t end_of_token_in_line_utf16_;
};

} // namespace hlasm_plugin::parser_library::lexing
#endif
