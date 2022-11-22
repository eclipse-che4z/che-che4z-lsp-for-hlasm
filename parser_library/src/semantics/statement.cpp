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

endevor_statement_si::endevor_statement_si(range stmt_range,
    range instruction_range,
    std::string_view copy_member,
    range copy_member_range,
    remarks_si remarks,
    context::id_storage& ids)
    : preprocessor_statement_si(std::move(stmt_range),
        label_si(range()),
        instruction_si(std::move(instruction_range), context::id_index("-INC"), true),
        operands_si(copy_member_range, {}),
        std::move(remarks),
        context::id_storage::well_known::COPY)
{
    operands.value.emplace_back(std::make_unique<expr_assembler_operand>(
        std::make_unique<expressions::mach_expr_symbol>(ids.add(copy_member), context::id_index(), copy_member_range),
        std::string(copy_member),
        std::move(copy_member_range)));
}

} // namespace hlasm_plugin::parser_library::semantics