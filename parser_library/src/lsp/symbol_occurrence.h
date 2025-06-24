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

#ifndef LSP_SYMBOL_OCCURRENCE_H
#define LSP_SYMBOL_OCCURRENCE_H

#include "context/id_index.h"
#include "context/macro.h"
#include "range.h"

namespace hlasm_plugin::parser_library::lsp {

enum class occurrence_kind : unsigned char
{
    ORD = 1,
    VAR = 2,
    SEQ = 4,
    INSTR = 8,
    INSTR_LIKE = 16,
    COPY_OP = 32,
};

constexpr occurrence_kind operator|(occurrence_kind l, occurrence_kind r) noexcept
{
    return static_cast<occurrence_kind>(static_cast<unsigned char>(l) | static_cast<unsigned char>(r));
}

constexpr occurrence_kind operator&(occurrence_kind l, occurrence_kind r) noexcept
{
    return static_cast<occurrence_kind>(static_cast<unsigned char>(l) & static_cast<unsigned char>(r));
}

constexpr bool any(occurrence_kind e) { return e != occurrence_kind(); }

struct symbol_occurrence
{
    occurrence_kind kind;
    bool evaluated_model = false;
    context::id_index name;
    range occurrence_range;

    // in case of INSTR kind, holds potential macro opcode
    const context::macro_definition* opcode = nullptr;

    symbol_occurrence(occurrence_kind kind, context::id_index name, const range& occurrence_range, bool evaluated_model)
        : kind(kind)
        , evaluated_model(evaluated_model)
        , name(name)
        , occurrence_range(occurrence_range)
    {}

    symbol_occurrence(context::id_index name, const context::macro_definition* opcode, const range& occurrence_range)
        : kind(occurrence_kind::INSTR)
        , name(name)
        , occurrence_range(occurrence_range)
        , opcode(opcode)
    {}

    // returns true, if this occurrence kind depends on a scope
    bool is_scoped() const { return kind == occurrence_kind::SEQ || kind == occurrence_kind::VAR; }

    bool is_similar(const symbol_occurrence& occ) const
    {
        using enum occurrence_kind;
        return kind == occ.kind && name == occ.name && opcode == occ.opcode
            || name == occ.name && (kind == INSTR_LIKE && occ.kind == INSTR || kind == INSTR && occ.kind == INSTR_LIKE);
    }

    bool operator==(const symbol_occurrence&) const noexcept = default;
};

} // namespace hlasm_plugin::parser_library::lsp

#endif
