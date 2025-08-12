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

#include <unordered_map>

#include "checking/asm_instr_check.h"
#include "checking/asm_instr_class.h"
#include "instructions/instruction.h"

namespace hlasm_plugin::parser_library::checking {

namespace {
struct instruction_adder
{
    std::unordered_map<std::string_view, std::unique_ptr<assembler_instruction>>& target;

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

const std::unordered_map<std::string_view, std::unique_ptr<assembler_instruction>> assembler_instruction_map = [] {
    std::unordered_map<std::string_view, std::unique_ptr<assembler_instruction>> result;
    instruction_adder a { result };

    using enum label_types;

    a.add<process>("*PROCESS", { NO_LABEL });
    a.add<acontrol>("ACONTROL", { SEQUENCE_SYMBOL, OPTIONAL });
    a.add<adata>("ADATA", { SEQUENCE_SYMBOL, OPTIONAL });
    a.add<ainsert>("AINSERT", { SEQUENCE_SYMBOL, OPTIONAL });
    a.add<alias>("ALIAS", { ORD_SYMBOL, VAR_SYMBOL });
    a.add<amode>("AMODE", { OPTIONAL, NAME });
    a.add<cattr>("CATTR", { CLASS_NAME });
    a.add<ccw>("CCW", { OPTIONAL, ORD_SYMBOL, SEQUENCE_SYMBOL, VAR_SYMBOL }, CCW_variant::CCW_CCW0);
    a.add<ccw>("CCW0", { OPTIONAL, ORD_SYMBOL, SEQUENCE_SYMBOL, VAR_SYMBOL }, CCW_variant::CCW_CCW0);
    a.add<ccw>("CCW1", { OPTIONAL, ORD_SYMBOL, SEQUENCE_SYMBOL, VAR_SYMBOL }, CCW_variant::CCW1);
    a.add<expression_instruction>("CEJECT", { OPTIONAL, SEQUENCE_SYMBOL });
    a.add<cnop>("CNOP", { OPTIONAL, ORD_SYMBOL, SEQUENCE_SYMBOL, VAR_SYMBOL });
    a.add<no_operands>("COM", { OPTIONAL, ORD_SYMBOL, SEQUENCE_SYMBOL, VAR_SYMBOL });
    a.add<copy>("COPY", { OPTIONAL, SEQUENCE_SYMBOL });
    a.add<no_operands>("CSECT", { OPTIONAL, ORD_SYMBOL, SEQUENCE_SYMBOL, VAR_SYMBOL });
    a.add<no_operands>("CXD", { OPTIONAL, ORD_SYMBOL, SEQUENCE_SYMBOL, VAR_SYMBOL });
    a.add<drop>("DROP", { OPTIONAL, SEQUENCE_SYMBOL });
    a.add<no_operands>("DSECT", { OPTIONAL, ORD_SYMBOL, SEQUENCE_SYMBOL, VAR_SYMBOL });
    a.add<no_operands>("EJECT", { OPTIONAL, SEQUENCE_SYMBOL });
    a.add<end>("END", { OPTIONAL, SEQUENCE_SYMBOL });
    a.add<entry>("ENTRY", { OPTIONAL, SEQUENCE_SYMBOL });
    a.add<equ>("EQU", { ORD_SYMBOL, VAR_SYMBOL });
    a.add<exitctl>("EXITCTL", { OPTIONAL, SEQUENCE_SYMBOL });
    a.add<external>("EXTRN", { OPTIONAL, SEQUENCE_SYMBOL });
    a.add<ictl>("ICTL", { NO_LABEL });
    a.add<iseq>("ISEQ", { OPTIONAL, SEQUENCE_SYMBOL });
    a.add<no_operands>("LOCTR", { ORD_SYMBOL, VAR_SYMBOL });
    a.add<no_operands>("LTORG", { ORD_SYMBOL, VAR_SYMBOL, SEQUENCE_SYMBOL, OPTIONAL });
    a.add<mnote>("MNOTE", { OPTIONAL, SEQUENCE_SYMBOL });
    a.add<opsyn>("OPSYN", { ORD_SYMBOL, VAR_SYMBOL, OPERATION_CODE });
    a.add<org>("ORG", { ORD_SYMBOL, VAR_SYMBOL, SEQUENCE_SYMBOL, OPTIONAL });
    a.add<stack_instr>("POP", { OPTIONAL, SEQUENCE_SYMBOL });
    a.add<print>("PRINT", { OPTIONAL, SEQUENCE_SYMBOL });
    a.add<punch>("PUNCH", { OPTIONAL, SEQUENCE_SYMBOL });
    a.add<stack_instr>("PUSH", { OPTIONAL, SEQUENCE_SYMBOL });
    a.add<no_operands>("REPRO", { OPTIONAL, SEQUENCE_SYMBOL });
    a.add<rmode>("RMODE", { OPTIONAL, NAME });
    a.add<no_operands>("RSECT", { OPTIONAL, ORD_SYMBOL, VAR_SYMBOL, SEQUENCE_SYMBOL });
    a.add<expression_instruction>("SPACE", { OPTIONAL, SEQUENCE_SYMBOL });
    a.add<expression_instruction>("START", { OPTIONAL, SEQUENCE_SYMBOL, ORD_SYMBOL, VAR_SYMBOL });
    a.add<title>("TITLE", { OPTIONAL, SEQUENCE_SYMBOL, VAR_SYMBOL, STRING });
    a.add<using_instr>("USING", { OPTIONAL, SEQUENCE_SYMBOL, VAR_SYMBOL, ORD_SYMBOL });
    a.add<external>("WXTRN", { OPTIONAL, SEQUENCE_SYMBOL });
    a.add<xattr>("XATTR", { ORD_SYMBOL, SEQUENCE_SYMBOL, VAR_SYMBOL });

    return result;
}();

} // namespace

bool check_asm_ops(std::string_view instruction_name,
    std::span<const asm_operand* const> ops,
    const range& stmt_range,
    const diagnostic_collector& add_diagnostic)
{
    const auto it = assembler_instruction_map.find(instruction_name);
    if (it == assembler_instruction_map.end())
        return false;

    return it->second->check(ops, stmt_range, add_diagnostic);
}

} // namespace hlasm_plugin::parser_library::checking
