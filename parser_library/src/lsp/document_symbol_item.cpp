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

bool is_permutation_with_permutations(const document_symbol_list_s& lhs, const document_symbol_list_s& rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    for (auto& item : lhs)
    {
        auto i = std::find(rhs.begin(), rhs.end(), item);
        if (i == rhs.end())
        {
            return false;
        }
        if (!is_permutation_with_permutations(item.children, i->children))
        {
            return false;
        }
    }
    return true;
}

bool operator==(const document_symbol_item_s& lhs, const document_symbol_item_s& rhs)
{
    return lhs.name == rhs.name && lhs.kind == rhs.kind && lhs.symbol_range == rhs.symbol_range
        && lhs.symbol_selection_range == rhs.symbol_selection_range
        && is_permutation_with_permutations(lhs.children, rhs.children);
}

} // namespace hlasm_plugin::parser_library::lsp