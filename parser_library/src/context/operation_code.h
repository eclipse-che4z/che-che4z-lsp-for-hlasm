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

#ifndef CONTEXT_OPERATION_CODE_H
#define CONTEXT_OPERATION_CODE_H

#include <variant>

#include "id_index.h"

namespace hlasm_plugin::parser_library::instructions {
class ca_instruction;
class assembler_instruction;
class machine_instruction;
class mnemonic_code;
} // namespace hlasm_plugin::parser_library::instructions

namespace hlasm_plugin::parser_library::context {
class macro_definition;


// structure that represents operation code of an instruction
struct opcode_t
{
    using opcode_variant = std::variant<std::monostate,
        const instructions::ca_instruction*,
        const instructions::assembler_instruction*,
        const instructions::machine_instruction*,
        const instructions::mnemonic_code*,
        macro_definition*>;

    id_index opcode;
    opcode_variant opcode_detail;

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return std::holds_alternative<std::monostate>(opcode_detail);
    }

    [[nodiscard]] constexpr bool is_macro() const noexcept
    {
        return std::holds_alternative<macro_definition*>(opcode_detail);
    }

    [[nodiscard]] constexpr bool is_asm() const noexcept { return !empty() && !is_macro(); }

    [[nodiscard]] constexpr macro_definition* get_macro_details() const noexcept
    {
        if (std::holds_alternative<macro_definition*>(opcode_detail))
            return std::get<macro_definition*>(opcode_detail);
        else
            return nullptr;
    }
};

} // namespace hlasm_plugin::parser_library::context


#endif
