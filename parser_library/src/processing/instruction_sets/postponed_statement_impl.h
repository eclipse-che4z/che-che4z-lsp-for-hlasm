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

#ifndef PROCESSING_POSTPONED_STATEMENT_IMPL_H
#define PROCESSING_POSTPONED_STATEMENT_IMPL_H

#include <variant>

#include "context/ordinary_assembly/postponed_statement.h"
#include "processing/rebuilt_statement.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

// implementation of postponed_statement interface
struct postponed_statement_impl : public context::postponed_statement, public resolved_statement
{
    postponed_statement_impl(rebuilt_statement stmt, context::processing_stack_t stmt_location_stack)
        : stmt(std::move(stmt))
        , stmt_location_stack(std::move(stmt_location_stack))
    {}

    rebuilt_statement stmt;
    context::processing_stack_t stmt_location_stack;

    virtual const semantics::label_si& label_ref() const override { return stmt.label_ref(); }
    virtual const semantics::instruction_si& instruction_ref() const override { return stmt.instruction_ref(); }
    virtual const semantics::operands_si& operands_ref() const override { return stmt.operands_ref(); }
    virtual const semantics::remarks_si& remarks_ref() const override { return stmt.remarks_ref(); }
    virtual const range& stmt_range_ref() const override { return stmt.stmt_range_ref(); }
    virtual const op_code& opcode_ref() const override { return stmt.opcode_ref(); }
    virtual processing_format format_ref() const override { return stmt.format_ref(); }

    virtual const context::processing_stack_t& location_stack() const { return stmt_location_stack; }
};



} // namespace processing
} // namespace parser_library
} // namespace hlasm_plugin
#endif
