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

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::checking;

complex_operand::complex_operand() {}

complex_operand::complex_operand(
    std::string operand_identifier, std::vector<std::unique_ptr<asm_operand>> operand_params)
    : operand_identifier(operand_identifier)
    , operand_parameters(std::move(operand_params)) {};

machine_operand::machine_operand() {}

bool machine_operand::is_operand_corresponding(int operand, parameter param)
{
    if (param.is_signed && is_size_corresponding_signed(operand, param.size))
        return true;
    else if (!param.is_signed && is_size_corresponding_unsigned(operand, param.size))
        return true;
    return false;
}

bool machine_operand::is_size_corresponding_signed(int operand, int size)
{
    auto boundary = 1ll << (size - 1);
    return operand < boundary && operand >= -boundary;
}

bool machine_operand::is_size_corresponding_unsigned(int operand, int size)
{
    return operand >= 0 && operand <= (1ll << size) - 1;
}

bool machine_operand::is_simple_operand(const machine_operand_format& operand)
{
    return (operand.first.is_signed == false && operand.first.size == 0 && operand.second.is_signed == false
        && operand.second.size == 0 && operand.first.type == machine_operand_type::NONE
        && operand.second.type == machine_operand_type::NONE);
}

address_operand::address_operand(address_state state, int displacement, int first, int second)
    : state(state)
    , displacement(displacement)
    , first_op(first)
    , second_op(second)
    , op_state(operand_state::PRESENT) {};

address_operand::address_operand(address_state state, int displacement, int first, int second, operand_state op_state)
    : state(state)
    , displacement(displacement)
    , first_op(first)
    , second_op(second)
    , op_state(op_state) {};

hlasm_plugin::parser_library::diagnostic_op address_operand::get_first_parameter_error(
    const machine_operand_type& op_type,
    const std::string& instr_name,
    long long from,
    long long to,
    const range& stmt_range) const
{
    switch (op_type)
    {
        case machine_operand_type::LENGTH: // D(L,B)
            return diagnostic_op::error_M132(instr_name, from, to, operand_range);
        case machine_operand_type::DIS_REG: // D(X,B)
            return diagnostic_op::error_M135(instr_name, from, to, operand_range);
        case machine_operand_type::REG: // D(R,B)
            return diagnostic_op::error_M133(instr_name, from, to, operand_range);
        case machine_operand_type::VEC_REG: // D(V,B)
            return diagnostic_op::error_M134(instr_name, from, to, operand_range);
    }
    assert(false);
    return diagnostic_op::error_I999(instr_name, stmt_range);
}

bool address_operand::check(diagnostic_op& diag,
    const machine_operand_format to_check,
    const std::string& instr_name,
    const range& stmt_range) const
{
    if (is_simple_operand(to_check))
    {
        // operand must be simple
        diag = get_simple_operand_expected(to_check, instr_name, stmt_range);
        return false;
    }
    if (op_state == operand_state::SECOND_OMITTED)
    {
        diag = diagnostic_op::error_M004(instr_name, stmt_range);
        return false;
    }
    if (state == address_state::RES_VALID)
        return true;
    else if (state == address_state::RES_INVALID)
    {
        diag = diagnostic_op::error_M010(instr_name, stmt_range);
        return false;
    }
    else
    {
        // check displacement
        if (!is_operand_corresponding(displacement, to_check.identifier))
        {
            if (to_check.identifier.is_signed)
            {
                auto boundary = 1ll << (to_check.identifier.size - 1);
                diag = diagnostic_op::error_M130(instr_name, -boundary, boundary - 1, operand_range);
            }
            else
                diag = diagnostic_op::error_M130(instr_name, 0, (1ll << to_check.identifier.size) - 1, operand_range);
            return false;
        }
        // check the D(B) operand format
        if (to_check.first.is_empty())
        {
            if (op_state != operand_state::ONE_OP)
            {
                // error, cannot be present
                diag = diagnostic_op::error_M104(instr_name, operand_range);
                return false;
            }
            // check second parameter, in all cases this is the base parameter
            if (!is_operand_corresponding(second_op, to_check.second))
            {
                diag = diagnostic_op::error_M131(instr_name, operand_range);
                return false;
            }
        }
        else
        {
            // therefore a D(X,B) format expected, can be specified either by D(X,B), D(X) or D(,B)

            // first check the base register in case it is specified
            if (op_state == operand_state::FIRST_OMITTED || op_state == operand_state::PRESENT)
            {
                if (!is_operand_corresponding(second_op, to_check.second))
                {
                    diag = diagnostic_op::error_M131(instr_name, operand_range);
                    return false;
                }
            }
            // base is now checked, now check the first parameter, which is specified either in D(X,B) or D(X)
            if (op_state != operand_state::FIRST_OMITTED)
            {
                int actual_val = 0;
                if (op_state == operand_state::PRESENT)
                    actual_val = first_op;
                else
                    actual_val = second_op;
                // check whether value is corresponding

                // length parameters can have +1 values specified
                if (to_check.first.type == machine_operand_type::LENGTH
                    && !is_length_corresponding(first_op, to_check.first.size))
                {
                    diag = get_first_parameter_error(
                        to_check.first.type, instr_name, 0, 1ll << to_check.first.size, stmt_range);
                    return false;
                }
                if (to_check.first.type != machine_operand_type::LENGTH
                    && !is_operand_corresponding(first_op, to_check.first))
                {
                    assert(!to_check.first.is_signed);
                    diag = get_first_parameter_error(
                        to_check.first.type, instr_name, 0, (1ll << to_check.first.size) - 1, stmt_range);
                    return false;
                }
            }
        }
        return true;
    }
}

bool hlasm_plugin::parser_library::checking::address_operand::is_length_corresponding(
    int param_value, int length_size) const
{
    auto boundary = 1ll << length_size;
    if (param_value > boundary || param_value < 0)
        return false;
    return true;
};

hlasm_plugin::parser_library::diagnostic_op machine_operand::get_simple_operand_expected(
    const machine_operand_format& op_format, const std::string& instr_name, const range& stmt_range) const
{
    switch (op_format.identifier.type)
    {
        case machine_operand_type::REG: // R
            return diagnostic_op::error_M110(instr_name, operand_range);
        case machine_operand_type::MASK: // M
            return diagnostic_op::error_M111(instr_name, operand_range);
        case machine_operand_type::IMM: // I
            return diagnostic_op::error_M112(instr_name, operand_range);
        case machine_operand_type::VEC_REG: // V
            return diagnostic_op::error_M114(instr_name, operand_range);
        case machine_operand_type::RELOC_IMM: // RI
            return diagnostic_op::error_M113(instr_name, operand_range);
    }
    assert(false);
    return diagnostic_op::error_I999(instr_name, stmt_range);
}

one_operand::one_operand()
    : operand_identifier("")
    , value(0)
    , is_default(true)
{}

one_operand::one_operand(std::string operand_identifier, int value)
    : operand_identifier(operand_identifier)
    , value(value)
    , is_default(false)
{}

one_operand::one_operand(std::string operand_identifier)
    : operand_identifier(operand_identifier)
    , value(0)
    , is_default(true)
{}

one_operand::one_operand(int value)
    : operand_identifier(std::to_string(value))
    , value(value)
    , is_default(false)
{}

one_operand::one_operand(std::string operand_identifier, range range)
    : operand(range)
    , operand_identifier(operand_identifier)
    , value(0)
    , is_default(true)
{}

one_operand::one_operand(int value, range range)
    : operand(range)
    , operand_identifier(std::to_string(value))
    , value(value)
    , is_default(false)
{}

one_operand::one_operand(const one_operand& op)
{
    operand_identifier = op.operand_identifier;
    value = op.value;
    is_default = op.is_default;
};
bool one_operand::check(
    diagnostic_op& diag, const machine_operand_format to_check, const std::string& instr_name, const range&) const
{
    if (!is_simple_operand(to_check))
    {
        // therefore it is an address operand, but represented only by a single value

        // check only the displacement
        if (!is_operand_corresponding(value, to_check.identifier))
        {
            if (to_check.identifier.is_signed)
            {
                auto boundary = 1ll << (to_check.identifier.size - 1);
                diag = diagnostic_op::error_M130(instr_name, -boundary, boundary - 1, operand_range);
            }
            else
                diag = diagnostic_op::error_M130(instr_name, 0, (1ll << to_check.identifier.size) - 1, operand_range);
            return false;
        }
        return true;
    }
    // it is a simple operand
    if (to_check.identifier.is_signed && !is_size_corresponding_signed(value, to_check.identifier.size))
    {
        auto boundary = 1ll << (to_check.identifier.size - 1);
        switch (to_check.identifier.type)
        {
            case machine_operand_type::IMM:
                diag = diagnostic_op::error_M122(instr_name, -boundary, boundary - 1, operand_range);
                break;
            case machine_operand_type::RELOC_IMM:
                diag = diagnostic_op::error_M123(instr_name, -boundary, boundary - 1, operand_range);
                break;
            default:
                assert(false);
        }
        return false;
    }
    if (!to_check.identifier.is_signed && !is_size_corresponding_unsigned(value, to_check.identifier.size))
    {
        auto boundary = (1ll << to_check.identifier.size) - 1;
        switch (to_check.identifier.type)
        {
            case machine_operand_type::REG:
                diag = diagnostic_op::error_M120(instr_name, operand_range);
                break;
            case machine_operand_type::MASK:
                diag = diagnostic_op::error_M121(instr_name, operand_range);
                break;
            case machine_operand_type::IMM:
                diag = diagnostic_op::error_M122(instr_name, 0, boundary, operand_range);
                break;
            case machine_operand_type::VEC_REG:
                diag = diagnostic_op::error_M124(instr_name, operand_range);
                break;
            default:
                assert(false);
        }
        return false;
    }
    return true;
}

empty_operand::empty_operand() {}

bool empty_operand::check(
    diagnostic_op& diag, const machine_operand_format, const std::string& instr_name, const range&) const
{
    diag = diagnostic_op::error_M003(instr_name, operand_range);
    return false;
}

bool parameter::is_empty() const { return (!is_signed && type == machine_operand_type::NONE && size == 0); }

std::string parameter::to_string() const
{
    std::string ret_val = "";
    switch (type)
    {
        case machine_operand_type::MASK:
            return "M";
        case machine_operand_type::REG:
            return "R";
        case machine_operand_type::IMM: {
            ret_val = "I";
            break;
        }
        case machine_operand_type::NONE:
            return "";
        case machine_operand_type::DISPLC: {
            ret_val = "D";
            break;
        }
        case machine_operand_type::BASE:
            return "B";
        case machine_operand_type::LENGTH: {
            ret_val = "L";
            break;
        }
        case machine_operand_type::RELOC_IMM: {
            ret_val = "RI";
            break;
        }
        case machine_operand_type::VEC_REG:
            return "V";
        case machine_operand_type::DIS_REG:
            return "X";
    }
    ret_val += std::to_string(size);
    if (is_signed)
        ret_val += "S";
    else
        ret_val += "U";
    return ret_val;
}

std::string machine_operand_format::to_string() const
{
    std::string ret_val = identifier.to_string();
    if (first.is_empty() && second.is_empty())
        return ret_val;
    if (first.is_empty())
        return (ret_val + "(" + second.to_string() + ")");
    // only second cannot be empty
    return (ret_val + "(" + first.to_string() + "," + second.to_string() + ")");
}
