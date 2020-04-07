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

#include "../include/shared/token_factory.h"

#include <assert.h> 

using namespace hlasm_plugin;
using namespace parser_library;
using namespace lexing;

token_factory::token_factory()
= default;

token_factory::~token_factory()
= default;

std::unique_ptr<token> token_factory::create(antlr4::TokenSource * source, antlr4::CharStream * stream, size_t type, size_t channel, size_t start, size_t stop, size_t line, size_t char_position_in_line, size_t index, size_t char_position_in_line_16, size_t end_of_token_in_line_utf16)
{
	return std::make_unique<token>(
		source,
		stream,
		type,
		channel,
		start,
		stop,
		line,
		char_position_in_line,
		index,
		char_position_in_line_16,
		end_of_token_in_line_utf16);
}
