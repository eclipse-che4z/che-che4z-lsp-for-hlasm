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

#include "source_info_processor.h"

#include <algorithm>
#include <utility>

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::semantics;

source_info_processor::source_info_processor(bool collect_hl_info)
    : collect_hl_info_(collect_hl_info) {};

void source_info_processor::process_hl_symbols(std::vector<token_info> symbols)
{
    process_hl_symbols(std::move(symbols), hl_info_.cont_info.continue_column);
}

void source_info_processor::process_hl_symbols(std::vector<token_info> symbols, size_t continue_column)
{
    for (auto& symbol : symbols)
    {
        add_hl_symbol(std::move(symbol), continue_column);
    }
}

void source_info_processor::finish() { std::sort(hl_info_.lines.begin(), hl_info_.lines.end()); }

const lines_info& source_info_processor::semantic_tokens() const { return hl_info_.lines; }

void source_info_processor::add_hl_symbol(token_info symbol)
{
    add_hl_symbol(std::move(symbol), hl_info_.cont_info.continue_column);
}

void source_info_processor::add_hl_symbol(token_info symbol, size_t continue_column)
{
    // file is open in IDE, get its highlighting
    if (!collect_hl_info_)
        return;
    if (symbol.scope == hl_scopes::continuation)
    {
        hl_info_.cont_info.continuation_positions.push_back(
            { symbol.token_range.start.line, symbol.token_range.start.column });
    }

    // split multi line symbols
    auto& rest = symbol;
    while (rest.token_range.start.line < rest.token_range.end.line)
    {
        // remove first line and add as separate token
        auto first = rest;
        first.token_range.end.line = first.token_range.start.line;
        first.token_range.end.column = hl_info_.cont_info.continuation_column;
        hl_info_.lines.push_back(std::move(first));
        rest.token_range.start.line++;
        rest.token_range.start.column = continue_column;
    }

    if (rest.token_range.start != rest.token_range.end) // do not add empty tokens
        hl_info_.lines.push_back(std::move(rest));
}
