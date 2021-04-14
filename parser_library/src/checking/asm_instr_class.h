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

#ifndef HLASMPLUGIN_PARSERLIBRARY_INSTR_CLASS_H
#define HLASMPLUGIN_PARSERLIBRARY_INSTR_CLASS_H

#include <algorithm>
#include <functional>
#include <iomanip>
#include <string>
#include <vector>

#include "checker_helper.h"
#include "diagnosable.h"
#include "instr_operand.h"

namespace hlasm_plugin::parser_library::checking {

// defining label types before instruction, used as parameter in assembler_instruction class
enum label_types
{
    SEQUENCE_SYMBOL,
    VAR_SYMBOL,
    ORD_SYMBOL,
    NAME,
    CLASS_NAME,
    NO_LABEL,
    OPTIONAL,
    OPERATION_CODE,
    STRING
};

// representation of assembler instruction for checking purposes
// serves as a ancestor class for specific assembler instructions
class assembler_instruction
{
public:
    const std::vector<label_types> allowed_types;
    const std::string name_of_instruction;
    int min_operands;
    int max_operands; // maximum number of operands, if not specified, value is -1
    virtual bool check(const std::vector<const asm_operand*>& to_check, const range&, const diagnostic_collector&) const
    {
        (void)to_check;
        return true;
    };
    assembler_instruction(
        std::vector<label_types> allowed_types, std::string name_of_instruction, int min_operands, int max_operands)
        : allowed_types(allowed_types)
        , name_of_instruction(name_of_instruction)
        , min_operands(min_operands)
        , max_operands(max_operands) {};
    assembler_instruction()
        : min_operands(0)
        , max_operands(0) {};
    virtual ~assembler_instruction() {};

protected:
    bool is_param_in_vector(const std::string& parameter, const std::vector<std::string>& options) const;

    bool operands_size_corresponding(const std::vector<const asm_operand*>& to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const;

    // functions for checking complex operands
    bool check_compat_operands(const std::vector<std::unique_ptr<asm_operand>>& input,
        const std::string& instr_name,
        const diagnostic_collector& add_diagnostic) const;

    bool check_flag_operand(
        const one_operand* input, const std::string& instr_name, const diagnostic_collector& add_diagnostic) const;

    bool check_process_flag_parameters(const std::vector<std::unique_ptr<asm_operand>>& input,
        const std::string& instr_name,
        const diagnostic_collector& add_diagnostic) const;

    bool check_optable_operands(const std::vector<std::unique_ptr<asm_operand>>& input,
        const std::string& instr_name,
        const diagnostic_collector& add_diagnostic) const;

    bool check_typecheck_operands(const std::vector<std::unique_ptr<asm_operand>>& input,
        const std::string& instr_name,
        const std::string op_name,
        const diagnostic_collector& add_diagnostic) const;

    // process instruction functions
    bool check_codepage_parameter(const one_operand& input, const diagnostic_collector& add_diagnostic) const;

    bool check_fail_parameters(const std::vector<std::unique_ptr<asm_operand>>& input,
        const std::string& instr_name,
        const diagnostic_collector& add_diagnostic) const;

    bool check_first_machine_operand(const std::string& input_str, const diagnostic_collector& add_diagnostic) const;

    bool check_pcontrol_parameters(const std::vector<std::unique_ptr<asm_operand>>& input,
        const std::string& instr_name,
        const diagnostic_collector& add_diagnostic) const;

    bool check_using_parameters(const std::vector<std::unique_ptr<asm_operand>>& input,
        const std::string& instr_name,
        const diagnostic_collector& add_diagnostic) const;

    bool check_xref_parameters(const std::vector<std::unique_ptr<asm_operand>>& input,
        const std::string& instr_name,
        const diagnostic_collector& add_diagnostic) const;

    bool check_suprwarn_parameters(const std::vector<std::unique_ptr<asm_operand>>& input,
        const std::string instr_name,
        const std::string& op_name,
        const diagnostic_collector& add_diagnostic) const;

    bool check_assembler_process_operand(const asm_operand* input, const diagnostic_collector& add_diagnostic) const;
};

} // namespace hlasm_plugin::parser_library::checking

#endif
