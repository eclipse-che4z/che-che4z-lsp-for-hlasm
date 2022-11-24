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

#include "expressions/mach_expr_term.h"
#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::semantics {

endevor_statement_si::endevor_statement_si(preproc_details details)
    : preprocessor_statement_si(std::move(details), context::id_storage::well_known::COPY)
{
    m_details.instruction.name = "-INC";
}

cics_statement_si::cics_statement_si(preproc_details details)
    : preprocessor_statement_si(std::move(details), context::id_index())
{}

} // namespace hlasm_plugin::parser_library::semantics