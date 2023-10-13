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

#include <misc/Interval.h>

using namespace hlasm_plugin::parser_library::lexing;

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
