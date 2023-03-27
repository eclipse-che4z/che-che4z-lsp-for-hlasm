/*
 * Copyright (c) 2023 Broadcom.
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

#include "statement_processor.h"

#include "semantics/statement_fields.h"

namespace hlasm_plugin::parser_library::processing {

std::optional<context::id_index> statement_processor::resolve_instruction(
    const semantics::instruction_si& instruction) const
{
    if (instruction.type == semantics::instruction_si_type::CONC)
        return resolve_concatenation(std::get<semantics::concat_chain>(instruction.value), instruction.field_range);
    else
        return std::get<context::id_index>(instruction.value);
}

} // namespace hlasm_plugin::parser_library::processing
