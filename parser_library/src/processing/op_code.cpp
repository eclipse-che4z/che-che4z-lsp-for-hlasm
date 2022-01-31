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

#include "context/instruction.h"

namespace hlasm_plugin::parser_library::processing {

inline unsigned char get_reladdr_bitmask(context::id_index id)
{
    if (!id || id->empty())
        return 0;

    if (auto p_instr = context::instruction::find_machine_instructions(*id))
        return p_instr->reladdr_mask.mask();

    if (auto p_mnemo = context::instruction::find_mnemonic_codes(*id))
        return p_mnemo->reladdr_mask.mask();

    return 0;
}

// Generates value of L'* expression
unsigned char processing_status_cache_key::generate_loctr_len(context::id_index id)
{
    if (id && !id->empty())
    {
        if (auto p_instr = context::instruction::find_machine_instructions(*id))
            return static_cast<unsigned char>(p_instr->size_for_alloc / 8);

        if (auto p_mnemo = context::instruction::find_mnemonic_codes(*id))
            return static_cast<unsigned char>(p_mnemo->instruction->size_for_alloc / 8);
    }
    return 1;
}

processing_status_cache_key::processing_status_cache_key(const processing_status& s)
    : form(s.first.form)
    , occurence(s.first.occurence)
    , is_alias(s.second.type == context::instruction_type::ASM && s.second.value && *s.second.value == "ALIAS")
    , loctr_len(generate_loctr_len(s.second.value))
    , rel_addr(get_reladdr_bitmask(s.second.value))
{}
} // namespace hlasm_plugin::parser_library::processing
