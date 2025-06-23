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

#include "instructions/instruction.h"

namespace hlasm_plugin::parser_library::processing {

constexpr auto common_processing_status_cache_key_details = std::pair((unsigned char)1, (unsigned char)0);

// (value of L'* expression, reladdr mask)
std::pair<unsigned char, unsigned char> get_processing_status_cache_key_details(const op_code& op) noexcept
{
    switch (op.type)
    {
        case context::instruction_type::MACH:
            return { (unsigned char)(op.instr_mach->size_in_bits() / 8), (unsigned char)op.instr_mach->reladdr_mask() };
        case context::instruction_type::MNEMO:
            return { (unsigned char)(op.instr_mnemo->size_in_bits() / 8),
                (unsigned char)op.instr_mnemo->reladdr_mask() };
        default:
            return common_processing_status_cache_key_details;
    }
}

// Generates value of L'* expression
unsigned char processing_status_cache_key::generate_loctr_len(const op_code& op) noexcept
{
    switch (op.type)
    {
        case context::instruction_type::MACH:
            return static_cast<unsigned char>(op.instr_mach->size_in_bits() / 8);
        case context::instruction_type::MNEMO:
            return static_cast<unsigned char>(op.instr_mnemo->size_in_bits() / 8);
        default:
            return 1;
    }
}

processing_status_cache_key::processing_status_cache_key(
    const processing_status& s, std::pair<unsigned char, unsigned char> details) noexcept
    : form(s.first.form)
    , occurrence(s.first.occurrence)
    , is_alias(s.second.type == context::instruction_type::ASM && s.second.value.to_string_view() == "ALIAS")
    , loctr_len(details.first)
    , rel_addr(details.second)
{}

processing_status_cache_key::processing_status_cache_key(const processing_status& s) noexcept
    : processing_status_cache_key(s, get_processing_status_cache_key_details(s.second))
{}

} // namespace hlasm_plugin::parser_library::processing
