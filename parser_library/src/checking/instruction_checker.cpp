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

#include "instruction_checker.h"

#include <map>

namespace hlasm_plugin::parser_library::checking {

bool assembler_checker::check(std::string_view instruction_name,
    const std::vector<const operand*>& operand_vector,
    const range& stmt_range,
    const diagnostic_collector& add_diagnostic) const
{
    try
    {
        std::vector<const asm_operand*> ops;
        for (auto& op : operand_vector)
            ops.push_back(dynamic_cast<const asm_operand*>(op));
        return assembler_instruction_map.at(instruction_name)->check(ops, stmt_range, add_diagnostic);
    }
    catch (...)
    {
        return false;
    }
}

namespace {
struct instruction_adder
{
    std::map<std::string_view, std::unique_ptr<assembler_instruction>>& target;

    void add(std::string_view name, std::unique_ptr<assembler_instruction> value) const
    {
        target.try_emplace(name, std::move(value));
    }

    template<typename T, typename... Rest>
    void add(std::string_view name, std::initializer_list<label_types> labels, Rest&&... rest) const
    {
        add(name, std::make_unique<T>(std::vector<label_types>(labels), name, std::forward<Rest>(rest)...));
    };
};
} // namespace

const std::map<std::string_view, std::unique_ptr<assembler_instruction>> assembler_checker::assembler_instruction_map =
    [] {
        std::map<std::string_view, std::unique_ptr<assembler_instruction>> result;
        instruction_adder a { result };

        a.add<process>("*PROCESS", { label_types::NO_LABEL });
        a.add<acontrol>("ACONTROL", { label_types::SEQUENCE_SYMBOL, label_types::OPTIONAL });
        a.add<adata>("ADATA", { label_types::SEQUENCE_SYMBOL, label_types::OPTIONAL });
        a.add<ainsert>("AINSERT", { label_types::SEQUENCE_SYMBOL, label_types::OPTIONAL });
        a.add<alias>("ALIAS", { label_types::ORD_SYMBOL, label_types::VAR_SYMBOL });
        a.add<amode>("AMODE", { label_types::OPTIONAL, label_types::NAME });
        a.add<cattr>("CATTR", { label_types::CLASS_NAME });
        a.add<ccw>("CCW",
            { label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL },
            CCW_variant::CCW_CCW0);
        a.add<ccw>("CCW0",
            { label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL },
            CCW_variant::CCW_CCW0);
        a.add<ccw>("CCW1",
            { label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL },
            CCW_variant::CCW1);
        a.add<expression_instruction>("CEJECT", { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL });
        a.add<cnop>("CNOP",
            { label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL });
        a.add<no_operands>("COM",
            { label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL });
        a.add<copy>("COPY", { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL });
        a.add<no_operands>("CSECT",
            { label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL });
        a.add<no_operands>("CXD",
            { label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL });
        a.add<dc>("DC",
            { label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL });
        a.add<drop>("DROP", { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL });
        a.add<ds_dxd>("DS",
            { label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL });
        a.add<no_operands>("DSECT",
            { label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL });
        a.add<ds_dxd>("DXD",
            { label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL });
        a.add<no_operands>("EJECT", { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL });
        a.add<end>("END", { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL });
        a.add<entry>("ENTRY", { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL });
        a.add<equ>("EQU", { label_types::ORD_SYMBOL, label_types::VAR_SYMBOL });
        a.add<exitctl>("EXITCTL", { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL });
        a.add<external>("EXTRN", { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL });
        a.add<ictl>("ICTL", { label_types::NO_LABEL });
        a.add<iseq>("ISEQ", { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL });
        a.add<no_operands>("LOCTR", { label_types::ORD_SYMBOL, label_types::VAR_SYMBOL });
        a.add<no_operands>("LTORG",
            { label_types::ORD_SYMBOL, label_types::VAR_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::OPTIONAL });
        a.add<mnote>("MNOTE", { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL });
        a.add<opsyn>("OPSYN", { label_types::ORD_SYMBOL, label_types::VAR_SYMBOL, label_types::OPERATION_CODE });
        a.add<org>("ORG",
            { label_types::ORD_SYMBOL, label_types::VAR_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::OPTIONAL });
        a.add<stack_instr>("POP", { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL });
        a.add<print>("PRINT", { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL });
        a.add<punch>("PUNCH", { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL });
        a.add<stack_instr>("PUSH", { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL });
        a.add<no_operands>("REPRO", { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL });
        a.add<rmode>("RMODE", { label_types::OPTIONAL, label_types::NAME });
        a.add<no_operands>("RSECT",
            { label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::VAR_SYMBOL, label_types::SEQUENCE_SYMBOL });
        a.add<expression_instruction>("SPACE", { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL });
        a.add<expression_instruction>("START",
            { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL, label_types::ORD_SYMBOL, label_types::VAR_SYMBOL });
        a.add<title>("TITLE",
            { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL, label_types::STRING });
        a.add<using_instr>("USING",
            { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL, label_types::ORD_SYMBOL });
        a.add<external>("WXTRN", { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL });
        a.add<xattr>("XATTR", { label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL });

        return result;
    }();

bool machine_checker::check(std::string_view instruction_name,
    const std::vector<const operand*>& operand_vector,
    const range& stmt_range,
    const diagnostic_collector& add_diagnostic) const
{
    // get operands
    std::vector<const machine_operand*> ops;
    for (auto& op : operand_vector)
        ops.push_back(dynamic_cast<const machine_operand*>(op));

    // instruction name is the mnemonic name in case of a mnemonic instruction

    auto [mi, _] = context::instruction::find_machine_instruction_or_mnemonic(instruction_name);

    assert(mi);

    return mi->check(instruction_name, ops, stmt_range, add_diagnostic);
}

} // namespace hlasm_plugin::parser_library::checking
