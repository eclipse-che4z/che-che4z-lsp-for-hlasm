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
#include "statement_fields.h"

// this file contains inherited structures from hlasm_statement that are used during the parsing

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

// structure representing core fields of statmenent
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
};

// statement with deferred operand and remark field
struct deferred_statement : public core_statement, public context::hlasm_statement
{
    virtual const std::string& deferred_ref() const = 0;
    virtual const range& deferred_range_ref() const = 0;

    virtual position statement_position() const override { return stmt_range_ref().start; }

protected:
    deferred_statement()
        : context::hlasm_statement(context::statement_kind::DEFERRED)
    {}
};

// implementation of deferred statement
// struct holding deferred semantic information (si) about whole instruction statement, whole logical line
struct statement_si_deferred : public deferred_statement
{
    statement_si_deferred(
        range stmt_range, label_si label, instruction_si instruction, std::string deferred_field, range deferred_range)
        : stmt_range(std::move(stmt_range))
        , label(std::move(label))
        , instruction(std::move(instruction))
        , deferred_field(std::move(deferred_field))
        , deferred_range(deferred_range)
    {}

    range stmt_range;

    label_si label;
    instruction_si instruction;
    std::string deferred_field;
    range deferred_range;

    virtual const label_si& label_ref() const override { return label; };
    virtual const instruction_si& instruction_ref() const override { return instruction; };
    virtual const std::string& deferred_ref() const override { return deferred_field; };
    virtual const range& deferred_range_ref() const override { return deferred_range; };
    virtual const range& stmt_range_ref() const override { return stmt_range; };
};

// struct holding full semantic information (si) about whole instruction statement, whole logical line
struct statement_si : public complete_statement
{
    statement_si(range stmt_range, label_si label, instruction_si instruction, operands_si operands, remarks_si remarks)
        : stmt_range(std::move(stmt_range))
        , label(std::move(label))
        , instruction(std::move(instruction))
        , operands(std::move(operands))
        , remarks(std::move(remarks))
    {}

    range stmt_range;

    label_si label;
    instruction_si instruction;
    operands_si operands;
    remarks_si remarks;

    virtual const label_si& label_ref() const override { return label; }
    virtual const instruction_si& instruction_ref() const override { return instruction; }
    virtual const operands_si& operands_ref() const override { return operands; }
    virtual const remarks_si& remarks_ref() const override { return remarks; }
    virtual const range& stmt_range_ref() const override { return stmt_range; }
};

// structure holding deferred statement that is now complete
struct statement_si_defer_done : public complete_statement
{
    statement_si_defer_done(
        std::shared_ptr<const deferred_statement> deferred_stmt, operands_si operands, remarks_si remarks)
        : deferred_stmt(deferred_stmt)
        , operands(std::move(operands))
        , remarks(std::move(remarks))
    {}

    std::shared_ptr<const deferred_statement> deferred_stmt;

    operands_si operands;
    remarks_si remarks;

    virtual const label_si& label_ref() const override { return deferred_stmt->label_ref(); }
    virtual const instruction_si& instruction_ref() const override { return deferred_stmt->instruction_ref(); }
    virtual const operands_si& operands_ref() const override { return operands; }
    virtual const remarks_si& remarks_ref() const override { return remarks; }
    virtual const range& stmt_range_ref() const override { return deferred_stmt->stmt_range_ref(); }
};

} // namespace semantics
} // namespace parser_library
} // namespace hlasm_plugin
#endif
