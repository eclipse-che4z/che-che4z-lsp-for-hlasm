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

#include "statement.h"

namespace hlasm_plugin::parser_library::semantics {

endevor_statement_si::endevor_statement_si(preproc_details details)
    : preprocessor_statement_si(std::move(details), true)
{
    m_details.instruction.nr.name = "-INC";
}

deferred_statement::deferred_statement(range stmt_range,
    label_si label,
    instruction_si instruction,
    deferred_operands_si deferred_operands,
    std::vector<diagnostic_op>&& diags,
    size_t operand_diags_start_index)
    : context::hlasm_statement(context::statement_kind::DEFERRED)
    , label(std::move(label))
    , instruction(std::move(instruction))
    , deferred_operands(std::move(deferred_operands))
    , statement_diagnostics(std::make_move_iterator(diags.begin()), std::make_move_iterator(diags.end()))
    , operand_diags_start_index(operand_diags_start_index)
    , stmt_range(stmt_range)
{}

preprocessor_statement_si::preprocessor_statement_si(preproc_details details, bool copylike)
    : m_details(std::move(details))
    , m_copylike(copylike)
{}

} // namespace hlasm_plugin::parser_library::semantics
