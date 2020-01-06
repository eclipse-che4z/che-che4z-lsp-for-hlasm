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

#ifndef HLASMPLUGIN_PARSER_LIBRARY_TOKENSTREAM_H
#define HLASMPLUGIN_PARSER_LIBRARY_TOKENSTREAM_H

#include <utility>
#include <memory>

#include "antlr4-runtime.h"
#include "lexer.h"


namespace hlasm_plugin {
namespace parser_library {

class PARSER_LIBRARY_EXPORT token_stream : public antlr4::BufferedTokenStream
{
	bool enabled_cont_;
	bool enabled_hidden_;
public:
	token_stream(antlr4::TokenSource* token_source);

	void enable_continuation();
	void disable_continuation();

	void enable_hidden();
	void disable_hidden();

	antlr4::Token* LT(ssize_t k) override;

	std::string getText(const antlr4::misc::Interval &interval) override;

	void rewind_input(lexer::stream_position pos, bool insert_EOLLN);
	bool consume_EOLLN();

protected:
	virtual ssize_t adjustSeekIndex(size_t i) override;

	virtual antlr4::Token* LB(size_t k) override;


	bool is_on_channel(antlr4::Token* token);

	size_t next_token_on_channel(size_t i);

	antlr4::Token* previous_token_on_channel(size_t i);

};

}
}
#endif
