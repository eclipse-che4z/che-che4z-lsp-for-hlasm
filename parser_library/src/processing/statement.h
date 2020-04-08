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

using complete_stmt_t = std::variant<semantics::statement_si,
    semantics::statement_si_defer_done,
    std::shared_ptr<semantics::statement_si_defer_done>>;

// statement that contains resolved operation code and also all semantic fields
struct resolved_statement : public context::hlasm_statement, public semantics::complete_statement
{
    virtual const op_code& opcode_ref() const = 0;

    virtual position statement_position() const override { return stmt_range_ref().start; }

    resolved_statement()
        : context::hlasm_statement(context::statement_kind::RESOLVED)
    {}
};

struct resolved_statement_impl : public resolved_statement
{
    resolved_statement_impl(semantics::statement_si stmt, op_code opcode)
        : opcode(opcode)
        , value(std::move(stmt))
    {}
    resolved_statement_impl(semantics::statement_si_defer_done stmt, op_code opcode)
        : opcode(opcode)
        , value(std::move(stmt))
    {}
    resolved_statement_impl(std::shared_ptr<semantics::statement_si_defer_done> stmt, op_code opcode)
        : opcode(opcode)
        , value(stmt)
    {}

    op_code opcode;
    complete_stmt_t value;

    virtual const semantics::label_si& label_ref() const { return get_stmt().label_ref(); }
    virtual const semantics::instruction_si& instruction_ref() const { return get_stmt().instruction_ref(); }
    virtual const semantics::operands_si& operands_ref() const { return get_stmt().operands_ref(); }
    virtual const semantics::remarks_si& remarks_ref() const { return get_stmt().remarks_ref(); }
    virtual const range& stmt_range_ref() const { return get_stmt().stmt_range_ref(); }
    virtual const op_code& opcode_ref() const { return opcode; }

private:
    const semantics::complete_statement& get_stmt() const
    {
        if (std::holds_alternative<semantics::statement_si>(value))
            return std::get<semantics::statement_si>(value);
        else if (std::holds_alternative<semantics::statement_si_defer_done>(value))
            return std::get<semantics::statement_si_defer_done>(value);
        else
            return *std::get<std::shared_ptr<semantics::statement_si_defer_done>>(value);
    }
};

// statement used for preprocessing of resolved statements
struct rebuilt_statement : public resolved_statement
{
    rebuilt_statement(const resolved_statement_impl& base_stmt,
        std::optional<semantics::label_si> label,
        std::optional<semantics::operands_si> operands)
        : base_value(&base_stmt)
        , rebuilt_label(std::move(label))
        , rebuilt_operands(std::move(operands))
    {}

    rebuilt_statement(resolved_statement_impl&& base_stmt,
        std::optional<semantics::label_si> label,
        std::optional<semantics::operands_si> operands)
        : base_value(std::move(base_stmt))
        , rebuilt_label(std::move(label))
        , rebuilt_operands(std::move(operands))
    {}

    std::variant<const resolved_statement_impl*, resolved_statement_impl> base_value;
    std::optional<semantics::label_si> rebuilt_label;
    std::optional<semantics::operands_si> rebuilt_operands;

    virtual const semantics::label_si& label_ref() const
    {
        return rebuilt_label ? *rebuilt_label : get_stmt().label_ref();
    }
    virtual const semantics::instruction_si& instruction_ref() const { return get_stmt().instruction_ref(); }
    virtual const semantics::operands_si& operands_ref() const
    {
        return rebuilt_operands ? *rebuilt_operands : get_stmt().operands_ref();
    }
    virtual const semantics::remarks_si& remarks_ref() const { return get_stmt().remarks_ref(); }
    virtual const range& stmt_range_ref() const { return get_stmt().stmt_range_ref(); }
    virtual const op_code& opcode_ref() const { return get_stmt().opcode_ref(); }

private:
    const resolved_statement& get_stmt() const
    {
        if (std::holds_alternative<const resolved_statement_impl*>(base_value))
            return *std::get<const resolved_statement_impl*>(base_value);
        else
            return std::get<resolved_statement_impl>(base_value);
    }
};


} // namespace processing
} // namespace parser_library
} // namespace hlasm_plugin
#endif
