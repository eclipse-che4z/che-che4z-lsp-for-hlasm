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

#include "context/hlasm_statement.h"
#include "diagnostic.h"
#include "statement_fields.h"

// this file contains inherited structures from hlasm_statement that are used during the parsing

namespace hlasm_plugin::parser_library::semantics {

// structure representing core fields of statement
struct core_statement
{
    virtual const range& stmt_range_ref() const = 0;
    virtual const label_si& label_ref() const = 0;
    virtual const instruction_si& instruction_ref() const = 0;

    virtual ~core_statement() = default;
};

// statement with all fields
struct complete_statement : public core_statement
{
    virtual const operands_si& operands_ref() const = 0;
    virtual const remarks_si& remarks_ref() const = 0;
    virtual std::span<const literal_si> literals() const = 0;
};

// statement with deferred operand and remark field
struct deferred_statement : public core_statement, public context::hlasm_statement
{
    virtual const deferred_operands_si& deferred_ref() const = 0;

    position statement_position() const override { return stmt_range_ref().start; }

    virtual std::span<const diagnostic_op> diagnostics_without_operands() const = 0;

protected:
    deferred_statement()
        : context::hlasm_statement(context::statement_kind::DEFERRED)
    {}
};

// implementation of deferred statement
// struct holding deferred semantic information (si) about whole instruction statement, whole logical line
struct statement_si_deferred : public deferred_statement
{
    statement_si_deferred(range stmt_range,
        label_si label,
        instruction_si instruction,
        deferred_operands_si deferred_operands,
        std::vector<diagnostic_op>&& diags,
        size_t operand_diags_start_index)
        : stmt_range(std::move(stmt_range))
        , label(std::move(label))
        , instruction(std::move(instruction))
        , deferred_operands(std::move(deferred_operands))
        , statement_diagnostics(std::make_move_iterator(diags.begin()), std::make_move_iterator(diags.end()))
        , operand_diags_start_index(operand_diags_start_index)
    {}

    range stmt_range;

    label_si label;
    instruction_si instruction;
    deferred_operands_si deferred_operands;

    std::vector<diagnostic_op> statement_diagnostics;
    size_t operand_diags_start_index;

    const label_si& label_ref() const override { return label; }
    const instruction_si& instruction_ref() const override { return instruction; }
    const deferred_operands_si& deferred_ref() const override { return deferred_operands; }
    const range& stmt_range_ref() const override { return stmt_range; }

    std::span<const diagnostic_op> diagnostics() const override
    {
        return { statement_diagnostics.data(), statement_diagnostics.data() + statement_diagnostics.size() };
    }
    std::span<const diagnostic_op> diagnostics_without_operands() const override
    {
        return { statement_diagnostics.data(), statement_diagnostics.data() + operand_diags_start_index };
    }
};

// struct holding full semantic information (si) about whole instruction statement, whole logical line
struct statement_si : public complete_statement
{
    statement_si(range stmt_range,
        label_si label,
        instruction_si instruction,
        operands_si operands,
        remarks_si remarks,
        std::vector<semantics::literal_si> collected_literals)
        : stmt_range(std::move(stmt_range))
        , label(std::move(label))
        , instruction(std::move(instruction))
        , operands(std::move(operands))
        , remarks(std::move(remarks))
        , collected_literals(std::move(collected_literals))
    {}

    range stmt_range;

    label_si label;
    instruction_si instruction;
    operands_si operands;
    remarks_si remarks;
    std::vector<semantics::literal_si> collected_literals;

    const label_si& label_ref() const override { return label; }
    const instruction_si& instruction_ref() const override { return instruction; }
    const operands_si& operands_ref() const override { return operands; }
    std::span<const semantics::literal_si> literals() const override { return collected_literals; }
    const remarks_si& remarks_ref() const override { return remarks; }
    const range& stmt_range_ref() const override { return stmt_range; }
};

// structure holding deferred statement that is now complete
struct statement_si_defer_done : public complete_statement
{
    statement_si_defer_done(std::shared_ptr<const deferred_statement> deferred_stmt,
        operands_si operands,
        remarks_si remarks,
        std::vector<semantics::literal_si> collected_literals)
        : deferred_stmt(deferred_stmt)
        , operands(std::move(operands))
        , remarks(std::move(remarks))
        , collected_literals(std::move(collected_literals))
    {}

    std::shared_ptr<const deferred_statement> deferred_stmt;

    operands_si operands;
    remarks_si remarks;
    std::vector<semantics::literal_si> collected_literals;

    const label_si& label_ref() const override { return deferred_stmt->label_ref(); }
    const instruction_si& instruction_ref() const override { return deferred_stmt->instruction_ref(); }
    const operands_si& operands_ref() const override { return operands; }
    std::span<const semantics::literal_si> literals() const override { return collected_literals; }
    const remarks_si& remarks_ref() const override { return remarks; }
    const range& stmt_range_ref() const override { return deferred_stmt->stmt_range_ref(); }
};

struct preprocessor_statement_si : public statement_si
{
    context::id_index m_resemblence;

    preprocessor_statement_si(range stmt_range,
        label_si label,
        instruction_si instruction,
        operands_si operands,
        remarks_si remarks,
        context::id_index resemblence)
        : statement_si(std::move(stmt_range),
            std::move(label),
            std::move(instruction),
            std::move(operands),
            std::move(remarks),
            {})
        , m_resemblence(resemblence)
    {}
};

struct endevor_statement_si : public preprocessor_statement_si
{
    endevor_statement_si(range stmt_range,
        std::string_view instruction,
        range instruction_range,
        std::string_view copy_member,
        range copy_member_range,
        remarks_si remarks,
        context::id_storage& ids);
};

struct cics_statement_si : public preprocessor_statement_si
{
    cics_statement_si(range stmt_range,
        std::string_view label,
        range label_range,
        std::string_view instruction,
        range instruction_range,
        std::vector<std::pair<std::string, range>>& operands,
        range operands_range,
        remarks_si remarks,
        context::id_storage& ids);
};

} // namespace hlasm_plugin::parser_library::semantics

#endif
