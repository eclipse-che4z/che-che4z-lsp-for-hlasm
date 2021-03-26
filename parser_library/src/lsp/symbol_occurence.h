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

#ifndef LSP_SYMBOL_OCCURENCE_H
#define LSP_SYMBOL_OCCURENCE_H

#include <vector>

#include "context/id_storage.h"
#include "range.h"

namespace hlasm_plugin::parser_library::lsp {

enum class occurence_kind
{
    ORD,
    VAR,
    SEQ,
    INSTR,
    COPY_OP
};

struct symbol_occurence
{
    occurence_kind kind;
    context::id_index name;
    range occurence_range;

    // in case of INSTR kind, holds potential macro opcode
    context::macro_def_ptr opcode = nullptr;

    symbol_occurence(occurence_kind kind, context::id_index name, const range& occurence_range)
        : kind(kind)
        , name(name)
        , occurence_range(occurence_range)
    {}

    symbol_occurence(context::id_index name, context::macro_def_ptr opcode, const range& occurence_range)
        : kind(occurence_kind::INSTR)
        , name(name)
        , occurence_range(occurence_range)
        , opcode(std::move(opcode))
    {}

    // returns true, if this occurence kind depends on a scope
    bool is_scoped() const { return kind == occurence_kind::SEQ || kind == occurence_kind::VAR; }

    bool is_similar(const symbol_occurence& occ) const
    {
        return kind == occ.kind && name == occ.name && opcode == occ.opcode;
    }
};

inline bool operator==(const symbol_occurence& lhs, const symbol_occurence& rhs)
{
    return lhs.is_similar(rhs) && lhs.occurence_range == rhs.occurence_range;
}

using occurence_storage = std::vector<symbol_occurence>;

} // namespace hlasm_plugin::parser_library::lsp

#endif