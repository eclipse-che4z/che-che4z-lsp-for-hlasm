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

#include "TokenStream.h"
#include "lexer.h"

namespace hlasm_plugin::parser_library::lexing {
struct u8string_with_newlines;
class token;

// custom implementation of antlr token stream
// helps to control the filtering of the token stream
class token_stream final : public antlr4::TokenStream
{
    lexer* token_source;
    size_t pos = 0;
    bool enabled_cont = false;
    bool fetched_eof = false;

public:
    explicit token_stream(lexer* token_source);

    // enable continuation token in the token stream
    void enable_continuation();
    // filter continuation token from the token stream
    void disable_continuation();

    void reset();

    auto get_line_limits() const { return token_source->get_line_limits(); }
    u8string_with_newlines get_text_with_newlines(const antlr4::misc::Interval& interval);

    void fill();
    std::vector<antlr4::Token*> get_tokens() const;

    /**************************************************************/
    antlr4::Token* LT(ssize_t k) override;
    antlr4::Token* get(size_t index) const override;
    antlr4::TokenSource* getTokenSource() const override;
    std::string getText(const antlr4::misc::Interval& interval) override;
    std::string getText() override;
    std::string getText(antlr4::RuleContext* ctx) override;
    std::string getText(antlr4::Token* start, antlr4::Token* stop) override;
    void consume() override;
    size_t LA(ssize_t i) override;
    ssize_t mark() override;
    void release(ssize_t marker) override;
    size_t index() override;
    void seek(size_t index) override;
    size_t size() override;
    std::string getSourceName() const override;

private:
    bool is_on_channel(const token* t) const;
};

} // namespace hlasm_plugin::parser_library::lexing
#endif
