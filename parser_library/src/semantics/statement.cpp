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

endevor_statement_si::endevor_statement_si(preproc_details details, context::id_storage& ids)
    : preprocessor_statement_si(std::move(stmt_range),
        label_si(range()),
        instruction_si(std::move(details.instruction.r), context::id_index("-INC"), true),
        operands_si(std::move(details.operands.second), {}),
        std::move(semantics::remarks_si(std::move(details.remarks.second), std::move(details.remarks.first))),
        context::id_storage::well_known::COPY)
{
    operands.value.emplace_back(std::make_unique<expr_assembler_operand>(
        std::make_unique<expressions::mach_expr_symbol>(
            ids.add(details.operands.first.front().name), context::id_index(), details.operands.first.front().r),
        std::string(details.operands.first.front().name),
        std::move(details.operands.first.front().r)));
}

cics_statement_si::cics_statement_si(preproc_details details, context::id_storage& ids)
    : preprocessor_statement_si(std::move(stmt_range),
        label_si(details.label.r, ord_symbol_string { ids.add(details.label.name), std::string(details.label.name) }),
        instruction_si(std::move(details.instruction.r), ids.add(details.instruction.name), true),
        operands_si(std::move(details.operands.second), {}),
        std::move(semantics::remarks_si(std::move(details.remarks.second), std::move(details.remarks.first))),
        context::id_index())
{
    for (auto& op : details.operands.first)
        this->operands.value.emplace_back(std::make_unique<expr_machine_operand>(
            std::make_unique<expressions::mach_expr_symbol>(ids.add(op.name), context::id_index(), op.r),
            std::move(op.r)));
}

} // namespace hlasm_plugin::parser_library::semantics