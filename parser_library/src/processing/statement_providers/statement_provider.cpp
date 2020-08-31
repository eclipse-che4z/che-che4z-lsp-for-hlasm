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

#include "statement_provider.h"

namespace hlasm_plugin::parser_library::processing {

statement_provider::statement_provider(const statement_provider_kind kind)
    : kind(kind)
{}

bool statement_provider::try_trigger_attribute_lookahead(const semantics::instruction_si& instruction)
{
    if (instruction.type != semantics::instruction_si_type::CONC)
        return false;

    const auto& chain = std::get<semantics::concat_chain>(instruction.value);

    return false;
}

bool statement_provider::try_trigger_attribute_lookahead(const context::hlasm_statement& statement) { return false; }

} // namespace hlasm_plugin::parser_library::processing
