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

#ifndef HLASMPLUGIN_PARSER_HLASMHTF_H
#define HLASMPLUGIN_PARSER_HLASMHTF_H

#include <memory>

#include "antlr4-runtime.h"

#include "TokenFactory.h"
#include "parser_library_export.h"
#include "token.h"

namespace hlasm_plugin::parser_library::lexing {

class token_factory
{
public:
    token_factory();

    token_factory(const token_factory&) = delete;
    token_factory& operator=(const token_factory&) = delete;
    token_factory& operator=(token_factory&&) = delete;
    token_factory(token_factory&&) = delete;

    ~token_factory();

    std::unique_ptr<token> create(antlr4::TokenSource* source,
        antlr4::CharStream* stream,
        size_t type,
        size_t channel,
        size_t start,
        size_t stop,
        size_t line,
        size_t char_position_in_line,
        size_t index,
        size_t char_position_in_line_16,
        size_t end_of_token_in_line_utf16);
};

} // namespace hlasm_plugin::parser_library::lexing
#endif
