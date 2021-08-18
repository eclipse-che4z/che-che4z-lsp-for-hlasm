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

#ifndef LSP_DOCUMENT_SYMBOL_ITEM_H
#define LSP_DOCUMENT_SYMBOL_ITEM_H

#include <algorithm>
#include <string>
#include <vector>

#include "protocol.h"

namespace hlasm_plugin::parser_library::lsp {

using document_symbol_list_s = std::vector<document_symbol_item_s>;

// representation of document symbol item based on LSP
struct document_symbol_item_s
{
public:
    document_symbol_item_s(std::string name, document_symbol_kind kind, range symbol_range);
    document_symbol_item_s(
        std::string name, document_symbol_kind kind, range symbol_range, document_symbol_list_s children);

    // several features of document symbol item from LSP
    std::string name;
    document_symbol_kind kind;
    range symbol_range;
    range symbol_selection_range;
    document_symbol_list_s children;

    std::vector<range> scope;
};

bool operator==(const document_symbol_item_s& lhs, const document_symbol_item_s& rhs);
bool is_permutation_with_permutations(const document_symbol_list_s& lhs, const document_symbol_list_s& rhs);
document_symbol_list_s::iterator document_symbol_no_children_find(
    document_symbol_list_s::iterator begin, document_symbol_list_s::iterator end, const document_symbol_item_s& item);
void operator+=(document_symbol_list_s& lhs, const document_symbol_list_s& rhs);

} // namespace hlasm_plugin::parser_library::lsp

#endif