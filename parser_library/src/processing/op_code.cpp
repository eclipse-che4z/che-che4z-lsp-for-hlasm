/*
 * Copyright (c) 2021 Broadcom.
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

#include "op_code.h"

#include <limits>

#include "context/instruction.h"

namespace hlasm_plugin::parser_library::processing {

// Generates a bitmask for an arbitrary machine instruction indicating which operands
// are of the RI type (and therefore are modified by transform_reloc_imm_operands)
unsigned char processing_status_cache_key::generate_reladdr_bitmask(context::id_index id)
{
    auto p_instr = context::instruction::machine_instructions.find(*id);
    if (p_instr == context::instruction::machine_instructions.end())
        return 0;

    unsigned char result = 0;

    const auto& instr = p_instr->second;
    assert(instr.operands.size() <= std::numeric_limits<decltype(result)>::digits);

    decltype(result) top_bit = 1 << (std::numeric_limits<decltype(result)>::digits - 1);

    for (const auto& op : instr.operands)
    {
        if (op.identifier.type == checking::machine_operand_type::RELOC_IMM)
            result |= top_bit;
        top_bit >>= 1;
    }

    return result;
}

processing_status_cache_key::processing_status_cache_key(const processing_status& s)
    : form(s.first.form)
    , occurence(s.first.occurence)
    , is_alias(s.second.type == context::instruction_type::ASM && s.second.value && *s.second.value == "ALIAS")
    , rel_addr(s.second.value ? generate_reladdr_bitmask(s.second.value) : 0)
{}
} // namespace hlasm_plugin::parser_library::processing
