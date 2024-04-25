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
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "context/ordinary_assembly/alignment.h"
#include "diagnostic.h"
#include "diagnostic_op.h"
#include "operand.h"

namespace hlasm_plugin::parser_library::checking {
class data_def_type;

enum class address_state
{
    RES_VALID,
    RES_INVALID,
    UNRES
};

/*
FIRST_OMITTED = D(,B)
SECOND_OMMITED = D(X,)
PRESENT - D(X,B)
ONE_OP - D(B)
*/
enum class operand_state
{
    FIRST_OMITTED,
    SECOND_OMITTED,
    PRESENT,
    ONE_OP
};

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

enum class machine_operand_type : uint8_t
{
    NONE,
    MASK,
    REG,
    IMM,
    DISPLC,
    BASE,
    LENGTH,
    VEC_REG,
    DIS_REG,
    RELOC_IMM,
};

// Describes a component of machine operand format. Specifies allowed values.
struct parameter
{
    bool is_signed;
    uint8_t size;
    machine_operand_type type;

    constexpr bool is_empty() const { return (!is_signed && type == machine_operand_type::NONE && size == 0); }

    bool operator==(const parameter&) const = default;

    std::string to_string() const;
};

// Representation of machine operand formats and serves as a template for the checker.
// Consists of 1 parameter when only simple operand is allowed and of 3 parameters when address operand is allowed
// D(F,S)
struct machine_operand_format
{
    parameter identifier; // used as displacement operand in address operand
    parameter first; // empty when simple operand
    parameter second; // empty when simple operand
    bool optional = false;

    constexpr machine_operand_format(parameter id, parameter first, parameter second, bool optional = false)
        : identifier(id)
        , first(first)
        , second(second)
        , optional(optional)
    {
        assert(!second.is_empty() || first.is_empty());
    };

    std::string to_string(std::optional<size_t> i = std::nullopt) const;
};

// Abstract class that represents a machine operand suitable for checking.
class machine_operand : public virtual operand
{
public:
    machine_operand();

    // check whether the operand satisfies its format
    virtual bool check(diagnostic_op& diag,
        const machine_operand_format& to_check,
        std::string_view instr_name,
        const range& stmt_range) const = 0;

    diagnostic_op get_simple_operand_expected(
        const machine_operand_format& op_format, std::string_view instr_name, const range& stmt_range) const;

    static bool is_size_corresponding_signed(int operand, int size);
    static bool is_size_corresponding_unsigned(int operand, int size);
    static bool is_operand_corresponding(int operand, parameter param);
    static bool is_simple_operand(const machine_operand_format& operand);
};

// Represents address operand D(B) or D(F,B)
class address_operand final : public machine_operand
{
public:
    address_state state;
    int displacement;
    int first_op;
    int second_op;
    operand_state op_state;

    address_operand(address_state state, int displacement, int first, int second);
    address_operand(address_state state, int displacement, int first, int second, operand_state op_state);

    diagnostic_op get_first_parameter_error(machine_operand_type op_type,
        std::string_view instr_name,
        long long from,
        long long to,
        const range& stmt_range) const;

    bool check(diagnostic_op& diag,
        const machine_operand_format& to_check,
        std::string_view instr_name,
        const range& range) const override;

    bool is_length_corresponding(int param_value, int length_size) const;
};

// class that represents both a simple operand both in assembler and machine instructions
class one_operand final : public asm_operand, public machine_operand
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

    one_operand(const one_operand& op);

    bool check(diagnostic_op& diag,
        const machine_operand_format& to_check,
        std::string_view instr_name,
        const range& stmt_range) const override;
};

class empty_operand final : public machine_operand, public asm_operand
{
public:
    empty_operand();
    explicit empty_operand(range r);

    bool check(diagnostic_op& diag,
        const machine_operand_format& to_check,
        std::string_view instr_name,
        const range& stmt_range) const override;
};

} // namespace hlasm_plugin::parser_library::checking

#endif
