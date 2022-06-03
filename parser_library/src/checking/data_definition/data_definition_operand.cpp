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

#include "data_definition_operand.h"

#include "checking/diagnostic_collector.h"

using namespace hlasm_plugin::parser_library::checking;
using namespace hlasm_plugin::parser_library;

const data_def_type* data_definition_operand::access_data_def_type() const
{
    return data_def_type::access_data_def_type(type.value, extension.value);
}

std::pair<const data_def_type*, bool> data_definition_operand::check_type_and_extension(
    const diagnostic_collector& add_diagnostic) const
{
    auto found = data_def_type::types_and_extensions.find({ type.value, extension.present ? extension.value : '\0' });

    if (found != data_def_type::types_and_extensions.end())
        return { found->second.get(), true };

    if (extension.present)
    {
        found = data_def_type::types_and_extensions.find({ type.value, '\0' });
        if (found != data_def_type::types_and_extensions.end())
        {
            add_diagnostic(diagnostic_op::error_D013(extension.rng, std::string(1, type.value)));
            return { found->second.get(), false };
        }
    }

    add_diagnostic(diagnostic_op::error_D012(type.rng));
    return { nullptr, false };
}

uint64_t data_definition_operand::get_length() const
{
    auto def_type = access_data_def_type();
    if (def_type)
        return def_type->get_length(*this);
    else
        return 0;
}

context::alignment data_definition_operand::get_alignment() const
{
    auto def_type = access_data_def_type();
    if (def_type)
        return def_type->get_alignment(length.present);
    else
        return context::no_align;
}

int16_t data_definition_operand::get_scale_attribute() const
{
    auto def_type = access_data_def_type();
    if (def_type)
        return def_type->get_scale_attribute(scale, reduce_nominal_value(nominal_value));
    else
        return 0;
}

uint32_t data_definition_operand::get_length_attribute() const
{
    auto def_type = access_data_def_type();
    if (def_type)
        return def_type->get_length_attribute(length, reduce_nominal_value(nominal_value));
    else
        return 0;
}

uint32_t data_definition_operand::get_integer_attribute() const
{
    auto def_type = access_data_def_type();
    if (def_type)
        return def_type->get_integer_attribute(length, scale, reduce_nominal_value(nominal_value));
    else
        return 0;
}


template<data_instr_type instr_type>
uint64_t data_definition_operand::get_operands_length(const std::vector<const data_definition_operand*>& operands)
{
    uint64_t operands_bit_length = 0;

    for (auto op : operands)
    {
        if (!op->check<instr_type>(diagnostic_collector()))
            return 0;

        if (op->length.len_type != checking::data_def_length_t::BIT)
        {
            // align to whole byte
            operands_bit_length = round_up(operands_bit_length, (uint64_t)8);

            // enforce data def alignment
            context::alignment al = op->get_alignment();

            operands_bit_length = round_up(operands_bit_length, (uint64_t)al.boundary * 8);
        }

        operands_bit_length += op->get_length();
    }
    // align to whole byte
    operands_bit_length = round_up(operands_bit_length, (uint64_t)8);

    // returns the length in bytes
    return operands_bit_length / 8;
}

template uint64_t data_definition_operand::get_operands_length<data_instr_type::DC>(
    const std::vector<const data_definition_operand*>& operands);
template uint64_t data_definition_operand::get_operands_length<data_instr_type::DS>(
    const std::vector<const data_definition_operand*>& operands);
