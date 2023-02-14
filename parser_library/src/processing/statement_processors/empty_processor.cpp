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

#include "empty_processor.h"

#include <utility>

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

empty_processor::empty_processor(analyzing_context ctx)
    : statement_processor(processing_kind::ORDINARY, std::move(ctx))
{}

std::optional<processing_status> empty_processor::get_processing_status(const semantics::instruction_si&) const
{
    return std::make_pair(processing_format(processing_kind::ORDINARY, processing_form::IGNORED), op_code());
}

void empty_processor::process_statement(context::shared_stmt_ptr) {}

void empty_processor::end_processing() {}

bool empty_processor::terminal_condition(const statement_provider_kind) const { return true; }

bool empty_processor::finished() { return true; }

void empty_processor::collect_diags() const {}
