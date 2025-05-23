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
#include <compare>
#include <functional>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <span>
#include <string>

#include "checking/instr_operand.h"
#include "diagnostic.h"
#include "id_storage.h"
#include "instruction_set_version.h"

namespace hlasm_plugin::parser_library {
class diagnostic_collector;
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::context {

enum class z_arch_affiliation : uint16_t
{
    NO_AFFILIATION = 0,
    SINCE_ZOP,
    SINCE_YOP,
    SINCE_Z9,
    SINCE_Z10,
    SINCE_Z11,
    SINCE_Z12,
    SINCE_Z13,
    SINCE_Z14,
    SINCE_Z15,
    SINCE_Z16,
};

struct instruction_set_affiliation
{
    z_arch_affiliation z_arch : 4;
    uint16_t esa : 1;
    uint16_t xa : 1;
    uint16_t _370 : 1;
    uint16_t dos : 1;
    uint16_t uni : 1;
};

constexpr bool operator<=(z_arch_affiliation z_affil, instruction_set_version instr_set) noexcept
{
    return static_cast<uint16_t>(z_affil) <= static_cast<uint16_t>(instr_set);
}

constexpr bool instruction_available(
    instruction_set_affiliation instr_set_affiliation, instruction_set_version active_instr_set) noexcept
{
    switch (active_instr_set)
    {
        case instruction_set_version::UNI:
            return instr_set_affiliation.uni;
        case instruction_set_version::DOS:
            return instr_set_affiliation.dos;
        case instruction_set_version::_370:
            return instr_set_affiliation._370;
        case instruction_set_version::XA:
            return instr_set_affiliation.xa;
        case instruction_set_version::ESA:
            return instr_set_affiliation.esa;
        case instruction_set_version::ZOP:
        case instruction_set_version::YOP:
        case instruction_set_version::Z9:
        case instruction_set_version::Z10:
        case instruction_set_version::Z11:
        case instruction_set_version::Z12:
        case instruction_set_version::Z13:
        case instruction_set_version::Z14:
        case instruction_set_version::Z15:
        case instruction_set_version::Z16:
            return instr_set_affiliation.z_arch == z_arch_affiliation::NO_AFFILIATION
                ? false
                : instr_set_affiliation.z_arch <= active_instr_set;
        default:
            return false;
    }
}

struct instruction_set_size
{
    size_t mnemonic;
    size_t machine;
    size_t ca;
    size_t assembler;

    constexpr size_t total() const { return mnemonic + machine + ca + assembler; }
};

const instruction_set_size& get_instruction_sizes(instruction_set_version v) noexcept;

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
    DIAGNOSE,

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
    VRR_j,
    VRR_k,
    VRS_d,
    VSI,
};

constexpr checking::parameter empty = { false, 0, checking::machine_operand_type::NONE };
constexpr checking::parameter reg = { false, 4, checking::machine_operand_type::REG };
constexpr checking::parameter reg_nz = {
    false, 4, checking::machine_operand_type::REG, checking::even_odd_register::NONE, 1
};
constexpr checking::parameter reg_2 = {
    false, 4, checking::machine_operand_type::REG, checking::even_odd_register::NONE, 2
};
constexpr checking::parameter reg_odd = {
    false, 4, checking::machine_operand_type::REG, checking::even_odd_register::ODD
};
constexpr checking::parameter reg_even = {
    false, 4, checking::machine_operand_type::REG, checking::even_odd_register::EVEN
};
constexpr checking::parameter reg_even_nz = {
    false, 4, checking::machine_operand_type::REG, checking::even_odd_register::EVEN, 2
};
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
constexpr checking::parameter imm_12u = { false, 12, checking::machine_operand_type::IMM };
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
constexpr checking::machine_operand_format reg_4_U_nz(reg_nz, empty, empty);
constexpr checking::machine_operand_format reg_4_U_2(reg_2, empty, empty);
constexpr checking::machine_operand_format reg_4_U_odd(reg_odd, empty, empty);
constexpr checking::machine_operand_format reg_4_U_even(reg_even, empty, empty);
constexpr checking::machine_operand_format reg_4_U_even_nz(reg_even_nz, empty, empty);
constexpr checking::machine_operand_format mask_4_U(mask, empty, empty);
constexpr checking::machine_operand_format imm_4_U(imm_4u, empty, empty);
constexpr checking::machine_operand_format imm_8_S(imm_8s, empty, empty);
constexpr checking::machine_operand_format imm_8_U(imm_8u, empty, empty);
constexpr checking::machine_operand_format imm_12_S(imm_12s, empty, empty);
constexpr checking::machine_operand_format imm_12_U(imm_12u, empty, empty);
constexpr checking::machine_operand_format imm_16_S(imm_16s, empty, empty);
constexpr checking::machine_operand_format imm_16_U(imm_16u, empty, empty);
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

    constexpr friend bool operator==(const reladdr_transform_mask& l, const reladdr_transform_mask& r) noexcept
    {
        return l.m_mask == r.m_mask;
    }
    constexpr friend bool operator!=(const reladdr_transform_mask& l, const reladdr_transform_mask& r) noexcept
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

    constexpr std::string_view to_string_view() const noexcept { return std::string_view(data.data(), len); }

    friend std::strong_ordering operator<=>(const inline_string& l, const inline_string& r) noexcept
    {
        return l.to_string_view() <=> r.to_string_view();
    }
};

enum class condition_code
{
    _0,
    _1,
    _2,
    _3,
};

class condition_code_explanation
{
    std::array<const char*, 5> text;
    std::array<unsigned char, 5> lengths;
    bool single_explanation;

    static constexpr bool identical(const char* t0, const char* t1, const char* t2, const char* t3, size_t n)
    {
        return std::equal(t0, t0 + n, t1, t1 + n) && std::equal(t0, t0 + n, t2, t2 + n)
            && std::equal(t0, t0 + n, t3, t3 + n);
    }

public:
    template<size_t L0>
    explicit consteval condition_code_explanation(const char (&t0)[L0]) noexcept requires(L0 > 1 && L0 < 256)
        : text { t0, t0, t0, t0 }
        , lengths { L0 - 1, L0 - 1, L0 - 1, L0 - 1 }
        , single_explanation(true)
    {}
    template<size_t L0, size_t L1, size_t L2, size_t L3>
    explicit consteval condition_code_explanation(
        const char (&t0)[L0], const char (&t1)[L1], const char (&t2)[L2], const char (&t3)[L3]) noexcept
        requires(L0 > 0 && L1 > 0 && L2 > 0 && L3 > 0 && L0 < 256 && L1 < 256 && L2 < 256 && L3 < 256)
        : text { L0 == 1 ? nullptr : t0, L1 == 1 ? nullptr : t1, L2 == 1 ? nullptr : t2, L3 == 1 ? nullptr : t3 }
        , lengths { L0 - 1, L1 - 1, L2 - 1, L3 - 1 }
        , single_explanation(L0 == L1 && L0 == L2 && L0 == L3 && identical(t0, t1, t2, t3, L0))
    {}
    template<size_t L0, size_t L1, size_t L2, size_t L3, size_t Qual>
    explicit consteval condition_code_explanation(const char (&t0)[L0],
        const char (&t1)[L1],
        const char (&t2)[L2],
        const char (&t3)[L3],
        const char (&qualification)[Qual]) noexcept
        requires(L0 > 0 && L1 > 0 && L2 > 0 && L3 > 0 && Qual > 1 && L0 < 256 && L1 < 256 && L2 < 256 && L3 < 256
                    && Qual < 256)
        : text { L0 == 1 ? nullptr : t0,
            L1 == 1 ? nullptr : t1,
            L2 == 1 ? nullptr : t2,
            L3 == 1 ? nullptr : t3,
            qualification }
        , lengths { L0 - 1, L1 - 1, L2 - 1, L3 - 1, Qual - 1 }
        , single_explanation(L0 == L1 && L0 == L2 && L0 == L3 && identical(t0, t1, t2, t3, L0))
    {}

    constexpr std::string_view tranlate_cc(condition_code cc) const noexcept
    {
        auto cc_val = static_cast<int>(cc);
        return std::string_view(text[cc_val], lengths[cc_val]);
    }

    constexpr std::string_view cc_qualification() const noexcept { return std::string_view(text[4], lengths[4]); }

    constexpr bool has_single_explanation() const noexcept { return single_explanation; }
};

extern constinit const condition_code_explanation condition_code_explanations[];

struct branch_info_argument
{
    signed char op : 4 = 0;
    unsigned char nonzero : 3 = 0;

    bool valid() const noexcept { return op != 0; }
    bool unknown_target() const noexcept { return op < 0; }
    int target() const noexcept { return op - 1; }
    bool branches_if_nonzero() const noexcept { return nonzero != 0; }
    int nonzero_arg() const noexcept { return nonzero - 1; }
};

struct machine_instruction_details
{
    const char* fullname;
    unsigned char fullname_length;
    unsigned char cc_explanation;
    bool privileged : 1;
    bool privileged_conditionally : 1;
    bool has_parameter_list : 1;
    branch_info_argument branch_argument;
};

struct instruction_format_definition
{
    std::span<const checking::machine_operand_format> op_format;

    mach_format format;
};

enum class privilege_status
{
    not_privileged,
    privileged,
    conditionally_privileged,
};

// machine instruction representation for checking
class machine_instruction
{
    enum class size_identifier : unsigned char
    {
        LENGTH_0 = 0,
        LENGTH_16,
        LENGTH_32,
        LENGTH_48,
    };

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
        auto interval = static_cast<int>(instruction_format);
        if (interval >= static_cast<int>(mach_format::length_48))
            return static_cast<char>(size_identifier::LENGTH_48);
        if (interval >= static_cast<int>(mach_format::length_32))
            return static_cast<char>(size_identifier::LENGTH_32);
        if (interval >= static_cast<int>(mach_format::length_16))
            return static_cast<char>(size_identifier::LENGTH_16);
        return static_cast<char>(size_identifier::LENGTH_0);
    }

    inline_string<7> m_name;

    unsigned short m_size_identifier : 2; // Size is only 16,32,48 bits i.e. 0x10,0x20,0x30 (low nibble is always zero)
    unsigned short m_page_no : 14; // PoP has less than 16k pages

    instruction_set_affiliation m_instr_set_affiliation;

    mach_format m_format;
    reladdr_transform_mask m_reladdr_mask;
    unsigned char m_optional_op_count;
    unsigned char m_operand_len;

    const checking::machine_operand_format* m_operands;
    machine_instruction_details m_details;

public:
    constexpr machine_instruction(std::string_view name,
        mach_format format,
        std::span<const checking::machine_operand_format> operands,
        unsigned short page_no,
        instruction_set_affiliation instr_set_affiliation,
        machine_instruction_details d)
        : m_name(name)
        , m_size_identifier(get_length_by_format(format))
        , m_page_no(page_no)
        , m_instr_set_affiliation(instr_set_affiliation)
        , m_format(format)
        , m_reladdr_mask(generate_reladdr_bitmask(operands))
        , m_optional_op_count(
              (unsigned char)std::ranges::count_if(operands, &checking::machine_operand_format::optional))
        , m_operand_len((unsigned char)operands.size())
        , m_operands(operands.data())
        , m_details(d)
    {
        assert(operands.size() <= max_operand_count);
    }
    constexpr machine_instruction(std::string_view name,
        instruction_format_definition ifd,
        unsigned short page_no,
        instruction_set_affiliation instr_set_affiliation,
        machine_instruction_details d)
        : machine_instruction(name, ifd.format, ifd.op_format, page_no, instr_set_affiliation, d)
    {}

    constexpr std::string_view name() const noexcept { return m_name.to_string_view(); }
    mach_format format() const noexcept { return m_format; }
    constexpr size_t size_in_bits() const noexcept
    {
        switch (static_cast<size_identifier>(m_size_identifier))
        {
            case size_identifier::LENGTH_0:
                return 0;
            case size_identifier::LENGTH_16:
                return 16;
            case size_identifier::LENGTH_32:
                return 32;
            default:
                return 48;
        }
    }
    constexpr reladdr_transform_mask reladdr_mask() const noexcept { return m_reladdr_mask; }
    constexpr std::span<const checking::machine_operand_format> operands() const noexcept
    {
        return std::span<const checking::machine_operand_format>(m_operands, m_operand_len);
    }
    constexpr size_t optional_operand_count() const noexcept { return m_optional_op_count; }
    constexpr const instruction_set_affiliation& instr_set_affiliation() const noexcept
    {
        return m_instr_set_affiliation;
    };

    bool check(std::string_view name_of_instruction,
        std::span<const checking::machine_operand* const> operands,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) const; // input vector is the vector of the actual incoming values

    static constexpr size_t max_operand_count = 16;

    constexpr std::string_view fullname() const noexcept
    {
        return std::string_view(m_details.fullname, m_details.fullname_length);
    }

    constexpr const auto& cc_explanation() const noexcept
    {
        return condition_code_explanations[m_details.cc_explanation];
    }

    constexpr size_t page_in_pop() const noexcept { return m_page_no; }

    constexpr bool has_parameter_list() const noexcept { return m_details.has_parameter_list; }
    constexpr privilege_status privileged() const noexcept
    {
        return static_cast<privilege_status>(m_details.privileged + m_details.privileged_conditionally * 2);
    }

    constexpr branch_info_argument branch_argument() const noexcept { return m_details.branch_argument; }
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

enum class mnemonic_transformation_kind : unsigned char
{
    value,
    copy,
    or_with,
    add_to,
    subtract_from,
    complement,
};

struct mnemonic_transformation
{
    unsigned char skip : 4 = 0;
    unsigned char source : 4 = 0;
    mnemonic_transformation_kind type : 4 = mnemonic_transformation_kind::value;
    bool insert : 1 = 1;
    bool reserved : 3 = 0;
    unsigned short value = 0;

    constexpr mnemonic_transformation() = default;
    constexpr mnemonic_transformation(unsigned short v)
        : value(v)
    {}
    constexpr mnemonic_transformation(unsigned char skip, unsigned short v, bool insert = true)
        : skip(skip)
        , insert(insert)
        , value(v)
    {
        assert(skip < machine_instruction::max_operand_count);
    }
    constexpr mnemonic_transformation(unsigned char skip, mnemonic_transformation_kind t, unsigned char src)
        : skip(skip)
        , source(src)
        , type(t)
    {
        assert(t == mnemonic_transformation_kind::copy);
        assert(skip < machine_instruction::max_operand_count);
        assert(src < machine_instruction::max_operand_count);
    }
    constexpr mnemonic_transformation(
        unsigned char skip, unsigned short v, mnemonic_transformation_kind t, unsigned char src, bool insert = true)
        : skip(skip)
        , source(src)
        , type(t)
        , insert(insert)
        , value(v)
    {
        assert(t != mnemonic_transformation_kind::copy && t != mnemonic_transformation_kind::value);
        assert(skip < machine_instruction::max_operand_count);
        assert(src < machine_instruction::max_operand_count);
    }

    constexpr bool has_source() const { return type != mnemonic_transformation_kind::value; }
};

// representation of mnemonic codes for machine instructions
class mnemonic_code
{
    const machine_instruction* m_instruction;

    std::array<mnemonic_transformation, 3> m_transform;
    unsigned char m_transform_count;

    reladdr_transform_mask m_reladdr_mask;

    instruction_set_affiliation m_instr_set_affiliation;

    inline_string<9> m_name;
    unsigned char m_op_min : 4 = 0;
    unsigned char m_op_max : 4 = 0;
    // unsigned char available = 0;

    //  Generates a bitmask for an arbitrary mnemonic indicating which operands
    //  are of the RI type (and therefore are modified by transform_reloc_imm_operands)
    static constexpr unsigned char generate_reladdr_bitmask(
        const machine_instruction* instruction, std::span<const mnemonic_transformation> transforms)
    {
        unsigned char result = 0;

        decltype(result) top_bit = 1 << (std::numeric_limits<decltype(result)>::digits - 1);

        auto transforms_b = transforms.begin();
        auto const transforms_e = transforms.end();

        for (size_t processed = 0; const auto& op : instruction->operands())
        {
            if (transforms_b != transforms_e && processed == transforms_b->skip)
            {
                assert(op.identifier.type == checking::machine_operand_type::IMM
                    || op.identifier.type == checking::machine_operand_type::MASK
                    || op.identifier.type == checking::machine_operand_type::REG
                    || op.identifier.type == checking::machine_operand_type::VEC_REG);
                top_bit >>= +!transforms_b++->insert;
                processed = 0;
                continue;
            }

            if (op.identifier.type == checking::machine_operand_type::RELOC_IMM)
                result |= top_bit;

            top_bit >>= 1;
            ++processed;
        }
        return result;
    }

public:
    constexpr mnemonic_code(std::string_view name,
        const machine_instruction* instr,
        std::initializer_list<const mnemonic_transformation> transform,
        instruction_set_affiliation instr_set_affiliation)
        : m_instruction(instr)
        , m_transform {}
        , m_transform_count((unsigned char)transform.size())
        , m_reladdr_mask(generate_reladdr_bitmask(instr, transform))
        , m_instr_set_affiliation(instr_set_affiliation)
        , m_name(name)
    {
        assert(transform.size() <= m_transform.size());
        std::ranges::copy(transform, m_transform.begin());
        const auto insert_count = std::ranges::count_if(transform, [](auto t) { return t.insert; });
        [[maybe_unused]] const auto total = std::accumulate(
            transform.begin(), transform.end(), (size_t)0, [](size_t res, auto t) { return res + t.skip + t.insert; });
        assert(total <= instr->operands().size());

        m_op_max = instr->operands().size() - insert_count;
        m_op_min = instr->operands().size() - instr->optional_operand_count() - insert_count;
        assert(m_op_max <= instr->operands().size());
        assert(m_op_min <= m_op_max);

        for ([[maybe_unused]] const auto& r : transform)
            assert(!r.has_source() || r.source < m_op_max);
    }

    constexpr const machine_instruction* instruction() const noexcept { return m_instruction; }
    constexpr size_t size_in_bits() const noexcept { return instruction()->size_in_bits(); }
    constexpr std::span<const mnemonic_transformation> operand_transformations() const noexcept
    {
        return { m_transform.data(), m_transform_count };
    }
    constexpr std::pair<size_t, size_t> operand_count() const noexcept { return { m_op_min, m_op_max }; }
    constexpr reladdr_transform_mask reladdr_mask() const noexcept { return m_reladdr_mask; }
    constexpr std::string_view name() const noexcept { return m_name.to_string_view(); }
    constexpr const instruction_set_affiliation& instr_set_affiliation() const noexcept
    {
        return m_instr_set_affiliation;
    };
};

// machine instruction common representation
class assembler_instruction
{
    inline_string<11> m_name;
    bool m_has_ord_symbols : 1, m_postpone_dependencies : 1;
    int m_min_operands;
    int m_max_operands; // -1 in case there is no max value
    std::string_view m_description; // used only for hover and completion

public:
    constexpr assembler_instruction(std::string_view name,
        int min_operands,
        int max_operands,
        bool has_ord_symbols,
        std::string_view description,
        bool postpone_dependencies = false)
        : m_name(name)
        , m_has_ord_symbols(has_ord_symbols)
        , m_postpone_dependencies(postpone_dependencies)
        , m_min_operands(min_operands)
        , m_max_operands(max_operands)
        , m_description(std::move(description)) {};

    constexpr auto name() const { return m_name.to_string_view(); }
    constexpr auto has_ord_symbols() const { return m_has_ord_symbols; }
    constexpr auto postpone_dependencies() const { return m_postpone_dependencies; }
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

    static const ca_instruction& get_ca_instructions(std::string_view name) noexcept;
    static const ca_instruction* find_ca_instructions(std::string_view name) noexcept;
    static std::span<const ca_instruction> all_ca_instructions() noexcept;

    static const assembler_instruction& get_assembler_instructions(std::string_view name) noexcept;
    static const assembler_instruction* find_assembler_instructions(std::string_view name) noexcept;
    static std::span<const assembler_instruction> all_assembler_instructions() noexcept;

    static const machine_instruction& get_machine_instructions(std::string_view name) noexcept;
    static const machine_instruction* find_machine_instructions(std::string_view name) noexcept;
    static std::span<const machine_instruction> all_machine_instructions() noexcept;

    static const mnemonic_code& get_mnemonic_codes(std::string_view name) noexcept;
    static const mnemonic_code* find_mnemonic_codes(std::string_view name) noexcept;
    static std::span<const mnemonic_code> all_mnemonic_codes() noexcept;

    static std::string_view mach_format_to_string(mach_format) noexcept;
};

} // namespace hlasm_plugin::parser_library::context

#endif
