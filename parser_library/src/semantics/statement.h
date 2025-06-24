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

#ifndef SEMANTICS_STATEMENT_H
#define SEMANTICS_STATEMENT_H

#include <optional>
#include <span>
#include <string>
#include <vector>

#include "context/hlasm_statement.h"
#include "diagnostic_op.h"
#include "range.h"
#include "statement_fields.h"

// this file contains inherited structures from hlasm_statement that are used during the parsing

namespace hlasm_plugin::parser_library::semantics {

// implementation of deferred statement
// struct holding deferred semantic information (si) about whole instruction statement, whole logical line
struct deferred_statement final : public context::hlasm_statement
{
    deferred_statement(range stmt_range,
        label_si label,
        instruction_si instruction,
        deferred_operands_si deferred_operands,
        std::vector<diagnostic_op>&& diags,
        size_t operand_diags_start_index);

    label_si label;
    instruction_si instruction;
    deferred_operands_si deferred_operands;

    std::vector<diagnostic_op> statement_diagnostics;
    size_t operand_diags_start_index;
    range stmt_range;

    const range& stmt_range_ref() const override { return stmt_range; }

    std::span<const diagnostic_op> diagnostics() const override
    {
        return { statement_diagnostics.data(), statement_diagnostics.data() + statement_diagnostics.size() };
    }
    std::span<const diagnostic_op> diagnostics_without_operands() const
    {
        return { statement_diagnostics.data(), statement_diagnostics.data() + operand_diags_start_index };
    }
};

struct preproc_details
{
    struct name_range
    {
        std::string name;
        range r;

        bool operator==(const name_range&) const = default;
    };

    struct instr
    {
        name_range nr;
        std::optional<range> preproc_specific_r;
    };

    range stmt_r;
    name_range label;
    instr instruction;
    std::vector<name_range> operands;
    std::vector<range> remarks;
};

struct preprocessor_statement_si
{
    preproc_details m_details;
    const bool m_copylike;

    preprocessor_statement_si(preproc_details details, bool copylike = false);
};

struct endevor_statement_si : public preprocessor_statement_si
{
    explicit endevor_statement_si(preproc_details details);
};

} // namespace hlasm_plugin::parser_library::semantics

#endif
