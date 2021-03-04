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

#ifndef LSP_COMPLETION_ITEM_H
#define LSP_COMPLETION_ITEM_H

#include <string>
#include <vector>

#include "protocol.h"

namespace hlasm_plugin::parser_library::lsp {

using completion_list_s = std::vector<completion_item_s>;

// representation of completion item based on LSP
struct completion_item_s
{
public:
    // contents directly passed via the contrusctor
    completion_item_s(std::string label,
        std::string detail,
        std::string insert_text,
        std::string documentation,
        completion_item_kind kind = completion_item_kind::mach_instr);

    std::vector<std::string> get_contents() const;
    // several features of completion item from LSP
    std::string label;
    std::string detail;
    std::string insert_text;
    std::string documentation;
    completion_item_kind kind;

    static const std::vector<completion_item_s> instruction_completion_items_;
};


} // namespace hlasm_plugin::parser_library::lsp

#endif
