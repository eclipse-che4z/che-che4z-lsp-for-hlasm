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

#include <set>
#include <string>
#include <vector>

#include "context/hlasm_context.h"
#include "protocol.h"

namespace hlasm_plugin::parser_library::lsp {

using completion_list_s = std::vector<completion_item_s>;

// representation of completion item based on LSP
struct completion_item_s
{
    // contents directly passed via the constructor
    completion_item_s(std::string label,
        std::string detail,
        std::string insert_text,
        std::string documentation,
        completion_item_kind kind = completion_item_kind::mach_instr);

    // several features of completion item from LSP
    std::string label;
    std::string detail;
    std::string insert_text;
    std::string documentation;
    completion_item_kind kind;

    struct label_comparer
    {
        using is_transparent = void;

        bool operator()(const completion_item_s& l, const completion_item_s& r) const { return l.label < r.label; }
        template<typename L>
        bool operator()(const L& l, const completion_item_s& r) const
        {
            return l < r.label;
        }
        template<typename R>
        bool operator()(const completion_item_s& l, const R& r) const
        {
            return l.label < r;
        }
    };

    static const std::set<completion_item_s, label_comparer> m_instruction_completion_items;
};

bool operator==(const completion_item_s& lhs, const completion_item_s& rhs);

} // namespace hlasm_plugin::parser_library::lsp

#endif
