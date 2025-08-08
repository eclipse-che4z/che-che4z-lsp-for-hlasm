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

#ifndef HLASMPLUGIN_PARSERLIBRARY_INSTRUCTIONS_INSTRUCTION_H
#define HLASMPLUGIN_PARSERLIBRARY_INSTRUCTIONS_INSTRUCTION_H

#include <array>
#include <compare>
#include <cstdint>
#include <span>
#include <string_view>

#include "instruction_set_version.h"

namespace hlasm_plugin::parser_library::instructions {

constexpr size_t arch_bitfield_width = 5;

enum class z_arch_affiliation : uint16_t
{
    NO_AFFILIATION = 0,
    ZOP,
    YOP,
    Z9,
    Z10,
    Z11,
    Z12,
    Z13,
    Z14,
    Z15,
    Z16,
    Z17,

    LAST = (1 << arch_bitfield_width) - 1,
};

struct instruction_set_affiliation
{
    z_arch_affiliation z_arch : arch_bitfield_width;
    z_arch_affiliation z_arch_removed : arch_bitfield_width;
    uint16_t esa : 1;
    uint16_t xa : 1;
    uint16_t _370 : 1;
    uint16_t dos : 1;
    uint16_t uni : 1;
};

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
        case instruction_set_version::Z17: {
            const auto from = (uint16_t)instr_set_affiliation.z_arch;
            const auto to = (uint16_t)instr_set_affiliation.z_arch_removed;
            const auto level = (unsigned char)active_instr_set;
            return from <= level && level < to;
        }
    }
    return false;
}

struct instruction_set_size
{
    size_t mnemonic;
    size_t machine;
    size_t ca;
    size_t assembler;

    constexpr size_t total() const noexcept { return mnemonic + machine + ca + assembler; }
};

enum class mach_format : unsigned char;

enum class machine_operand_type : uint8_t
{
    NONE,
    MASK,
    REG,
    IMM,
    DISP,
    DISP_IDX,
    BASE,
    LENGTH,
    VEC_REG,
    IDX_REG,
    RELOC_IMM,
};

enum class even_odd_register : uint8_t
{
    NONE,
    ODD,
    EVEN,
};

// Describes a component of machine operand format. Specifies allowed values.
struct parameter
{
    bool is_signed : 1;
    uint8_t size : 7;
    machine_operand_type type : 4;
    even_odd_register evenodd : 2 = even_odd_register::NONE;
    uint8_t min_register : 2 = 0;

    bool operator==(const parameter&) const = default;

    constexpr bool is_empty() const { return *this == parameter {}; }
};

// Representation of machine operand formats and serves as a template for the checker.
// Consists of 1 parameter when only simple operand is allowed and of 3 parameters when address operand is allowed
// D(F,S)
struct machine_operand_format
{
    parameter identifier; // used as displacement operand in address operand
    parameter first; // empty when simple operand
    parameter second; // empty when simple operand
    bool optional = false;

    consteval machine_operand_format(parameter id, parameter first, parameter second, bool optional = false) noexcept;

    static constinit const machine_operand_format empty;
};

constexpr parameter empty = { false, 0, machine_operand_type::NONE };
constexpr parameter reg = { false, 4, machine_operand_type::REG };
constexpr parameter reg_nz = { false, 4, machine_operand_type::REG, even_odd_register::NONE, 1 };
constexpr parameter reg_2 = { false, 4, machine_operand_type::REG, even_odd_register::NONE, 2 };
constexpr parameter reg_odd = { false, 4, machine_operand_type::REG, even_odd_register::ODD };
constexpr parameter reg_even = { false, 4, machine_operand_type::REG, even_odd_register::EVEN };
constexpr parameter reg_even_nz = { false, 4, machine_operand_type::REG, even_odd_register::EVEN, 2 };
constexpr parameter idx_reg = { false, 4, machine_operand_type::IDX_REG };
constexpr parameter idx_reg_r = { false, 4, machine_operand_type::REG };
constexpr parameter mask = { false, 4, machine_operand_type::MASK };
constexpr parameter dis_12u = { false, 12, machine_operand_type::DISP };
constexpr parameter dis_20s = { true, 20, machine_operand_type::DISP };
constexpr parameter dis_idx_20s = { true, 20, machine_operand_type::DISP_IDX };
constexpr parameter base_ = { false, 4, machine_operand_type::BASE };
constexpr parameter length_8 = { false, 8, machine_operand_type::LENGTH };
constexpr parameter length_4 = { false, 4, machine_operand_type::LENGTH };
constexpr parameter imm_4u = { false, 4, machine_operand_type::IMM };
constexpr parameter imm_8s = { true, 8, machine_operand_type::IMM };
constexpr parameter imm_8u = { false, 8, machine_operand_type::IMM };
constexpr parameter imm_12s = { true, 12, machine_operand_type::IMM };
constexpr parameter imm_12u = { false, 12, machine_operand_type::IMM };
constexpr parameter imm_16s = { true, 16, machine_operand_type::IMM };
constexpr parameter imm_16u = { false, 16, machine_operand_type::IMM };
constexpr parameter imm_24s = { true, 24, machine_operand_type::IMM };
constexpr parameter imm_32s = { true, 32, machine_operand_type::IMM };
constexpr parameter imm_32u = { false, 32, machine_operand_type::IMM };
constexpr parameter vec_reg = { false, 5, machine_operand_type::VEC_REG };
constexpr parameter reladdr_imm_12s = { true, 12, machine_operand_type::RELOC_IMM };
constexpr parameter reladdr_imm_16s = { true, 16, machine_operand_type::RELOC_IMM };
constexpr parameter reladdr_imm_24s = { true, 24, machine_operand_type::RELOC_IMM };
constexpr parameter reladdr_imm_32s = { true, 32, machine_operand_type::RELOC_IMM };

enum class reladdr_transform_mask : unsigned char
{
};

template<unsigned char n>
class inline_string
{
    unsigned char len;
    std::array<char, n> data;

public:
    explicit consteval inline_string(std::string_view s) noexcept;

    constexpr std::string_view to_string_view() const noexcept { return std::string_view(data.data(), len); }

    friend std::strong_ordering operator<=>(const inline_string& l, const inline_string& r) noexcept
    {
        return l.to_string_view() <=> r.to_string_view();
    }

    static constexpr auto max_len = n;
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
    std::array<unsigned short, 5> text;
    std::array<unsigned char, 5> lengths;
    bool single_explanation;

    static constinit const char s_texts[];

public:
    explicit consteval condition_code_explanation(
        const unsigned short* t, const unsigned char* l, bool single) noexcept;

    constexpr std::string_view tranlate_cc(condition_code cc) const noexcept
    {
        auto cc_val = static_cast<int>(cc);
        return std::string_view(s_texts + text[cc_val], lengths[cc_val]);
    }

    constexpr std::string_view cc_qualification() const noexcept
    {
        return std::string_view(s_texts + text[4], lengths[4]);
    }

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
    unsigned short fullname_offset;
    unsigned char fullname_length;
    unsigned char cc_explanation;
    bool privileged : 1;
    bool privileged_conditionally : 1;
    bool has_parameter_list : 1;
    branch_info_argument branch_argument;
};

struct instruction_format_definition
{
    unsigned short op_format_offset;
    unsigned char op_format_len;

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
    friend class mnemonic_code;

    enum class size_identifier : unsigned char
    {
        LENGTH_0 = 0,
        LENGTH_16,
        LENGTH_32,
        LENGTH_48,
    };

    static consteval char get_length_by_format(mach_format instruction_format) noexcept;

    inline_string<7> m_name;

    unsigned short m_size_identifier : 2; // Size is only 16,32,48 bits i.e. 0x10,0x20,0x30 (low nibble is always zero)
    unsigned short m_page_no : 14; // PoP has less than 16k pages

    instruction_set_affiliation m_instr_set_affiliation;

    mach_format m_format;
    reladdr_transform_mask m_reladdr_mask;

    unsigned char m_optional_op_count;
    unsigned char m_operand_len;
    unsigned short m_operands_offset;

    unsigned short m_fullname_offset;
    unsigned char m_fullname_length;

    unsigned char m_cc_explanation;
    bool m_privileged : 1;
    bool m_privileged_conditionally : 1;
    bool m_has_parameter_list : 1;
    branch_info_argument m_branch_argument;

    static constinit const char s_fullnames[];
    static constinit const machine_operand_format s_operands[];

public:
    consteval machine_instruction(std::string_view name,
        mach_format format,
        unsigned short operand_offset,
        unsigned char operand_len,
        unsigned short page_no,
        instruction_set_affiliation instr_set_affiliation,
        machine_instruction_details d) noexcept;
    consteval machine_instruction(std::string_view name,
        instruction_format_definition ifd,
        unsigned short page_no,
        instruction_set_affiliation instr_set_affiliation,
        machine_instruction_details d) noexcept;

    constexpr std::string_view name() const noexcept { return m_name.to_string_view(); }
    constexpr mach_format format() const noexcept { return m_format; }
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
    constexpr std::span<const machine_operand_format> operands() const noexcept
    {
        return std::span<const machine_operand_format>(s_operands + m_operands_offset, m_operand_len);
    }
    constexpr size_t optional_operand_count() const noexcept { return m_optional_op_count; }
    constexpr const instruction_set_affiliation& instr_set_affiliation() const noexcept
    {
        return m_instr_set_affiliation;
    };

    static constexpr size_t max_operand_count = 16;

    constexpr std::string_view fullname() const noexcept
    {
        return std::string_view(s_fullnames + m_fullname_offset, m_fullname_length);
    }

    constexpr const auto& cc_explanation() const noexcept { return condition_code_explanations[m_cc_explanation]; }

    constexpr size_t page_in_pop() const noexcept { return m_page_no; }

    constexpr bool has_parameter_list() const noexcept { return m_has_parameter_list; }
    constexpr privilege_status privileged() const noexcept
    {
        return static_cast<privilege_status>(m_privileged + m_privileged_conditionally * 2);
    }

    constexpr branch_info_argument branch_argument() const noexcept { return m_branch_argument; }

    constexpr std::pair<size_t, size_t> operand_count() const noexcept
    {
        return { m_operand_len - m_optional_op_count, m_operand_len };
    }

    static constexpr auto max_name_len = decltype(m_name)::max_len;
};
extern constinit const machine_instruction g_machine_instructions[];

class ca_instruction
{
    inline_string<6> m_name;
    bool m_operandless;

public:
    consteval ca_instruction(std::string_view n, bool opless) noexcept;

    constexpr auto name() const noexcept { return m_name.to_string_view(); }
    constexpr auto operandless() const noexcept { return m_operandless; }

    static constexpr auto max_name_len = decltype(m_name)::max_len;
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

    consteval mnemonic_transformation() = default;
    consteval mnemonic_transformation(unsigned short v) noexcept;
    consteval mnemonic_transformation(unsigned char skip, unsigned short v, bool insert = true) noexcept;
    consteval mnemonic_transformation(unsigned char skip, mnemonic_transformation_kind t, unsigned char src) noexcept;
    consteval mnemonic_transformation(unsigned char skip,
        unsigned short v,
        mnemonic_transformation_kind t,
        unsigned char src,
        bool insert = true) noexcept;

    constexpr bool has_source() const noexcept { return type != mnemonic_transformation_kind::value; }
};

// representation of mnemonic codes for machine instructions
class mnemonic_code
{
    unsigned short m_instruction;

    std::array<mnemonic_transformation, 3> m_transform;
    unsigned char m_transform_count;

    reladdr_transform_mask m_reladdr_mask;

    instruction_set_affiliation m_instr_set_affiliation;

    inline_string<9> m_name;
    unsigned char m_op_min = 0;
    unsigned char m_op_max = 0;

public:
    consteval mnemonic_code(std::string_view name,
        unsigned short instr,
        instruction_set_affiliation instr_set_affiliation,
        std::initializer_list<const mnemonic_transformation> transform) noexcept;

    constexpr const machine_instruction& instruction() const noexcept { return g_machine_instructions[m_instruction]; }
    constexpr size_t size_in_bits() const noexcept { return instruction().size_in_bits(); }
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

    static constexpr auto max_name_len = decltype(m_name)::max_len;
};

enum class data_def_instruction : unsigned char
{
    NONE,
    DC_TYPE,
    DS_TYPE,
};

class assembler_instruction
{
    inline_string<9> m_name;
    bool m_has_ord_symbols : 1, m_postpone_dependencies : 1;
    data_def_instruction m_data_def : 2;
    signed char m_min_operands;
    signed char m_max_operands; // -1 in case there is no max value
    unsigned char m_desc_len;
    unsigned short m_desc_offset;

    static constinit const char s_descriptions[];

public:
    consteval assembler_instruction(std::string_view name,
        signed char min_operands,
        signed char max_operands,
        bool has_ord_symbols,
        unsigned short desc_off,
        unsigned char desc_len,
        bool postpone_dependencies = false,
        data_def_instruction data_def = data_def_instruction::NONE) noexcept;

    constexpr auto name() const noexcept { return m_name.to_string_view(); }
    constexpr auto has_ord_symbols() const noexcept { return m_has_ord_symbols; }
    constexpr auto postpone_dependencies() const noexcept { return m_postpone_dependencies; }
    constexpr auto data_def_type() const noexcept { return m_data_def; }
    constexpr auto min_operands() const noexcept { return m_min_operands; }
    constexpr auto max_operands() const noexcept { return m_max_operands; }
    constexpr auto description() const noexcept { return std::string_view(s_descriptions + m_desc_offset, m_desc_len); }

    static constexpr auto max_name_len = decltype(m_name)::max_len;
};

const instruction_set_size& get_instruction_sizes(instruction_set_version v) noexcept;
const instruction_set_size& get_instruction_sizes() noexcept;

const ca_instruction& get_ca_instructions(std::string_view name) noexcept;
const ca_instruction* find_ca_instructions(std::string_view name) noexcept;
std::span<const ca_instruction> all_ca_instructions() noexcept;

const assembler_instruction& get_assembler_instructions(std::string_view name) noexcept;
const assembler_instruction* find_assembler_instructions(std::string_view name) noexcept;
std::span<const assembler_instruction> all_assembler_instructions() noexcept;

const machine_instruction& get_machine_instructions(std::string_view name) noexcept;
const machine_instruction* find_machine_instructions(std::string_view name) noexcept;
std::span<const machine_instruction> all_machine_instructions() noexcept;

const mnemonic_code& get_mnemonic_codes(std::string_view name) noexcept;
const mnemonic_code* find_mnemonic_codes(std::string_view name) noexcept;
std::span<const mnemonic_code> all_mnemonic_codes() noexcept;

std::string_view mach_format_to_string(mach_format) noexcept;
} // namespace hlasm_plugin::parser_library::instructions

#endif
