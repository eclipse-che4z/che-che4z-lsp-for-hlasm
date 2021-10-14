/*
 * Copyright (c) 2021 Broadcom.
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

#include "document_symbol_item.h"

#include "utils/similar.h"

namespace hlasm_plugin::parser_library::lsp {

document_symbol_item_s::document_symbol_item_s(std::string name, document_symbol_kind kind, range symbol_range)
    : name(name)
    , kind(kind)
    , symbol_range(symbol_range)
    , symbol_selection_range(symbol_range)
{}
document_symbol_item_s::document_symbol_item_s(
    std::string name, document_symbol_kind kind, range symbol_range, document_symbol_list_s children)
    : name(name)
    , kind(kind)
    , symbol_range(symbol_range)
    , symbol_selection_range(symbol_range)
    , children(children)
{}

bool is_similar(const document_symbol_list_s& l, const document_symbol_list_s& r)
{
    return l.size() == r.size() && std::is_permutation(l.begin(), l.end(), r.begin(), utils::is_similar);
}

bool is_similar(const document_symbol_item_s& l, const document_symbol_item_s& r)
{
    return l.name == r.name && l.kind == r.kind && l.symbol_range == r.symbol_range
        && l.symbol_selection_range == r.symbol_selection_range && is_similar(l.children, r.children);
}

} // namespace hlasm_plugin::parser_library::lsp