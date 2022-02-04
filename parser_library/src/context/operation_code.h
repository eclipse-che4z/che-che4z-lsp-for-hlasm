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

#include "id_storage.h"
#include "macro.h"

namespace hlasm_plugin::parser_library::context {
struct ca_instruction;
struct assembler_instruction;
class machine_instruction;
struct mnemonic_code;

// structure that represents operation code of an instruction
struct opcode_t
{
    using opcode_variant = std::variant<std::monostate,
        const ca_instruction*,
        const assembler_instruction*,
        const machine_instruction*,
        const mnemonic_code*,
        macro_def_ptr>;

    id_index opcode = nullptr;
    opcode_variant opcode_detail;

    explicit operator bool() const { return !std::holds_alternative<std::monostate>(opcode_detail); }
};

} // namespace hlasm_plugin::parser_library::context


#endif
