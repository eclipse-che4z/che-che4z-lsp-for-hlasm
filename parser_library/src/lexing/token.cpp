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

#include "token.h"

#include <CharStream.h>
#include <Interval.h>

using namespace hlasm_plugin::parser_library::lexing;

size_t token::get_end_of_token_in_line_utf16() const { return end_of_token_in_line_utf16_; }

::token::token(antlr4::TokenSource* source,
    antlr4::CharStream* input,
    size_t type,
    size_t channel,
    size_t start,
    size_t stop,
    size_t line,
    size_t char_position_in_line,
    size_t token_index,
    size_t char_position_in_line_16,
    size_t end_of_token_in_line_utf16)
    : source_(source)
    , input_(input)
    , type_(type)
    , channel_(channel)
    , start_(start)
    , stop_(stop)
    , line_(line)
    , char_position_in_line_(char_position_in_line)
    , token_index_(token_index)
    , char_position_in_line_16_(char_position_in_line_16)
    , end_of_token_in_line_utf16_(end_of_token_in_line_utf16)
{ }

std::string token::getText() const
{
    antlr4::CharStream* input = getInputStream();
    if (input == nullptr)
    {
        return "";
    }
    size_t n = input->size();
    if (start_ < n && stop_ < n)
    {
        return input->getText(antlr4::misc::Interval(start_, stop_));
    }
    return "<EOF>";
}

size_t token::getType() const { return type_; }

size_t token::getLine() const { return line_; }

size_t token::getCharPositionInLine() const { return get_char_position_in_line_16(); }

size_t token::getChannel() const { return channel_; }

size_t token::getTokenIndex() const { return token_index_; }

size_t token::getStartIndex() const { return start_; }

size_t token::getStopIndex() const { return stop_; }

antlr4::TokenSource* token::getTokenSource() const { return source_; }

antlr4::CharStream* token::getInputStream() const { return input_; }

void replace_all(std::string& str, std::string const& from, std::string const& to)
{
    std::string new_string;
    new_string.reserve(str.length()); // avoids a few memory allocations

    std::string::size_type lastPos = 0;
    std::string::size_type findPos;

    while (std::string::npos != (findPos = str.find(from, lastPos)))
    {
        new_string.append(str, lastPos, findPos - lastPos);
        new_string += to;
        lastPos = findPos + from.length();
    }

    // Care for the rest after last occurrence
    new_string += str.substr(lastPos);

    str.swap(new_string);
}

std::string token::toString() const
{
    std::stringstream ss;

    std::string channel_str;
    if (channel_ > 0)
    {
        channel_str = ",channel=" + std::to_string(channel_);
    }
    std::string txt = getText();
    if (!txt.empty())
    {
        replace_all(txt, "\n", "\\n");
        replace_all(txt, "\r", "\\r");
        replace_all(txt, "\t", "\\t");
    }
    else
    {
        txt = "<no text>";
    }

    std::string type_string = std::to_string(static_cast<ssize_t>(type_));

    ss << "[@" << static_cast<ssize_t>(getTokenIndex()) << "," << static_cast<ssize_t>(start_) << ":"
       << static_cast<ssize_t>(stop_) << "='" << txt << "',<" << type_string << ">" << channel_str << "," << line_
       << ":" << getCharPositionInLine() << "]";

    return ss.str();
}

size_t token::get_char_position_in_line_16() const { return char_position_in_line_16_; }
