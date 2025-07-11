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

#ifndef HLASMPLUGIN_PARSERLIBRARY_INSTR_OPERAND_H
#define HLASMPLUGIN_PARSERLIBRARY_INSTR_OPERAND_H

#include <assert.h>
#include <memory>
#include <string>
#include <vector>

#include "operand.h"

namespace hlasm_plugin::parser_library::instructions {
struct parameter;
struct machine_operand_format;
enum class machine_operand_type : uint8_t;
} // namespace hlasm_plugin::parser_library::instructions

namespace hlasm_plugin::parser_library::checking {
class data_def_type;

// extended class representing complex operands
// contains vector of all parameters of operand - for example FLAG, COMPAT, OPTABLE...
class complex_operand final : public asm_operand
{
public:
    std::string operand_identifier;
    std::vector<std::unique_ptr<asm_operand>> operand_parameters;

    complex_operand();
    complex_operand(std::string operand_identifier, std::vector<std::unique_ptr<asm_operand>> operand_params);
};

constexpr bool is_size_corresponding_signed(int operand, int size)
{
    auto boundary = 1LL << (size - 1);
    return operand < boundary && operand >= -boundary;
}

constexpr bool is_size_corresponding_unsigned(int operand, int size)
{
    return operand >= 0 && operand <= (1LL << size) - 1;
}

// class that represents both a simple operand both in assembler and machine instructions
class one_operand final : public asm_operand
{
public:
    // the string value of the operand
    std::string operand_identifier;
    int value;
    bool is_default;

    one_operand();

    one_operand(std::string operand_identifier, int value);

    one_operand(std::string operand_identifier);

    one_operand(int value);

    one_operand(std::string operand_identifier, range range);

    one_operand(int value, range range);
};

class empty_operand final : public asm_operand
{
public:
    empty_operand() = default;
    explicit empty_operand(range r);
};

} // namespace hlasm_plugin::parser_library::checking

#endif
