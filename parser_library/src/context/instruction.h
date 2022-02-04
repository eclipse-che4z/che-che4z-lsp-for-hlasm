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
#include <limits>
#include <map>
#include <set>
#include <span>
#include <string>

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
    VSI,
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
    explicit constexpr reladdr_transform_mask(unsigned char m)
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

template<unsigned char n>
class inline_string
{
    unsigned char len;
    std::array<char, n> data;

public:
    explicit constexpr inline_string(std::string_view s)
        : len((unsigned char)s.size())
        , data {}
    {
        assert(s.size() <= n);
        size_t i = 0;
        for (char c : s)
            data[i++] = c;
    }

    constexpr std::string_view to_string_view() const { return std::string_view(data.data(), len); }

    friend std::strong_ordering operator<=>(const inline_string& l, const inline_string& r)
    {
        return l.to_string_view() <=> r.to_string_view();
    }
};


struct instruction_format_definition
{
    std::span<const checking::machine_operand_format> op_format;

    mach_format format;
};

// machine instruction representation for checking
class machine_instruction
{
    // Generates a bitmask for an arbitrary machine instruction indicating which operands
    // are of the RI type (and therefore are modified by transform_reloc_imm_operands)
    static constexpr unsigned char generate_reladdr_bitmask(std::span<const checking::machine_operand_format> operands)
    {
        unsigned char result = 0;

        assert(operands.size() <= std::numeric_limits<decltype(result)>::digits);

        decltype(result) top_bit = 1 << (std::numeric_limits<decltype(result)>::digits - 1);

        for (const auto& op : operands)
        {
            if (op.identifier.type == checking::machine_operand_type::RELOC_IMM)
                result |= top_bit;
            top_bit >>= 1;
        }

        return result;
    }

    static constexpr char get_length_by_format(mach_format instruction_format)
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

    inline_string<7> m_name;

    mach_format m_format;
    char m_size_in_bits;
    unsigned short m_page_no;

    reladdr_transform_mask m_reladdr_mask;
    unsigned char m_optional_op_count;
    unsigned char m_operand_len;

    const checking::machine_operand_format* m_operands;

public:
    constexpr machine_instruction(std::string_view name,
        mach_format format,
        std::span<const checking::machine_operand_format> operands,
        unsigned short page_no)
        : m_name(name)
        , m_format(format)
        , m_size_in_bits(get_length_by_format(format))
        , m_page_no(page_no)
        , m_reladdr_mask(generate_reladdr_bitmask(operands))
        , m_optional_op_count(
              (unsigned char)std::ranges::count(operands, true, &checking::machine_operand_format::optional))
        , m_operand_len((unsigned char)operands.size())
        , m_operands(operands.data())
    {
        assert(operands.size() <= std::numeric_limits<decltype(m_operand_len)>::max());
    }
    constexpr machine_instruction(std::string_view name, instruction_format_definition ifd, unsigned short page_no)
        : machine_instruction(name, ifd.format, ifd.op_format, page_no)
    {}

    constexpr std::string_view name() const { return m_name.to_string_view(); }
    mach_format format() const { return m_format; }
    constexpr size_t page_no() const { return m_page_no; }
    constexpr size_t size_in_bits() const { return m_size_in_bits; }
    constexpr reladdr_transform_mask reladdr_mask() const { return m_reladdr_mask; }
    constexpr std::span<const checking::machine_operand_format> operands() const
    {
        return std::span<const checking::machine_operand_format>(m_operands, m_operand_len);
    }
    constexpr size_t optional_operand_count() const { return m_optional_op_count; }

    bool check_nth_operand(size_t place, const checking::machine_operand* operand);
    bool check(std::string_view name_of_instruction,
        const std::vector<const checking::machine_operand*> operands,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const; // input vector is the vector of the actual incoming values
};

class ca_instruction
{
    inline_string<6> m_name;
    bool m_operandless;

public:
    constexpr ca_instruction(std::string_view n, bool opless)
        : m_name(n)
        , m_operandless(opless)
    {}

    constexpr auto name() const { return m_name.to_string_view(); }
    constexpr auto operandless() const { return m_operandless; }
};

// representation of mnemonic codes for machine instructions
class mnemonic_code
{
    const machine_instruction* m_instruction;

    // first goes place, then value
    std::array<std::pair<unsigned char, unsigned char>, 3> m_replaced;
    unsigned char m_replaced_count;

    reladdr_transform_mask m_reladdr_mask;

    inline_string<9> m_name;

    // Generates a bitmask for an arbitrary mnemonit indicating which operands
    // are of the RI type (and therefore are modified by transform_reloc_imm_operands)
    static constexpr unsigned char generate_reladdr_bitmask(const machine_instruction* instruction,
        std::initializer_list<const std::pair<unsigned char, unsigned char>> replaced)
    {
        unsigned char result = 0;

        decltype(result) top_bit = 1 << (std::numeric_limits<decltype(result)>::digits - 1);

        auto replaced_b = replaced.begin();
        auto const replaced_e = replaced.end();

        size_t position = 0;
        for (const auto& op : instruction->operands())
        {
            if (replaced_b != replaced_e && position == replaced_b->first)
            {
                ++replaced_b;
                ++position;
                continue;
            }

            if (op.identifier.type == checking::machine_operand_type::RELOC_IMM)
                result |= top_bit;
            top_bit >>= 1;

            ++position;
        }
        return result;
    }

public:
    constexpr mnemonic_code(std::string_view name,
        const machine_instruction* instr,
        std::initializer_list<const std::pair<unsigned char, unsigned char>> replaced)
        : m_instruction(instr)
        , m_replaced {}
        , m_replaced_count((unsigned char)replaced.size())
        , m_reladdr_mask(generate_reladdr_bitmask(instr, replaced))
        , m_name(name)
    {
        assert(replaced.size() <= m_replaced.size());
        size_t i = 0;
        for (const auto& r : replaced)
            m_replaced[i++] = r;
    };

    constexpr const machine_instruction* instruction() const { return m_instruction; }
    constexpr std::span<const std::pair<unsigned char, unsigned char>> replaced_operands() const
    {
        return { m_replaced.data(), m_replaced_count };
    }
    constexpr size_t operand_count() const
    {
        return m_instruction->operands().size() + m_instruction->optional_operand_count() - m_replaced_count;
    }
    constexpr reladdr_transform_mask reladdr_mask() const { return m_reladdr_mask; }
    constexpr std::string_view name() const { return m_name.to_string_view(); }
};

// machine instruction common representation
class assembler_instruction
{
    inline_string<11> m_name;
    bool m_has_ord_symbols;
    int m_min_operands;
    int m_max_operands; // -1 in case there is no max value
    std::string_view m_description; // used only for hover and completion

public:
    constexpr assembler_instruction(
        std::string_view name, int min_operands, int max_operands, bool has_ord_symbols, std::string_view description)
        : m_name(name)
        , m_has_ord_symbols(has_ord_symbols)
        , m_min_operands(min_operands)
        , m_max_operands(max_operands)
        , m_description(std::move(description)) {};

    constexpr auto name() const { return m_name.to_string_view(); }
    constexpr auto has_ord_symbols() const { return m_has_ord_symbols; }
    constexpr auto min_operands() const { return m_min_operands; }
    constexpr auto max_operands() const { return m_max_operands; }
    constexpr auto description() const { return m_description; }
};

// static class holding string names of instructions with theirs additional info
class instruction
{
public:
    /*
    min_operands - minimal number of operands, non-negative integer, always defined
    max_operands - if not defined (can be infinite), value is -1, otherwise a non-negative integer
    */

    static const ca_instruction& get_ca_instructions(std::string_view name);
    static const ca_instruction* find_ca_instructions(std::string_view name);
    static std::span<const ca_instruction> all_ca_instructions();

    static const assembler_instruction& get_assembler_instructions(std::string_view name);
    static const assembler_instruction* find_assembler_instructions(std::string_view name);
    static std::span<const assembler_instruction> all_assembler_instructions();

    static const machine_instruction& get_machine_instructions(std::string_view name);
    static const machine_instruction* find_machine_instructions(std::string_view name);
    static std::span<const machine_instruction> all_machine_instructions();

    static const mnemonic_code& get_mnemonic_codes(std::string_view name);
    static const mnemonic_code* find_mnemonic_codes(std::string_view name);
    static std::span<const mnemonic_code> all_mnemonic_codes();

    static std::string_view mach_format_to_string(mach_format);
};

} // namespace hlasm_plugin::parser_library::context

#endif
