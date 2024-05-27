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

void source_info_processor::process_hl_symbols(std::span<const token_info> symbols)
{
    process_hl_symbols(symbols, hl_info_.cont_info.continue_column);
}

void source_info_processor::process_hl_symbols(std::span<const token_info> symbols, size_t continue_column)
{
    for (const auto& symbol : symbols)
    {
        add_hl_symbol(symbol, continue_column);
    }
}

void source_info_processor::finish()
{
    std::ranges::sort(hl_info_.lines, {}, [](const auto& e) { return e.token_range.start; });
}

lines_info source_info_processor::take_semantic_tokens() { return std::move(hl_info_.lines); }

void source_info_processor::add_hl_symbol(const token_info& symbol)
{
    add_hl_symbol(symbol, hl_info_.cont_info.continue_column);
}

void source_info_processor::add_hl_symbol(const token_info& symbol, size_t continue_column)
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
    auto start = symbol.token_range.start;
    while (start.line < symbol.token_range.end.line)
    {
        hl_info_.lines.emplace_back(start, position(start.line, hl_info_.cont_info.continuation_column), symbol.scope);
        start.line++;
        start.column = continue_column;
    }

    if (start != symbol.token_range.end) // do not add empty tokens
        hl_info_.lines.emplace_back(start, symbol.token_range.end, symbol.scope);
}
