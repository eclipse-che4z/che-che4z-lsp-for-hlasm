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

// This file implements checking of data definition
// asm instructions from the assembler checker:
// DC, DS, DXD

#include "checking/asm_instr_check.h"
#include "checking/diagnostic_collector.h"
#include "data_definition_operand.h"

namespace hlasm_plugin::parser_library::checking {

data::data(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction)
    : assembler_instruction(allowed_types, name_of_instruction, 1, -1)
{}

bool data::check_data(std::span<const asm_operand* const> to_check,
    data_instr_type instr_type,
    const range& stmt_range,
    const diagnostic_collector& add_diagnostic) const
{
    if (!operands_size_corresponding(to_check, stmt_range, add_diagnostic))
        return false;

    auto operands_bit_length = 0ULL;
    bool ret = true;
    for (const auto& operand : to_check)
    {
        const auto op = dynamic_cast<const data_definition_operand*>(operand);
        if (!op)
        {
            add_diagnostic(diagnostic_op::error_A004_data_def_expected());
            ret = false;
            continue;
        }

        const auto [def_type, exact_match] = op->check_type_and_extension(add_diagnostic);
        ret &= exact_match && def_type->check(*op, instr_type, add_diagnostic);

        if (!ret)
            continue;

        if (op->length.len_type != checking::data_def_length_t::BIT)
        {
            // align to whole byte
            operands_bit_length = round_up(operands_bit_length, 8ULL);

            // enforce data def alignment
            const context::alignment al = def_type->get_alignment(op->length.present);

            operands_bit_length = round_up(operands_bit_length, al.boundary * 8ULL);
        }

        operands_bit_length += def_type->get_length(*op);
    }

    if (!ret)
        return false;

    // align to whole byte
    operands_bit_length = round_up(operands_bit_length, 8ULL);

    if (operands_bit_length / 8 > INT32_MAX)
    {
        add_diagnostic(diagnostic_op::error_D029(stmt_range));
        ret = false;
    }

    return ret;
}

dc::dc(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction)
    : data(allowed_types, name_of_instruction)
{}

bool dc::check(std::span<const asm_operand* const> to_check,
    const range& stmt_range,
    const diagnostic_collector& add_diagnostic) const
{
    return check_data(to_check, data_instr_type::DC, stmt_range, add_diagnostic);
}

ds_dxd::ds_dxd(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction)
    : data(allowed_types, name_of_instruction) {};

bool ds_dxd::check(std::span<const asm_operand* const> to_check,
    const range& stmt_range,
    const diagnostic_collector& add_diagnostic) const
{
    return check_data(to_check, data_instr_type::DS, stmt_range, add_diagnostic);
}

} // namespace hlasm_plugin::parser_library::checking
