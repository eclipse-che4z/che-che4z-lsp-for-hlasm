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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CHECKING_DATA_DEFINITION_OPERAND_H
#define HLASMPLUGIN_PARSERLIBRARY_CHECKING_DATA_DEFINITION_OPERAND_H

#include "checking/diagnostic_collector.h"
#include "checking/operand.h"
#include "data_def_fields.h"
#include "data_def_type_base.h"

namespace hlasm_plugin::parser_library::checking {
// Represents evaluated (resolved machine expressions) data definition operand suitable for checking.
class data_definition_operand : public asm_operand
{
public:
    using num_t = int32_t;

    data_def_field<num_t> dupl_factor;
    data_def_field<char> type;
    data_def_field<char> extension;
    data_def_length_t length;

    exponent_modifier_t exponent;
    scale_modifier_t scale;

    nominal_value_t nominal_value;

    const data_def_type* access_data_def_type() const;

    template<data_instr_type instr_type>
    bool check(const diagnostic_collector& add_diagnostic) const;

    uint64_t get_length() const;
    context::alignment get_alignment() const;
    int16_t get_scale_attribute() const;
    uint32_t get_length_attribute() const;
    uint32_t get_integer_attribute() const;

    template<data_instr_type instr_type>
    static uint64_t get_operands_length(const std::vector<const data_definition_operand*>& operands);


private:
    bool check_type_and_extension(const diagnostic_collector& add_diagnostic) const;
};


//************************* template functions implementation ************************
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

template<data_instr_type instr_type>
bool data_definition_operand::check(const diagnostic_collector& add_diagnostic) const
{
    if (!check_type_and_extension(add_diagnostic))
        return false;

    return access_data_def_type()->check<instr_type>(*this, add_diagnostic);
}


} // namespace hlasm_plugin::parser_library::checking


#endif
