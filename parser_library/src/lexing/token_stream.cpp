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

#include "token_stream.h"

#include "RuleContext.h"
#include "misc/Interval.h"
#include "token.h"

using namespace hlasm_plugin::parser_library::lexing;
using namespace antlr4;

token_stream::token_stream(lexer* token_source)
    : token_source(token_source)
{}

void token_stream::enable_continuation() { enabled_cont = true; }

void token_stream::disable_continuation() { enabled_cont = false; }

antlr4::Token* token_stream::LT(ssize_t k)
{
    if (k == 0)
        return nullptr;

    auto p = pos;
    if (k > 0)
    {
        size_t token_count = token_source->token_count();
        while (true)
        {
            if (p >= token_count)
            {
                if (fetched_eof)
                    return token_source->get_token(token_count - 1);
                fetched_eof |= !token_source->more_tokens();
                token_count = token_source->token_count();
                continue;
            }
            auto t = token_source->get_token(p);
            k -= is_on_channel(t);
            if (k == 0)
                return t;
            ++p;
        }
    }
    else
    {
        while (true)
        {
            if (p-- == 0)
                return nullptr;
            auto t = token_source->get_token(p);
            k += is_on_channel(t);
            if (k == 0)
                return t;
        }
    }
}

antlr4::Token* token_stream::get(size_t index) const
{
    if (index >= token_source->token_count())
        throw std::logic_error("token_stream::get");
    return token_source->get_token(index);
}

antlr4::TokenSource* token_stream::getTokenSource() const { return nullptr; }

std::string token_stream::getText(const antlr4::misc::Interval& interval)
{
    size_t start = interval.a;
    size_t stop = interval.b;
    if (start == INVALID_INDEX || stop == INVALID_INDEX)
    {
        return "";
    }

    if (const auto limit = token_source->token_count(); stop >= limit)
    {
        stop = limit - 1;
    }

    std::string ss;
    for (size_t i = start; i <= stop; i++)
    {
        const auto* t = token_source->get_token(i);
        if (t->getType() == Token::EOF)
        {
            break;
        }
        ss.append(t->getText());
    }
    return ss;
}

std::string token_stream::getText() { return getText(antlr4::misc::Interval(0, token_source->token_count() - 1)); }

std::string token_stream::getText(antlr4::RuleContext* ctx) { return getText(ctx->getSourceInterval()); }

std::string token_stream::getText(antlr4::Token* start, antlr4::Token* stop)
{
    if (start != nullptr && stop != nullptr)
    {
        return getText(misc::Interval(start->getTokenIndex(), stop->getTokenIndex()));
    }
    return "";
}

void token_stream::consume()
{
    size_t token_count = token_source->token_count();
    do
    {
        if (pos + 1 >= token_count)
        {
            if (fetched_eof)
                break;
            fetched_eof |= !token_source->more_tokens();
            token_count = token_source->token_count();
            continue;
        }
    } while (!is_on_channel(token_source->get_token(++pos)));
}

size_t token_stream::LA(ssize_t i) { return LT(i)->getType(); }

ssize_t token_stream::mark() { return 0; }

void token_stream::release(ssize_t)
{
    // nothing to do
}

size_t token_stream::index() { return pos; }

void token_stream::seek(size_t index)
{
    pos = index;
    size_t token_count = token_source->token_count();
    while (true)
    {
        if (pos >= token_count)
        {
            if (fetched_eof)
            {
                pos = token_count - 1;
                break;
            }
            fetched_eof |= !token_source->more_tokens();
            token_count = token_source->token_count();
            continue;
        }
        if (is_on_channel(token_source->get_token(pos)))
            break;
        ++pos;
    }
}

size_t token_stream::size() { return token_source->token_count(); }

std::string token_stream::getSourceName() const { return "token_stream"; }

void token_stream::reset()
{
    enabled_cont = false;
    fetched_eof = false;
    pos = 0;
}

bool token_stream::is_on_channel(const token* t) const
{
    return t->getChannel() == lexer::Channels::DEFAULT_CHANNEL || (enabled_cont && t->getType() == lexer::CONTINUATION)
        || t->getType() == antlr4::Token::EOF;
}

void token_stream::fill()
{
    if (fetched_eof)
        return;
    while (token_source->more_tokens())
        ;
    fetched_eof = true;
}

std::vector<antlr4::Token*> token_stream::get_tokens() const
{
    std::vector<antlr4::Token*> result;

    const auto count = token_source->token_count();
    result.reserve(count);
    for (size_t i = 0; i < count; ++i)
        result.push_back(token_source->get_token(i));
    return result;
}
