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

#include "id_storage.h"
#include "instruction.h"
#include "macro.h"

namespace hlasm_plugin::parser_library::context {

// structure that represents operation code of an instruction
struct opcode_t
{
    macro_def_ptr macro_opcode;

    id_index machine_opcode;
    instruction::instruction_array machine_source;

    opcode_t()
        : macro_opcode(nullptr)
        , machine_opcode(nullptr)
        , machine_source(instruction::instruction_array::CA)
    {}

    explicit operator bool() const { return macro_opcode || machine_opcode; }
};

} // namespace hlasm_plugin::parser_library::context


#endif
