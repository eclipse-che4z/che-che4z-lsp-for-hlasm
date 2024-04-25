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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DOCUMENT_SYMBOL_ITEM_H
#define HLASMPLUGIN_PARSERLIBRARY_DOCUMENT_SYMBOL_ITEM_H

#include <string>
#include <vector>

#include "range.h"

namespace hlasm_plugin::parser_library {

enum class document_symbol_kind
{
    DAT = 0,
    EQU = 1,
    MACH = 2,
    UNKNOWN = 3,
    VAR = 4,
    SEQ = 5,
    COMMON = 6,
    DUMMY = 7,
    EXECUTABLE = 8,
    READONLY = 9,
    MACRO = 10,
    ASM = 11,
    EXTERNAL = 12,
    WEAK_EXTERNAL = 13,
    TITLE = 14,
};


// representation of document symbol item based on LSP
struct document_symbol_item
{
public:
    document_symbol_item(std::string name, document_symbol_kind kind, range symbol_range);
    document_symbol_item(
        std::string name, document_symbol_kind kind, range symbol_range, std::vector<document_symbol_item> children);

    // several features of document symbol item from LSP
    std::string name;
    document_symbol_kind kind;
    range symbol_range;
    range symbol_selection_range;
    std::vector<document_symbol_item> children;

    std::vector<range> scope;
};

} // namespace hlasm_plugin::parser_library

#endif
