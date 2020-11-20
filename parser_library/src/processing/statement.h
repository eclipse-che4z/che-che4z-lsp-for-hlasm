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

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

// statement that contains resolved operation code and also all semantic fields
struct resolved_statement : public context::hlasm_statement, public semantics::complete_statement
{
    virtual const op_code& opcode_ref() const = 0;
    virtual processing_format format_ref() const = 0;

    virtual position statement_position() const override { return stmt_range_ref().start; }

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

    std::shared_ptr<const semantics::complete_statement> base_stmt;
    processing_status status;

    virtual const semantics::label_si& label_ref() const override { return base_stmt->label_ref(); }
    virtual const semantics::instruction_si& instruction_ref() const override { return base_stmt->instruction_ref(); }
    virtual const semantics::operands_si& operands_ref() const override { return base_stmt->operands_ref(); }
    virtual const semantics::remarks_si& remarks_ref() const override { return base_stmt->remarks_ref(); }
    virtual const range& stmt_range_ref() const override { return base_stmt->stmt_range_ref(); }
    virtual const op_code& opcode_ref() const override { return status.second; }
    virtual processing_format format_ref() const override { return status.first; }
};

// statement used for preprocessing of resolved statements
struct rebuilt_statement : public resolved_statement
{
    rebuilt_statement(std::shared_ptr<const resolved_statement> base_stmt,
        std::optional<semantics::label_si> label,
        std::optional<semantics::operands_si> operands)
        : base_stmt(base_stmt)
        , rebuilt_label(std::move(label))
        , rebuilt_operands(std::move(operands))
    {}

    std::shared_ptr<const resolved_statement> base_stmt;
    std::optional<semantics::label_si> rebuilt_label;
    std::optional<semantics::operands_si> rebuilt_operands;

    virtual const semantics::label_si& label_ref() const
    {
        return rebuilt_label ? *rebuilt_label : base_stmt->label_ref();
    }
    virtual const semantics::instruction_si& instruction_ref() const { return base_stmt->instruction_ref(); }
    virtual const semantics::operands_si& operands_ref() const
    {
        return rebuilt_operands ? *rebuilt_operands : base_stmt->operands_ref();
    }
    virtual const semantics::remarks_si& remarks_ref() const { return base_stmt->remarks_ref(); }
    virtual const range& stmt_range_ref() const { return base_stmt->stmt_range_ref(); }
    virtual const op_code& opcode_ref() const { return base_stmt->opcode_ref(); }
    virtual processing_format format_ref() const { return base_stmt->format_ref(); }
};


} // namespace processing
} // namespace parser_library
} // namespace hlasm_plugin
#endif
