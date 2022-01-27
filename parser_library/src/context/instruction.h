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

#include <algorithm>
#include <array>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "checking/instr_operand.h"
#include "diagnostic.h"
#include "id_storage.h"
namespace hlasm_plugin::parser_library::context {

// all mach_format types for operands of machine instructions:
// formats with length 16 are arranged in range (0,2),formats with length 32 are arranged in range(3,20),formats with
// length 48 are arranged in range (21,77)
enum class mach_format : unsigned char
{
    length_16,
    E = length_16,
    I,
    RR,

    length_32,
    IE = length_32,
    RRD,
    RRE,
    RRF_a,
    RRF_b,
    RRF_c,
    RRF_d,
    RRF_e,
    RI_a,
    RI_b,
    RI_c,
    RS_a,
    RS_b,
    RSI,
    RX_a,
    RX_b,
    S,
    SI,

    length_48,
    MII = length_48,
    RIE_a,
    RIE_b,
    RIE_c,
    RIE_d,
    RIE_e,
    RIE_f,
    RIE_g,
    RIL_a,
    RIL_b,
    RIL_c,
    RIS,
    RRS,
    RSL_a,
    RSL_b,
    RSY_a,
    RSY_b,
    RXE,
    RXF,
    RXY_a,
    RXY_b,
    SIL,
    SIY,
    SMI,
    SS_a,
    SS_b,
    SS_c,
    SS_d,
    SS_e,
    SS_f,
    SSE,
    SSF,
    VRI_a,
    VRI_b,
    VRI_c,
    VRI_d,
    VRI_e,
    VRI_f,
    VRR_a,
    VRR_b,
    VRR_c,
    VRR_d,
    VRR_e,
    VRR_f,
    VRS_a,
    VRS_b,
    VRS_c,
    VRV,
    VRX,
    VRI_g,
    VRI_h,
    VRI_i,
    VRR_g,
    VRR_h,
    VRR_i,
    VRS_d,
    VSI
};

constexpr checking::parameter empty = { false, 0, checking::machine_operand_type::NONE };
constexpr checking::parameter reg = { false, 4, checking::machine_operand_type::REG };
constexpr checking::parameter dis_reg = { false, 4, checking::machine_operand_type::DIS_REG };
constexpr checking::parameter dis_reg_r = { false, 4, checking::machine_operand_type::REG };
constexpr checking::parameter mask = { false, 4, checking::machine_operand_type::MASK };
constexpr checking::parameter dis_12u = { false, 12, checking::machine_operand_type::DISPLC };
constexpr checking::parameter dis_20s = { true, 20, checking::machine_operand_type::DISPLC };
constexpr checking::parameter base_ = { false, 4, checking::machine_operand_type::BASE };
constexpr checking::parameter length_8 = { false, 8, checking::machine_operand_type::LENGTH };
constexpr checking::parameter length_4 = { false, 4, checking::machine_operand_type::LENGTH };
constexpr checking::parameter imm_4u = { false, 4, checking::machine_operand_type::IMM };
constexpr checking::parameter imm_8s = { true, 8, checking::machine_operand_type::IMM };
constexpr checking::parameter imm_8u = { false, 8, checking::machine_operand_type::IMM };
constexpr checking::parameter imm_12s = { true, 12, checking::machine_operand_type::IMM };
constexpr checking::parameter imm_16s = { true, 16, checking::machine_operand_type::IMM };
constexpr checking::parameter imm_16u = { false, 16, checking::machine_operand_type::IMM };
constexpr checking::parameter imm_24s = { true, 24, checking::machine_operand_type::IMM };
constexpr checking::parameter imm_32s = { true, 32, checking::machine_operand_type::IMM };
constexpr checking::parameter imm_32u = { false, 32, checking::machine_operand_type::IMM };
constexpr checking::parameter vec_reg = { false, 5, checking::machine_operand_type::VEC_REG };
constexpr checking::parameter reladdr_imm_12s = { true, 12, checking::machine_operand_type::RELOC_IMM };
constexpr checking::parameter reladdr_imm_16s = { true, 16, checking::machine_operand_type::RELOC_IMM };
constexpr checking::parameter reladdr_imm_24s = { true, 24, checking::machine_operand_type::RELOC_IMM };
constexpr checking::parameter reladdr_imm_32s = { true, 32, checking::machine_operand_type::RELOC_IMM };

/*
Rules for displacement operands:
With DB formats
        - must be in format D(B), otherwise throw an error
        - parser returns this in (displacement, 0, base, true) format
With DXB Formats
        - can be either D(X,B) or D(,B) - in this case, the X is replaced with 0
        - parser returns this in (displacement, x, base, false) format
*/
constexpr checking::machine_operand_format db_12_4_U(dis_12u, empty, base_);
constexpr checking::machine_operand_format db_20_4_S(dis_20s, empty, base_);
constexpr checking::machine_operand_format drb_12_4x4_U(dis_12u, dis_reg_r, base_);
constexpr checking::machine_operand_format dxb_12_4x4_U(dis_12u, dis_reg, base_);
constexpr checking::machine_operand_format dxb_20_4x4_S(dis_20s, dis_reg, base_);
constexpr checking::machine_operand_format dvb_12_5x4_U(dis_12u, vec_reg, base_);
constexpr checking::machine_operand_format reg_4_U(reg, empty, empty);
constexpr checking::machine_operand_format mask_4_U(mask, empty, empty);
constexpr checking::machine_operand_format imm_4_U(imm_4u, empty, empty);
constexpr checking::machine_operand_format imm_8_S(imm_8s, empty, empty);
constexpr checking::machine_operand_format imm_8_U(imm_8u, empty, empty);
constexpr checking::machine_operand_format imm_16_U(imm_16u, empty, empty);
constexpr checking::machine_operand_format imm_12_S(imm_12s, empty, empty);
constexpr checking::machine_operand_format imm_16_S(imm_16s, empty, empty);
constexpr checking::machine_operand_format imm_32_S(imm_32s, empty, empty);
constexpr checking::machine_operand_format imm_32_U(imm_32u, empty, empty);
constexpr checking::machine_operand_format vec_reg_5_U(vec_reg, empty, empty);
constexpr checking::machine_operand_format db_12_8x4L_U(dis_12u, length_8, base_);
constexpr checking::machine_operand_format db_12_4x4L_U(dis_12u, length_4, base_);
constexpr checking::machine_operand_format rel_addr_imm_12_S(reladdr_imm_12s, empty, empty);
constexpr checking::machine_operand_format rel_addr_imm_16_S(reladdr_imm_16s, empty, empty);
constexpr checking::machine_operand_format rel_addr_imm_24_S(reladdr_imm_24s, empty, empty);
constexpr checking::machine_operand_format rel_addr_imm_32_S(reladdr_imm_32s, empty, empty);

// optional variants
constexpr checking::machine_operand_format db_12_4_U_opt(dis_12u, empty, base_, true);
constexpr checking::machine_operand_format db_20_4_S_opt(dis_20s, empty, base_, true);
constexpr checking::machine_operand_format drb_12_4x4_U_opt(dis_12u, dis_reg_r, base_, true);
constexpr checking::machine_operand_format dxb_12_4x4_U_opt(dis_12u, dis_reg, base_, true);
constexpr checking::machine_operand_format dxb_20_4x4_S_opt(dis_20s, dis_reg, base_, true);
constexpr checking::machine_operand_format dvb_12_5x4_U_opt(dis_12u, vec_reg, base_, true);
constexpr checking::machine_operand_format reg_4_U_opt(reg, empty, empty, true);
constexpr checking::machine_operand_format mask_4_U_opt(mask, empty, empty, true);
constexpr checking::machine_operand_format imm_4_U_opt(imm_4u, empty, empty, true);
constexpr checking::machine_operand_format imm_8_S_opt(imm_8s, empty, empty, true);
constexpr checking::machine_operand_format imm_8_U_opt(imm_8u, empty, empty, true);
constexpr checking::machine_operand_format imm_16_U_opt(imm_16u, empty, empty, true);
constexpr checking::machine_operand_format imm_12_S_opt(imm_12s, empty, empty, true);
constexpr checking::machine_operand_format imm_16_S_opt(imm_16s, empty, empty, true);
constexpr checking::machine_operand_format imm_32_S_opt(imm_32s, empty, empty, true);
constexpr checking::machine_operand_format imm_32_U_opt(imm_32u, empty, empty, true);
constexpr checking::machine_operand_format vec_reg_5_U_opt(vec_reg, empty, empty, true);
constexpr checking::machine_operand_format db_12_8x4L_U_opt(dis_12u, length_8, base_, true);
constexpr checking::machine_operand_format db_12_4x4L_U_opt(dis_12u, length_4, base_, true);
constexpr checking::machine_operand_format rel_addr_imm_12_S_opt(reladdr_imm_12s, empty, empty, true);
constexpr checking::machine_operand_format rel_addr_imm_16_S_opt(reladdr_imm_16s, empty, empty, true);
constexpr checking::machine_operand_format rel_addr_imm_24_S_opt(reladdr_imm_24s, empty, empty, true);
constexpr checking::machine_operand_format rel_addr_imm_32_S_opt(reladdr_imm_32s, empty, empty, true);

class reladdr_transform_mask
{
    unsigned char m_mask;

public:
    constexpr reladdr_transform_mask(unsigned char m)
        : m_mask(m)
    {}
    constexpr unsigned char mask() const { return m_mask; }

    constexpr friend bool operator==(const reladdr_transform_mask& l, const reladdr_transform_mask& r)
    {
        return l.m_mask == r.m_mask;
    }
    constexpr friend bool operator!=(const reladdr_transform_mask& l, const reladdr_transform_mask& r)
    {
        return !(l == r);
    }
};

// machine instruction representation for checking
struct machine_instruction
{
    static unsigned char generate_reladdr_bitmask(const std::vector<checking::machine_operand_format>& operands);

    std::string_view instr_name;
    mach_format format;
    char size_for_alloc;
    short page_no;
    short no_optional;
    reladdr_transform_mask reladdr_mask;
    std::vector<checking::machine_operand_format> operands; // what the vector of operands should look like


    machine_instruction(std::string_view name,
        mach_format format,
        std::vector<checking::machine_operand_format> operands,
        short page_no)
        : instr_name(name)
        , format(format)
        , size_for_alloc(get_length_by_format(format))
        , page_no(page_no)
        , no_optional((short)std::count_if(operands.begin(), operands.end(), [](const auto& o) { return o.optional; }))
        , reladdr_mask(generate_reladdr_bitmask(operands))
        , operands(std::move(operands))
    {}

    bool check_nth_operand(size_t place, const checking::machine_operand* operand);

    static char get_length_by_format(mach_format instruction_format)
    {
        auto interval = (int)(instruction_format);
        if (interval >= (int)mach_format::length_48)
            return 48;
        if (interval >= (int)mach_format::length_32)
            return 32;
        if (interval >= (int)mach_format::length_16)
            return 16;
        return 0;
    }
    bool check(std::string_view name_of_instruction,
        const std::vector<const checking::machine_operand*> operands,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const; // input vector is the vector of the actual incoming values
};

struct machine_instruction_comparer
{
    using is_transparent = void;

    bool operator()(const machine_instruction& l, const machine_instruction& r) const
    {
        return l.instr_name < r.instr_name;
    }
    template<typename L>
    bool operator()(const L& l, const machine_instruction& r) const
    {
        return l < r.instr_name;
    }
    template<typename R>
    bool operator()(const machine_instruction& l, const R& r) const
    {
        return l.instr_name < r;
    }
};

struct ca_instruction
{
    std::string name;
    bool operandless;
};

// representation of mnemonic codes for machine instructions
struct mnemonic_code
{
    static unsigned char generate_reladdr_bitmask(
        const machine_instruction* instruction, const std::vector<std::pair<size_t, size_t>>& replaced);

public:
    mnemonic_code(const machine_instruction* instr, std::vector<std::pair<size_t, size_t>> replaced)
        : instruction(instr)
        , replaced(replaced)
        , reladdr_mask(generate_reladdr_bitmask(instr, replaced)) {};

    const machine_instruction* instruction;

    // first goes place, then value
    std::vector<std::pair<size_t, size_t>> replaced;

    reladdr_transform_mask reladdr_mask;

    size_t operand_count() const { return instruction->operands.size() + instruction->no_optional - replaced.size(); }
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
    /*
    min_operands - minimal number of operands, non-negative integer, always defined
    max_operands - if not defined (can be infinite), value is -1, otherwise a non-negative integer
    */

    static const std::vector<ca_instruction> ca_instructions;

    static const std::map<std::string, assembler_instruction> assembler_instructions;

    static const machine_instruction& get_machine_instructions(std::string_view name);
    static const machine_instruction* find_machine_instructions(std::string_view name);
    static const std::set<machine_instruction, machine_instruction_comparer>& all_machine_instructions();

    static const std::map<std::string, mnemonic_code> mnemonic_codes;

    static const std::map<mach_format, std::string> mach_format_to_string;
};

} // namespace hlasm_plugin::parser_library::context

#endif
