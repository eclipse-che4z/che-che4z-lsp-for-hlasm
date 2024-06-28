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
struct resolved_statement : public context::hlasm_statement
{
    virtual const semantics::label_si& label_ref() const = 0;
    virtual const semantics::instruction_si& instruction_ref() const = 0;
    virtual const semantics::operands_si& operands_ref() const = 0;
    virtual const semantics::remarks_si& remarks_ref() const = 0;
    virtual std::span<const semantics::literal_si> literals() const = 0;

    virtual const op_code& opcode_ref() const = 0;
    virtual processing_format format_ref() const = 0;

    resolved_statement()
        : context::hlasm_statement(context::statement_kind::RESOLVED)
    {}

protected:
    ~resolved_statement() = default;
};

// statement used for preprocessing of resolved statements
struct rebuilt_statement final : public resolved_statement
{
    rebuilt_statement(std::shared_ptr<const resolved_statement> base_stmt,
        std::optional<semantics::label_si> label,
        std::optional<semantics::operands_si> operands,
        std::optional<std::vector<semantics::literal_si>> literals)
        : base_stmt(std::move(base_stmt))
        , rebuilt_label(std::move(label))
        , rebuilt_operands(std::move(operands))
        , rebuilt_literals(std::move(literals))
    {}

    std::shared_ptr<const resolved_statement> base_stmt;
    std::optional<semantics::label_si> rebuilt_label;
    std::optional<semantics::operands_si> rebuilt_operands;
    std::optional<std::vector<semantics::literal_si>> rebuilt_literals;

    const range& stmt_range_ref() const override { return base_stmt->stmt_range_ref(); }

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
    const op_code& opcode_ref() const override { return base_stmt->opcode_ref(); }
    processing_format format_ref() const override { return base_stmt->format_ref(); }


    std::span<const diagnostic_op> diagnostics() const override { return base_stmt->diagnostics(); }
};

} // namespace hlasm_plugin::parser_library::processing
#endif
