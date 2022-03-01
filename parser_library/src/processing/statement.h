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

#ifndef PROCESSING_STATEMENT_H
#define PROCESSING_STATEMENT_H

#include <optional>
#include <variant>

#include "op_code.h"
#include "semantics/statement.h"

// this file contains inherited structures from hlasm_statement that are used during the processing

namespace hlasm_plugin::parser_library::processing {

// statement that contains resolved operation code and also all semantic fields
struct resolved_statement : public context::hlasm_statement, public semantics::complete_statement
{
    virtual const op_code& opcode_ref() const = 0;
    virtual processing_format format_ref() const = 0;

    position statement_position() const override { return stmt_range_ref().start; }

    resolved_statement()
        : context::hlasm_statement(context::statement_kind::RESOLVED)
    {}
};

struct resolved_statement_impl : public resolved_statement
{
    resolved_statement_impl(std::shared_ptr<const semantics::complete_statement> base_stmt, processing_status status)
        : base_stmt(std::move(base_stmt))
        , status(std::move(status))
    {}
    resolved_statement_impl(std::shared_ptr<const semantics::complete_statement> base_stmt,
        processing_status status,
        std::vector<diagnostic_op>&& diags)
        : base_stmt(std::move(base_stmt))
        , status(std::move(status))
        , statement_diagnostics(std::make_move_iterator(diags.begin()), std::make_move_iterator(diags.end()))
    {}

    std::shared_ptr<const semantics::complete_statement> base_stmt;
    processing_status status;
    std::vector<diagnostic_op> statement_diagnostics;

    const semantics::label_si& label_ref() const override { return base_stmt->label_ref(); }
    const semantics::instruction_si& instruction_ref() const override { return base_stmt->instruction_ref(); }
    const semantics::operands_si& operands_ref() const override { return base_stmt->operands_ref(); }
    const semantics::remarks_si& remarks_ref() const override { return base_stmt->remarks_ref(); }
    const range& stmt_range_ref() const override { return base_stmt->stmt_range_ref(); }
    std::span<const semantics::literal_si> literals() const override { return base_stmt->literals(); }
    const op_code& opcode_ref() const override { return status.second; }
    processing_format format_ref() const override { return status.first; }
    std::span<const diagnostic_op> diagnostics() const override
    {
        return { statement_diagnostics.data(), statement_diagnostics.data() + statement_diagnostics.size() };
    }
};

// statement used for preprocessing of resolved statements
struct rebuilt_statement : public resolved_statement
{
    rebuilt_statement(std::shared_ptr<const resolved_statement> base_stmt,
        std::optional<semantics::label_si> label,
        std::optional<semantics::operands_si> operands,
        std::optional<std::vector<semantics::literal_si>> literals)
        : base_stmt(base_stmt)
        , rebuilt_label(std::move(label))
        , rebuilt_operands(std::move(operands))
        , rebuilt_literals(std::move(literals))
    {}

    std::shared_ptr<const resolved_statement> base_stmt;
    std::optional<semantics::label_si> rebuilt_label;
    std::optional<semantics::operands_si> rebuilt_operands;
    std::optional<std::vector<semantics::literal_si>> rebuilt_literals;

    const semantics::label_si& label_ref() const override
    {
        return rebuilt_label ? *rebuilt_label : base_stmt->label_ref();
    }
    const semantics::instruction_si& instruction_ref() const override { return base_stmt->instruction_ref(); }
    const semantics::operands_si& operands_ref() const override
    {
        return rebuilt_operands ? *rebuilt_operands : base_stmt->operands_ref();
    }
    std::span<const semantics::literal_si> literals() const override
    {
        return rebuilt_literals ? *rebuilt_literals : base_stmt->literals();
    }
    const semantics::remarks_si& remarks_ref() const override { return base_stmt->remarks_ref(); }
    const range& stmt_range_ref() const override { return base_stmt->stmt_range_ref(); }
    const op_code& opcode_ref() const override { return base_stmt->opcode_ref(); }
    processing_format format_ref() const override { return base_stmt->format_ref(); }


    std::span<const diagnostic_op> diagnostics() const override { return base_stmt->diagnostics(); }
};

} // namespace hlasm_plugin::parser_library::processing
#endif
