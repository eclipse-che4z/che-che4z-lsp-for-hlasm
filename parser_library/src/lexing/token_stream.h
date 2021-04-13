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

#include <memory>
#include <utility>

#include "antlr4-runtime.h"

#include "lexer.h"


namespace hlasm_plugin {
namespace parser_library {
namespace lexing {

// custom implementation of antlr token stream
// helps to control the filtering of the token stream
class token_stream : public antlr4::BufferedTokenStream
{
    bool enabled_cont_;
    bool enabled_hidden_;
    bool needSetup_;

public:
    token_stream(antlr4::TokenSource* token_source);

    // enable continuation token in the token stream
    void enable_continuation();
    // filter continuation token from the token stream
    void disable_continuation();

    // enable hidden channel tokens in the token stream
    void enable_hidden();
    // disable hidden token channel
    void disable_hidden();

    antlr4::Token* LT(ssize_t k) override;

    std::string getText(const antlr4::misc::Interval& interval) override;

    void rewind_input(lexer::stream_position pos);

    void reset() override;
    // prepares this object to append more tokens
    void append();

protected:
    ssize_t adjustSeekIndex(size_t i) override;

    antlr4::Token* LB(size_t k) override;

    void setup() override;

    bool is_on_channel(antlr4::Token* token);

    size_t next_token_on_channel(size_t i);

    size_t previous_token_on_channel(size_t i);

private:
    std::vector<decltype(_tokens)> tokens_;
};

} // namespace lexing
} // namespace parser_library
} // namespace hlasm_plugin
#endif
