/*
 * Copyright (c) 2022 Broadcom.
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

#include "endevor_processor.h"

#include "processing/instruction_sets/asm_processor.h"

namespace hlasm_plugin::parser_library::processing {

endevor_processor::endevor_processor(analyzing_context ctx, workspaces::parse_lib_provider& lib_provider)
    : statement_processor(processing_kind::ORDINARY, std::move(ctx))
    , m_lib_provider(lib_provider)
{}

processing_status endevor_processor::get_processing_status(const semantics::instruction_si&) const
{
    return std::make_pair(processing_format(processing_kind::ORDINARY, processing_form::IGNORED), op_code());
}

void endevor_processor::process_statement(context::shared_stmt_ptr s)
{
    auto prepro_stmt = s->access_preproc();
    if (!prepro_stmt)
        return;

    asm_processor::process_copy(*prepro_stmt, ctx, m_lib_provider, this);
}

void endevor_processor::end_processing() {}

bool endevor_processor::terminal_condition(const statement_provider_kind) const { return true; }

bool endevor_processor::finished() { return false; }

void endevor_processor::collect_diags() const {}

} // namespace hlasm_plugin::parser_library::processing