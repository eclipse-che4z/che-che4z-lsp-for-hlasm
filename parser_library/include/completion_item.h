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

#ifndef HLASMPLUGIN_PARSERLIBRARY_LSP_COMPLETION_ITEM_H
#define HLASMPLUGIN_PARSERLIBRARY_LSP_COMPLETION_ITEM_H

#include <string>

namespace hlasm_plugin::parser_library {

enum class completion_item_kind
{
    mach_instr = 0,
    asm_instr = 1,
    ca_instr = 2,
    macro = 3,
    var_sym = 4,
    seq_sym = 5,
    ord_sym = 6,
};

// representation of completion item based on LSP
struct completion_item
{
    // contents directly passed via the constructor
    completion_item(std::string label,
        std::string detail,
        std::string insert_text,
        std::string documentation,
        completion_item_kind kind = completion_item_kind::mach_instr,
        bool snippet = false,
        std::string suggestion_for = {});

    // several features of completion item from LSP
    std::string label;
    std::string detail;
    std::string insert_text;
    std::string documentation;
    completion_item_kind kind;
    bool snippet = false;
    std::string suggestion_for;

    bool operator==(const completion_item&) const noexcept;
};

} // namespace hlasm_plugin::parser_library

#endif
