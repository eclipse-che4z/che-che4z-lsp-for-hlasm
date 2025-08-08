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

#ifndef PROCESSING_OP_CODE_H
#define PROCESSING_OP_CODE_H

#include <utility>

#include "context/id_index.h"
#include "context/instruction_type.h"
#include "processing_format.h"

namespace hlasm_plugin::parser_library::instructions {
class assembler_instruction;
class mnemonic_code;
class machine_instruction;
} // namespace hlasm_plugin::parser_library::instructions

namespace hlasm_plugin::parser_library::context {
class macro_definition;
} // namespace hlasm_plugin::parser_library::context

namespace hlasm_plugin::parser_library::processing {

// structure holding resolved operation code of the instruction (solving OPSYNs and so on)
struct op_code
{
    using enum context::instruction_type;

    constexpr op_code() noexcept
        : type(UNDEF)
    {}
    constexpr op_code(context::id_index value, context::instruction_type type) noexcept
        : value(value)
        , type(type)
    {
        assert(type != MAC && type != MACH && type != MNEMO && type != ASM);
    }
    constexpr op_code(context::id_index value, context::macro_definition* mac_def) noexcept
        : value(value)
        , mac_def(mac_def)
        , type(MAC)
    {}
    constexpr op_code(context::id_index value, const instructions::assembler_instruction* asm_instr) noexcept
        : value(value)
        , instr_asm(asm_instr)
        , type(ASM)
    {}
    constexpr op_code(context::id_index value, const instructions::machine_instruction* mach_instr) noexcept
        : value(value)
        , instr_mach(mach_instr)
        , type(MACH)
    {}
    constexpr op_code(context::id_index value, const instructions::mnemonic_code* mach_mnemo) noexcept
        : value(value)
        , instr_mnemo(mach_mnemo)
        , type(MNEMO)
    {}

    context::id_index value;
    union
    {
        const void* _empty = nullptr;
        context::macro_definition* mac_def;
        const instructions::assembler_instruction* instr_asm;
        const instructions::machine_instruction* instr_mach;
        const instructions::mnemonic_code* instr_mnemo;
    };
    context::instruction_type type;
};

using processing_status = std::pair<processing_format, op_code>;

class processing_status_cache_key
{
    processing_form form;
    operand_occurrence occurrence;
    unsigned char loctr_len;
    unsigned char rel_addr;

    explicit processing_status_cache_key(
        const processing_status& s, std::pair<unsigned char, unsigned char> details) noexcept;

public:
    friend bool operator==(processing_status_cache_key l, processing_status_cache_key r) = default;

    explicit processing_status_cache_key(const processing_status& s) noexcept;

    static unsigned char generate_loctr_len(const op_code& op) noexcept;
};

} // namespace hlasm_plugin::parser_library::processing
#endif
