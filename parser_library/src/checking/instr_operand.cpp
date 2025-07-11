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


#include "instr_operand.h"

#include <algorithm>

#include "data_definition/data_def_type_base.h"
#include "instructions/instruction.h"

namespace hlasm_plugin::parser_library::checking {

asm_operand::asm_operand(const range& r) noexcept
    : operand(r)
{}

complex_operand::complex_operand() = default;

complex_operand::complex_operand(
    std::string operand_identifier, std::vector<std::unique_ptr<asm_operand>> operand_params)
    : operand_identifier(std::move(operand_identifier))
    , operand_parameters(std::move(operand_params)) {};

one_operand::one_operand()
    : operand_identifier("")
    , value(0)
    , is_default(true)
{}

one_operand::one_operand(std::string operand_identifier, int value)
    : operand_identifier(std::move(operand_identifier))
    , value(value)
    , is_default(false)
{}

one_operand::one_operand(std::string operand_identifier)
    : operand_identifier(std::move(operand_identifier))
    , value(0)
    , is_default(true)
{}

one_operand::one_operand(int value)
    : operand_identifier(std::to_string(value))
    , value(value)
    , is_default(false)
{}

one_operand::one_operand(std::string operand_identifier, range range)
    : asm_operand(range)
    , operand_identifier(std::move(operand_identifier))
    , value(0)
    , is_default(true)
{}

one_operand::one_operand(int value, range range)
    : asm_operand(range)
    , operand_identifier(std::to_string(value))
    , value(value)
    , is_default(false)
{}

empty_operand::empty_operand(range r)
    : asm_operand(r)
{}
} // namespace hlasm_plugin::parser_library::checking
