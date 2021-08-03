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
assembler_checker::assembler_checker() { initialize_assembler_map(); }

bool assembler_checker::check(const std::string& instruction_name,
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

std::map<std::string, std::unique_ptr<assembler_instruction>> assembler_checker::assembler_instruction_map = {};

void assembler_checker::initialize_assembler_map()
{
    assembler_instruction_map.insert(std::make_pair(
        "*PROCESS", std::make_unique<process>(std::vector<label_types> { label_types::NO_LABEL }, "*PROCESS")));
    assembler_instruction_map.insert(std::make_pair("ACONTROL",
        std::make_unique<acontrol>(
            std::vector<label_types> { label_types::SEQUENCE_SYMBOL, label_types::OPTIONAL }, "ACONTROL")));
    assembler_instruction_map.insert(std::make_pair("ADATA",
        std::make_unique<adata>(
            std::vector<label_types> { label_types::SEQUENCE_SYMBOL, label_types::OPTIONAL }, "ADATA")));
    assembler_instruction_map.insert(std::make_pair("AINSERT",
        std::make_unique<ainsert>(
            std::vector<label_types> { label_types::SEQUENCE_SYMBOL, label_types::OPTIONAL }, "AINSERT")));
    assembler_instruction_map.insert(std::make_pair("ALIAS",
        std::make_unique<alias>(
            std::vector<label_types> { label_types::ORD_SYMBOL, label_types::VAR_SYMBOL }, "ALIAS")));
    assembler_instruction_map.insert(std::make_pair("AMODE",
        std::make_unique<amode>(std::vector<label_types> { label_types::OPTIONAL, label_types::NAME }, "AMODE")));
    assembler_instruction_map.insert(std::make_pair(
        "CATTR", std::make_unique<cattr>(std::vector<label_types> { label_types::CLASS_NAME }, "CATTR")));
    assembler_instruction_map.insert(std::make_pair("CCW",
        std::make_unique<ccw<CCW_variant::CCW_CCW0>>(
            std::vector<label_types> {
                label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL },
            "CCW")));
    assembler_instruction_map.insert(std::make_pair("CCW0",
        std::make_unique<ccw<CCW_variant::CCW_CCW0>>(
            std::vector<label_types> {
                label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL },
            "CCW0")));
    assembler_instruction_map.insert(std::make_pair("CCW1",
        std::make_unique<ccw<CCW_variant::CCW1>>(
            std::vector<label_types> {
                label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL },
            "CCW1")));
    assembler_instruction_map.insert(std::make_pair("CEJECT",
        std::make_unique<expression_instruction>(
            std::vector<label_types> { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL }, "CEJECT")));
    assembler_instruction_map.insert(std::make_pair("CNOP",
        std::make_unique<cnop>(
            std::vector<label_types> {
                label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL },
            "CNOP")));
    assembler_instruction_map.insert(std::make_pair("COM",
        std::make_unique<no_operands>(
            std::vector<label_types> {
                label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL },
            "COM")));
    assembler_instruction_map.insert(std::make_pair("COPY",
        std::make_unique<copy>(
            std::vector<label_types> { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL }, "COPY")));
    assembler_instruction_map.insert(std::make_pair("COPY",
        std::make_unique<copy>(
            std::vector<label_types> { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL }, "COPY")));
    assembler_instruction_map.insert(std::make_pair("CSECT",
        std::make_unique<no_operands>(
            std::vector<label_types> {
                label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL },
            "CSECT")));
    assembler_instruction_map.insert(std::make_pair("CXD",
        std::make_unique<no_operands>(
            std::vector<label_types> {
                label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL },
            "CXD")));
    assembler_instruction_map.insert(std::make_pair("DC",
        std::make_unique<dc>(
            std::vector<label_types> {
                label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL },
            "DC")));
    assembler_instruction_map.insert(std::make_pair("DROP",
        std::make_unique<drop>(
            std::vector<label_types> { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL }, "DROP")));
    assembler_instruction_map.insert(std::make_pair("DS",
        std::make_unique<ds_dxd>(
            std::vector<label_types> {
                label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL },
            "DS")));
    assembler_instruction_map.insert(std::make_pair("DSECT",
        std::make_unique<no_operands>(
            std::vector<label_types> {
                label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL },
            "DSECT")));
    assembler_instruction_map.insert(std::make_pair("DXD",
        std::make_unique<ds_dxd>(
            std::vector<label_types> {
                label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL },
            "DXD")));
    assembler_instruction_map.insert(std::make_pair("EJECT",
        std::make_unique<no_operands>(
            std::vector<label_types> { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL }, "EJECT")));
    assembler_instruction_map.insert(std::make_pair("END",
        std::make_unique<end>(
            std::vector<label_types> { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL }, "END")));
    assembler_instruction_map.insert(std::make_pair("ENTRY",
        std::make_unique<entry>(
            std::vector<label_types> { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL }, "ENTRY")));
    assembler_instruction_map.insert(std::make_pair("EQU",
        std::make_unique<equ>(std::vector<label_types> { label_types::ORD_SYMBOL, label_types::VAR_SYMBOL }, "EQU")));
    assembler_instruction_map.insert(std::make_pair("EXITCTL",
        std::make_unique<exitctl>(
            std::vector<label_types> { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL }, "EXITCTL")));
    assembler_instruction_map.insert(std::make_pair("EXTRN",
        std::make_unique<external>(
            std::vector<label_types> { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL }, "EXTRN")));
    assembler_instruction_map.insert(
        std::make_pair("ICTL", std::make_unique<ictl>(std::vector<label_types> { label_types::NO_LABEL }, "ICTL")));
    assembler_instruction_map.insert(std::make_pair("ISEQ",
        std::make_unique<iseq>(
            std::vector<label_types> { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL }, "ISEQ")));
    assembler_instruction_map.insert(std::make_pair("LOCTR",
        std::make_unique<no_operands>(
            std::vector<label_types> { label_types::ORD_SYMBOL, label_types::VAR_SYMBOL }, "LOCTR")));
    assembler_instruction_map.insert(std::make_pair("LTORG",
        std::make_unique<no_operands>(
            std::vector<label_types> {
                label_types::ORD_SYMBOL, label_types::VAR_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::OPTIONAL },
            "LTORG")));
    assembler_instruction_map.insert(std::make_pair("MNOTE",
        std::make_unique<mnote>(
            std::vector<label_types> { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL }, "MNOTE")));
    assembler_instruction_map.insert(std::make_pair("OPSYN",
        std::make_unique<opsyn>(
            std::vector<label_types> { label_types::ORD_SYMBOL, label_types::VAR_SYMBOL, label_types::OPERATION_CODE },
            "OPSYN")));
    assembler_instruction_map.insert(std::make_pair("ORG",
        std::make_unique<org>(
            std::vector<label_types> {
                label_types::ORD_SYMBOL, label_types::VAR_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::OPTIONAL },
            "ORG")));
    assembler_instruction_map.insert(std::make_pair("POP",
        std::make_unique<stack_instr>(
            std::vector<label_types> { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL }, "POP")));
    assembler_instruction_map.insert(std::make_pair("PRINT",
        std::make_unique<print>(
            std::vector<label_types> { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL }, "PRINT")));
    assembler_instruction_map.insert(std::make_pair("PUNCH",
        std::make_unique<punch>(
            std::vector<label_types> { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL }, "PUNCH")));
    assembler_instruction_map.insert(std::make_pair("PUSH",
        std::make_unique<stack_instr>(
            std::vector<label_types> { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL }, "PUSH")));
    assembler_instruction_map.insert(std::make_pair("REPRO",
        std::make_unique<no_operands>(
            std::vector<label_types> { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL }, "REPRO")));
    assembler_instruction_map.insert(std::make_pair("RMODE",
        std::make_unique<rmode>(std::vector<label_types> { label_types::OPTIONAL, label_types::NAME }, "RMODE")));
    assembler_instruction_map.insert(std::make_pair("RSECT",
        std::make_unique<no_operands>(
            std::vector<label_types> {
                label_types::OPTIONAL, label_types::ORD_SYMBOL, label_types::VAR_SYMBOL, label_types::SEQUENCE_SYMBOL },
            "RSECT")));
    assembler_instruction_map.insert(std::make_pair("SPACE",
        std::make_unique<expression_instruction>(
            std::vector<label_types> { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL }, "SPACE")));
    assembler_instruction_map.insert(std::make_pair("START",
        std::make_unique<expression_instruction>(
            std::vector<label_types> {
                label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL, label_types::ORD_SYMBOL, label_types::VAR_SYMBOL },
            "START")));
    assembler_instruction_map.insert(std::make_pair("TITLE",
        std::make_unique<title>(
            std::vector<label_types> {
                label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL, label_types::STRING },
            "TITLE")));
    assembler_instruction_map.insert(std::make_pair("USING",
        std::make_unique<using_instr>(
            std::vector<label_types> {
                label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL, label_types::ORD_SYMBOL },
            "USING")));
    assembler_instruction_map.insert(std::make_pair("WXTRN",
        std::make_unique<external>(
            std::vector<label_types> { label_types::OPTIONAL, label_types::SEQUENCE_SYMBOL }, "WXTRN")));
    assembler_instruction_map.insert(std::make_pair("XATTR",
        std::make_unique<xattr>(
            std::vector<label_types> { label_types::ORD_SYMBOL, label_types::SEQUENCE_SYMBOL, label_types::VAR_SYMBOL },
            "XATTR")));
}

bool machine_checker::check(const std::string& instruction_name,
    const std::vector<const operand*>& operand_vector,
    const range& stmt_range,
    const diagnostic_collector& add_diagnostic) const
{
    // get operands
    std::vector<const machine_operand*> ops;
    for (auto& op : operand_vector)
        ops.push_back(dynamic_cast<const machine_operand*>(op));

    // instruction name is the mnemonic name in case of a mnemonic instruction

    std::string mach_name = instruction_name;

    // instruction is a mnemonic instruction
    if (context::instruction::mnemonic_codes.find(instruction_name) != context::instruction::mnemonic_codes.end())
        mach_name = context::instruction::mnemonic_codes.at(instruction_name).instruction->instr_name;

    return context::instruction::machine_instructions.at(mach_name).check(
        instruction_name, ops, stmt_range, add_diagnostic);
}

} // namespace hlasm_plugin::parser_library::checking