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

#ifndef HLASMPLUGIN_PARSERLIBRARY_ASSEMBLER_INSTRUCTION_H
#define HLASMPLUGIN_PARSERLIBRARY_ASSEMBLER_INSTRUCTION_H

// This file defines classes that implements checking for all
// assembler instructions.

#include "asm_instr_class.h"
#include "data_definition/data_def_type_base.h"

namespace hlasm_plugin::parser_library {
class diagnostic_collector;
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::checking {
/*
TO DO - notes
- dxd, dc, ds instructions - if evaluation and control takes places before checker, these instructions need only one
class for all of them and need to just check complexity - TO DO
- operand evaluation before passing arguments to this class
- parameters to class need to be already parsed in a vector
- all parameters passed to class need to be uppercase
*/

constexpr int TITLE_max_length = 102;
constexpr int string_max_length = 80;
constexpr int ORG_max_boundary_val = 4096;
constexpr int ORG_min_boundary_val = 2;
constexpr int MNOTE_max_message_length = 1020;
constexpr int MNOTE_max_operands_length = 1024;
constexpr int ISEQ_min_op_val = 1;
constexpr int ISEQ_max_op_val = 80;
constexpr int ICTL_begin_min_val = 1;
constexpr int ICTL_begin_max_val = 40;
constexpr int ICTL_end_min_val = 41;
constexpr int ICTL_end_max_val = 80;
constexpr int ICTL_continuation_min_val = 2;
constexpr int ICTL_continuation_max_val = 40;
constexpr int ICTL_begin_end_diff = 5;
constexpr int EQU_max_length_att_val = 65535;
constexpr int ENTRY_max_operands = 65535;
constexpr int END_lang_first_par_size = 10;
constexpr int END_lang_second_par_size = 4;
constexpr int END_lang_third_par_size = 5;
constexpr int CNOP_max_boundary_val = 4096;
constexpr int CNOP_min_boundary_val = 0;
constexpr int CNOP_byte_boundary_diff = 2;
constexpr int ALIAS_max_val = 0xFE;
constexpr int ALIAS_min_val = 0x42;

class xattr final : public assembler_instruction
{
public:
    xattr(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class using_instr final : public assembler_instruction
{
public:
    using_instr(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class title final : public assembler_instruction
{
public:
    title(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class rmode final : public assembler_instruction
{
public:
    rmode(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class punch final : public assembler_instruction
{
public:
    punch(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class print final : public assembler_instruction
{
public:
    print(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class stack_instr final : public assembler_instruction
{
public:
    stack_instr(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class org final : public assembler_instruction
{
public:
    org(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

// class for opsyn instruction, TO DO - operation code checker (need to be previously defined)
class opsyn final : public assembler_instruction
{
public:
    opsyn(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class mnote final : public assembler_instruction
{
public:
    mnote(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class iseq final : public assembler_instruction
{
public:
    iseq(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class ictl final : public assembler_instruction
{
public:
    ictl(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

// class for external instructions (extrn and wxtrn), TO DO check external symbols
class external final : public assembler_instruction
{
public:
    external(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class exitctl final : public assembler_instruction
{
public:
    exitctl(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

// class for equ instruction, TO DO - check value?
class equ final : public assembler_instruction
{
public:
    equ(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

// class for entry instruction, TO DO - check entry point symbols
class entry final : public assembler_instruction
{
public:
    entry(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class end final : public assembler_instruction
{
public:
    end(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class drop final : public assembler_instruction
{
public:
    drop(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector&) const override;
};

// class for copy instruction, TO DO - parse member
class copy final : public assembler_instruction
{
public:
    copy(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class cnop final : public assembler_instruction
{
public:
    cnop(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

enum class CCW_variant
{
    CCW_CCW0,
    CCW1
};

// class for ccw (and ccw0, ccw1) instruction, operands can be expressions, TO DO
class ccw final : public assembler_instruction
{
    CCW_variant variant_;

public:
    ccw(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction, CCW_variant variant);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

// class for instruction requiring only one expression_instruction (defining for example number of lines) - ceject,
// space and start instr
class expression_instruction final : public assembler_instruction
{
public:
    expression_instruction(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class cattr final : public assembler_instruction
{
public:
    cattr(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class amode final : public assembler_instruction
{
public:
    amode(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class alias final : public assembler_instruction
{
public:
    alias(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class ainsert final : public assembler_instruction
{
public:
    ainsert(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class acontrol final : public assembler_instruction
{
public:
    acontrol(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

class adata final : public assembler_instruction
{
public:
    adata(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

// class for instructions without operands
class no_operands final : public assembler_instruction
{
public:
    no_operands(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

// class for process instruction
class process final : public assembler_instruction
{
public:
    process(const std::vector<label_types>& allowed_types, std::string_view name_of_instruction);
    bool check(std::span<const asm_operand* const> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const override;
};

} // namespace hlasm_plugin::parser_library::checking

#endif
