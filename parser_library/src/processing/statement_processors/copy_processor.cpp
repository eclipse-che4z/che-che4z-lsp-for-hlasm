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

#include "copy_processor.h"

#include "context/hlasm_context.h"
#include "context/well_known.h"
#include "macrodef_processor.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

copy_processor::copy_processor(
    const analyzing_context& ctx, processing_state_listener& listener, copy_start_data start, diagnosable_ctx& diag_ctx)
    : statement_processor(processing_kind::COPY, ctx, diag_ctx)
    , listener_(listener)
    , start_(std::move(start))
    , macro_nest_(0)
    , result_ { {}, {}, start_.member_name, false }
    , first_statement_(true)
{}


std::optional<context::id_index> copy_processor::resolve_concatenation(
    const semantics::concat_chain&, const range&) const
{
    return std::nullopt;
}

std::optional<processing_status> copy_processor::get_processing_status(
    const std::optional<context::id_index>& instruction, const range&) const
{
    auto status = macrodef_processor::get_macro_processing_status(instruction, hlasm_ctx);
    status.first.kind = processing_kind::COPY;
    return status;
}

void copy_processor::process_statement(context::shared_stmt_ptr statement)
{
    if (first_statement_)
    {
        result_.definition_location = hlasm_ctx.current_statement_location();
        first_statement_ = false;
    }

    if (auto res_stmt = statement->access_resolved())
    {
        if (res_stmt->opcode_ref().value == context::well_known::MACRO)
            process_MACRO();
        else if (res_stmt->opcode_ref().value == context::well_known::MEND)
            process_MEND();
    }

    result_.definition.push_back(std::move(statement));
}

void copy_processor::end_processing()
{
    if (first_statement_)
    {
        result_.definition_location = hlasm_ctx.current_statement_location(); // empty file
    }

    if (macro_nest_ > 0)
    {
        range r(hlasm_ctx.current_statement_position());
        add_diagnostic(diagnostic_op::error_E061(start_.member_name.to_string_view(), r));
        result_.invalid_member = true;
    }

    hlasm_ctx.pop_statement_processing();

    listener_.finish_copy_member(std::move(result_));
}

bool copy_processor::terminal_condition(const statement_provider_kind prov_kind) const
{
    return prov_kind == statement_provider_kind::OPEN;
}

bool copy_processor::finished() { return false; }

void copy_processor::process_MACRO() { ++macro_nest_; }

void copy_processor::process_MEND()
{
    --macro_nest_;
    if (macro_nest_ < 0)
    {
        range r(hlasm_ctx.current_statement_position());
        add_diagnostic(diagnostic_op::error_E061(start_.member_name.to_string_view(), r));
        result_.invalid_member = true;
    }
}
