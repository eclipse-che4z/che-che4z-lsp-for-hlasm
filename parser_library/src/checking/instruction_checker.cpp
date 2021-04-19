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

std::map<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
    assembler_checker::assembler_instruction_map = {};

void assembler_checker::initialize_assembler_map()
{
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>(
            "*PROCESS",
            std::make_unique<hlasm_plugin::parser_library::checking::process>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::NO_LABEL },
                "*PROCESS")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>(
            "ACONTROL",
            std::make_unique<hlasm_plugin::parser_library::checking::acontrol>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL },
                "ACONTROL")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("ADATA",
            std::make_unique<hlasm_plugin::parser_library::checking::adata>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL },
                "ADATA")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>(
            "AINSERT",
            std::make_unique<hlasm_plugin::parser_library::checking::ainsert>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL },
                "AINSERT")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("ALIAS",
            std::make_unique<hlasm_plugin::parser_library::checking::alias>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL },
                "ALIAS")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("AMODE",
            std::make_unique<hlasm_plugin::parser_library::checking::amode>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::NAME },
                "AMODE")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("CATTR",
            std::make_unique<hlasm_plugin::parser_library::checking::cattr>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::CLASS_NAME },
                "CATTR")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("CCW",
            std::make_unique<hlasm_plugin::parser_library::checking::ccw>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL },
                "CCW")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("CCW0",
            std::make_unique<hlasm_plugin::parser_library::checking::ccw>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL },
                "CCW0")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("CCW1",
            std::make_unique<hlasm_plugin::parser_library::checking::ccw>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL },
                "CCW1")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("CEJECT",
            std::make_unique<hlasm_plugin::parser_library::checking::expression_instruction>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL },
                "CEJECT")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("CNOP",
            std::make_unique<hlasm_plugin::parser_library::checking::cnop>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL },
                "CNOP")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("COM",
            std::make_unique<hlasm_plugin::parser_library::checking::no_operands>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL },
                "COM")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("COPY",
            std::make_unique<hlasm_plugin::parser_library::checking::copy>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL },
                "COPY")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("COPY",
            std::make_unique<hlasm_plugin::parser_library::checking::copy>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL },
                "COPY")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("CSECT",
            std::make_unique<hlasm_plugin::parser_library::checking::no_operands>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL },
                "CSECT")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("CXD",
            std::make_unique<hlasm_plugin::parser_library::checking::no_operands>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL },
                "CXD")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("DC",
            std::make_unique<hlasm_plugin::parser_library::checking::dc>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL },
                "DC")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("DROP",
            std::make_unique<hlasm_plugin::parser_library::checking::drop>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL },
                "DROP")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("DS",
            std::make_unique<hlasm_plugin::parser_library::checking::ds_dxd>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL },
                "DS")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("DSECT",
            std::make_unique<hlasm_plugin::parser_library::checking::no_operands>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL },
                "DSECT")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("DXD",
            std::make_unique<hlasm_plugin::parser_library::checking::ds_dxd>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL },
                "DXD")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("EJECT",
            std::make_unique<hlasm_plugin::parser_library::checking::no_operands>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL },
                "EJECT")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("END",
            std::make_unique<hlasm_plugin::parser_library::checking::end>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL },
                "END")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("ENTRY",
            std::make_unique<hlasm_plugin::parser_library::checking::entry>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL },
                "ENTRY")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("EQU",
            std::make_unique<hlasm_plugin::parser_library::checking::equ>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL },
                "EQU")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>(
            "EXITCTL",
            std::make_unique<hlasm_plugin::parser_library::checking::exitctl>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL },
                "EXITCTL")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("EXTRN",
            std::make_unique<hlasm_plugin::parser_library::checking::external>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL },
                "EXTRN")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("ICTL",
            std::make_unique<hlasm_plugin::parser_library::checking::ictl>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::NO_LABEL },
                "ICTL")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("ISEQ",
            std::make_unique<hlasm_plugin::parser_library::checking::iseq>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL },
                "ISEQ")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("LOCTR",
            std::make_unique<hlasm_plugin::parser_library::checking::no_operands>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL },
                "LOCTR")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("LTORG",
            std::make_unique<hlasm_plugin::parser_library::checking::no_operands>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL },
                "LTORG")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("MNOTE",
            std::make_unique<hlasm_plugin::parser_library::checking::mnote>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL },
                "MNOTE")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("OPSYN",
            std::make_unique<hlasm_plugin::parser_library::checking::opsyn>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::OPERATION_CODE },
                "OPSYN")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("ORG",
            std::make_unique<hlasm_plugin::parser_library::checking::org>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL },
                "ORG")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("POP",
            std::make_unique<hlasm_plugin::parser_library::checking::stack_instr>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL },
                "POP")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("PRINT",
            std::make_unique<hlasm_plugin::parser_library::checking::print>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL },
                "PRINT")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("PUNCH",
            std::make_unique<hlasm_plugin::parser_library::checking::punch>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL },
                "PUNCH")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("PUSH",
            std::make_unique<hlasm_plugin::parser_library::checking::stack_instr>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL },
                "PUSH")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("REPRO",
            std::make_unique<hlasm_plugin::parser_library::checking::no_operands>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL },
                "REPRO")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("RMODE",
            std::make_unique<hlasm_plugin::parser_library::checking::rmode>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::NAME },
                "RMODE")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("RSECT",
            std::make_unique<hlasm_plugin::parser_library::checking::no_operands>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL },
                "RSECT")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("SPACE",
            std::make_unique<hlasm_plugin::parser_library::checking::expression_instruction>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL },
                "SPACE")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("START",
            std::make_unique<hlasm_plugin::parser_library::checking::expression_instruction>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL },
                "START")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("TITLE",
            std::make_unique<hlasm_plugin::parser_library::checking::title>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::STRING },
                "TITLE")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("USING",
            std::make_unique<hlasm_plugin::parser_library::checking::using_instr>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL },
                "USING")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("WXTRN",
            std::make_unique<hlasm_plugin::parser_library::checking::external>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL },
                "WXTRN")));
    assembler_instruction_map.insert(
        std::pair<std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>("XATTR",
            std::make_unique<hlasm_plugin::parser_library::checking::xattr>(
                std::vector<hlasm_plugin::parser_library::checking::label_types> {
                    hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
                    hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL },
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
    if (hlasm_plugin::parser_library::context::instruction::mnemonic_codes.find(instruction_name)
        != hlasm_plugin::parser_library::context::instruction::mnemonic_codes.end())
        mach_name = hlasm_plugin::parser_library::context::instruction::mnemonic_codes.at(instruction_name).instruction;

    return hlasm_plugin::parser_library::context::instruction::machine_instructions.at(mach_name)->check(
        instruction_name, ops, stmt_range, add_diagnostic);
}

} // namespace hlasm_plugin::parser_library::checking