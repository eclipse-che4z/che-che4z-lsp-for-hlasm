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

#include "../include/shared/token_stream.h"
#include "../include/shared/lexer.h"

using namespace hlasm_plugin::parser_library;
using namespace antlr4;

token_stream::token_stream(antlr4::TokenSource* token_source) 
	: antlr4::BufferedTokenStream(token_source), enabled_cont_(false),enabled_hidden_(false), needSetup_(true) {}

void token_stream::enable_continuation() { enabled_cont_ = true; }

void token_stream::disable_continuation() { enabled_cont_ = false; }

void token_stream::enable_hidden() { enabled_hidden_ = true; }

void token_stream::disable_hidden() { enabled_hidden_ = false; }

void token_stream::rewind_input(lexer::stream_position pos, bool insert_EOLLN)
{
	auto& lexer_tmp = dynamic_cast<lexer&>(*_tokenSource);
	
	if (lexer_tmp.last_lln_end_position().offset == pos.offset) // no rewind needed
		return;

	lexer_tmp.rewind_input(pos);

	if (_tokens.back()->getType() != lexer::EOLLN || (_tokens.size() > 1 && _tokens[_tokens.size() - 2]->getType() == lexer::EOLLN))
	{
		auto index_tmp = _tokens.back()->getTokenIndex();
		_tokens.pop_back();
		lexer_tmp.delete_token(index_tmp);
		_p = _tokens.size();
	}

	if (insert_EOLLN)
		lexer_tmp.insert_EOLLN();

	_fetchedEOF = false;
	sync(_p);
}

void token_stream::reset()
{
	_tokens.clear();
	_fetchedEOF = false;
	_p = 0;
	sync(0);
}

void token_stream::append()
{
	if (needSetup_)
		setup();
	else
	{

		_fetchedEOF = false;
		++_p;
		sync(_p);
	}

}

antlr4::Token * hlasm_plugin::parser_library::token_stream::LT(ssize_t k)
{
	lazyInit();
	if (k == 0)
		return nullptr;

	if (k < 0) 
		return LB(-k);

	size_t i = _p;

	for (ssize_t n = 1; n < k; ++n)
	{
		if (sync(i + 1)) 
			i = next_token_on_channel(i + 1);
	}

	return get(next_token_on_channel(i));
}

std::string hlasm_plugin::parser_library::token_stream::getText(const antlr4::misc::Interval & interval)
{
	lazyInit();
	size_t start = interval.a;
	size_t stop = interval.b;
	if (start == INVALID_INDEX || stop == INVALID_INDEX) {
		return "";
	}
	if (stop >= _tokens.size()) {
		stop = _tokens.size() - 1;
	}

	sync(stop);

	std::stringstream ss;
	for (size_t i = start; i <= stop; i++) {
		Token *t = _tokens[i].get();
		if (t->getType() == Token::EOF) {
			break;
		}
		ss << t->getText();
	}
	return ss.str();
}

ssize_t hlasm_plugin::parser_library::token_stream::adjustSeekIndex(size_t i)
{
	if (needSetup_)
	{
		sync(i);

		if (i >= size())
			return size() - 1;

		auto token = get(i);

		while (!is_on_channel(token))
		{
			++i;
			sync(i);
			token = get(i);
		}
		return i;
	}

	if (i == _p + 1)
		return next_token_on_channel(i);
	else
		return i;
}

void hlasm_plugin::parser_library::token_stream::setup() {
	BufferedTokenStream::setup();
	needSetup_ = false;
}

antlr4::Token * hlasm_plugin::parser_library::token_stream::LB(size_t k)
{
	if (k == 0 || _p < k || size() == 0) 
		return nullptr;

	size_t i = _p;
	size_t n = 0;

	while (n<k)
	{
		if (i == 0)
			return nullptr;
		else
			--i;

		antlr4::Token* token = get(i);

		if (is_on_channel(token)) 
			++n;
	}

	return get(i);
}

size_t hlasm_plugin::parser_library::token_stream::next_token_on_channel(size_t i)
{
	sync(i);

	if (i >= size())
		return size() - 1;

	auto token = get(_p);

	size_t to_consume = i - _p;
	i = _p;

	while (!is_on_channel(token) || to_consume != 0)
	{
		to_consume -= is_on_channel(token) ? 1 : 0;
		++i;
		sync(i);
		token = get(i);
	}

	return i;
}

size_t hlasm_plugin::parser_library::token_stream::previous_token_on_channel(size_t i)
{
	sync(i);

	if (i >= size()) 
	{
		if (size() == 0)
			return 0;
		else
			return size() - 1;
	}

	auto to_skip = _p - i;
	i = _p - 1;

	while (true)
	{
		antlr4::Token* token = get(i);

		if (is_on_channel(token))
		{
			if (--to_skip == 0)
				return i;
		}

		if (i == 0)
			return 0;
		else
			--i;
	}
}

bool hlasm_plugin::parser_library::token_stream::is_on_channel(antlr4::Token * token)
{
	return token->getChannel() == lexer::Channels::DEFAULT_CHANNEL 
		|| (enabled_hidden_ && token->getChannel() == lexer::Channels::HIDDEN_CHANNEL)
		|| (enabled_cont_ && token->getType() == lexer::CONTINUATION) 
		|| token->getType() == antlr4::Token::EOF;
}
