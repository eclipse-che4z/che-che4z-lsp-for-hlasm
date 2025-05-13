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

constexpr auto common_processing_status_cache_key_details = std::pair((unsigned char)1, (unsigned char)0);

// (value of L'* expression, reladdr mask)
inline std::pair<unsigned char, unsigned char> get_processing_status_cache_key_details(std::string_view id) noexcept
{
    if (!id.empty())
    {
        const auto [mi, mn] = context::instruction::find_machine_instruction_or_mnemonic(id);

        if (mn)
            return std::pair(static_cast<unsigned char>(mi->size_in_bits() / 8), mn->reladdr_mask().mask());
        if (mi)
            return std::pair(static_cast<unsigned char>(mi->size_in_bits() / 8), mi->reladdr_mask().mask());
    }
    return common_processing_status_cache_key_details;
}

// Generates value of L'* expression
unsigned char processing_status_cache_key::generate_loctr_len(std::string_view id) noexcept
{
    if (!id.empty())
    {
        if (const auto [mi, _] = context::instruction::find_machine_instruction_or_mnemonic(id); mi)
            return static_cast<unsigned char>(mi->size_in_bits() / 8);
    }
    return 1;
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
    : processing_status_cache_key(s,
          s.second.type != context::instruction_type::MACH
              ? common_processing_status_cache_key_details
              : get_processing_status_cache_key_details(s.second.value.to_string_view()))
{}
} // namespace hlasm_plugin::parser_library::processing
