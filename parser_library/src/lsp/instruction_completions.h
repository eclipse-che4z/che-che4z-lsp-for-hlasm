/*
 * Copyright (c) 2024 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_INSTRUCTION_COMPLETIONS_H
#define HLASMPLUGIN_PARSERLIBRARY_INSTRUCTION_COMPLETIONS_H

#include <set>

#include "completion_item.h"

namespace hlasm_plugin::parser_library::lsp {

struct completion_label_comparer
{
    using is_transparent = void;

    bool operator()(const completion_item& l, const completion_item& r) const { return l.label < r.label; }
    template<typename L>
    bool operator()(const L& l, const completion_item& r) const
    {
        return l < r.label;
    }
    template<typename R>
    bool operator()(const completion_item& l, const R& r) const
    {
        return l.label < r;
    }
};

extern const std::set<completion_item, completion_label_comparer> instruction_completion_items;

} // namespace hlasm_plugin::parser_library::lsp

#endif
