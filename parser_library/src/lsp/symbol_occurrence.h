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

#include <vector>

#include "context/id_storage.h"
#include "context/macro.h"
#include "range.h"

namespace hlasm_plugin::parser_library::lsp {

enum class occurrence_kind
{
    ORD,
    VAR,
    SEQ,
    INSTR,
    INSTR_LIKE,
    COPY_OP
};

struct symbol_occurrence
{
    occurrence_kind kind;
    bool evaluated_model = false;
    context::id_index name;
    range occurrence_range;

    // in case of INSTR kind, holds potential macro opcode
    context::macro_def_ptr opcode = nullptr;

    symbol_occurrence(occurrence_kind kind, context::id_index name, const range& occurrence_range, bool evaluated_model)
        : kind(kind)
        , evaluated_model(evaluated_model)
        , name(name)
        , occurrence_range(occurrence_range)
    {}

    symbol_occurrence(context::id_index name, context::macro_def_ptr opcode, const range& occurrence_range)
        : kind(occurrence_kind::INSTR)
        , name(name)
        , occurrence_range(occurrence_range)
        , opcode(std::move(opcode))
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

using occurrence_storage = std::vector<symbol_occurrence>;

} // namespace hlasm_plugin::parser_library::lsp

#endif