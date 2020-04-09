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
#include "data_def_types.h"

using namespace hlasm_plugin::parser_library::checking;
using namespace hlasm_plugin::parser_library;



data::data(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction)
    : assembler_instruction(allowed_types, name_of_instruction, 1, -1)
{}

template<data_instr_type instr_type>
bool data::check_data(const std::vector<const asm_operand*>& to_check,
    const range& stmt_range,
    const diagnostic_collector& add_diagnostic) const
{
    if (!operands_size_corresponding(to_check, stmt_range, add_diagnostic))
        return false;

    std::vector<const data_definition_operand*> get_len_ops;
    get_len_ops.reserve(to_check.size());
    bool ret = true;
    for (const auto& operand : to_check)
    {
        auto ddef = dynamic_cast<const data_definition_operand*>(operand);
        if (ddef)
        {
            get_len_ops.push_back(ddef);
            ret &= ddef->check<instr_type>(add_diagnostic);
        }
        else
        {
            add_diagnostic(diagnostic_op::error_A004_data_def_expected());
            ret = false;
        }
    }

    if (!ret)
        return false;

    uint64_t len = data_definition_operand::get_operands_length<instr_type>(get_len_ops);
    if (len > INT32_MAX)
    {
        add_diagnostic(diagnostic_op::error_D029(stmt_range));
        ret = false;
    }

    return ret;
}

dc::dc(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction)
    : data(allowed_types, name_of_instruction)
{}

bool dc::check(const std::vector<const asm_operand*>& to_check,
    const range& stmt_range,
    const diagnostic_collector& add_diagnostic) const
{
    return check_data<data_instr_type::DC>(to_check, stmt_range, add_diagnostic);
}

ds_dxd::ds_dxd(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction)
    : data(allowed_types, name_of_instruction) {};

bool ds_dxd::check(const std::vector<const asm_operand*>& to_check,
    const range& stmt_range,
    const diagnostic_collector& add_diagnostic) const
{
    return check_data<data_instr_type::DS>(to_check, stmt_range, add_diagnostic);
}
