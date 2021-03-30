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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CONTEXT_INSTRUCTION_H
#define HLASMPLUGIN_PARSERLIBRARY_CONTEXT_INSTRUCTION_H

#include <array>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "checking/instr_operand.h"
#include "diagnostic.h"
#include "id_storage.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

// all mach_format types for operands of machine instructions:
enum class mach_format
{
    E = 16,
    I = 16,
    IE = 32,
    MII = 48,
    RI_a = 32,
    RI_b = 32,
    RI_c = 32,
    RIE_a = 48,
    RIE_b = 48,
    RIE_c = 48,
    RIE_d = 48,
    RIE_e = 48,
    RIE_f = 48,
    RIE_g = 48,
    RIL_a = 48,
    RIL_b = 48,
    RIL_c = 48,
    RIS = 48,
    RR = 16,
    RRD = 32,
    RRE = 32,
    RRF_a = 32,
    RRF_b = 32,
    RRF_c = 32,
    RRF_d = 32,
    RRF_e = 32,
    RRS = 48,
    RS_a = 32,
    RS_b = 32,
    RSI = 32,
    RSL_a = 48,
    RSL_b = 48,
    RSY_a = 48,
    RSY_b = 48,
    RX_a = 32,
    RX_b = 32,
    RXE = 48,
    RXF = 48,
    RXY_a = 48,
    RXY_b = 48,
    S = 32,
    SI = 32,
    SIL = 48,
    SIY = 48,
    SMI = 48,
    SS_a = 48,
    SS_b = 48,
    SS_c = 48,
    SS_d = 48,
    SS_e = 48,
    SS_f = 48,
    SSE = 48,
    SSF = 48,
    VRI_a = 48,
    VRI_b = 48,
    VRI_c = 48,
    VRI_d = 48,
    VRI_e = 48,
    VRI_f = 48,
    VRR_a = 48,
    VRR_b = 48,
    VRR_c = 48,
    VRR_d = 48,
    VRR_e = 48,
    VRR_f = 48,
    VRS_a = 48,
    VRS_b = 48,
    VRS_c,
    VRV = 48,
    VRX = 48,
    VRI_g = 48,
    VRI_h = 48,
    VRI_i = 48,
    VRR_g = 48,
    VRR_h = 48,
    VRR_i = 48,
    VRS_d = 48,
    VSI = 48
};

const checking::parameter empty = { false, 0, checking::machine_operand_type::NONE };
const checking::parameter reg = { false, 4, checking::machine_operand_type::REG };
const checking::parameter dis_reg = { false, 4, checking::machine_operand_type::DIS_REG };
const checking::parameter dis_reg_r = { false, 4, checking::machine_operand_type::REG };
const checking::parameter mask = { false, 4, checking::machine_operand_type::MASK };
const checking::parameter dis_12u = { false, 12, checking::machine_operand_type::DISPLC };
const checking::parameter dis_20s = { true, 20, checking::machine_operand_type::DISPLC };
const checking::parameter base_ = { false, 4, checking::machine_operand_type::BASE };
const checking::parameter length_8 = { false, 8, checking::machine_operand_type::LENGTH };
const checking::parameter length_4 = { false, 4, checking::machine_operand_type::LENGTH };
const checking::parameter imm_4u = { false, 4, checking::machine_operand_type::IMM };
const checking::parameter imm_8s = { true, 8, checking::machine_operand_type::IMM };
const checking::parameter imm_8u = { false, 8, checking::machine_operand_type::IMM };
const checking::parameter imm_12s = { true, 12, checking::machine_operand_type::IMM };
const checking::parameter imm_16s = { true, 16, checking::machine_operand_type::IMM };
const checking::parameter imm_16u = { false, 16, checking::machine_operand_type::IMM };
const checking::parameter imm_24s = { true, 24, checking::machine_operand_type::IMM };
const checking::parameter imm_32s = { true, 32, checking::machine_operand_type::IMM };
const checking::parameter imm_32u = { false, 32, checking::machine_operand_type::IMM };
const checking::parameter vec_reg = { false, 4, checking::machine_operand_type::VEC_REG };
const checking::parameter reg_imm_12s = { true, 12, checking::machine_operand_type::REG_IMM };
const checking::parameter reg_imm_16s = { true, 16, checking::machine_operand_type::REG_IMM };
const checking::parameter reg_imm_24s = { true, 24, checking::machine_operand_type::REG_IMM };
const checking::parameter reg_imm_32s = { true, 32, checking::machine_operand_type::REG_IMM };

/*
Rules for displacement operands:
With DB formats
        - must be in format D(B), otherwise throw an error
        - parser returns this in (displacement, 0, base, true) format
With DXB Formats
        - can be either D(X,B) or D(,B) - in this case, the X is replaced with 0
        - parser returns this in (displacement, x, base, false) format
*/
const checking::machine_operand_format db_12_4_U = checking::machine_operand_format(dis_12u, empty, base_);
const checking::machine_operand_format db_20_4_S = checking::machine_operand_format(dis_20s, empty, base_);
const checking::machine_operand_format drb_12_4x4_U = checking::machine_operand_format(dis_12u, dis_reg_r, base_);
const checking::machine_operand_format dxb_12_4x4_U = checking::machine_operand_format(dis_12u, dis_reg, base_);
const checking::machine_operand_format dxb_20_4x4_S = checking::machine_operand_format(dis_20s, dis_reg, base_);
const checking::machine_operand_format dvb_12_4x4_U = checking::machine_operand_format(dis_12u, vec_reg, base_);
const checking::machine_operand_format reg_4_U = checking::machine_operand_format(reg, empty, empty);
const checking::machine_operand_format mask_4_U = checking::machine_operand_format(mask, empty, empty);
const checking::machine_operand_format imm_4_U = checking::machine_operand_format(imm_4u, empty, empty);
const checking::machine_operand_format imm_8_S = checking::machine_operand_format(imm_8s, empty, empty);
const checking::machine_operand_format imm_8_U = checking::machine_operand_format(imm_8u, empty, empty);
const checking::machine_operand_format imm_16_U = checking::machine_operand_format(imm_16u, empty, empty);
const checking::machine_operand_format imm_12_S = checking::machine_operand_format(imm_12s, empty, empty);
const checking::machine_operand_format imm_16_S = checking::machine_operand_format(imm_16s, empty, empty);
const checking::machine_operand_format imm_32_S = checking::machine_operand_format(imm_32s, empty, empty);
const checking::machine_operand_format imm_32_U = checking::machine_operand_format(imm_32u, empty, empty);
const checking::machine_operand_format vec_reg_4_U = checking::machine_operand_format(vec_reg, empty, empty);
const checking::machine_operand_format db_12_8x4L_U = checking::machine_operand_format(dis_12u, length_8, base_);
const checking::machine_operand_format db_12_4x4L_U = checking::machine_operand_format(dis_12u, length_4, base_);
const checking::machine_operand_format reg_imm_12_S = checking::machine_operand_format(reg_imm_12s, empty, empty);
const checking::machine_operand_format reg_imm_16_S = checking::machine_operand_format(reg_imm_16s, empty, empty);
const checking::machine_operand_format reg_imm_24_S = checking::machine_operand_format(reg_imm_24s, empty, empty);
const checking::machine_operand_format reg_imm_32_S = checking::machine_operand_format(reg_imm_32s, empty, empty);

// machine instruction representation for checking
class machine_instruction
{
public:
    std::string instr_name;
    mach_format format;
    std::vector<checking::machine_operand_format> operands; // what the vector of operands should look like
    size_t size_for_alloc;
    int no_optional;
    size_t page_no;

    machine_instruction(const std::string& name,
        mach_format format,
        std::vector<checking::machine_operand_format> operands,
        int no_optional,

        size_t page_no)
        : instr_name(name)
        , format(format)
        , operands(operands)
        , size_for_alloc(get_length_by_format(format))
        , no_optional(no_optional)
        , page_no(page_no) {};
    machine_instruction(const std::string& name,
        mach_format format,
        std::vector<checking::machine_operand_format> operands,
        size_t page_no)
        : machine_instruction(name, format, operands, 0, page_no)
    {}

    bool check_nth_operand(size_t place, const checking::machine_operand* operand);
    const int& get_length_by_format(mach_format instruction_format)
    {
        int value = static_cast<int>(instruction_format);
        return value;
    }
    virtual bool check(const std::string& name_of_instruction,
        const std::vector<const checking::machine_operand*> operands,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic); // input vector is the vector of the actual incoming values

    // std::vector<diag_range> & get_diagnostics()
    void clear_diagnostics();
    std::vector<diagnostic_op> diagnostics;
    virtual ~machine_instruction() = default;
};

using machine_instruction_ptr = std::unique_ptr<machine_instruction>;

struct ca_instruction
{
    std::string name;
    bool operandless;
};

// representation of mnemonic codes for machine instructions
struct mnemonic_code
{
public:
    mnemonic_code(std::string instr, std::vector<std::pair<size_t, size_t>> replaced)
        : instruction(instr)
        , replaced(replaced) {};

    std::string instruction;
    // first goes place, then value
    std::vector<std::pair<size_t, size_t>> replaced;
};

// machine instruction common representation
struct assembler_instruction
{
    int min_operands;
    int max_operands; // -1 in case there is no max value
    bool has_ord_symbols;
    std::string description; // used only for hover and completion

    assembler_instruction(int min_operands, int max_operands, bool has_ord_symbols, std::string description)
        : min_operands(min_operands)
        , max_operands(max_operands)
        , has_ord_symbols(has_ord_symbols)
        , description(std::move(description)) {};
};

// static class holding string names of instructions with theirs additional info
class instruction
{
public:
    enum class instruction_array
    {
        CA,
        ASM,
        MACH,
        MNEM
    };

    static std::map<const std::string, machine_instruction_ptr> get_machine_instructions();

    static std::map<const std::string, mnemonic_code> get_mnemonic_codes();

    /*
    min_operands - minimal number of operands, non-negative integer, always defined
    max_operands - if not defined (can be infinite), value is -1, otherwise a non-negative integer
    */

    static const std::vector<ca_instruction> ca_instructions;

    static const std::map<const std::string, assembler_instruction> assembler_instructions;

    // static const std::vector<std::string> macro_processing_instructions;

    static std::map<const std::string, machine_instruction_ptr> machine_instructions;

    static const std::map<const std::string, mnemonic_code> mnemonic_codes;

    static const std::map<mach_format, const std::string> mach_format_to_string;
};


} // namespace context
} // namespace parser_library
} // namespace hlasm_plugin

#endif
