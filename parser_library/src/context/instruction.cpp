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

#include "instruction.h"

#include <algorithm>
#include <limits>

using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library::checking;
using namespace hlasm_plugin::parser_library;

std::string_view instruction::mach_format_to_string(mach_format f)
{
    switch (f)
    {
        case mach_format::E:
            return "E";
        case mach_format::I:
            return "I";
        case mach_format::IE:
            return "IE";
        case mach_format::MII:
            return "MII";
        case mach_format::RI_a:
            return "RI-a";
        case mach_format::RI_b:
            return "RI-b";
        case mach_format::RI_c:
            return "RI-c";
        case mach_format::RIE_a:
            return "RIE-a";
        case mach_format::RIE_b:
            return "RIE-b";
        case mach_format::RIE_c:
            return "RIE-c";
        case mach_format::RIE_d:
            return "RIE-d";
        case mach_format::RIE_e:
            return "RIE-e";
        case mach_format::RIE_f:
            return "RIE-f";
        case mach_format::RIE_g:
            return "RIE-g";
        case mach_format::RIL_a:
            return "RIL-a";
        case mach_format::RIL_b:
            return "RIL-b";
        case mach_format::RIL_c:
            return "RIL-c";
        case mach_format::RIS:
            return "RIS";
        case mach_format::RR:
            return "RR";
        case mach_format::RRD:
            return "RRD";
        case mach_format::RRE:
            return "RRE";
        case mach_format::RRF_a:
            return "RRF-a";
        case mach_format::RRF_b:
            return "RRF-b";
        case mach_format::RRF_c:
            return "RRF-c";
        case mach_format::RRF_d:
            return "RRF-d";
        case mach_format::RRF_e:
            return "RRF-e";
        case mach_format::RRS:
            return "RRS";
        case mach_format::RS_a:
            return "RS-a";
        case mach_format::RS_b:
            return "RS-b";
        case mach_format::RSI:
            return "RSI";
        case mach_format::RSL_a:
            return "RSL-a";
        case mach_format::RSL_b:
            return "RSL-b";
        case mach_format::RSY_a:
            return "RSY-a";
        case mach_format::RSY_b:
            return "RSY-b";
        case mach_format::RX_a:
            return "RX-a";
        case mach_format::RX_b:
            return "RX-b";
        case mach_format::RXE:
            return "RXE";
        case mach_format::RXF:
            return "RXF";
        case mach_format::RXY_a:
            return "RXY-a";
        case mach_format::RXY_b:
            return "RXY-b";
        case mach_format::S:
            return "S";
        case mach_format::SI:
            return "SI";
        case mach_format::SIL:
            return "SIL";
        case mach_format::SIY:
            return "SIY";
        case mach_format::SMI:
            return "SMI";
        case mach_format::SS_a:
            return "SS-a";
        case mach_format::SS_b:
            return "SS-b";
        case mach_format::SS_c:
            return "SS-c";
        case mach_format::SS_d:
            return "SS-d";
        case mach_format::SS_e:
            return "SS-e";
        case mach_format::SS_f:
            return "SS-f";
        case mach_format::SSE:
            return "SSE";
        case mach_format::SSF:
            return "SSF";
        case mach_format::VRI_a:
            return "VRI-a";
        case mach_format::VRI_b:
            return "VRI-b";
        case mach_format::VRI_c:
            return "VRI-c";
        case mach_format::VRI_d:
            return "VRI-d";
        case mach_format::VRI_e:
            return "VRI-e";
        case mach_format::VRI_f:
            return "VRI-f";
        case mach_format::VRI_g:
            return "VRI-g";
        case mach_format::VRI_h:
            return "VRI-h";
        case mach_format::VRI_i:
            return "VRI-i";
        case mach_format::VRR_a:
            return "VRR-a";
        case mach_format::VRR_b:
            return "VRR-b";
        case mach_format::VRR_c:
            return "VRR-c";
        case mach_format::VRR_d:
            return "VRR-d";
        case mach_format::VRR_e:
            return "VRR-e";
        case mach_format::VRR_f:
            return "VRR-f";
        case mach_format::VRR_g:
            return "VRR-g";
        case mach_format::VRR_h:
            return "VRR-h";
        case mach_format::VRR_i:
            return "VRR-i";
        case mach_format::VRS_a:
            return "VRS-a";
        case mach_format::VRS_b:
            return "VRS-b";
        case mach_format::VRS_c:
            return "VRS-c";
        case mach_format::VRS_d:
            return "VRS-d";
        case mach_format::VSI:
            return "VSI";
        case mach_format::VRV:
            return "VRV";
        case mach_format::VRX:
            return "VRX";
        default:
            assert(false);
            return "";
    }
}

constexpr ca_instruction ca_instructions[] = {
    { "ACTR", false },
    { "AEJECT", true },
    { "AGO", false },
    { "AIF", false },
    { "ANOP", true },
    { "AREAD", false },
    { "ASPACE", false },
    { "GBLA", false },
    { "GBLB", false },
    { "GBLC", false },
    { "LCLA", false },
    { "LCLB", false },
    { "LCLC", false },
    { "MACRO", true },
    { "MEND", true },
    { "MEXIT", true },
    { "MHELP", false },
    { "SETA", false },
    { "SETB", false },
    { "SETC", false },
};
#if __cpp_lib_ranges
static_assert(std::ranges::is_sorted(ca_instructions, {}, &ca_instruction::name));

const ca_instruction* instruction::find_ca_instructions(std::string_view name)
{
    auto it = std::ranges::lower_bound(ca_instructions, name, {}, &ca_instruction::name);

    if (it == std::ranges::end(ca_instructions) || it->name() != name)
        return nullptr;
    return &*it;
}
#else
static_assert(std::is_sorted(std::begin(ca_instructions), std::end(ca_instructions), [](const auto& l, const auto& r) {
    return l.name() < r.name();
}));

const ca_instruction* instruction::find_ca_instructions(std::string_view name)
{
    auto it = std::lower_bound(
        std::begin(ca_instructions), std::end(ca_instructions), name, [](const auto& l, const auto& r) {
            return l.name() < r;
        });

    if (it == std::end(ca_instructions) || it->name() != name)
        return nullptr;
    return &*it;
}
#endif

const ca_instruction& instruction::get_ca_instructions(std::string_view name)
{
    auto result = find_ca_instructions(name);
    assert(result);
    return *result;
}

std::span<const ca_instruction> instruction::all_ca_instructions() { return ca_instructions; }

constexpr assembler_instruction assembler_instructions[] = {
    { "*PROCESS", 1, -1, true, "" }, // TO DO
    { "ACONTROL", 1, -1, false, "<selection>+" },
    { "ADATA", 5, 5, false, "value1,value2,value3,value4,character_string" },
    { "AINSERT", 2, 2, false, "'record',BACK|FRONT" },
    { "ALIAS", 1, 1, false, "alias_string" },
    { "AMODE", 1, 1, false, "amode_option" },
    { "CATTR", 1, -1, false, "attribute+" },
    { "CCW", 4, 4, true, "command_code,data_address,flags,data_count" },
    { "CCW0", 4, 4, true, "command_code,data_address,flags,data_count" },
    { "CCW1", 4, 4, true, "command_code,data_address,flags,data_count" },
    { "CEJECT", 0, 1, true, "?number_of_lines" },
    { "CNOP", 2, 2, true, "byte,boundary" },
    { "COM", 0, 0, false, "" },
    { "COPY", 1, 1, false, "member" },
    { "CSECT", 0, 0, false, "" },
    { "CXD", 0, 0, false, "" },
    { "DC", 1, -1, true, "<operand>+" },
    { "DROP", 0, -1, true, "?<<base_register|label>+>", true },
    { "DS", 1, -1, true, "<operand>+" },
    { "DSECT", 0, 0, false, "" },
    { "DXD", 1, -1, true, "<operand>+" },
    { "EJECT", 0, 0, false, "" },
    { "END", 0, 2, true, "?expression,?language" },
    { "ENTRY", 1, -1, true, "entry_point+" },
    { "EQU",
        1,
        5,
        true,
        "value,?<length_attribute_value>,?<type_attribute_value>,?<program_type_value>,?<assembler_type_value>" },
    { "EXITCTL", 2, 5, false, "exit_type,control_value+" },
    { "EXTRN", 1, -1, false, "<external_symbol>+|PART(<external_symbol>+" },
    { "ICTL", 1, 3, false, "begin,?<end>,?<continue>" },
    { "ISEQ", 0, 2, false, "?<left,right>" },
    { "LOCTR", 0, 0, false, "" },
    { "LTORG", 0, 0, false, "" },
    { "MNOTE", 1, 2, false, "?<<severity|*|>,>message" },
    { "OPSYN", 0, 1, false, "?operation_code_2" },
    { "ORG", 0, 3, true, "expression?<,boundary?<,offset>>" },
    { "POP", 1, 4, false, "<PRINT|USING|ACONTROL>+,?NOPRINT" },
    { "PRINT", 1, -1, false, "operand+" },
    { "PUNCH", 1, 1, false, "string" },
    { "PUSH", 1, 4, false, "<PRINT|USING|ACONTROL>+,?NOPRINT" },
    { "REPRO", 0, 0, false, "" },
    { "RMODE", 1, 1, false, "rmode_option" },
    { "RSECT", 0, 0, false, "" },
    { "SPACE", 0, 1, true, "?number_of_lines" },
    { "START", 0, 1, true, "?expression" },
    { "TITLE", 1, 1, false, "title_string" },
    { "USING", 2, 17, true, "operand+", true },
    { "WXTRN", 1, -1, false, "<external_symbol>+|PART(<external_symbol>+" },
    { "XATTR", 1, -1, false, "attribute+" },

};
#ifdef __cpp_lib_ranges
static_assert(std::ranges::is_sorted(assembler_instructions, {}, &assembler_instruction::name));

const assembler_instruction* instruction::find_assembler_instructions(std::string_view instr)
{
    auto it = std::ranges::lower_bound(assembler_instructions, instr, {}, &assembler_instruction::name);
    if (it == std::ranges::end(assembler_instructions) || it->name() != instr)
        return nullptr;
    return &*it;
}
#else
static_assert(std::is_sorted(std::begin(assembler_instructions),
    std::end(assembler_instructions),
    [](const auto& l, const auto& r) { return l.name() < r.name(); }));

const assembler_instruction* instruction::find_assembler_instructions(std::string_view instr)
{
    auto it = std::lower_bound(
        std::begin(assembler_instructions), std::end(assembler_instructions), instr, [](const auto& l, const auto& r) {
            return l.name() < r;
        });
    if (it == std::end(assembler_instructions) || it->name() != instr)
        return nullptr;
    return &*it;
}
#endif

const assembler_instruction& instruction::get_assembler_instructions(std::string_view instr)
{
    auto result = find_assembler_instructions(instr);
    assert(result);
    return *result;
}

std::span<const assembler_instruction> instruction::all_assembler_instructions() { return assembler_instructions; }

bool hlasm_plugin::parser_library::context::machine_instruction::check_nth_operand(
    size_t place, const checking::machine_operand* operand)
{
    diagnostic_op diag;
    const range stmt_range = range();
    if (operand->check(diag, m_operands[place], m_name.to_string_view(), stmt_range))
        return true;
    return false;
}

bool hlasm_plugin::parser_library::context::machine_instruction::check(std::string_view name_of_instruction,
    const std::vector<const checking::machine_operand*> to_check,
    const range& stmt_range,
    const diagnostic_collector& add_diagnostic) const
{
    // check size of operands
    int diff = (int)m_operand_len - (int)to_check.size();
    if (diff > m_optional_op_count || diff < 0)
    {
        add_diagnostic(diagnostic_op::error_optional_number_of_operands(
            name_of_instruction, m_optional_op_count, (int)m_operand_len, stmt_range));
        return false;
    }
    bool error = false;
    for (size_t i = 0; i < to_check.size(); i++)
    {
        assert(to_check[i] != nullptr);
        diagnostic_op diag;
        if (!(*to_check[i]).check(diag, m_operands[i], name_of_instruction, stmt_range))
        {
            add_diagnostic(diag);
            error = true;
        }
    };
    return (!error);
}

template<mach_format F, const machine_operand_format&... Ops>
class instruction_format_definition_factory
{
    static constexpr std::array<machine_operand_format, sizeof...(Ops)> format = { Ops... };

public:
    static constexpr instruction_format_definition def() { return { { format.data(), sizeof...(Ops) }, F }; }
};
template<mach_format F>
class instruction_format_definition_factory<F>
{
public:
    static constexpr instruction_format_definition def() { return { {}, F }; }
};

// clang-format off
constexpr auto E_0 = instruction_format_definition_factory<mach_format::E>::def();
constexpr auto I_1 = instruction_format_definition_factory<mach_format::I, imm_8_U>::def();
constexpr auto IE_2 = instruction_format_definition_factory<mach_format::IE, imm_4_U, imm_4_U>::def();
constexpr auto MII_3 = instruction_format_definition_factory<mach_format::MII, mask_4_U, rel_addr_imm_12_S, rel_addr_imm_24_S>::def();
constexpr auto RI_a_2_s = instruction_format_definition_factory<mach_format::RI_a, reg_4_U, imm_16_S>::def();
constexpr auto RI_a_2_u = instruction_format_definition_factory<mach_format::RI_a, reg_4_U, imm_16_U>::def();
constexpr auto RI_b_2 = instruction_format_definition_factory<mach_format::RI_b, reg_4_U, rel_addr_imm_16_S>::def();
constexpr auto RI_c_2 = instruction_format_definition_factory<mach_format::RI_c, mask_4_U, rel_addr_imm_16_S>::def();
constexpr auto RIE_a_3 = instruction_format_definition_factory<mach_format::RIE_a, reg_4_U, imm_16_S, mask_4_U>::def();
constexpr auto RIE_b_4 = instruction_format_definition_factory<mach_format::RIE_b, reg_4_U, reg_4_U, mask_4_U, rel_addr_imm_16_S>::def();
constexpr auto RIE_c_4 = instruction_format_definition_factory<mach_format::RIE_c, reg_4_U, imm_8_S, mask_4_U, rel_addr_imm_16_S>::def();
constexpr auto RIE_d_3 = instruction_format_definition_factory<mach_format::RIE_d, reg_4_U, reg_4_U, imm_16_S>::def();
constexpr auto RIE_e_3 = instruction_format_definition_factory<mach_format::RIE_e, reg_4_U, reg_4_U, rel_addr_imm_16_S>::def();
constexpr auto RIE_f_5 = instruction_format_definition_factory<mach_format::RIE_f, reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt>::def();
constexpr auto RIE_g_3 = instruction_format_definition_factory<mach_format::RIE_g, reg_4_U, imm_16_S, mask_4_U>::def();
constexpr auto RIL_a_2 = instruction_format_definition_factory<mach_format::RIL_a, reg_4_U, imm_32_S>::def();
constexpr auto RIL_b_2 = instruction_format_definition_factory<mach_format::RIL_b, reg_4_U, rel_addr_imm_32_S>::def();
constexpr auto RIL_c_2 = instruction_format_definition_factory<mach_format::RIL_c, mask_4_U, rel_addr_imm_32_S>::def();
constexpr auto RIS_4 = instruction_format_definition_factory<mach_format::RIS, reg_4_U, imm_8_S, mask_4_U, db_12_4_U>::def();
constexpr auto RR_1 = instruction_format_definition_factory<mach_format::RR, reg_4_U>::def();
constexpr auto RR_2_m = instruction_format_definition_factory<mach_format::RR, mask_4_U, reg_4_U>::def();
constexpr auto RR_2 = instruction_format_definition_factory<mach_format::RR, reg_4_U, reg_4_U>::def();
constexpr auto RRD_3 = instruction_format_definition_factory<mach_format::RRD, reg_4_U, reg_4_U, reg_4_U>::def();
constexpr auto RRE_0 = instruction_format_definition_factory<mach_format::RRE>::def();
constexpr auto RRE_1 = instruction_format_definition_factory<mach_format::RRE, reg_4_U>::def();
constexpr auto RRE_2 = instruction_format_definition_factory<mach_format::RRE, reg_4_U, reg_4_U>::def();
constexpr auto RRF_a_3 = instruction_format_definition_factory<mach_format::RRF_a, reg_4_U, reg_4_U, reg_4_U>::def();
constexpr auto RRF_a_4 = instruction_format_definition_factory<mach_format::RRF_a, reg_4_U, reg_4_U, reg_4_U, mask_4_U>::def();
constexpr auto RRF_a_4_opt = instruction_format_definition_factory<mach_format::RRF_a, reg_4_U, reg_4_U, reg_4_U_opt, mask_4_U_opt>::def();
constexpr auto RRF_b_3 = instruction_format_definition_factory<mach_format::RRF_b, reg_4_U, reg_4_U, reg_4_U>::def();
constexpr auto RRF_b_4 = instruction_format_definition_factory<mach_format::RRF_b, reg_4_U, reg_4_U, reg_4_U, mask_4_U>::def();
constexpr auto RRF_b_4_opt = instruction_format_definition_factory<mach_format::RRF_b, reg_4_U, reg_4_U, reg_4_U, mask_4_U_opt>::def();
constexpr auto RRF_c_3 = instruction_format_definition_factory<mach_format::RRF_c, reg_4_U, reg_4_U, mask_4_U>::def();
constexpr auto RRF_c_3_opt = instruction_format_definition_factory<mach_format::RRF_c, reg_4_U, reg_4_U, mask_4_U_opt>::def();
constexpr auto RRF_d_3 = instruction_format_definition_factory<mach_format::RRF_d, reg_4_U, reg_4_U, mask_4_U>::def();
constexpr auto RRF_e_3 = instruction_format_definition_factory<mach_format::RRF_e, reg_4_U, mask_4_U, reg_4_U>::def();
constexpr auto RRF_e_4 = instruction_format_definition_factory<mach_format::RRF_e, reg_4_U, mask_4_U, reg_4_U, mask_4_U>::def();
constexpr auto RRS_4 = instruction_format_definition_factory<mach_format::RRS, reg_4_U, reg_4_U, mask_4_U, db_12_4_U>::def();
constexpr auto RS_a_2 = instruction_format_definition_factory<mach_format::RS_a, reg_4_U, db_12_4_U>::def();
constexpr auto RS_a_3 = instruction_format_definition_factory<mach_format::RS_a, reg_4_U, reg_4_U, db_12_4_U>::def();
constexpr auto RS_b_3 = instruction_format_definition_factory<mach_format::RS_b, reg_4_U, mask_4_U, db_12_4_U>::def();
constexpr auto RSI_3 = instruction_format_definition_factory<mach_format::RSI, reg_4_U, reg_4_U, rel_addr_imm_16_S>::def();
constexpr auto RSL_a_1 = instruction_format_definition_factory<mach_format::RSL_a, db_12_4x4L_U>::def();
constexpr auto RSL_b_3 = instruction_format_definition_factory<mach_format::RSL_b, reg_4_U, db_12_8x4L_U, mask_4_U>::def();
constexpr auto RSY_a_3 = instruction_format_definition_factory<mach_format::RSY_a, reg_4_U, reg_4_U, db_20_4_S>::def();
constexpr auto RSY_b_3_su = instruction_format_definition_factory<mach_format::RSY_b, reg_4_U, db_20_4_S, mask_4_U>::def();
constexpr auto RSY_b_3_us = instruction_format_definition_factory<mach_format::RSY_b, reg_4_U, mask_4_U, db_20_4_S>::def();
constexpr auto RSY_b_3_ux = instruction_format_definition_factory<mach_format::RSY_b, reg_4_U, mask_4_U, dxb_20_4x4_S>::def();
constexpr auto RX_a_2_ux = instruction_format_definition_factory<mach_format::RX_a, reg_4_U, dxb_12_4x4_U>::def();
constexpr auto RX_a_2 = instruction_format_definition_factory<mach_format::RX_a, reg_4_U, reg_4_U>::def();
constexpr auto RX_b_2 = instruction_format_definition_factory<mach_format::RX_b, mask_4_U, dxb_12_4x4_U>::def();
constexpr auto RXE_2 = instruction_format_definition_factory<mach_format::RXE, reg_4_U, dxb_12_4x4_U>::def();
constexpr auto RXE_3_xm = instruction_format_definition_factory<mach_format::RXE, reg_4_U, dxb_12_4x4_U, mask_4_U>::def();
constexpr auto RXF_3_x = instruction_format_definition_factory<mach_format::RXF, reg_4_U, reg_4_U, dxb_12_4x4_U>::def();
constexpr auto RXY_a_2 = instruction_format_definition_factory<mach_format::RXY_a, reg_4_U, dxb_20_4x4_S>::def();
constexpr auto RXY_b_2 = instruction_format_definition_factory<mach_format::RXY_b, mask_4_U, dxb_20_4x4_S>::def();
constexpr auto S_0 = instruction_format_definition_factory<mach_format::S>::def();
constexpr auto S_1_u = instruction_format_definition_factory<mach_format::S, db_12_4_U>::def();
constexpr auto S_1_s = instruction_format_definition_factory<mach_format::S, db_20_4_S>::def();
constexpr auto SI_1 = instruction_format_definition_factory<mach_format::SI, db_12_4_U>::def();
constexpr auto SI_2_s = instruction_format_definition_factory<mach_format::SI, db_12_4_U, imm_8_S>::def();
constexpr auto SI_2_u = instruction_format_definition_factory<mach_format::SI, db_12_4_U, imm_8_U>::def();
constexpr auto SIL_2_s = instruction_format_definition_factory<mach_format::SIL, db_12_4_U, imm_16_S>::def();
constexpr auto SIL_2_u = instruction_format_definition_factory<mach_format::SIL, db_12_4_U, imm_16_U>::def();
constexpr auto SIY_2_uu = instruction_format_definition_factory<mach_format::SIY, db_12_4_U, imm_8_U>::def();
constexpr auto SIY_2_ss = instruction_format_definition_factory<mach_format::SIY, db_20_4_S, imm_8_S>::def();
constexpr auto SIY_2_su = instruction_format_definition_factory<mach_format::SIY, db_20_4_S, imm_8_U>::def();
constexpr auto SMI_3 = instruction_format_definition_factory<mach_format::SMI, mask_4_U, rel_addr_imm_16_S, db_12_4_U>::def();
constexpr auto SS_a_2_u = instruction_format_definition_factory<mach_format::SS_a, db_12_8x4L_U, db_12_4_U>::def();
constexpr auto SS_a_2_s = instruction_format_definition_factory<mach_format::SS_a, db_12_8x4L_U, db_20_4_S>::def();
constexpr auto SS_b_2 = instruction_format_definition_factory<mach_format::SS_b, db_12_4x4L_U, db_12_4x4L_U>::def();
constexpr auto SS_c_3 = instruction_format_definition_factory<mach_format::SS_c, db_12_4x4L_U, db_12_4_U, imm_4_U>::def();
constexpr auto SS_d_3 = instruction_format_definition_factory<mach_format::SS_d, drb_12_4x4_U, db_12_4_U, reg_4_U>::def();
constexpr auto SS_e_4_br = instruction_format_definition_factory<mach_format::SS_e, reg_4_U, db_12_4_U, reg_4_U, db_12_4_U>::def();
constexpr auto SS_e_4_rb = instruction_format_definition_factory<mach_format::SS_e, reg_4_U, reg_4_U, db_12_4_U, db_12_4_U>::def();
constexpr auto SS_f_2 = instruction_format_definition_factory<mach_format::SS_f, db_12_4_U, db_12_8x4L_U>::def();
constexpr auto SSE_2 = instruction_format_definition_factory<mach_format::SSE, db_12_4_U, db_12_4_U>::def();
constexpr auto SSF_3_dr = instruction_format_definition_factory<mach_format::SSF, db_12_4_U, db_12_4_U, reg_4_U>::def();
constexpr auto SSF_3_rd = instruction_format_definition_factory<mach_format::SSF, reg_4_U, db_12_4_U, db_12_4_U>::def();
constexpr auto VRI_a_2 = instruction_format_definition_factory<mach_format::VRI_a, vec_reg_5_U, imm_16_U>::def();
constexpr auto VRI_a_3 = instruction_format_definition_factory<mach_format::VRI_a, vec_reg_5_U, imm_16_S, mask_4_U>::def();
constexpr auto VRI_b_4 = instruction_format_definition_factory<mach_format::VRI_b, vec_reg_5_U, imm_8_U, imm_8_U, mask_4_U>::def();
constexpr auto VRI_c_4 = instruction_format_definition_factory<mach_format::VRI_c, vec_reg_5_U, vec_reg_5_U, imm_16_U, mask_4_U>::def();
constexpr auto VRI_d_4 = instruction_format_definition_factory<mach_format::VRI_d, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U>::def();
constexpr auto VRI_d_5 = instruction_format_definition_factory<mach_format::VRI_d, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U>::def();
constexpr auto VRI_e_5 = instruction_format_definition_factory<mach_format::VRI_e, vec_reg_5_U, vec_reg_5_U, imm_12_S, mask_4_U, mask_4_U>::def();
constexpr auto VRI_f_5 = instruction_format_definition_factory<mach_format::VRI_f, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U>::def();
constexpr auto VRI_g_5_s = instruction_format_definition_factory<mach_format::VRI_g, vec_reg_5_U, vec_reg_5_U, imm_8_U, imm_8_S, mask_4_U>::def();
constexpr auto VRI_g_5_u = instruction_format_definition_factory<mach_format::VRI_g, vec_reg_5_U, vec_reg_5_U, imm_8_U, imm_8_U, mask_4_U>::def();
constexpr auto VRI_h_3 = instruction_format_definition_factory<mach_format::VRI_h, vec_reg_5_U, imm_16_S, imm_4_U>::def();
constexpr auto VRI_i_4 = instruction_format_definition_factory<mach_format::VRI_i, vec_reg_5_U, reg_4_U, imm_8_S, mask_4_U>::def();
constexpr auto VRR_a_2 = instruction_format_definition_factory<mach_format::VRR_a, vec_reg_5_U, vec_reg_5_U>::def();
constexpr auto VRR_a_3 = instruction_format_definition_factory<mach_format::VRR_a, vec_reg_5_U, vec_reg_5_U, mask_4_U>::def();
constexpr auto VRR_a_4 = instruction_format_definition_factory<mach_format::VRR_a, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U>::def();
constexpr auto VRR_a_4_opt = instruction_format_definition_factory<mach_format::VRR_a, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U_opt>::def();
constexpr auto VRR_a_5 = instruction_format_definition_factory<mach_format::VRR_a, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U>::def();
constexpr auto VRR_b_5 = instruction_format_definition_factory<mach_format::VRR_b, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U>::def();
constexpr auto VRR_b_5_opt = instruction_format_definition_factory<mach_format::VRR_b, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U_opt>::def();
constexpr auto VRR_c_3 = instruction_format_definition_factory<mach_format::VRR_c, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U>::def();
constexpr auto VRR_c_4 = instruction_format_definition_factory<mach_format::VRR_c, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U>::def();
constexpr auto VRR_c_5 = instruction_format_definition_factory<mach_format::VRR_c, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U>::def();
constexpr auto VRR_c_6 = instruction_format_definition_factory<mach_format::VRR_c, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U>::def();
constexpr auto VRR_d_5 = instruction_format_definition_factory<mach_format::VRR_d, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U>::def();
constexpr auto VRR_d_6 = instruction_format_definition_factory<mach_format::VRR_d, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U>::def();
constexpr auto VRR_d_6_opt = instruction_format_definition_factory<mach_format::VRR_d, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U_opt>::def();
constexpr auto VRR_e_4 = instruction_format_definition_factory<mach_format::VRR_e, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U>::def();
constexpr auto VRR_e_6 = instruction_format_definition_factory<mach_format::VRR_e, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U>::def();
constexpr auto VRR_f_3 = instruction_format_definition_factory<mach_format::VRR_f, vec_reg_5_U, reg_4_U, reg_4_U>::def();
constexpr auto VRR_g_1 = instruction_format_definition_factory<mach_format::VRR_g, vec_reg_5_U>::def();
constexpr auto VRR_h_3 = instruction_format_definition_factory<mach_format::VRR_h, vec_reg_5_U, vec_reg_5_U, mask_4_U>::def();
constexpr auto VRR_i_3 = instruction_format_definition_factory<mach_format::VRR_i, reg_4_U, vec_reg_5_U, mask_4_U>::def();
constexpr auto VRS_a_4 = instruction_format_definition_factory<mach_format::VRS_a, vec_reg_5_U, vec_reg_5_U, db_12_4_U, mask_4_U>::def();
constexpr auto VRS_a_4_opt = instruction_format_definition_factory<mach_format::VRS_a, vec_reg_5_U, vec_reg_5_U, db_12_4_U, mask_4_U_opt>::def();
constexpr auto VRS_b_3 = instruction_format_definition_factory<mach_format::VRS_b, vec_reg_5_U, reg_4_U, db_12_4_U>::def();
constexpr auto VRS_b_4 = instruction_format_definition_factory<mach_format::VRS_b, vec_reg_5_U, reg_4_U, db_12_4_U, mask_4_U>::def();
constexpr auto VRS_c_4 = instruction_format_definition_factory<mach_format::VRS_c, reg_4_U, vec_reg_5_U, db_12_4_U, mask_4_U>::def();
constexpr auto VRS_d_3 = instruction_format_definition_factory<mach_format::VRS_d, vec_reg_5_U, reg_4_U, db_12_4_U>::def();
constexpr auto VRV_3 = instruction_format_definition_factory<mach_format::VRV, vec_reg_5_U, dvb_12_5x4_U, mask_4_U>::def();
constexpr auto VRX_3 = instruction_format_definition_factory<mach_format::VRX, vec_reg_5_U, dxb_12_4x4_U, mask_4_U>::def();
constexpr auto VRX_3_opt = instruction_format_definition_factory<mach_format::VRX, vec_reg_5_U, dxb_12_4x4_U, mask_4_U_opt>::def();
constexpr auto VSI_3 = instruction_format_definition_factory<mach_format::VSI, vec_reg_5_U, db_12_4_U, imm_8_U>::def();
// clang-format on


constexpr machine_instruction machine_instructions[] = {
    { "A", RX_a_2_ux, 510 },
    { "AD", RX_a_2_ux, 1412 },
    { "ADB", RXE_2, 1445 },
    { "ADBR", RRE_2, 1445 },
    { "ADDFRR", RRE_2, 7 },
    { "ADR", RR_2, 1412 },
    { "ADTR", RRF_a_3, 1491 },
    { "ADTRA", RRF_a_4, 1491 },
    { "AE", RX_a_2_ux, 1412 },
    { "AEB", RXE_2, 1445 },
    { "AEBR", RRE_2, 1445 },
    { "AER", RR_2, 1412 },
    { "AFI", RIL_a_2, 511 },
    { "AG", RXY_a_2, 511 },
    { "AGF", RXY_a_2, 511 },
    { "AGFI", RIL_a_2, 511 },
    { "AGFR", RRE_2, 510 },
    { "AGH", RXY_a_2, 512 },
    { "AGHI", RI_a_2_s, 513 },
    { "AGHIK", RIE_d_3, 511 },
    { "AGR", RRE_2, 510 },
    { "AGRK", RRF_a_3, 510 },
    { "AGSI", SIY_2_ss, 511 },
    { "AH", RX_a_2_ux, 512 },
    { "AHHHR", RRF_a_3, 513 },
    { "AHHLR", RRF_a_3, 513 },
    { "AHI", RI_a_2_s, 512 },
    { "AHIK", RIE_d_3, 511 },
    { "AHY", RXY_a_2, 512 },
    { "AIH", RIL_a_2, 513 },
    { "AL", RX_a_2_ux, 514 },
    { "ALC", RXY_a_2, 515 },
    { "ALCG", RXY_a_2, 515 },
    { "ALCGR", RRE_2, 515 },
    { "ALCR", RRE_2, 515 },
    { "ALFI", RIL_a_2, 514 },
    { "ALG", RXY_a_2, 514 },
    { "ALGF", RXY_a_2, 514 },
    { "ALGFI", RIL_a_2, 514 },
    { "ALGFR", RRE_2, 514 },
    { "ALGHSIK", RIE_d_3, 516 },
    { "ALGR", RRE_2, 514 },
    { "ALGRK", RRF_a_3, 514 },
    { "ALGSI", SIY_2_ss, 516 },
    { "ALHHHR", RRF_a_3, 515 },
    { "ALHHLR", RRF_a_3, 515 },
    { "ALHSIK", RIE_d_3, 516 },
    { "ALR", RR_2, 514 },
    { "ALRK", RRF_a_3, 514 },
    { "ALSI", SIY_2_ss, 516 },
    { "ALSIH", RIL_a_2, 517 },
    { "ALSIHN", RIL_a_2, 517 },
    { "ALY", RXY_a_2, 514 },
    { "AP", SS_b_2, 920 },
    { "AR", RR_2, 510 },
    { "ARK", RRF_a_3, 510 },
    { "ASI", SIY_2_ss, 511 },
    { "AU", RX_a_2_ux, 1413 },
    { "AUR", RR_2, 1413 },
    { "AW", RX_a_2_ux, 1413 },
    { "AWR", RR_2, 1413 },
    { "AXBR", RRE_2, 1445 },
    { "AXR", RR_2, 1412 },
    { "AXTR", RRF_a_3, 1491 },
    { "AXTRA", RRF_a_4, 1491 },
    { "AY", RXY_a_2, 511 },
    { "BAKR", RRE_2, 993 },
    { "BAL", RX_a_2_ux, 519 },
    { "BALR", RR_2, 519 },
    { "BAS", RX_a_2_ux, 520 },
    { "BASR", RR_2, 520 },
    { "BASSM", RX_a_2, 520 },
    { "BC", RX_b_2, 524 },
    { "BCR", RR_2_m, 524 },
    { "BCT", RX_a_2_ux, 525 },
    { "BCTG", RXY_a_2, 525 },
    { "BCTGR", RRE_2, 525 },
    { "BCTR", RR_2, 525 },
    { "BIC", RXY_b_2, 523 },
    { "BPP", SMI_3, 527 },
    { "BPRP", MII_3, 527 },
    { "BRAS", RI_b_2, 530 },
    { "BRASL", RIL_b_2, 530 },
    { "BRC", RI_c_2, 530 },
    { "BRCL", RIL_c_2, 530 },
    { "BRCT", RI_b_2, 531 },
    { "BRCTG", RI_b_2, 531 },
    { "BRCTH", RIL_b_2, 531 },
    { "BRXH", RSI_3, 532 },
    { "BRXHG", RIE_e_3, 532 },
    { "BRXLE", RSI_3, 532 },
    { "BRXLG", RIE_e_3, 532 },
    { "BSA", RRE_2, 989 },
    { "BSG", RRE_2, 995 },
    { "BSM", RR_2, 522 },
    { "BXH", RS_a_3, 526 },
    { "BXHG", RSY_a_3, 526 },
    { "BXLE", RS_a_3, 526 },
    { "BXLEG", RSY_a_3, 526 },
    { "C", RX_a_2_ux, 618 },
    { "CD", RX_a_2_ux, 1414 },
    { "CDB", RXE_2, 1447 },
    { "CDBR", RRE_2, 1447 },
    { "CDFBR", RRE_2, 1449 },
    { "CDFBRA", RRF_e_4, 1449 },
    { "CDFR", RRE_2, 1415 },
    { "CDFTR", RRF_e_4, 1496 },
    { "CDGBR", RRE_2, 1449 },
    { "CDGBRA", RRF_e_4, 1449 },
    { "CDGR", RRE_2, 1415 },
    { "CDGTR", RRE_2, 1496 },
    { "CDGTRA", RRF_e_4, 1496 },
    { "CDLFBR", RRF_e_4, 1451 },
    { "CDLFTR", RRF_e_4, 1497 },
    { "CDLGBR", RRF_e_4, 1451 },
    { "CDLGTR", RRF_e_4, 1497 },
    { "CDPT", RSL_b_3, 1498 },
    { "CDR", RR_2, 1414 },
    { "CDS", RS_a_3, 628 },
    { "CDSG", RSY_a_3, 628 },
    { "CDSTR", RRE_2, 1500 },
    { "CDSY", RSY_a_3, 628 },
    { "CDTR", RRE_2, 1494 },
    { "CDUTR", RRE_2, 1500 },
    { "CDZT", RSL_b_3, 1501 },
    { "CE", RX_a_2_ux, 1414 },
    { "CEB", RXE_2, 1447 },
    { "CEBR", RRE_2, 1447 },
    { "CEDTR", RRE_2, 1495 },
    { "CEFBR", RRE_2, 1449 },
    { "CEFBRA", RRF_e_4, 1449 },
    { "CEFR", RRE_2, 1415 },
    { "CEGBR", RRE_2, 1449 },
    { "CEGBRA", RRF_e_4, 1449 },
    { "CEGR", RRE_2, 1415 },
    { "CELFBR", RRF_e_4, 1451 },
    { "CELGBR", RRF_e_4, 1451 },
    { "CER", RR_2, 1414 },
    { "CEXTR", RRE_2, 1495 },
    { "CFC", S_1_u, 621 },
    { "CFDBR", RRF_e_3, 1452 },
    { "CFDBRA", RRF_e_4, 1452 },
    { "CFDR", RRF_e_3, 1415 },
    { "CFDTR", RRF_e_4, 1502 },
    { "CFEBR", RRF_e_3, 1452 },
    { "CFEBRA", RRF_e_4, 1452 },
    { "CFER", RRF_e_3, 1415 },
    { "CFI", RIL_a_2, 618 },
    { "CFXBR", RRF_e_3, 1452 },
    { "CFXBRA", RRF_e_4, 1452 },
    { "CFXR", RRF_e_3, 1415 },
    { "CFXTR", RRF_e_4, 1502 },
    { "CG", RXY_a_2, 618 },
    { "CGDBR", RRF_e_3, 1452 },
    { "CGDBRA", RRF_e_4, 1452 },
    { "CGDR", RRF_e_3, 1415 },
    { "CGDTR", RRF_e_3, 1501 },
    { "CGDTRA", RRF_e_4, 1502 },
    { "CGEBR", RRF_e_3, 1452 },
    { "CGEBRA", RRF_e_4, 1452 },
    { "CGER", RRF_e_3, 1415 },
    { "CGF", RXY_a_2, 618 },
    { "CGFI", RIL_a_2, 619 },
    { "CGFR", RRE_2, 618 },
    { "CGFRL", RIL_b_2, 619 },
    { "CGH", RXY_a_2, 634 },
    { "CGHI", RI_a_2_s, 634 },
    { "CGHRL", RIL_b_2, 634 },
    { "CGHSI", SIL_2_s, 634 },
    { "CGIB", RIS_4, 620 },
    { "CGIJ", RIE_c_4, 620 },
    { "CGIT", RIE_a_3, 633 },
    { "CGR", RRE_2, 618 },
    { "CGRB", RRS_4, 619 },
    { "CGRJ", RIE_b_4, 620 },
    { "CGRL", RIL_b_2, 619 },
    { "CGRT", RRF_c_3, 633 },
    { "CGXBR", RRF_e_3, 1452 },
    { "CGXBRA", RRF_e_4, 1452 },
    { "CGXR", RRF_e_3, 1415 },
    { "CGXTR", RRF_e_3, 1501 },
    { "CGXTRA", RRF_e_4, 1502 },
    { "CH", RX_a_2_ux, 634 },
    { "CHF", RXY_a_2, 635 },
    { "CHHR", RRE_2, 635 },
    { "CHHSI", SIL_2_s, 634 },
    { "CHI", RI_a_2_s, 634 },
    { "CHLR", RRE_2, 635 },
    { "CHRL", RIL_b_2, 634 },
    { "CHSI", SIL_2_s, 634 },
    { "CHY", RXY_a_2, 634 },
    { "CIB", RIS_4, 620 },
    { "CIH", RIL_a_2, 635 },
    { "CIJ", RIE_c_4, 620 },
    { "CIT", RIE_a_3, 633 },
    { "CKSM", RRE_2, 533 },
    { "CL", RX_a_2_ux, 636 },
    { "CLC", SS_a_2_u, 636 },
    { "CLCL", RR_2, 642 },
    { "CLCLE", RS_a_3, 644 },
    { "CLCLU", RSY_a_3, 647 },
    { "CLFDBR", RRF_e_4, 1455 },
    { "CLFDTR", RRF_e_4, 1504 },
    { "CLFEBR", RRF_e_4, 1455 },
    { "CLFHSI", SIL_2_u, 636 },
    { "CLFI", RIL_a_2, 636 },
    { "CLFIT", RIE_a_3, 640 },
    { "CLFXBR", RRF_e_4, 1455 },
    { "CLFXTR", RRF_e_4, 1504 },
    { "CLG", RXY_a_2, 636 },
    { "CLGDBR", RRF_e_4, 1455 },
    { "CLGDTR", RRF_e_4, 1504 },
    { "CLGEBR", RRF_e_4, 1455 },
    { "CLGF", RXY_a_2, 636 },
    { "CLGFI", RIL_a_2, 636 },
    { "CLGFR", RRE_2, 636 },
    { "CLGFRL", RIL_b_2, 637 },
    { "CLGHRL", RIL_b_2, 637 },
    { "CLGHSI", SIL_2_u, 636 },
    { "CLGIB", RIS_4, 638 },
    { "CLGIJ", RIE_c_4, 638 },
    { "CLGIT", RIE_a_3, 640 },
    { "CLGR", RRE_2, 636 },
    { "CLGRB", RRS_4, 638 },
    { "CLGRJ", RIE_b_4, 638 },
    { "CLGRL", RIL_b_2, 637 },
    { "CLGRT", RRF_c_3, 639 },
    { "CLGT", RSY_b_3_ux, 639 },
    { "CLGXBR", RRF_e_4, 1455 },
    { "CLGXTR", RRF_e_4, 1504 },
    { "CLHF", RXY_a_2, 641 },
    { "CLHHR", RRE_2, 641 },
    { "CLHHSI", SIL_2_u, 636 },
    { "CLHLR", RRE_2, 641 },
    { "CLHRL", RIL_b_2, 637 },
    { "CLI", SI_2_u, 636 },
    { "CLIB", RIS_4, 638 },
    { "CLIH", RIL_a_2, 642 },
    { "CLIJ", RIE_c_4, 638 },
    { "CLIY", SIY_2_uu, 636 },
    { "CLM", RS_b_3, 641 },
    { "CLMH", RSY_b_3_us, 641 },
    { "CLMY", RSY_b_3_us, 641 },
    { "CLR", RR_2, 636 },
    { "CLRB", RRS_4, 638 },
    { "CLRCH", S_1_u, 367 },
    { "CLRIO", S_1_u, 368 },
    { "CLRJ", RIE_b_4, 638 },
    { "CLRL", RIL_b_2, 637 },
    { "CLRT", RRF_c_3, 639 },
    { "CLST", RRE_2, 650 },
    { "CLT", RSY_b_3_ux, 639 },
    { "CLY", RXY_a_2, 636 },
    { "CMPSC", RRE_2, 654 },
    { "CONCS", S_1_u, 263 },
    { "CP", SS_b_2, 921 },
    { "CPDT", RSL_b_3, 1505 },
    { "CPSDR", RRF_b_3, 958 },
    { "CPXT", RSL_b_3, 1505 },
    { "CPYA", RRE_2, 736 },
    { "CR", RR_2, 618 },
    { "CRB", RRS_4, 619 },
    { "CRDTE", RRF_b_4_opt, 999 },
    { "CRJ", RIE_b_4, 619 },
    { "CRL", RIL_b_2, 619 },
    { "CRT", RRF_c_3, 633 },
    { "CS", RS_a_3, 628 },
    { "CSCH", S_0, 1217 },
    { "CSDTR", RRF_d_3, 1507 },
    { "CSG", RSY_a_3, 628 },
    { "CSP", RRE_2, 1003 },
    { "CSPG", RRE_2, 1003 },
    { "CSST", SSF_3_dr, 630 },
    { "CSXTR", RRF_d_3, 1507 },
    { "CSY", RSY_a_3, 628 },
    { "CU12", RRF_c_3_opt, 728 },
    { "CU14", RRF_c_3_opt, 732 },
    { "CU21", RRF_c_3_opt, 718 },
    { "CU24", RRF_c_3_opt, 715 },
    { "CU41", RRE_2, 725 },
    { "CU42", RRE_2, 722 },
    { "CUDTR", RRE_2, 1507 },
    { "CUSE", RRE_2, 651 },
    { "CUTFU", RRF_c_3_opt, 728 },
    { "CUUTF", RRF_c_3_opt, 718 },
    { "CUXTR", RRE_2, 1507 },
    { "CVB", RX_a_2_ux, 714 },
    { "CVBG", RXY_a_2, 714 },
    { "CVBY", RXY_a_2, 714 },
    { "CVD", RX_a_2_ux, 715 },
    { "CVDG", RXY_a_2, 715 },
    { "CVDY", RXY_a_2, 715 },
    { "CXBR", RRE_2, 1447 },
    { "CXFBR", RRE_2, 1449 },
    { "CXFBRA", RRF_e_4, 1449 },
    { "CXFR", RRE_2, 1415 },
    { "CXFTR", RRF_e_4, 1496 },
    { "CXGBR", RRE_2, 1449 },
    { "CXGBRA", RRF_e_4, 1449 },
    { "CXGR", RRE_2, 1415 },
    { "CXGTR", RRE_2, 1496 },
    { "CXGTRA", RRF_e_4, 1496 },
    { "CXLFBR", RRF_e_4, 1451 },
    { "CXLFTR", RRF_e_4, 1497 },
    { "CXLGBR", RRF_e_4, 1451 },
    { "CXLGTR", RRF_e_4, 1497 },
    { "CXPT", RSL_b_3, 1498 },
    { "CXR", RRE_2, 1414 },
    { "CXSTR", RRE_2, 1500 },
    { "CXTR", RRE_2, 1494 },
    { "CXUTR", RRE_2, 1500 },
    { "CXZT", RSL_b_3, 1501 },
    { "CY", RXY_a_2, 618 },
    { "CZDT", RSL_b_3, 1508 },
    { "CZXT", RSL_b_3, 1508 },
    { "D", RX_a_2_ux, 736 },
    { "DD", RX_a_2_ux, 1416 },
    { "DDB", RXE_2, 1457 },
    { "DDBR", RRE_2, 1457 },
    { "DDR", RR_2, 1416 },
    { "DDTR", RRF_a_3, 1509 },
    { "DDTRA", RRF_a_4, 1509 },
    { "DE", RX_a_2_ux, 1416 },
    { "DEB", RXE_2, 1457 },
    { "DEBR", RRE_2, 1457 },
    { "DER", RR_2, 1416 },
    { "DFLTCC", RRF_a_3, 1714 },
    { "DIDBR", RRF_b_4, 1458 },
    { "DIEBR", RRF_b_4, 1458 },
    { "DISCS", S_1_u, 265 },
    { "DL", RXY_a_2, 737 },
    { "DLG", RXY_a_2, 737 },
    { "DLGR", RRE_2, 737 },
    { "DLR", RRE_2, 737 },
    { "DP", SS_b_2, 921 },
    { "DR", RR_2, 736 },
    { "DSG", RXY_a_2, 738 },
    { "DSGF", RXY_a_2, 738 },
    { "DSGFR", RRE_2, 738 },
    { "DSGR", RRE_2, 738 },
    { "DXBR", RRE_2, 1457 },
    { "DXR", RRE_2, 1416 },
    { "DXTR", RRF_a_3, 1509 },
    { "DXTRA", RRF_a_4, 1509 },
    { "EAR", RRE_2, 741 },
    { "ECAG", RSY_a_3, 741 },
    { "ECCTR", RRE_2, 39 },
    { "ECPGA", RRE_2, 39 },
    { "ECTG", SSF_3_dr, 744 },
    { "ED", SS_a_2_u, 922 },
    { "EDMK", SS_a_2_u, 925 },
    { "EEDTR", RRE_2, 1511 },
    { "EEXTR", RRE_2, 1511 },
    { "EFPC", RRE_1, 958 },
    { "EPAIR", RRE_1, 1006 },
    { "EPAR", RRE_1, 1006 },
    { "EPCTR", RRE_2, 39 },
    { "EPSW", RRE_2, 745 },
    { "EREG", RRE_2, 1007 },
    { "EREGG", RRE_2, 1007 },
    { "ESAIR", RRE_1, 1007 },
    { "ESAR", RRE_1, 1006 },
    { "ESDTR", RRE_2, 1511 },
    { "ESEA", RRE_1, 1006 },
    { "ESTA", RRE_2, 1008 },
    { "ESXTR", RRE_2, 1511 },
    { "ETND", RRE_1, 745 },
    { "EX", RX_a_2_ux, 740 },
    { "EXRL", RIL_b_2, 740 },
    { "FIDBR", RRF_e_3, 1462 },
    { "FIDBRA", RRF_e_4, 1462 },
    { "FIDR", RRE_2, 1419 },
    { "FIDTR", RRF_e_4, 1514 },
    { "FIEBR", RRF_e_3, 1462 },
    { "FIEBRA", RRF_e_4, 1462 },
    { "FIER", RRE_2, 1419 },
    { "FIXBR", RRF_e_3, 1462 },
    { "FIXBRA", RRF_e_4, 1462 },
    { "FIXR", RRE_2, 1419 },
    { "FIXTR", RRF_e_4, 1514 },
    { "FLOGR", RRE_2, 746 },
    { "HDR", RR_2, 1417 },
    { "HDV", S_1_u, 129 },
    { "HER", RR_2, 1417 },
    { "HIO", S_1_u, 129 },
    { "HSCH", S_0, 1218 },
    { "IAC", RRE_1, 1011 },
    { "IC", RX_a_2_ux, 746 },
    { "ICM", RS_b_3, 746 },
    { "ICMH", RSY_b_3_us, 746 },
    { "ICMY", RSY_b_3_us, 746 },
    { "ICY", RXY_a_2, 746 },
    { "IDTE", RRF_b_4_opt, 1014 },
    { "IEDTR", RRF_b_3, 1512 },
    { "IEXTR", RRF_b_3, 1512 },
    { "IIHF", RIL_a_2, 747 },
    { "IIHH", RI_a_2_u, 747 },
    { "IIHL", RI_a_2_u, 747 },
    { "IILF", RIL_a_2, 747 },
    { "IILH", RI_a_2_u, 747 },
    { "IILL", RI_a_2_u, 747 },
    { "IPK", S_0, 1012 },
    { "IPM", RRE_1, 748 },
    { "IPTE", RRF_a_4_opt, 1019 },
    { "IRBM", RRE_2, 1012 },
    { "ISK", RR_2, 268 },
    { "ISKE", RRE_2, 1012 },
    { "IVSK", RRE_2, 1013 },
    { "KDB", RXE_2, 1448 },
    { "KDBR", RRE_2, 1448 },
    { "KDSA", RRE_2, 1700 },
    { "KDTR", RRE_2, 1495 },
    { "KEB", RXE_2, 1448 },
    { "KEBR", RRE_2, 1448 },
    { "KIMD", RRE_2, 672 },
    { "KLMD", RRE_2, 685 },
    { "KM", RRE_2, 537 },
    { "KMA", RRF_b_3, 562 },
    { "KMAC", RRE_2, 703 },
    { "KMC", RRE_2, 537 },
    { "KMCTR", RRF_b_3, 591 },
    { "KMF", RRE_2, 576 },
    { "KMO", RRE_2, 604 },
    { "KXBR", RRE_2, 1448 },
    { "KXTR", RRE_2, 1495 },
    { "L", RX_a_2_ux, 748 },
    { "LA", RX_a_2_ux, 750 },
    { "LAA", RSY_a_3, 752 },
    { "LAAG", RSY_a_3, 752 },
    { "LAAL", RSY_a_3, 752 },
    { "LAALG", RSY_a_3, 752 },
    { "LAE", RX_a_2_ux, 750 },
    { "LAEY", RXY_a_2, 750 },
    { "LAM", RS_a_3, 749 },
    { "LAMY", RSY_a_3, 749 },
    { "LAN", RSY_a_3, 753 },
    { "LANG", RSY_a_3, 753 },
    { "LAO", RSY_a_3, 754 },
    { "LAOG", RSY_a_3, 754 },
    { "LARL", RIL_b_2, 751 },
    { "LASP", SSE_2, 1023 },
    { "LAT", RXY_a_2, 755 },
    { "LAX", RSY_a_3, 753 },
    { "LAXG", RSY_a_3, 753 },
    { "LAY", RXY_a_2, 750 },
    { "LB", RXY_a_2, 756 },
    { "LBH", RXY_a_2, 756 },
    { "LBR", RRE_2, 756 },
    { "LCBB", RXE_3_xm, 757 },
    { "LCCTL", S_1_u, 40 },
    { "LCDBR", RRE_2, 1461 },
    { "LCDFR", RRE_2, 959 },
    { "LCDR", RR_2, 1418 },
    { "LCEBR", RRE_2, 1461 },
    { "LCER", RR_2, 1418 },
    { "LCGFR", RRE_2, 757 },
    { "LCGR", RRE_2, 757 },
    { "LCR", RR_2, 756 },
    { "LCTL", RS_a_3, 1032 },
    { "LCTLG", RSY_a_3, 1032 },
    { "LCXBR", RRE_2, 1461 },
    { "LCXR", RRE_2, 1418 },
    { "LD", RX_a_2_ux, 959 },
    { "LDE", RXE_2, 1419 },
    { "LDEB", RRE_2, 1464 },
    { "LDEBR", RRE_2, 1463 },
    { "LDER", RRE_2, 1419 },
    { "LDETR", RRF_d_3, 1517 },
    { "LDGR", RRE_2, 962 },
    { "LDR", RR_2, 959 },
    { "LDXBR", RRE_2, 1465 },
    { "LDXBRA", RRF_e_4, 1465 },
    { "LDXR", RR_2, 1421 },
    { "LDXTR", RRF_e_4, 1518 },
    { "LDY", RXY_a_2, 959 },
    { "LE", RX_a_2_ux, 959 },
    { "LEDBR", RRE_2, 1465 },
    { "LEDBRA", RRF_e_4, 1465 },
    { "LEDR", RR_2, 1421 },
    { "LEDTR", RRF_e_4, 1518 },
    { "LER", RR_2, 959 },
    { "LEXBR", RRE_2, 1465 },
    { "LEXBRA", RRF_e_4, 1465 },
    { "LEXR", RRE_2, 1421 },
    { "LEY", RXY_a_2, 959 },
    { "LFAS", S_1_u, 960 },
    { "LFH", RXY_a_2, 762 },
    { "LFHAT", RXY_a_2, 762 },
    { "LFPC", S_1_u, 959 },
    { "LG", RXY_a_2, 748 },
    { "LGAT", RXY_a_2, 755 },
    { "LGB", RXY_a_2, 756 },
    { "LGBR", RRE_2, 756 },
    { "LGDR", RRE_2, 962 },
    { "LGF", RXY_a_2, 748 },
    { "LGFI", RIL_a_2, 748 },
    { "LGFR", RRE_2, 748 },
    { "LGFRL", RIL_b_2, 748 },
    { "LGG", RXY_a_2, 758 },
    { "LGH", RXY_a_2, 760 },
    { "LGHI", RI_a_2_s, 760 },
    { "LGHR", RRE_2, 760 },
    { "LGHRL", RIL_b_2, 760 },
    { "LGR", RRE_2, 748 },
    { "LGRL", RIL_b_2, 748 },
    { "LGSC", RXY_a_2, 759 },
    { "LH", RX_a_2_ux, 760 },
    { "LHH", RXY_a_2, 761 },
    { "LHI", RI_a_2_s, 760 },
    { "LHR", RRE_2, 760 },
    { "LHRL", RIL_b_2, 760 },
    { "LHY", RXY_a_2, 760 },
    { "LLC", RXY_a_2, 763 },
    { "LLCH", RXY_a_2, 764 },
    { "LLCR", RRE_2, 763 },
    { "LLGC", RXY_a_2, 763 },
    { "LLGCR", RRE_2, 763 },
    { "LLGF", RXY_a_2, 762 },
    { "LLGFAT", RXY_a_2, 763 },
    { "LLGFR", RRE_2, 762 },
    { "LLGFRL", RIL_b_2, 762 },
    { "LLGFSG", RXY_a_2, 758 },
    { "LLGH", RXY_a_2, 764 },
    { "LLGHR", RRE_2, 764 },
    { "LLGHRL", RIL_b_2, 764 },
    { "LLGT", RXY_a_2, 766 },
    { "LLGTAT", RXY_a_2, 766 },
    { "LLGTR", RRE_2, 765 },
    { "LLH", RXY_a_2, 764 },
    { "LLHH", RXY_a_2, 765 },
    { "LLHR", RRE_2, 764 },
    { "LLHRL", RIL_b_2, 764 },
    { "LLIHF", RIL_a_2, 765 },
    { "LLIHH", RI_a_2_u, 765 },
    { "LLIHL", RI_a_2_u, 765 },
    { "LLILF", RIL_a_2, 765 },
    { "LLILH", RI_a_2_u, 765 },
    { "LLILL", RI_a_2_u, 765 },
    { "LLZRGF", RXY_a_2, 763 },
    { "LM", RS_a_3, 766 },
    { "LMD", SS_e_4_rb, 767 },
    { "LMG", RSY_a_3, 766 },
    { "LMH", RSY_a_3, 767 },
    { "LMY", RSY_a_3, 766 },
    { "LNDBR", RRE_2, 1464 },
    { "LNDFR", RRE_2, 962 },
    { "LNDR", RR_2, 1420 },
    { "LNEBR", RRE_2, 1464 },
    { "LNER", RR_2, 1420 },
    { "LNGFR", RRE_2, 768 },
    { "LNGR", RRE_2, 767 },
    { "LNR", RR_2, 767 },
    { "LNXBR", RRE_2, 1464 },
    { "LNXR", RRE_2, 1420 },
    { "LOC", RSY_b_3_su, 768 },
    { "LOCFH", RSY_b_3_su, 768 },
    { "LOCFHR", RRF_c_3, 768 },
    { "LOCG", RSY_b_3_su, 768 },
    { "LOCGHI", RIE_g_3, 761 },
    { "LOCGR", RRF_c_3, 768 },
    { "LOCHHI", RIE_g_3, 761 },
    { "LOCHI", RIE_g_3, 761 },
    { "LOCR", RRF_c_3, 768 },
    { "LPCTL", S_1_u, 41 },
    { "LPD", SSF_3_rd, 769 },
    { "LPDBR", RRE_2, 1465 },
    { "LPDFR", RRE_2, 962 },
    { "LPDG", SSF_3_rd, 769 },
    { "LPDR", RR_2, 1420 },
    { "LPEBR", RRE_2, 1465 },
    { "LPER", RR_2, 1420 },
    { "LPGFR", RRE_2, 771 },
    { "LPGR", RRE_2, 771 },
    { "LPP", S_1_u, 11 },
    { "LPQ", RXY_a_2, 770 },
    { "LPR", RR_2, 771 },
    { "LPSW", SI_1, 1036 },
    { "LPSWE", S_1_u, 1037 },
    { "LPTEA", RRF_b_4, 1032 },
    { "LPXBR", RRE_2, 1465 },
    { "LPXR", RRE_2, 1420 },
    { "LR", RR_2, 748 },
    { "LRA", RX_a_2_ux, 1038 },
    { "LRAG", RXY_a_2, 1038 },
    { "LRAY", RXY_a_2, 1038 },
    { "LRDR", RR_2, 1421 },
    { "LRER", RR_2, 1421 },
    { "LRL", RIL_b_2, 748 },
    { "LRV", RXY_a_2, 771 },
    { "LRVG", RXY_a_2, 771 },
    { "LRVGR", RRE_2, 771 },
    { "LRVH", RXY_a_2, 771 },
    { "LRVR", RRE_2, 771 },
    { "LSCTL", S_1_u, 42 },
    { "LT", RXY_a_2, 755 },
    { "LTDBR", RRE_2, 1461 },
    { "LTDR", RR_2, 1417 },
    { "LTDTR", RRE_2, 1513 },
    { "LTEBR", RRE_2, 1461 },
    { "LTER", RR_2, 1417 },
    { "LTG", RXY_a_2, 755 },
    { "LTGF", RXY_a_2, 755 },
    { "LTGFR", RRE_2, 754 },
    { "LTGR", RRE_2, 754 },
    { "LTR", RR_2, 754 },
    { "LTXBR", RRE_2, 1461 },
    { "LTXR", RRE_2, 1418 },
    { "LTXTR", RRE_2, 1513 },
    { "LURA", RRE_2, 1042 },
    { "LURAG", RRE_2, 1042 },
    { "LXD", RXE_2, 1419 },
    { "LXDB", RRE_2, 1464 },
    { "LXDBR", RRE_2, 1463 },
    { "LXDR", RRE_2, 1419 },
    { "LXDTR", RRF_d_3, 1517 },
    { "LXE", RXE_2, 1419 },
    { "LXEB", RRE_2, 1464 },
    { "LXEBR", RRE_2, 1463 },
    { "LXER", RRE_2, 1419 },
    { "LXR", RRE_2, 959 },
    { "LY", RXY_a_2, 748 },
    { "LZDR", RRE_1, 963 },
    { "LZER", RRE_1, 963 },
    { "LZRF", RXY_a_2, 755 },
    { "LZRG", RXY_a_2, 755 },
    { "LZXR", RRE_1, 963 },
    { "M", RX_a_2_ux, 788 },
    { "MAD", RXF_3_x, 1423 },
    { "MADB", RXF_3_x, 1468 },
    { "MADBR", RRD_3, 1468 },
    { "MADR", RRD_3, 1423 },
    { "MAE", RXF_3_x, 1423 },
    { "MAEB", RXF_3_x, 1468 },
    { "MAEBR", RRD_3, 1468 },
    { "MAER", RRD_3, 1423 },
    { "MAY", RXF_3_x, 1424 },
    { "MAYH", RXF_3_x, 1424 },
    { "MAYHR", RRD_3, 1424 },
    { "MAYL", RXF_3_x, 1424 },
    { "MAYLR", RRD_3, 1424 },
    { "MAYR", RRD_3, 1424 },
    { "MC", SI_2_s, 772 },
    { "MD", RX_a_2_ux, 1422 },
    { "MDB", RXE_2, 1467 },
    { "MDBR", RRE_2, 1467 },
    { "MDE", RX_a_2_ux, 1422 },
    { "MDEB", RXE_2, 1467 },
    { "MDEBR", RRE_2, 1467 },
    { "MDER", RR_2, 1421 },
    { "MDR", RR_2, 1421 },
    { "MDTR", RRF_a_3, 1519 },
    { "MDTRA", RRF_a_4, 1520 },
    { "ME", RX_a_2_ux, 1422 },
    { "MEE", RXE_2, 1422 },
    { "MEEB", RXE_2, 1467 },
    { "MEEBR", RRE_2, 1467 },
    { "MEER", RRE_2, 1421 },
    { "MER", RR_2, 1421 },
    { "MFY", RXY_a_2, 788 },
    { "MG", RXY_a_2, 788 },
    { "MGH", RXY_a_2, 789 },
    { "MGHI", RI_a_2_s, 789 },
    { "MGRK", RRF_a_3, 788 },
    { "MH", RX_a_2_ux, 789 },
    { "MHI", RI_a_2_s, 789 },
    { "MHY", RXY_a_2, 789 },
    { "ML", RXY_a_2, 790 },
    { "MLG", RXY_a_2, 790 },
    { "MLGR", RRE_2, 790 },
    { "MLR", RRE_2, 790 },
    { "MP", SS_b_2, 926 },
    { "MR", RR_2, 788 },
    { "MS", RX_a_2_ux, 791 },
    { "MSC", RXY_a_2, 791 },
    { "MSCH", S_1_u, 1219 },
    { "MSD", RXF_3_x, 1423 },
    { "MSDB", RXF_3_x, 1468 },
    { "MSDBR", RRD_3, 1468 },
    { "MSDR", RRD_3, 1423 },
    { "MSE", RXF_3_x, 1423 },
    { "MSEB", RXF_3_x, 1468 },
    { "MSEBR", RRD_3, 1468 },
    { "MSER", RRD_3, 1423 },
    { "MSFI", RIL_a_2, 791 },
    { "MSG", RXY_a_2, 791 },
    { "MSGC", RXY_a_2, 791 },
    { "MSGF", RXY_a_2, 791 },
    { "MSGFI", RIL_a_2, 791 },
    { "MSGFR", RRE_2, 791 },
    { "MSGR", RRE_2, 791 },
    { "MSGRKC", RRF_a_3, 791 },
    { "MSR", RRE_2, 791 },
    { "MSRKC", RRF_a_3, 791 },
    { "MSTA", RRE_1, 1043 },
    { "MSY", RXY_a_2, 791 },
    { "MVC", SS_a_2_u, 773 },
    { "MVCDK", SSE_2, 1048 },
    { "MVCIN", SS_a_2_u, 774 },
    { "MVCK", SS_d_3, 1049 },
    { "MVCL", RR_2, 774 },
    { "MVCLE", RS_a_3, 778 },
    { "MVCLU", RSY_a_3, 781 },
    { "MVCOS", SSF_3_dr, 1050 },
    { "MVCP", SS_d_3, 1046 },
    { "MVCRL", SSE_2, 788 },
    { "MVCS", SS_d_3, 1046 },
    { "MVCSK", SSE_2, 1053 },
    { "MVGHI", SIL_2_s, 773 },
    { "MVHHI", SIL_2_s, 773 },
    { "MVHI", SIL_2_s, 773 },
    { "MVI", SI_2_u, 773 },
    { "MVIY", SIY_2_uu, 773 },
    { "MVN", SS_a_2_u, 785 },
    { "MVO", SS_b_2, 786 },
    { "MVPG", RRE_2, 1044 },
    { "MVST", RRE_2, 785 },
    { "MVZ", SS_a_2_u, 787 },
    { "MXBR", RRE_2, 1467 },
    { "MXD", RX_a_2_ux, 1422 },
    { "MXDB", RXE_2, 1467 },
    { "MXDBR", RRE_2, 1467 },
    { "MXDR", RR_2, 1421 },
    { "MXR", RR_2, 1421 },
    { "MXTR", RRF_a_3, 1519 },
    { "MXTRA", RRF_a_4, 1520 },
    { "MY", RXF_3_x, 1426 },
    { "MYH", RXF_3_x, 1426 },
    { "MYHR", RRD_3, 1426 },
    { "MYL", RXF_3_x, 1426 },
    { "MYLR", RRD_3, 1426 },
    { "MYR", RRD_3, 1426 },
    { "N", RX_a_2_ux, 517 },
    { "NC", SS_a_2_u, 518 },
    { "NCGRK", RRF_a_3, 522 },
    { "NCRK", RRF_a_3, 522 },
    { "NG", RXY_a_2, 517 },
    { "NGR", RRE_2, 517 },
    { "NGRK", RRF_a_3, 517 },
    { "NI", SI_2_u, 517 },
    { "NIAI", IE_2, 792 },
    { "NIHF", RIL_a_2, 518 },
    { "NIHH", RI_a_2_u, 518 },
    { "NIHL", RI_a_2_u, 518 },
    { "NILF", RIL_a_2, 519 },
    { "NILH", RI_a_2_u, 519 },
    { "NILL", RI_a_2_u, 519 },
    { "NIY", SIY_2_su, 518 },
    { "NNGRK", RRF_a_3, 796 },
    { "NNRK", RRF_a_3, 796 },
    { "NOGRK", RRF_a_3, 799 },
    { "NORK", RRF_a_3, 799 },
    { "NR", RR_2, 517 },
    { "NRK", RRF_a_3, 517 },
    { "NTSTG", RXY_a_2, 794 },
    { "NXGRK", RRF_a_3, 799 },
    { "NXRK", RRF_a_3, 799 },
    { "NY", RXY_a_2, 517 },
    { "O", RX_a_2_ux, 794 },
    { "OC", SS_a_2_u, 795 },
    { "OCGRK", RRF_a_3, 802 },
    { "OCRK", RRF_a_3, 802 },
    { "OG", RXY_a_2, 795 },
    { "OGR", RRE_2, 794 },
    { "OGRK", RRF_a_3, 794 },
    { "OI", SI_2_u, 795 },
    { "OIHF", RIL_a_2, 796 },
    { "OIHH", RI_a_2_u, 796 },
    { "OIHL", RI_a_2_u, 796 },
    { "OILF", RIL_a_2, 796 },
    { "OILH", RI_a_2_u, 796 },
    { "OILL", RI_a_2_u, 796 },
    { "OIY", SIY_2_su, 795 },
    { "OR", RR_2, 794 },
    { "ORK", RRF_a_3, 794 },
    { "OY", RXY_a_2, 794 },
    { "PACK", SS_b_2, 796 },
    { "PALB", RRE_0, 1098 },
    { "PC", S_1_u, 1072 },
    { "PCC", RRE_0, 799 },
    { "PCKMO", RRE_0, 1056 },
    { "PFD", RXY_b_2, 843 },
    { "PFDRL", RIL_c_2, 843 },
    { "PFMF", RRE_2, 1059 },
    { "PFPO", E_0, 963 },
    { "PGIN", RRE_2, 1054 },
    { "PGOUT", RRE_2, 1055 },
    { "PKA", SS_f_2, 797 },
    { "PKU", SS_f_2, 798 },
    { "PLO", SS_e_4_br, 815 },
    { "POPCNT", RRF_c_3_opt, 853 },
    { "PPA", RRF_c_3, 829 },
    { "PPNO", RRE_2, 830 },
    { "PR", E_0, 1085 },
    { "PRNO", RRE_2, 830 },
    { "PT", RRE_2, 1089 },
    { "PTF", RRE_1, 1071 },
    { "PTFF", E_0, 1063 },
    { "PTI", RRE_2, 1089 },
    { "PTLB", S_0, 1098 },
    { "QADTR", RRF_b_4, 1521 },
    { "QAXTR", RRF_b_4, 1521 },
    { "QCTRI", S_1_u, 43 },
    { "QSI", S_1_u, 45 },
    { "RCHP", S_0, 1221 },
    { "RISBG", RIE_f_5, 847 },
    { "RISBGN", RIE_f_5, 847 },
    { "RISBGNZ", RIE_f_5, 860 },
    { "RISBGZ", RIE_f_5, 858 },
    { "RISBHG", RIE_f_5, 848 },
    { "RISBHGZ", RIE_f_5, 860 },
    { "RISBLG", RIE_f_5, 849 },
    { "RISBLGZ", RIE_f_5, 860 },
    { "RLL", RSY_a_3, 845 },
    { "RLLG", RSY_a_3, 845 },
    { "RNSBG", RIE_f_5, 845 },
    { "RNSBGT", RIE_f_5, 845 },
    { "ROSBG", RIE_f_5, 846 },
    { "ROSBGT", RIE_f_5, 858 },
    { "RP", S_1_u, 1099 },
    { "RRB", S_1_u, 295 },
    { "RRBE", RRE_2, 1098 },
    { "RRBM", RRE_2, 1099 },
    { "RRDTR", RRF_b_4, 1524 },
    { "RRXTR", RRF_b_4, 1524 },
    { "RSCH", S_0, 1222 },
    { "RXSBG", RIE_f_5, 846 },
    { "RXSBGT", RIE_f_5, 846 },
    { "S", RX_a_2_ux, 872 },
    { "SAC", S_1_u, 1102 },
    { "SACF", S_1_u, 1102 },
    { "SAL", S_0, 1224 },
    { "SAM24", E_0, 854 },
    { "SAM31", E_0, 854 },
    { "SAM64", E_0, 854 },
    { "SAR", RRE_2, 854 },
    { "SCCTR", RRE_2, 46 },
    { "SCHM", S_0, 1225 },
    { "SCK", S_1_u, 1103 },
    { "SCKC", S_1_u, 1104 },
    { "SCKPF", E_0, 1105 },
    { "SD", RX_a_2_ux, 1428 },
    { "SDB", RXE_2, 1470 },
    { "SDBR", RRE_2, 1470 },
    { "SDR", RR_2, 1428 },
    { "SDTR", RRF_a_3, 1527 },
    { "SDTRA", RRF_a_4, 1527 },
    { "SE", RX_a_2_ux, 1428 },
    { "SEB", RXE_2, 1470 },
    { "SEBR", RRE_2, 1470 },
    { "SELFHR", RRF_a_4, 864 },
    { "SELGR", RRF_a_4, 864 },
    { "SELR", RRF_a_4, 864 },
    { "SER", RR_2, 1428 },
    { "SFASR", RRE_1, 976 },
    { "SFPC", RRE_1, 975 },
    { "SG", RXY_a_2, 872 },
    { "SGF", RXY_a_2, 872 },
    { "SGFR", RRE_2, 871 },
    { "SGH", RXY_a_2, 872 },
    { "SGR", RRE_2, 871 },
    { "SGRK", RRF_a_3, 872 },
    { "SH", RX_a_2_ux, 872 },
    { "SHHHR", RRF_a_3, 873 },
    { "SHHLR", RRF_a_3, 873 },
    { "SHY", RXY_a_2, 872 },
    { "SIE", S_1_u, 7 },
    { "SIGP", RS_a_3, 1115 },
    { "SIO", S_1_u, 129 },
    { "SIOF", S_1_u, 129 },
    { "SL", RX_a_2_ux, 874 },
    { "SLA", RS_a_2, 856 },
    { "SLAG", RSY_a_3, 856 },
    { "SLAK", RSY_a_3, 856 },
    { "SLB", RXY_a_2, 875 },
    { "SLBG", RXY_a_2, 875 },
    { "SLBGR", RRE_2, 875 },
    { "SLBR", RRE_2, 875 },
    { "SLDA", RS_a_2, 855 },
    { "SLDL", RS_a_2, 856 },
    { "SLDT", RXF_3_x, 1526 },
    { "SLFI", RIL_a_2, 874 },
    { "SLG", RXY_a_2, 874 },
    { "SLGF", RXY_a_2, 874 },
    { "SLGFI", RIL_a_2, 874 },
    { "SLGFR", RRE_2, 873 },
    { "SLGR", RRE_2, 873 },
    { "SLGRK", RRF_a_3, 873 },
    { "SLHHHR", RRF_a_3, 875 },
    { "SLHHLR", RRF_a_3, 875 },
    { "SLL", RS_a_2, 857 },
    { "SLLG", RSY_a_3, 857 },
    { "SLLK", RSY_a_3, 857 },
    { "SLR", RR_2, 873 },
    { "SLRK", RRF_a_3, 873 },
    { "SLXT", RXF_3_x, 1526 },
    { "SLY", RXY_a_2, 874 },
    { "SORTL", RRE_2, 19 },
    { "SP", SS_b_2, 927 },
    { "SPCTR", RRE_2, 47 },
    { "SPKA", S_1_u, 1106 },
    { "SPM", RR_1, 855 },
    { "SPT", S_1_u, 1105 },
    { "SPX", S_1_u, 1105 },
    { "SQD", RXE_2, 1427 },
    { "SQDB", RXE_2, 1470 },
    { "SQDBR", RRE_2, 1470 },
    { "SQDR", RRE_2, 1427 },
    { "SQE", RXE_2, 1427 },
    { "SQEB", RXE_2, 1470 },
    { "SQEBR", RRE_2, 1470 },
    { "SQER", RRE_2, 1427 },
    { "SQXBR", RRE_2, 1470 },
    { "SQXR", RRE_2, 1427 },
    { "SR", RR_2, 871 },
    { "SRA", RS_a_2, 859 },
    { "SRAG", RSY_a_3, 859 },
    { "SRAK", RSY_a_3, 859 },
    { "SRDA", RS_a_2, 858 },
    { "SRDL", RS_a_2, 858 },
    { "SRDT", RXF_3_x, 1526 },
    { "SRK", RRF_a_3, 871 },
    { "SRL", RS_a_2, 860 },
    { "SRLG", RSY_a_3, 860 },
    { "SRLK", RSY_a_3, 860 },
    { "SRNM", S_1_u, 975 },
    { "SRNMB", S_1_u, 975 },
    { "SRNMT", S_1_u, 975 },
    { "SRP", SS_c_3, 926 },
    { "SRST", RRE_2, 850 },
    { "SRSTU", RRE_2, 852 },
    { "SRXT", RXF_3_x, 1526 },
    { "SSAIR", RRE_1, 1107 },
    { "SSAR", RRE_1, 1107 },
    { "SSCH", S_1_u, 1227 },
    { "SSK", RR_2, 304 },
    { "SSKE", RRF_c_3_opt, 1112 },
    { "SSM", SI_1, 1115 },
    { "ST", RX_a_2_ux, 860 },
    { "STAM", RS_a_3, 861 },
    { "STAMY", RSY_a_3, 861 },
    { "STAP", S_1_u, 1118 },
    { "STC", RX_a_2_ux, 862 },
    { "STCH", RXY_a_2, 862 },
    { "STCK", S_1_u, 863 },
    { "STCKC", S_1_u, 1117 },
    { "STCKE", S_1_u, 864 },
    { "STCKF", S_1_u, 863 },
    { "STCM", RS_b_3, 862 },
    { "STCMH", RSY_b_3_us, 862 },
    { "STCMY", RSY_b_3_us, 862 },
    { "STCPS", S_1_u, 1228 },
    { "STCRW", S_1_u, 1229 },
    { "STCTG", RSY_a_3, 1117 },
    { "STCTL", RS_a_3, 1117 },
    { "STCY", RXY_a_2, 862 },
    { "STD", RX_a_2_ux, 976 },
    { "STDY", RXY_a_2, 977 },
    { "STE", RX_a_2_ux, 976 },
    { "STEY", RXY_a_2, 977 },
    { "STFH", RXY_a_2, 868 },
    { "STFL", S_1_u, 1120 },
    { "STFLE", S_1_s, 866 },
    { "STFPC", S_1_u, 977 },
    { "STG", RXY_a_2, 861 },
    { "STGRL", RIL_b_2, 861 },
    { "STGSC", RXY_a_2, 867 },
    { "STH", RX_a_2_ux, 867 },
    { "STHH", RXY_a_2, 868 },
    { "STHRL", RIL_b_2, 868 },
    { "STHY", RXY_a_2, 868 },
    { "STIDC", S_1_u, 129 },
    { "STIDP", S_1_u, 1118 },
    { "STM", RS_a_3, 869 },
    { "STMG", RSY_a_3, 869 },
    { "STMH", RSY_a_3, 869 },
    { "STMY", RSY_a_3, 869 },
    { "STNSM", SI_2_u, 1146 },
    { "STOC", RSY_b_3_su, 869 },
    { "STOCFH", RSY_b_3_su, 870 },
    { "STOCG", RSY_b_3_su, 869 },
    { "STOSM", SI_2_u, 1146 },
    { "STPQ", RXY_a_2, 870 },
    { "STPT", S_1_u, 1120 },
    { "STPX", S_1_u, 1121 },
    { "STRAG", SSE_2, 1121 },
    { "STRL", RIL_b_2, 861 },
    { "STRV", RXY_a_2, 871 },
    { "STRVG", RXY_a_2, 871 },
    { "STRVH", RXY_a_2, 871 },
    { "STSCH", S_1_u, 1230 },
    { "STSI", S_1_u, 1122 },
    { "STURA", RRE_2, 1147 },
    { "STURG", RRE_2, 1147 },
    { "STY", RXY_a_2, 861 },
    { "SU", RX_a_2_ux, 1429 },
    { "SUR", RR_2, 1429 },
    { "SVC", I_1, 876 },
    { "SW", RX_a_2_ux, 1429 },
    { "SWR", RR_2, 1429 },
    { "SXBR", RRE_2, 1470 },
    { "SXR", RR_2, 1428 },
    { "SXTR", RRF_a_3, 1527 },
    { "SXTRA", RRF_a_4, 1527 },
    { "SY", RXY_a_2, 872 },
    { "TABORT", S_1_u, 878 },
    { "TAM", E_0, 876 },
    { "TAR", RRE_2, 1147 },
    { "TB", RRE_2, 1149 },
    { "TBDR", RRF_e_3, 956 },
    { "TBEDR", RRF_e_3, 956 },
    { "TBEGIN", SIL_2_s, 879 },
    { "TBEGINC", SIL_2_s, 883 },
    { "TCDB", RXE_2, 1471 },
    { "TCEB", RXE_2, 1471 },
    { "TCH", S_1_u, 384 },
    { "TCXB", RXE_2, 1471 },
    { "TDCDT", RXE_2, 1528 },
    { "TDCET", RXE_2, 1528 },
    { "TDCXT", RXE_2, 1528 },
    { "TDGDT", RXE_2, 1529 },
    { "TDGET", RXE_2, 1529 },
    { "TDGXT", RXE_2, 1529 },
    { "TEND", S_0, 885 },
    { "THDER", RRE_2, 955 },
    { "THDR", RRE_2, 955 },
    { "TIO", S_1_u, 385 },
    { "TM", SI_2_u, 877 },
    { "TMH", RI_a_2_u, 877 },
    { "TMHH", RI_a_2_u, 877 },
    { "TMHL", RI_a_2_u, 877 },
    { "TML", RI_a_2_u, 877 },
    { "TMLH", RI_a_2_u, 877 },
    { "TMLL", RI_a_2_u, 877 },
    { "TMY", SIY_2_su, 877 },
    { "TP", RSL_a_1, 928 },
    { "TPEI", RRE_2, 1151 },
    { "TPI", S_1_u, 1231 },
    { "TPROT", SSE_2, 1152 },
    { "TR", SS_a_2_u, 886 },
    { "TRACE", RS_a_3, 1155 },
    { "TRACG", RSY_a_3, 1155 },
    { "TRAP2", E_0, 1156 },
    { "TRAP4", S_1_u, 1156 },
    { "TRE", RRE_2, 893 },
    { "TROO", RRF_c_3_opt, 895 },
    { "TROT", RRF_c_3_opt, 895 },
    { "TRT", SS_a_2_u, 887 },
    { "TRTE", RRF_c_3_opt, 887 },
    { "TRTO", RRF_c_3_opt, 895 },
    { "TRTR", SS_a_2_u, 892 },
    { "TRTRE", RRF_c_3_opt, 888 },
    { "TRTT", RRF_c_3_opt, 895 },
    { "TS", SI_1, 876 },
    { "TSCH", S_1_u, 1232 },
    { "UNPK", SS_b_2, 900 },
    { "UNPKA", SS_a_2_u, 901 },
    { "UNPKU", SS_a_2_u, 902 },
    { "UPT", E_0, 903 },
    { "VA", VRR_c_4, 1557 },
    { "VAC", VRR_d_5, 1558 },
    { "VACC", VRR_c_4, 1558 },
    { "VACCC", VRR_d_5, 1559 },
    { "VACD", RI_a_2_u, 0 },
    { "VACE", RI_a_2_u, 0 },
    { "VACRS", RRE_2, 0 },
    { "VACSV", RRE_2, 0 },
    { "VAD", RI_a_2_u, 0 },
    { "VADS", RI_a_2_u, 0 },
    { "VAE", RI_a_2_u, 0 },
    { "VAES", RI_a_2_u, 0 },
    { "VAP", VRI_f_5, 1643 },
    { "VAS", RI_a_2_u, 0 },
    { "VAVG", VRR_c_4, 1560 },
    { "VAVGL", VRR_c_4, 1560 },
    { "VBPERM", VRR_c_3, 1536 },
    { "VC", RI_a_2_u, 0 },
    { "VCD", RI_a_2_u, 0 },
    { "VCDS", RI_a_2_u, 0 },
    { "VCE", RI_a_2_u, 0 },
    { "VCEQ", VRR_b_5, 1561 },
    { "VCES", RI_a_2_u, 0 },
    { "VCFPL", VRR_a_5, 1643 },
    { "VCFPS", VRR_a_5, 1641 },
    { "VCFPS", VRR_a_5, 1607 },
    { "VCH", VRR_b_5, 1562 },
    { "VCHL", VRR_b_5, 1563 },
    { "VCKSM", VRR_c_3, 1560 },
    { "VCLFP", VRR_a_5, 1611 },
    { "VCLGD", VRR_a_5, 1611 },
    { "VCLZ", VRR_a_3, 1564 },
    { "VCOVM", RRE_2, 0 },
    { "VCP", VRR_h_3, 1644 },
    { "VCS", RI_a_2_u, 0 },
    { "VCSFP", VRR_a_5, 1644 },
    { "VCTZ", VRR_a_3, 1564 },
    { "VCVB", VRR_i_3, 1645 },
    { "VCVBG", VRR_i_3, 1645 },
    { "VCVD", VRI_i_4, 1646 },
    { "VCVDG", VRI_i_4, 1646 },
    { "VCVM", RRE_2, 0 },
    { "VCZVM", RRE_2, 0 },
    { "VDD", RI_a_2_u, 0 },
    { "VDDS", RI_a_2_u, 0 },
    { "VDDS", RI_a_2_u, 0 },
    { "VDE", RI_a_2_u, 0 },
    { "VDES", RI_a_2_u, 0 },
    { "VDP", VRI_f_5, 1648 },
    { "VEC", VRR_a_3, 1561 },
    { "VECL", VRR_a_3, 1561 },
    { "VERIM", VRI_d_5, 1576 },
    { "VERLL", VRS_a_4, 1575 },
    { "VERLLV", VRR_c_4, 1575 },
    { "VESL", VRS_a_4, 1577 },
    { "VESLV", VRR_c_4, 1577 },
    { "VESRA", VRS_a_4, 1577 },
    { "VESRAV", VRR_c_4, 1577 },
    { "VESRL", VRS_a_4, 1578 },
    { "VESRLV", VRR_c_4, 1578 },
    { "VFA", VRR_c_5, 1595 },
    { "VFAE", VRR_b_5_opt, 1585 },
    { "VFCE", VRR_c_6, 1601 },
    { "VFCH", VRR_c_6, 1603 },
    { "VFCHE", VRR_c_6, 1605 },
    { "VFD", VRR_c_5, 1613 },
    { "VFEE", VRR_b_5_opt, 1587 },
    { "VFENE", VRR_b_5_opt, 1588 },
    { "VFI", VRR_a_5, 1615 },
    { "VFLL", VRR_a_4, 1617 },
    { "VFLR", VRR_a_5, 1618 },
    { "VFM", VRR_c_5, 1631 },
    { "VFMA", VRR_e_6, 1633 },
    { "VFMAX", VRR_c_6, 1619 },
    { "VFMIN", VRR_c_6, 1625 },
    { "VFMS", VRR_e_6, 1633 },
    { "VFNMA", VRR_e_6, 1633 },
    { "VFNMS", VRR_e_6, 1633 },
    { "VFPSO", VRR_a_5, 1635 },
    { "VFS", VRR_c_5, 1637 },
    { "VFSQ", VRR_a_4, 1636 },
    { "VFTCI", VRI_e_5, 1638 },
    { "VGBM", VRI_a_2, 1537 },
    { "VGEF", VRV_3, 1536 },
    { "VGEG", VRV_3, 1536 },
    { "VGFM", VRR_c_4, 1565 },
    { "VGFMA", VRR_d_5, 1566 },
    { "VGM", VRI_b_4, 1537 },
    { "VISTR", VRR_a_4_opt, 1589 },
    { "VL", VRX_3_opt, 1538 },
    { "VLBB", VRX_3, 1542 },
    { "VLBIX", RRE_2, 0 },
    { "VLBR", VRX_3, 1563 },
    { "VLBRREP", VRX_3, 1562 },
    { "VLC", VRR_a_3, 1566 },
    { "VLCVM", RRE_2, 0 },
    { "VLD", RI_a_2_u, 0 },
    { "VLEB", VRX_3, 1538 },
    { "VLEBRG", VRX_3, 1561 },
    { "VLEBRH", VRX_3, 1561 },
    { "VLEF", VRX_3, 1539 },
    { "VLEG", VRX_3, 1539 },
    { "VLEH", VRX_3, 1539 },
    { "VLEIB", VRI_a_3, 1539 },
    { "VLEIF", VRI_a_3, 1539 },
    { "VLEIG", VRI_a_3, 1539 },
    { "VLEIH", VRI_a_3, 1539 },
    { "VLELD", RRE_2, 0 },
    { "VLELE", RRE_2, 0 },
    { "VLER", VRX_3, 1564 },
    { "VLGV", VRS_c_4, 1539 },
    { "VLH", RI_a_2_u, 0 },
    { "VLI", RRE_2, 0 },
    { "VLID", RRE_2, 0 },
    { "VLINT", RI_a_2_u, 0 },
    { "VLIP", VRI_h_3, 1649 },
    { "VLL", VRS_b_3, 1543 },
    { "VLLEBRZ", VRX_3, 1562 },
    { "VLLEZ", VRX_3, 1540 },
    { "VLM", VRS_a_4_opt, 1541 },
    { "VLMD", RI_a_2_u, 0 },
    { "VLP", VRR_a_3, 1566 },
    { "VLR", VRR_a_2, 1538 },
    { "VLREP", VRX_3, 1538 },
    { "VLRL", VSI_3, 1541 },
    { "VLRLR", VRS_d_3, 1541 },
    { "VLVCA", RRE_2, 0 },
    { "VLVCU", RRE_2, 0 },
    { "VLVG", VRS_b_4, 1543 },
    { "VLVGP", VRR_f_3, 1543 },
    { "VLVM", RRE_2, 0 },
    { "VLY", RI_a_2_u, 0 },
    { "VLYD", RI_a_2_u, 0 },
    { "VM", RI_a_2_u, 0 },
    { "VMAD", RI_a_2_u, 0 },
    { "VMADS", RI_a_2_u, 0 },
    { "VMAE", VRR_d_5, 1569 },
    { "VMAES", RI_a_2_u, 0 },
    { "VMAH", VRR_d_5, 1569 },
    { "VMAL", VRR_d_5, 1568 },
    { "VMALE", VRR_d_5, 1569 },
    { "VMALH", VRR_d_5, 1569 },
    { "VMALO", VRR_d_5, 1570 },
    { "VMAO", VRR_d_5, 1570 },
    { "VMCD", RI_a_2_u, 0 },
    { "VMCE", RI_a_2_u, 0 },
    { "VMD", RI_a_2_u, 0 },
    { "VMD", RI_a_2_u, 0 },
    { "VMDS", RI_a_2_u, 0 },
    { "VME", VRR_c_4, 1572 },
    { "VMES", RI_a_2_u, 0 },
    { "VMH", VRR_c_4, 1570 },
    { "VML", VRR_c_4, 1571 },
    { "VMLE", VRR_c_4, 1572 },
    { "VMLH", VRR_c_4, 1571 },
    { "VMLO", VRR_c_4, 1572 },
    { "VMN", VRR_c_4, 1567 },
    { "VMNL", VRR_c_4, 1568 },
    { "VMNSD", RRE_2, 0 },
    { "VMNSE", RRE_2, 0 },
    { "VMO", VRR_c_4, 1572 },
    { "VMP", VRI_f_5, 1650 },
    { "VMRH", VRR_c_4, 1544 },
    { "VMRL", VRR_c_4, 1544 },
    { "VMRRS", RRE_2, 0 },
    { "VMRSV", RRE_2, 0 },
    { "VMS", RI_a_2_u, 0 },
    { "VMSD", RI_a_2_u, 0 },
    { "VMSDS", RI_a_2_u, 0 },
    { "VMSE", RI_a_2_u, 0 },
    { "VMSES", RI_a_2_u, 0 },
    { "VMSL", VRR_d_6, 1573 },
    { "VMSP", VRI_f_5, 1651 },
    { "VMX", VRR_c_4, 1567 },
    { "VMXAD", RRE_2, 0 },
    { "VMXAE", RRE_2, 0 },
    { "VMXL", VRR_c_4, 1567 },
    { "VMXSE", RRE_2, 0 },
    { "VN", VRR_c_3, 1559 },
    { "VNC", VRR_c_3, 1559 },
    { "VNN", VRR_c_3, 1574 },
    { "VNO", VRR_c_3, 1574 },
    { "VNS", RI_a_2_u, 0 },
    { "VNVM", RRE_2, 0 },
    { "VNX", VRR_c_3, 1574 },
    { "VO", VRR_c_3, 1574 },
    { "VOC", VRR_c_3, 1575 },
    { "VOS", RI_a_2_u, 0 },
    { "VOVM", RRE_2, 0 },
    { "VOVM", RRE_2, 0 },
    { "VPDI", VRR_c_4, 1547 },
    { "VPERM", VRR_e_4, 1547 },
    { "VPK", VRR_c_4, 1545 },
    { "VPKLS", VRR_b_5, 1546 },
    { "VPKS", VRR_b_5, 1545 },
    { "VPKZ", VSI_3, 1652 },
    { "VPOPCT", VRR_a_3, 1575 },
    { "VPSOP", VRI_g_5_u, 1653 },
    { "VRCL", RRE_2, 0 },
    { "VREP", VRI_c_4, 1547 },
    { "VREPI", VRI_a_3, 1548 },
    { "VRP", VRI_f_5, 1654 },
    { "VRRS", RRE_2, 0 },
    { "VRSV", RRE_2, 0 },
    { "VRSVC", RRE_2, 0 },
    { "VS", VRR_c_4, 1580 },
    { "VSBCBI", VRR_d_5, 1582 },
    { "VSBI", VRR_d_5, 1581 },
    { "VSCBI", VRR_c_4, 1581 },
    { "VSCEF", VRV_3, 1548 },
    { "VSCEG", VRV_3, 1548 },
    { "VSD", RI_a_2_u, 0 },
    { "VSD", RI_a_2_u, 0 },
    { "VSDP", VRI_f_5, 1656 },
    { "VSDS", RI_a_2_u, 0 },
    { "VSDS", RI_a_2_u, 0 },
    { "VSE", RI_a_2_u, 0 },
    { "VSEG", VRR_a_3, 1549 },
    { "VSEL", VRR_e_4, 1549 },
    { "VSES", RI_a_2_u, 0 },
    { "VSL", VRR_c_3, 1579 },
    { "VSLB", VRR_c_3, 1579 },
    { "VSLD", VRI_d_4, 1607 },
    { "VSLDB", VRI_d_4, 1579 },
    { "VSLL", RRE_2, 0 },
    { "VSP", VRI_f_5, 1658 },
    { "VSPSD", RRE_2, 0 },
    { "VSRA", VRR_c_3, 1579 },
    { "VSRAB", VRR_c_3, 1580 },
    { "VSRD", VRI_d_4, 1608 },
    { "VSRL", VRR_c_3, 1580 },
    { "VSRLB", VRR_c_3, 1580 },
    { "VSRP", VRI_g_5_s, 1657 },
    { "VSRRS", RRE_2, 0 },
    { "VSRSV", RRE_2, 0 },
    { "VSS", RI_a_2_u, 0 },
    { "VST", VRX_3_opt, 1550 },
    { "VSTBR", VRX_3, 1576 },
    { "VSTD", RI_a_2_u, 0 },
    { "VSTEB", VRX_3, 1550 },
    { "VSTEBRF", VRX_3, 1576 },
    { "VSTEBRG", VRX_3, 1576 },
    { "VSTEBRH", VRX_3, 1576 },
    { "VSTEF", VRX_3, 1550 },
    { "VSTEG", VRX_3, 1550 },
    { "VSTEH", VRX_3, 1550 },
    { "VSTER", VRX_3, 1578 },
    { "VSTH", RI_a_2_u, 0 },
    { "VSTI", RRE_2, 0 },
    { "VSTID", RRE_2, 0 },
    { "VSTK", RI_a_2_u, 0 },
    { "VSTKD", RI_a_2_u, 0 },
    { "VSTL", VRS_b_3, 1552 },
    { "VSTM", VRS_a_4_opt, 1551 },
    { "VSTMD", RI_a_2_u, 0 },
    { "VSTRC", VRR_d_6_opt, 1590 },
    { "VSTRL", VSI_3, 1551 },
    { "VSTRLR", VRS_d_3, 1551 },
    { "VSTRS", VRR_d_6_opt, 1622 },
    { "VSTVM", RRE_2, 0 },
    { "VSTVP", RRE_2, 0 },
    { "VSUM", VRR_c_4, 1583 },
    { "VSUMG", VRR_c_4, 1582 },
    { "VSUMQ", VRR_c_4, 1583 },
    { "VSVMM", RRE_2, 0 },
    { "VTM", VRR_a_2, 1584 },
    { "VTP", VRR_g_1, 1660 },
    { "VTVM", RRE_2, 0 },
    { "VUPH", VRR_a_3, 1552 },
    { "VUPKZ", VSI_3, 1660 },
    { "VUPL", VRR_a_3, 1553 },
    { "VUPLH", VRR_a_3, 1553 },
    { "VUPLL", VRR_a_3, 1554 },
    { "VX", VRR_c_3, 1565 },
    { "VXELD", RRE_2, 0 },
    { "VXELE", RRE_2, 0 },
    { "VXS", RI_a_2_u, 0 },
    { "VXVC", RRE_2, 0 },
    { "VXVM", RRE_2, 0 },
    { "VXVMM", RRE_2, 0 },
    { "VZPSD", RRE_2, 0 },
    { "WFC", VRR_a_4, 1599 },
    { "WFK", VRR_a_4, 1600 },
    { "X", RX_a_2_ux, 738 },
    { "XC", SS_a_2_s, 739 },
    { "XG", RXY_a_2, 738 },
    { "XGR", RRE_2, 738 },
    { "XGRK", RRF_a_3, 738 },
    { "XI", SI_2_u, 739 },
    { "XIHF", RIL_a_2, 740 },
    { "XILF", RIL_a_2, 740 },
    { "XIY", SIY_2_su, 739 },
    { "XR", RR_2, 738 },
    { "XRK", RRF_a_3, 738 },
    { "XSCH", S_0, 1215 },
    { "XY", RXY_a_2, 738 },
    { "ZAP", SS_b_2, 928 },
};
#ifdef __cpp_lib_ranges
static_assert(std::ranges::is_sorted(machine_instructions, {}, &machine_instruction::name));

const machine_instruction* instruction::find_machine_instructions(std::string_view name)
{
    auto it = std::ranges::lower_bound(machine_instructions, name, {}, &machine_instruction::name);
    if (it == std::ranges::end(machine_instructions) || it->name() != name)
        return nullptr;
    return &*it;
}

constexpr const machine_instruction* find_mi(std::string_view name)
{
    auto it = std::ranges::lower_bound(machine_instructions, name, {}, &machine_instruction::name);
    assert(it != std::ranges::end(machine_instructions) && it->name() == name);
    return &*it;
}
#else
static_assert(std::is_sorted(std::begin(machine_instructions),
    std::end(machine_instructions),
    [](const auto& l, const auto& r) { return l.name() < r.name(); }));

const machine_instruction* instruction::find_machine_instructions(std::string_view name)
{
    auto it = std::lower_bound(
        std::begin(machine_instructions), std::end(machine_instructions), name, [](const auto& l, const auto& r) {
            return l.name() < r;
        });
    if (it == std::end(machine_instructions) || it->name() != name)
        return nullptr;
    return &*it;
}

constexpr const machine_instruction* find_mi(std::string_view name)
{
    auto it = std::lower_bound(
        std::begin(machine_instructions), std::end(machine_instructions), name, [](const auto& l, const auto& r) {
            return l.name() < r;
        });
    assert(it != std::end(machine_instructions) && it->name() == name);
    return &*it;
}
#endif

const machine_instruction& instruction::get_machine_instructions(std::string_view name)
{
    auto mi = find_machine_instructions(name);
    assert(mi);
    return *mi;
}

std::span<const machine_instruction> instruction::all_machine_instructions() { return machine_instructions; }

constexpr auto mi_BC = find_mi("BC");
constexpr auto mi_BCR = find_mi("BCR");
constexpr auto mi_BIC = find_mi("BIC");
constexpr auto mi_BRAS = find_mi("BRAS");
constexpr auto mi_BRASL = find_mi("BRASL");
constexpr auto mi_BRC = find_mi("BRC");
constexpr auto mi_BRCL = find_mi("BRCL");
constexpr auto mi_BRCT = find_mi("BRCT");
constexpr auto mi_BRCTG = find_mi("BRCTG");
constexpr auto mi_BRXH = find_mi("BRXH");
constexpr auto mi_BRXHG = find_mi("BRXHG");
constexpr auto mi_BRXLE = find_mi("BRXLE");
constexpr auto mi_BRXLG = find_mi("BRXLG");
constexpr auto mi_CGIB = find_mi("CGIB");
constexpr auto mi_CGIJ = find_mi("CGIJ");
constexpr auto mi_CGIT = find_mi("CGIT");
constexpr auto mi_CGRB = find_mi("CGRB");
constexpr auto mi_CGRJ = find_mi("CGRJ");
constexpr auto mi_CGRT = find_mi("CGRT");
constexpr auto mi_CIB = find_mi("CIB");
constexpr auto mi_CIJ = find_mi("CIJ");
constexpr auto mi_CIT = find_mi("CIT");
constexpr auto mi_CLFIT = find_mi("CLFIT");
constexpr auto mi_CLGIB = find_mi("CLGIB");
constexpr auto mi_CLGIJ = find_mi("CLGIJ");
constexpr auto mi_CLGIT = find_mi("CLGIT");
constexpr auto mi_CLGRB = find_mi("CLGRB");
constexpr auto mi_CLGRJ = find_mi("CLGRJ");
constexpr auto mi_CLGRT = find_mi("CLGRT");
constexpr auto mi_CLGT = find_mi("CLGT");
constexpr auto mi_CLIB = find_mi("CLIB");
constexpr auto mi_CLIJ = find_mi("CLIJ");
constexpr auto mi_CLRB = find_mi("CLRB");
constexpr auto mi_CLRJ = find_mi("CLRJ");
constexpr auto mi_CLRT = find_mi("CLRT");
constexpr auto mi_CLT = find_mi("CLT");
constexpr auto mi_CRB = find_mi("CRB");
constexpr auto mi_CRJ = find_mi("CRJ");
constexpr auto mi_CRT = find_mi("CRT");
constexpr auto mi_LOC = find_mi("LOC");
constexpr auto mi_LOCFH = find_mi("LOCFH");
constexpr auto mi_LOCFHR = find_mi("LOCFHR");
constexpr auto mi_LOCG = find_mi("LOCG");
constexpr auto mi_LOCGHI = find_mi("LOCGHI");
constexpr auto mi_LOCGR = find_mi("LOCGR");
constexpr auto mi_LOCHHI = find_mi("LOCHHI");
constexpr auto mi_LOCHI = find_mi("LOCHI");
constexpr auto mi_LOCR = find_mi("LOCR");
constexpr auto mi_NOGRK = find_mi("NOGRK");
constexpr auto mi_NORK = find_mi("NORK");
constexpr auto mi_RISBHGZ = find_mi("RISBHGZ");
constexpr auto mi_RISBLGZ = find_mi("RISBLGZ");
constexpr auto mi_RNSBG = find_mi("RNSBG");
constexpr auto mi_ROSBG = find_mi("ROSBG");
constexpr auto mi_RXSBG = find_mi("RXSBG");
constexpr auto mi_SELFHR = find_mi("SELFHR");
constexpr auto mi_SELGR = find_mi("SELGR");
constexpr auto mi_SELR = find_mi("SELR");
constexpr auto mi_STOC = find_mi("STOC");
constexpr auto mi_STOCFH = find_mi("STOCFH");
constexpr auto mi_STOCG = find_mi("STOCG");
constexpr auto mi_VA = find_mi("VA");
constexpr auto mi_VAC = find_mi("VAC");
constexpr auto mi_VACC = find_mi("VACC");
constexpr auto mi_VACCC = find_mi("VACCC");
constexpr auto mi_VAVG = find_mi("VAVG");
constexpr auto mi_VAVGL = find_mi("VAVGL");
constexpr auto mi_VCEQ = find_mi("VCEQ");
constexpr auto mi_VCFPL = find_mi("VCFPL");
constexpr auto mi_VCFPS = find_mi("VCFPS");
constexpr auto mi_VCH = find_mi("VCH");
constexpr auto mi_VCHL = find_mi("VCHL");
constexpr auto mi_VCLFP = find_mi("VCLFP");
constexpr auto mi_VCLGD = find_mi("VCLGD");
constexpr auto mi_VCLZ = find_mi("VCLZ");
constexpr auto mi_VCSFP = find_mi("VCSFP");
constexpr auto mi_VEC = find_mi("VEC");
constexpr auto mi_VECL = find_mi("VECL");
constexpr auto mi_VERIM = find_mi("VERIM");
constexpr auto mi_VERLL = find_mi("VERLL");
constexpr auto mi_VERLLV = find_mi("VERLLV");
constexpr auto mi_VESL = find_mi("VESL");
constexpr auto mi_VESLV = find_mi("VESLV");
constexpr auto mi_VESRA = find_mi("VESRA");
constexpr auto mi_VESRAV = find_mi("VESRAV");
constexpr auto mi_VESRL = find_mi("VESRL");
constexpr auto mi_VESRLV = find_mi("VESRLV");
constexpr auto mi_VFA = find_mi("VFA");
constexpr auto mi_VFAE = find_mi("VFAE");
constexpr auto mi_VFCE = find_mi("VFCE");
constexpr auto mi_VFCH = find_mi("VFCH");
constexpr auto mi_VFCHE = find_mi("VFCHE");
constexpr auto mi_VFD = find_mi("VFD");
constexpr auto mi_VFEE = find_mi("VFEE");
constexpr auto mi_VFENE = find_mi("VFENE");
constexpr auto mi_VFI = find_mi("VFI");
constexpr auto mi_VFLL = find_mi("VFLL");
constexpr auto mi_VFLR = find_mi("VFLR");
constexpr auto mi_VFM = find_mi("VFM");
constexpr auto mi_VFMA = find_mi("VFMA");
constexpr auto mi_VFMAX = find_mi("VFMAX");
constexpr auto mi_VFMIN = find_mi("VFMIN");
constexpr auto mi_VFMS = find_mi("VFMS");
constexpr auto mi_VFNMA = find_mi("VFNMA");
constexpr auto mi_VFNMS = find_mi("VFNMS");
constexpr auto mi_VFPSO = find_mi("VFPSO");
constexpr auto mi_VFS = find_mi("VFS");
constexpr auto mi_VFSQ = find_mi("VFSQ");
constexpr auto mi_VFTCI = find_mi("VFTCI");
constexpr auto mi_VGBM = find_mi("VGBM");
constexpr auto mi_VGFM = find_mi("VGFM");
constexpr auto mi_VGFMA = find_mi("VGFMA");
constexpr auto mi_VGM = find_mi("VGM");
constexpr auto mi_VISTR = find_mi("VISTR");
constexpr auto mi_VLBR = find_mi("VLBR");
constexpr auto mi_VLBRREP = find_mi("VLBRREP");
constexpr auto mi_VLC = find_mi("VLC");
constexpr auto mi_VLER = find_mi("VLER");
constexpr auto mi_VLGV = find_mi("VLGV");
constexpr auto mi_VLLEBRZ = find_mi("VLLEBRZ");
constexpr auto mi_VLLEZ = find_mi("VLLEZ");
constexpr auto mi_VLP = find_mi("VLP");
constexpr auto mi_VLREP = find_mi("VLREP");
constexpr auto mi_VLVG = find_mi("VLVG");
constexpr auto mi_VMAE = find_mi("VMAE");
constexpr auto mi_VMAH = find_mi("VMAH");
constexpr auto mi_VMAL = find_mi("VMAL");
constexpr auto mi_VMALE = find_mi("VMALE");
constexpr auto mi_VMALH = find_mi("VMALH");
constexpr auto mi_VMALO = find_mi("VMALO");
constexpr auto mi_VMAO = find_mi("VMAO");
constexpr auto mi_VME = find_mi("VME");
constexpr auto mi_VMH = find_mi("VMH");
constexpr auto mi_VML = find_mi("VML");
constexpr auto mi_VMLE = find_mi("VMLE");
constexpr auto mi_VMLH = find_mi("VMLH");
constexpr auto mi_VMLO = find_mi("VMLO");
constexpr auto mi_VMN = find_mi("VMN");
constexpr auto mi_VMNL = find_mi("VMNL");
constexpr auto mi_VMO = find_mi("VMO");
constexpr auto mi_VMRH = find_mi("VMRH");
constexpr auto mi_VMRL = find_mi("VMRL");
constexpr auto mi_VMSL = find_mi("VMSL");
constexpr auto mi_VMX = find_mi("VMX");
constexpr auto mi_VMXL = find_mi("VMXL");
constexpr auto mi_VNO = find_mi("VNO");
constexpr auto mi_VPK = find_mi("VPK");
constexpr auto mi_VPKLS = find_mi("VPKLS");
constexpr auto mi_VPKS = find_mi("VPKS");
constexpr auto mi_VPOPCT = find_mi("VPOPCT");
constexpr auto mi_VREP = find_mi("VREP");
constexpr auto mi_VREPI = find_mi("VREPI");
constexpr auto mi_VS = find_mi("VS");
constexpr auto mi_VSBCBI = find_mi("VSBCBI");
constexpr auto mi_VSBI = find_mi("VSBI");
constexpr auto mi_VSCBI = find_mi("VSCBI");
constexpr auto mi_VSEG = find_mi("VSEG");
constexpr auto mi_VSTBR = find_mi("VSTBR");
constexpr auto mi_VSTEBRF = find_mi("VSTEBRF");
constexpr auto mi_VSTEBRG = find_mi("VSTEBRG");
constexpr auto mi_VSTER = find_mi("VSTER");
constexpr auto mi_VSTRC = find_mi("VSTRC");
constexpr auto mi_VSTRS = find_mi("VSTRS");
constexpr auto mi_VSUM = find_mi("VSUM");
constexpr auto mi_VSUMG = find_mi("VSUMG");
constexpr auto mi_VSUMQ = find_mi("VSUMQ");
constexpr auto mi_VUPH = find_mi("VUPH");
constexpr auto mi_VUPL = find_mi("VUPL");
constexpr auto mi_VUPLH = find_mi("VUPLH");
constexpr auto mi_VUPLL = find_mi("VUPLL");
constexpr auto mi_WFC = find_mi("WFC");
constexpr auto mi_WFK = find_mi("WFK");

constexpr mnemonic_code mnemonic_codes[] = {
    { "B", mi_BC, { { 0, 15 } } },
    { "BE", mi_BC, { { 0, 8 } } },
    { "BER", mi_BCR, { { 0, 8 } } },
    { "BH", mi_BC, { { 0, 2 } } },
    { "BHR", mi_BCR, { { 0, 2 } } },
    { "BI", mi_BIC, { { 0, 15 } } },
    { "BIE", mi_BIC, { { 0, 8 } } },
    { "BIH", mi_BIC, { { 0, 2 } } },
    { "BIL", mi_BIC, { { 0, 4 } } },
    { "BIM", mi_BIC, { { 0, 4 } } },
    { "BINE", mi_BIC, { { 0, 7 } } },
    { "BINH", mi_BIC, { { 0, 13 } } },
    { "BINL", mi_BIC, { { 0, 11 } } },
    { "BINM", mi_BIC, { { 0, 11 } } },
    { "BINO", mi_BIC, { { 0, 14 } } },
    { "BINP", mi_BIC, { { 0, 13 } } },
    { "BINZ", mi_BIC, { { 0, 7 } } },
    { "BIO", mi_BIC, { { 0, 1 } } },
    { "BIP", mi_BIC, { { 0, 2 } } },
    { "BIZ", mi_BIC, { { 0, 8 } } },
    { "BL", mi_BC, { { 0, 4 } } },
    { "BLR", mi_BCR, { { 0, 4 } } },
    { "BM", mi_BC, { { 0, 4 } } },
    { "BMR", mi_BCR, { { 0, 4 } } },
    { "BNE", mi_BC, { { 0, 7 } } },
    { "BNER", mi_BCR, { { 0, 7 } } },
    { "BNH", mi_BC, { { 0, 13 } } },
    { "BNHR", mi_BCR, { { 0, 13 } } },
    { "BNL", mi_BC, { { 0, 11 } } },
    { "BNLR", mi_BCR, { { 0, 11 } } },
    { "BNM", mi_BC, { { 0, 11 } } },
    { "BNMR", mi_BCR, { { 0, 11 } } },
    { "BNO", mi_BC, { { 0, 14 } } },
    { "BNOR", mi_BCR, { { 0, 14 } } },
    { "BNP", mi_BC, { { 0, 13 } } },
    { "BNPR", mi_BCR, { { 0, 13 } } },
    { "BNZ", mi_BC, { { 0, 7 } } },
    { "BNZR", mi_BCR, { { 0, 7 } } },
    { "BO", mi_BC, { { 0, 1 } } },
    { "BOR", mi_BCR, { { 0, 1 } } },
    { "BP", mi_BC, { { 0, 2 } } },
    { "BPR", mi_BCR, { { 0, 2 } } },
    { "BR", mi_BCR, { { 0, 15 } } },
    { "BRE", mi_BRC, { { 0, 8 } } },
    { "BREL", mi_BRCL, { { 0, 8 } } },
    { "BRH", mi_BRC, { { 0, 2 } } },
    { "BRHL", mi_BRCL, { { 0, 2 } } },
    { "BRL", mi_BRC, { { 0, 4 } } },
    { "BRLL", mi_BRCL, { { 0, 4 } } },
    { "BRM", mi_BRC, { { 0, 4 } } },
    { "BRML", mi_BRCL, { { 0, 4 } } },
    { "BRNE", mi_BRC, { { 0, 7 } } },
    { "BRNEL", mi_BRCL, { { 0, 7 } } },
    { "BRNH", mi_BRC, { { 0, 13 } } },
    { "BRNHL", mi_BRCL, { { 0, 13 } } },
    { "BRNL", mi_BRC, { { 0, 11 } } },
    { "BRNLL", mi_BRCL, { { 0, 11 } } },
    { "BRNM", mi_BRC, { { 0, 11 } } },
    { "BRNML", mi_BRCL, { { 0, 11 } } },
    { "BRNO", mi_BRC, { { 0, 14 } } },
    { "BRNOL", mi_BRCL, { { 0, 14 } } },
    { "BRNP", mi_BRC, { { 0, 13 } } },
    { "BRNPL", mi_BRCL, { { 0, 13 } } },
    { "BRNZ", mi_BRC, { { 0, 7 } } },
    { "BRNZL", mi_BRCL, { { 0, 7 } } },
    { "BRO", mi_BRC, { { 0, 1 } } },
    { "BROL", mi_BRCL, { { 0, 1 } } },
    { "BRP", mi_BRC, { { 0, 2 } } },
    { "BRPL", mi_BRCL, { { 0, 2 } } },
    { "BRU", mi_BRC, { { 0, 15 } } },
    { "BRUL", mi_BRCL, { { 0, 15 } } },
    { "BRZ", mi_BRC, { { 0, 8 } } },
    { "BRZL", mi_BRCL, { { 0, 8 } } },
    { "BZ", mi_BC, { { 0, 8 } } },
    { "BZR", mi_BCR, { { 0, 8 } } },
    { "CGIBE", mi_CGIB, { { 2, 8 } } },
    { "CGIBH", mi_CGIB, { { 2, 2 } } },
    { "CGIBL", mi_CGIB, { { 2, 4 } } },
    { "CGIBNE", mi_CGIB, { { 2, 6 } } },
    { "CGIBNH", mi_CGIB, { { 2, 12 } } },
    { "CGIBNL", mi_CGIB, { { 2, 10 } } },
    { "CGIJE", mi_CGIJ, { { 2, 8 } } },
    { "CGIJH", mi_CGIJ, { { 2, 2 } } },
    { "CGIJL", mi_CGIJ, { { 2, 4 } } },
    { "CGIJNE", mi_CGIJ, { { 2, 6 } } },
    { "CGIJNH", mi_CGIJ, { { 2, 12 } } },
    { "CGIJNL", mi_CGIJ, { { 2, 10 } } },
    { "CGITE", mi_CGIT, { { 2, 8 } } },
    { "CGITH", mi_CGIT, { { 2, 2 } } },
    { "CGITL", mi_CGIT, { { 2, 4 } } },
    { "CGITNE", mi_CGIT, { { 2, 6 } } },
    { "CGITNH", mi_CGIT, { { 2, 12 } } },
    { "CGITNL", mi_CGIT, { { 2, 10 } } },
    { "CGRBE", mi_CGRB, { { 2, 8 } } },
    { "CGRBH", mi_CGRB, { { 2, 2 } } },
    { "CGRBL", mi_CGRB, { { 2, 4 } } },
    { "CGRBNE", mi_CGRB, { { 2, 6 } } },
    { "CGRBNH", mi_CGRB, { { 2, 12 } } },
    { "CGRBNL", mi_CGRB, { { 2, 10 } } },
    { "CGRJE", mi_CGRJ, { { 2, 8 } } },
    { "CGRJH", mi_CGRJ, { { 2, 2 } } },
    { "CGRJL", mi_CGRJ, { { 2, 4 } } },
    { "CGRJNE", mi_CGRJ, { { 2, 6 } } },
    { "CGRJNH", mi_CGRJ, { { 2, 12 } } },
    { "CGRJNL", mi_CGRJ, { { 2, 10 } } },
    { "CGRTE", mi_CGRT, { { 2, 8 } } },
    { "CGRTH", mi_CGRT, { { 2, 2 } } },
    { "CGRTL", mi_CGRT, { { 2, 4 } } },
    { "CGRTNE", mi_CGRT, { { 2, 6 } } },
    { "CGRTNH", mi_CGRT, { { 2, 12 } } },
    { "CGRTNL", mi_CGRT, { { 2, 10 } } },
    { "CIBE", mi_CIB, { { 2, 8 } } },
    { "CIBH", mi_CIB, { { 2, 2 } } },
    { "CIBL", mi_CIB, { { 2, 4 } } },
    { "CIBNE", mi_CIB, { { 2, 6 } } },
    { "CIBNH", mi_CIB, { { 2, 12 } } },
    { "CIBNL", mi_CIB, { { 2, 10 } } },
    { "CIJE", mi_CIJ, { { 2, 8 } } },
    { "CIJH", mi_CIJ, { { 2, 2 } } },
    { "CIJL", mi_CIJ, { { 2, 4 } } },
    { "CIJNE", mi_CIJ, { { 2, 6 } } },
    { "CIJNH", mi_CIJ, { { 2, 12 } } },
    { "CIJNL", mi_CIJ, { { 2, 10 } } },
    { "CITE", mi_CIT, { { 2, 8 } } },
    { "CITH", mi_CIT, { { 2, 2 } } },
    { "CITL", mi_CIT, { { 2, 4 } } },
    { "CITNE", mi_CIT, { { 2, 6 } } },
    { "CITNH", mi_CIT, { { 2, 12 } } },
    { "CITNL", mi_CIT, { { 2, 10 } } },
    { "CLFITE", mi_CLFIT, { { 2, 8 } } },
    { "CLFITH", mi_CLFIT, { { 2, 2 } } },
    { "CLFITL", mi_CLFIT, { { 2, 4 } } },
    { "CLFITNE", mi_CLFIT, { { 2, 6 } } },
    { "CLFITNH", mi_CLFIT, { { 2, 12 } } },
    { "CLFITNL", mi_CLFIT, { { 2, 10 } } },
    { "CLGIBE", mi_CLGIB, { { 2, 8 } } },
    { "CLGIBH", mi_CLGIB, { { 2, 2 } } },
    { "CLGIBL", mi_CLGIB, { { 2, 4 } } },
    { "CLGIBNE", mi_CLGIB, { { 2, 6 } } },
    { "CLGIBNH", mi_CLGIB, { { 2, 12 } } },
    { "CLGIBNL", mi_CLGIB, { { 2, 10 } } },
    { "CLGIJE", mi_CLGIJ, { { 2, 8 } } },
    { "CLGIJH", mi_CLGIJ, { { 2, 2 } } },
    { "CLGIJL", mi_CLGIJ, { { 2, 4 } } },
    { "CLGIJNE", mi_CLGIJ, { { 2, 6 } } },
    { "CLGIJNH", mi_CLGIJ, { { 2, 12 } } },
    { "CLGIJNL", mi_CLGIJ, { { 2, 10 } } },
    { "CLGITE", mi_CLGIT, { { 2, 8 } } },
    { "CLGITH", mi_CLGIT, { { 2, 2 } } },
    { "CLGITL", mi_CLGIT, { { 2, 4 } } },
    { "CLGITNE", mi_CLGIT, { { 2, 6 } } },
    { "CLGITNH", mi_CLGIT, { { 2, 12 } } },
    { "CLGITNL", mi_CLGIT, { { 2, 10 } } },
    { "CLGRBE", mi_CLGRB, { { 2, 8 } } },
    { "CLGRBH", mi_CLGRB, { { 2, 2 } } },
    { "CLGRBL", mi_CLGRB, { { 2, 4 } } },
    { "CLGRBNE", mi_CLGRB, { { 2, 6 } } },
    { "CLGRBNH", mi_CLGRB, { { 2, 12 } } },
    { "CLGRBNL", mi_CLGRB, { { 2, 10 } } },
    { "CLGRJE", mi_CLGRJ, { { 2, 8 } } },
    { "CLGRJH", mi_CLGRJ, { { 2, 2 } } },
    { "CLGRJL", mi_CLGRJ, { { 2, 4 } } },
    { "CLGRJNE", mi_CLGRJ, { { 2, 6 } } },
    { "CLGRJNH", mi_CLGRJ, { { 2, 12 } } },
    { "CLGRJNL", mi_CLGRJ, { { 2, 10 } } },
    { "CLGRTE", mi_CLGRT, { { 2, 8 } } },
    { "CLGRTH", mi_CLGRT, { { 2, 2 } } },
    { "CLGRTL", mi_CLGRT, { { 2, 4 } } },
    { "CLGRTNE", mi_CLGRT, { { 2, 6 } } },
    { "CLGRTNH", mi_CLGRT, { { 2, 12 } } },
    { "CLGRTNL", mi_CLGRT, { { 2, 10 } } },
    { "CLGTE", mi_CLGT, { { 1, 8 } } },
    { "CLGTH", mi_CLGT, { { 1, 2 } } },
    { "CLGTL", mi_CLGT, { { 1, 4 } } },
    { "CLGTNE", mi_CLGT, { { 1, 6 } } },
    { "CLGTNH", mi_CLGT, { { 1, 12 } } },
    { "CLGTNL", mi_CLGT, { { 1, 10 } } },
    { "CLIBE", mi_CLIB, { { 2, 8 } } },
    { "CLIBH", mi_CLIB, { { 2, 2 } } },
    { "CLIBL", mi_CLIB, { { 2, 4 } } },
    { "CLIBNE", mi_CLIB, { { 2, 6 } } },
    { "CLIBNH", mi_CLIB, { { 2, 12 } } },
    { "CLIBNL", mi_CLIB, { { 2, 10 } } },
    { "CLIJE", mi_CLIJ, { { 2, 8 } } },
    { "CLIJH", mi_CLIJ, { { 2, 2 } } },
    { "CLIJL", mi_CLIJ, { { 2, 4 } } },
    { "CLIJNE", mi_CLIJ, { { 2, 6 } } },
    { "CLIJNH", mi_CLIJ, { { 2, 12 } } },
    { "CLIJNL", mi_CLIJ, { { 2, 10 } } },
    { "CLRBE", mi_CLRB, { { 2, 8 } } },
    { "CLRBH", mi_CLRB, { { 2, 2 } } },
    { "CLRBL", mi_CLRB, { { 2, 4 } } },
    { "CLRBNE", mi_CLRB, { { 2, 6 } } },
    { "CLRBNH", mi_CLRB, { { 2, 12 } } },
    { "CLRBNL", mi_CLRB, { { 2, 10 } } },
    { "CLRJE", mi_CLRJ, { { 2, 8 } } },
    { "CLRJH", mi_CLRJ, { { 2, 2 } } },
    { "CLRJL", mi_CLRJ, { { 2, 4 } } },
    { "CLRJNE", mi_CLRJ, { { 2, 6 } } },
    { "CLRJNH", mi_CLRJ, { { 2, 12 } } },
    { "CLRJNL", mi_CLRJ, { { 2, 10 } } },
    { "CLRTE", mi_CLRT, { { 2, 8 } } },
    { "CLRTH", mi_CLRT, { { 2, 2 } } },
    { "CLRTL", mi_CLRT, { { 2, 4 } } },
    { "CLRTNE", mi_CLRT, { { 2, 6 } } },
    { "CLRTNH", mi_CLRT, { { 2, 12 } } },
    { "CLRTNL", mi_CLRT, { { 2, 10 } } },
    { "CLTE", mi_CLT, { { 1, 8 } } },
    { "CLTH", mi_CLT, { { 1, 2 } } },
    { "CLTL", mi_CLT, { { 1, 4 } } },
    { "CLTNE", mi_CLT, { { 1, 6 } } },
    { "CLTNH", mi_CLT, { { 1, 12 } } },
    { "CLTNL", mi_CLT, { { 1, 10 } } },
    { "CRBE", mi_CRB, { { 2, 8 } } },
    { "CRBH", mi_CRB, { { 2, 2 } } },
    { "CRBL", mi_CRB, { { 2, 4 } } },
    { "CRBNE", mi_CRB, { { 2, 6 } } },
    { "CRBNH", mi_CRB, { { 2, 12 } } },
    { "CRBNL", mi_CRB, { { 2, 10 } } },
    { "CRJE", mi_CRJ, { { 2, 8 } } },
    { "CRJH", mi_CRJ, { { 2, 2 } } },
    { "CRJL", mi_CRJ, { { 2, 4 } } },
    { "CRJNE", mi_CRJ, { { 2, 6 } } },
    { "CRJNH", mi_CRJ, { { 2, 12 } } },
    { "CRJNL", mi_CRJ, { { 2, 10 } } },
    { "CRTE", mi_CRT, { { 2, 8 } } },
    { "CRTH", mi_CRT, { { 2, 2 } } },
    { "CRTL", mi_CRT, { { 2, 4 } } },
    { "CRTNE", mi_CRT, { { 2, 6 } } },
    { "CRTNH", mi_CRT, { { 2, 12 } } },
    { "CRTNL", mi_CRT, { { 2, 10 } } },
    { "J", mi_BRC, { { 0, 15 } } },
    { "JAS", mi_BRAS, {} },
    { "JASL", mi_BRASL, {} },
    { "JC", mi_BRC, {} },
    { "JCT", mi_BRCT, {} },
    { "JCTG", mi_BRCTG, {} },
    { "JE", mi_BRC, { { 0, 8 } } },
    { "JH", mi_BRC, { { 0, 2 } } },
    { "JL", mi_BRC, { { 0, 4 } } },
    { "JLE", mi_BRCL, { { 0, 8 } } },
    { "JLH", mi_BRCL, { { 0, 2 } } },
    { "JLL", mi_BRCL, { { 0, 4 } } },
    { "JLM", mi_BRCL, { { 0, 4 } } },
    { "JLNE", mi_BRCL, { { 0, 7 } } },
    { "JLNH", mi_BRCL, { { 0, 13 } } },
    { "JLNL", mi_BRCL, { { 0, 11 } } },
    { "JLNM", mi_BRCL, { { 0, 11 } } },
    { "JLNO", mi_BRCL, { { 0, 14 } } },
    { "JLNOP", mi_BRCL, { { 0, 0 } } },
    { "JLNP", mi_BRCL, { { 0, 13 } } },
    { "JLNZ", mi_BRCL, { { 0, 7 } } },
    { "JLO", mi_BRCL, { { 0, 1 } } },
    { "JLP", mi_BRCL, { { 0, 2 } } },
    { "JLU", mi_BRCL, { { 0, 15 } } },
    { "JLZ", mi_BRCL, { { 0, 8 } } },
    { "JM", mi_BRC, { { 0, 4 } } },
    { "JNE", mi_BRC, { { 0, 7 } } },
    { "JNH", mi_BRC, { { 0, 13 } } },
    { "JNL", mi_BRC, { { 0, 11 } } },
    { "JNM", mi_BRC, { { 0, 11 } } },
    { "JNO", mi_BRC, { { 0, 14 } } },
    { "JNOP", mi_BRC, { { 0, 0 } } },
    { "JNP", mi_BRC, { { 0, 13 } } },
    { "JNZ", mi_BRC, { { 0, 7 } } },
    { "JO", mi_BRC, { { 0, 1 } } },
    { "JP", mi_BRC, { { 0, 2 } } },
    { "JXH", mi_BRXH, {} },
    { "JXHG", mi_BRXHG, {} },
    { "JXLE", mi_BRXLE, {} },
    { "JXLEG", mi_BRXLG, {} },
    { "JZ", mi_BRC, { { 0, 8 } } },
    { "LDRV", mi_VLLEBRZ, { { 2, 3 } } },
    { "LERV", mi_VLLEBRZ, { { 2, 6 } } },
    { "LHHR", mi_RISBHGZ, { { 2, 0 }, { 3, 31 } } },
    { "LHLR", mi_RISBHGZ, { { 2, 0 }, { 3, 31 }, { 4, 32 } } },
    { "LLCHHR", mi_RISBHGZ, { { 2, 24 }, { 3, 31 } } },
    { "LLCHLR", mi_RISBHGZ, { { 2, 24 }, { 3, 31 }, { 4, 32 } } },
    { "LLCLHR", mi_RISBLGZ, { { 2, 24 }, { 3, 31 }, { 4, 32 } } },
    { "LLHFR", mi_RISBLGZ, { { 2, 0 }, { 3, 31 }, { 4, 32 } } },
    { "LLHHHR", mi_RISBHGZ, { { 2, 16 }, { 3, 31 } } },
    { "LLHHLR", mi_RISBHGZ, { { 2, 16 }, { 3, 31 }, { 4, 32 } } },
    { "LLHLHR", mi_RISBLGZ, { { 2, 16 }, { 3, 31 }, { 4, 32 } } },
    { "LOCE", mi_LOC, { { 2, 8 } } },
    { "LOCFHE", mi_LOCFH, { { 2, 8 } } },
    { "LOCFHH", mi_LOCFH, { { 2, 2 } } },
    { "LOCFHL", mi_LOCFH, { { 2, 4 } } },
    { "LOCFHNE", mi_LOCFH, { { 2, 7 } } },
    { "LOCFHNH", mi_LOCFH, { { 2, 13 } } },
    { "LOCFHNL", mi_LOCFH, { { 2, 11 } } },
    { "LOCFHNO", mi_LOCFH, { { 2, 14 } } },
    { "LOCFHO", mi_LOCFH, { { 2, 1 } } },
    { "LOCFHRE", mi_LOCFHR, { { 2, 8 } } },
    { "LOCFHRH", mi_LOCFHR, { { 2, 2 } } },
    { "LOCFHRL", mi_LOCFHR, { { 2, 4 } } },
    { "LOCFHRNE", mi_LOCFHR, { { 2, 7 } } },
    { "LOCFHRNH", mi_LOCFHR, { { 2, 13 } } },
    { "LOCFHRNL", mi_LOCFHR, { { 2, 11 } } },
    { "LOCFHRNO", mi_LOCFHR, { { 2, 14 } } },
    { "LOCFHRO", mi_LOCFHR, { { 2, 1 } } },
    { "LOCGE", mi_LOCG, { { 2, 8 } } },
    { "LOCGH", mi_LOCG, { { 2, 2 } } },
    { "LOCGHIE", mi_LOCGHI, { { 2, 8 } } },
    { "LOCGHIH", mi_LOCGHI, { { 2, 2 } } },
    { "LOCGHIL", mi_LOCGHI, { { 2, 4 } } },
    { "LOCGHINE", mi_LOCGHI, { { 2, 7 } } },
    { "LOCGHINH", mi_LOCGHI, { { 2, 13 } } },
    { "LOCGHINL", mi_LOCGHI, { { 2, 11 } } },
    { "LOCGHINO", mi_LOCGHI, { { 2, 14 } } },
    { "LOCGHIO", mi_LOCGHI, { { 2, 1 } } },
    { "LOCGL", mi_LOCG, { { 2, 4 } } },
    { "LOCGNE", mi_LOCG, { { 2, 6 } } },
    { "LOCGNH", mi_LOCG, { { 2, 12 } } },
    { "LOCGNL", mi_LOCG, { { 2, 10 } } },
    { "LOCGNO", mi_LOCG, { { 2, 14 } } },
    { "LOCGO", mi_LOCG, { { 2, 1 } } },
    { "LOCGRE", mi_LOCGR, { { 2, 8 } } },
    { "LOCGRH", mi_LOCGR, { { 2, 2 } } },
    { "LOCGRL", mi_LOCGR, { { 2, 4 } } },
    { "LOCGRNE", mi_LOCGR, { { 2, 6 } } },
    { "LOCGRNH", mi_LOCGR, { { 2, 12 } } },
    { "LOCGRNL", mi_LOCGR, { { 2, 10 } } },
    { "LOCGRNO", mi_LOCGR, { { 2, 14 } } },
    { "LOCGRO", mi_LOCGR, { { 2, 1 } } },
    { "LOCH", mi_LOC, { { 2, 2 } } },
    { "LOCHHIE", mi_LOCHHI, { { 2, 8 } } },
    { "LOCHHIH", mi_LOCHHI, { { 2, 2 } } },
    { "LOCHHIL", mi_LOCHHI, { { 2, 4 } } },
    { "LOCHHINE", mi_LOCHHI, { { 2, 7 } } },
    { "LOCHHINH", mi_LOCHHI, { { 2, 13 } } },
    { "LOCHHINL", mi_LOCHHI, { { 2, 11 } } },
    { "LOCHHINO", mi_LOCHHI, { { 2, 14 } } },
    { "LOCHHIO", mi_LOCHHI, { { 2, 1 } } },
    { "LOCHIE", mi_LOCHI, { { 2, 8 } } },
    { "LOCHIH", mi_LOCHI, { { 2, 2 } } },
    { "LOCHIL", mi_LOCHI, { { 2, 4 } } },
    { "LOCHINE", mi_LOCHI, { { 2, 7 } } },
    { "LOCHINH", mi_LOCHI, { { 2, 13 } } },
    { "LOCHINL", mi_LOCHI, { { 2, 11 } } },
    { "LOCHINO", mi_LOCHI, { { 2, 14 } } },
    { "LOCHIO", mi_LOCHI, { { 2, 1 } } },
    { "LOCL", mi_LOC, { { 2, 4 } } },
    { "LOCNE", mi_LOC, { { 2, 6 } } },
    { "LOCNH", mi_LOC, { { 2, 12 } } },
    { "LOCNL", mi_LOC, { { 2, 10 } } },
    { "LOCNO", mi_LOC, { { 2, 14 } } },
    { "LOCO", mi_LOC, { { 2, 1 } } },
    { "LOCRE", mi_LOCR, { { 2, 8 } } },
    { "LOCRH", mi_LOCR, { { 2, 2 } } },
    { "LOCRL", mi_LOCR, { { 2, 4 } } },
    { "LOCRNE", mi_LOCR, { { 2, 6 } } },
    { "LOCRNH", mi_LOCR, { { 2, 12 } } },
    { "LOCRNL", mi_LOCR, { { 2, 10 } } },
    { "LOCRNO", mi_LOCR, { { 2, 14 } } },
    { "LOCRO", mi_LOCR, { { 2, 1 } } },
    { "NHHR", mi_RNSBG, { { 2, 0 }, { 3, 31 } } },
    { "NHLR", mi_RNSBG, { { 2, 0 }, { 3, 31 }, { 4, 32 } } },
    { "NLHR", mi_RNSBG, { { 2, 32 }, { 3, 63 }, { 4, 32 } } },
    { "NOP", mi_BC, { { 0, 0 } } },
    { "NOPR", mi_BCR, { { 0, 0 } } },
    { "NOTGR", mi_NOGRK, { { 2, 0 } } }, // operand with index 2 was omitted
    { "NOTR", mi_NORK, { { 2, 0 } } }, // operand with index 2 was omitted
    { "OHHR", mi_ROSBG, { { 2, 0 }, { 3, 31 } } },
    { "OHLR", mi_ROSBG, { { 2, 0 }, { 3, 31 }, { 4, 32 } } },
    { "OLHR", mi_ROSBG, { { 2, 32 }, { 3, 63 }, { 4, 32 } } },
    { "SELFHRE", mi_SELFHR, { { 3, 8 } } },
    { "SELFHRH", mi_SELFHR, { { 3, 2 } } },
    { "SELFHRL", mi_SELFHR, { { 3, 4 } } },
    { "SELFHRNE", mi_SELFHR, { { 3, 7 } } },
    { "SELFHRNH", mi_SELFHR, { { 3, 13 } } },
    { "SELFHRNL", mi_SELFHR, { { 3, 11 } } },
    { "SELFHRNO", mi_SELFHR, { { 3, 14 } } },
    { "SELFHRO", mi_SELFHR, { { 3, 1 } } },
    { "SELGRE", mi_SELGR, { { 3, 8 } } },
    { "SELGRH", mi_SELGR, { { 3, 2 } } },
    { "SELGRL", mi_SELGR, { { 3, 4 } } },
    { "SELGRNE", mi_SELGR, { { 3, 7 } } },
    { "SELGRNH", mi_SELGR, { { 3, 13 } } },
    { "SELGRNL", mi_SELGR, { { 3, 11 } } },
    { "SELGRNO", mi_SELGR, { { 3, 14 } } },
    { "SELGRO", mi_SELGR, { { 3, 1 } } },
    { "SELRE", mi_SELR, { { 3, 8 } } },
    { "SELRH", mi_SELR, { { 3, 2 } } },
    { "SELRL", mi_SELR, { { 3, 4 } } },
    { "SELRNE", mi_SELR, { { 3, 7 } } },
    { "SELRNH", mi_SELR, { { 3, 13 } } },
    { "SELRNL", mi_SELR, { { 3, 11 } } },
    { "SELRNO", mi_SELR, { { 3, 14 } } },
    { "SELRO", mi_SELR, { { 3, 1 } } },
    { "STDRV", mi_VSTEBRG, { { 2, 0 } } },
    { "STERV", mi_VSTEBRF, { { 2, 0 } } },
    { "STOCE", mi_STOC, { { 2, 8 } } },
    { "STOCFHE", mi_STOCFH, { { 2, 8 } } },
    { "STOCFHH", mi_STOCFH, { { 2, 2 } } },
    { "STOCFHL", mi_STOCFH, { { 2, 4 } } },
    { "STOCFHNE", mi_STOCFH, { { 2, 7 } } },
    { "STOCFHNH", mi_STOCFH, { { 2, 13 } } },
    { "STOCFHNL", mi_STOCFH, { { 2, 11 } } },
    { "STOCFHNO", mi_STOCFH, { { 2, 14 } } },
    { "STOCFHO", mi_STOCFH, { { 2, 1 } } },
    { "STOCGE", mi_STOCG, { { 2, 8 } } },
    { "STOCGH", mi_STOCG, { { 2, 2 } } },
    { "STOCGL", mi_STOCG, { { 2, 4 } } },
    { "STOCGNE", mi_STOCG, { { 2, 6 } } },
    { "STOCGNH", mi_STOCG, { { 2, 12 } } },
    { "STOCGNL", mi_STOCG, { { 2, 10 } } },
    { "STOCGNO", mi_STOCG, { { 2, 14 } } },
    { "STOCGO", mi_STOCG, { { 2, 1 } } },
    { "STOCH", mi_STOC, { { 2, 2 } } },
    { "STOCL", mi_STOC, { { 2, 4 } } },
    { "STOCNE", mi_STOC, { { 2, 6 } } },
    { "STOCNH", mi_STOC, { { 2, 12 } } },
    { "STOCNL", mi_STOC, { { 2, 10 } } },
    { "STOCNO", mi_STOC, { { 2, 14 } } },
    { "STOCO", mi_STOC, { { 2, 1 } } },
    { "VAB", mi_VA, { { 3, 0 } } },
    { "VACCB", mi_VACC, { { 3, 0 } } },
    { "VACCCQ", mi_VACCC, { { 3, 4 } } },
    { "VACCF", mi_VACC, { { 3, 2 } } },
    { "VACCG", mi_VACC, { { 3, 3 } } },
    { "VACCH", mi_VACC, { { 3, 1 } } },
    { "VACCQ", mi_VACC, { { 3, 4 } } },
    { "VACQ", mi_VAC, { { 3, 4 } } },
    { "VAF", mi_VA, { { 3, 2 } } },
    { "VAG", mi_VA, { { 3, 3 } } },
    { "VAH", mi_VA, { { 3, 1 } } },
    { "VAQ", mi_VA, { { 3, 4 } } },
    { "VAVGB", mi_VAVG, { { 3, 0 } } },
    { "VAVGF", mi_VAVG, { { 3, 2 } } },
    { "VAVGG", mi_VAVG, { { 3, 3 } } },
    { "VAVGH", mi_VAVG, { { 3, 1 } } },
    { "VAVGLB", mi_VAVGL, { { 3, 0 } } },
    { "VAVGLF", mi_VAVGL, { { 3, 2 } } },
    { "VAVGLG", mi_VAVGL, { { 3, 3 } } },
    { "VAVGLH", mi_VAVGL, { { 3, 1 } } },
    { "VCDG", mi_VCFPS, {} },
    { "VCDGB", mi_VCFPS, { { 2, 3 } } },
    { "VCDLG", mi_VCFPL, {} },
    { "VCDLGB", mi_VCFPL, { { 2, 3 } } },
    { "VCEFB", mi_VCFPS, { { 2, 0 } } },
    { "VCELFB", mi_VCFPL, { { 2, 0 } } },
    { "VCEQB", mi_VCEQ, { { 3, 0 }, { 4, 0 } } },
    { "VCEQBS", mi_VCEQ, { { 3, 0 }, { 4, 1 } } },
    { "VCEQF", mi_VCEQ, { { 3, 2 }, { 4, 0 } } },
    { "VCEQFS", mi_VCEQ, { { 3, 2 }, { 4, 1 } } },
    { "VCEQG", mi_VCEQ, { { 3, 3 }, { 4, 0 } } },
    { "VCEQGS", mi_VCEQ, { { 3, 3 }, { 4, 1 } } },
    { "VCEQH", mi_VCEQ, { { 3, 1 }, { 4, 0 } } },
    { "VCEQHS", mi_VCEQ, { { 3, 1 }, { 4, 1 } } },
    { "VCFEB", mi_VCSFP, { { 2, 2 } } },
    { "VCGD", mi_VCSFP, {} },
    { "VCGDB", mi_VCSFP, { { 2, 3 } } },
    { "VCHB", mi_VCH, { { 3, 0 }, { 4, 0 } } },
    { "VCHBS", mi_VCH, { { 3, 0 }, { 4, 1 } } },
    { "VCHF", mi_VCH, { { 3, 2 }, { 4, 0 } } },
    { "VCHFS", mi_VCH, { { 3, 2 }, { 4, 1 } } },
    { "VCHG", mi_VCH, { { 3, 3 }, { 4, 0 } } },
    { "VCHGS", mi_VCH, { { 3, 3 }, { 4, 1 } } },
    { "VCHH", mi_VCH, { { 3, 1 }, { 4, 0 } } },
    { "VCHHS", mi_VCH, { { 3, 1 }, { 4, 1 } } },
    { "VCHLB", mi_VCHL, { { 3, 0 }, { 4, 0 } } },
    { "VCHLBS", mi_VCHL, { { 3, 0 }, { 4, 1 } } },
    { "VCHLF", mi_VCHL, { { 3, 2 }, { 4, 0 } } },
    { "VCHLFS", mi_VCHL, { { 3, 2 }, { 4, 1 } } },
    { "VCHLG", mi_VCHL, { { 3, 3 }, { 4, 0 } } },
    { "VCHLGS", mi_VCHL, { { 3, 3 }, { 4, 1 } } },
    { "VCHLH", mi_VCHL, { { 3, 1 }, { 4, 0 } } },
    { "VCHLHS", mi_VCHL, { { 3, 1 }, { 4, 1 } } },
    { "VCLFEB", mi_VCLFP, { { 2, 0 } } },
    { "VCLGDB", mi_VCLGD, { { 2, 3 } } },
    { "VCLZB", mi_VCLZ, { { 2, 0 } } },
    { "VCLZF", mi_VCLZ, { { 2, 2 } } },
    { "VCLZG", mi_VCLZ, { { 2, 3 } } },
    { "VCLZH", mi_VCLZ, { { 2, 1 } } },
    { "VECB", mi_VEC, { { 2, 0 } } },
    { "VECF", mi_VEC, { { 2, 2 } } },
    { "VECG", mi_VEC, { { 2, 3 } } },
    { "VECH", mi_VEC, { { 2, 1 } } },
    { "VECLB", mi_VECL, { { 2, 0 } } },
    { "VECLF", mi_VECL, { { 2, 2 } } },
    { "VECLG", mi_VECL, { { 2, 3 } } },
    { "VECLH", mi_VECL, { { 2, 1 } } },
    { "VERIMB", mi_VERIM, { { 4, 0 } } },
    { "VERIMF", mi_VERIM, { { 4, 2 } } },
    { "VERIMG", mi_VERIM, { { 4, 3 } } },
    { "VERIMH", mi_VERIM, { { 4, 1 } } },
    { "VERLLB", mi_VERLL, { { 3, 0 } } },
    { "VERLLF", mi_VERLL, { { 3, 2 } } },
    { "VERLLG", mi_VERLL, { { 3, 3 } } },
    { "VERLLH", mi_VERLL, { { 3, 1 } } },
    { "VERLLVB", mi_VERLLV, { { 3, 0 } } },
    { "VERLLVF", mi_VERLLV, { { 3, 2 } } },
    { "VERLLVG", mi_VERLLV, { { 3, 3 } } },
    { "VERLLVH", mi_VERLLV, { { 3, 1 } } },
    { "VESLB", mi_VESL, { { 3, 0 } } },
    { "VESLF", mi_VESL, { { 3, 2 } } },
    { "VESLG", mi_VESL, { { 3, 3 } } },
    { "VESLH", mi_VESL, { { 3, 1 } } },
    { "VESLVB", mi_VESLV, { { 3, 0 } } },
    { "VESLVF", mi_VESLV, { { 3, 2 } } },
    { "VESLVG", mi_VESLV, { { 3, 3 } } },
    { "VESLVH", mi_VESLV, { { 3, 1 } } },
    { "VESRAB", mi_VESRA, { { 3, 0 } } },
    { "VESRAF", mi_VESRA, { { 3, 2 } } },
    { "VESRAG", mi_VESRA, { { 3, 3 } } },
    { "VESRAH", mi_VESRA, { { 3, 1 } } },
    { "VESRAVB", mi_VESRAV, { { 3, 0 } } },
    { "VESRAVF", mi_VESRAV, { { 3, 2 } } },
    { "VESRAVG", mi_VESRAV, { { 3, 3 } } },
    { "VESRAVH", mi_VESRAV, { { 3, 1 } } },
    { "VESRLB", mi_VESRL, { { 3, 0 } } },
    { "VESRLF", mi_VESRL, { { 3, 2 } } },
    { "VESRLG", mi_VESRL, { { 3, 3 } } },
    { "VESRLH", mi_VESRL, { { 3, 1 } } },
    { "VESRLVB", mi_VESRLV, { { 3, 0 } } },
    { "VESRLVF", mi_VESRLV, { { 3, 2 } } },
    { "VESRLVG", mi_VESRLV, { { 3, 3 } } },
    { "VESRLVH", mi_VESRLV, { { 3, 1 } } },
    { "VFADB", mi_VFA, { { 3, 3 }, { 4, 0 } } },
    { "VFAEB", mi_VFAE, { { 3, 0 } } },
    { "VFAEBS", mi_VFAE, { { 3, 0 } } }, // operand with index 4 ORed with 1
    { "VFAEF", mi_VFAE, { { 3, 2 } } },
    { "VFAEFS", mi_VFAE, { { 3, 2 } } }, // operand with index 4 ORed with 1
    { "VFAEH", mi_VFAE, { { 3, 1 } } },
    { "VFAEHS", mi_VFAE, { { 3, 1 } } }, // operand with index 4 ORed with 1
    { "VFAEZB", mi_VFAE, { { 3, 0 } } }, // operand with index 4 ORed with 2
    { "VFAEZBS", mi_VFAE, { { 3, 0 } } }, // operand with index 4 ORed with 3
    { "VFAEZF", mi_VFAE, { { 3, 2 } } }, // operand with index 4 ORed with 2
    { "VFAEZFS", mi_VFAE, { { 3, 2 } } }, // operand with index 4 ORed with 3
    { "VFAEZH", mi_VFAE, { { 3, 1 } } }, // operand with index 4 ORed with 2
    { "VFAEZHS", mi_VFAE, { { 3, 1 } } }, // operand with index 4 ORed with 3
    { "VFASB", mi_VFA, { { 3, 2 }, { 4, 0 } } },
    { "VFCEDB", mi_VFCE, { { 3, 3 }, { 4, 0 }, { 5, 0 } } },
    { "VFCEDBS", mi_VFCE, { { 3, 3 }, { 4, 0 }, { 5, 1 } } },
    { "VFCESB", mi_VFCE, { { 3, 2 }, { 4, 0 }, { 5, 0 } } },
    { "VFCESBS", mi_VFCE, { { 3, 2 }, { 4, 0 }, { 5, 1 } } },
    { "VFCHDB", mi_VFCH, { { 3, 3 }, { 4, 0 }, { 5, 0 } } },
    { "VFCHDBS", mi_VFCH, { { 3, 3 }, { 4, 0 }, { 5, 1 } } },
    { "VFCHEDB", mi_VFCHE, { { 3, 3 }, { 4, 0 }, { 5, 0 } } },
    { "VFCHEDBS", mi_VFCHE, { { 3, 3 }, { 4, 0 }, { 5, 1 } } },
    { "VFCHESB", mi_VFCHE, { { 3, 2 }, { 4, 0 }, { 5, 0 } } },
    { "VFCHESBS", mi_VFCHE, { { 3, 2 }, { 4, 0 }, { 5, 1 } } },
    { "VFCHSB", mi_VFCH, { { 3, 2 }, { 4, 0 }, { 5, 0 } } },
    { "VFCHSBS", mi_VFCH, { { 3, 2 }, { 4, 0 }, { 5, 1 } } },
    { "VFDDB", mi_VFD, { { 3, 3 }, { 4, 0 } } },
    { "VFDSB", mi_VFD, { { 3, 2 }, { 4, 0 } } },
    { "VFEEB", mi_VFEE, { { 3, 0 } } },
    { "VFEEBS", mi_VFEE, { { 3, 0 }, { 4, 1 } } },
    { "VFEEF", mi_VFEE, { { 3, 2 } } },
    { "VFEEFS", mi_VFEE, { { 3, 2 }, { 4, 1 } } },
    { "VFEEGS", mi_VFEE, { { 3, 1 }, { 4, 1 } } },
    { "VFEEH", mi_VFEE, { { 3, 1 } } },
    { "VFEEZB", mi_VFEE, { { 3, 0 }, { 4, 2 } } },
    { "VFEEZBS", mi_VFEE, { { 3, 0 }, { 4, 3 } } },
    { "VFEEZF", mi_VFEE, { { 3, 2 }, { 4, 2 } } },
    { "VFEEZFS", mi_VFEE, { { 3, 2 }, { 4, 3 } } },
    { "VFEEZH", mi_VFEE, { { 3, 1 }, { 4, 2 } } },
    { "VFEEZHS", mi_VFEE, { { 3, 1 }, { 4, 3 } } },
    { "VFENEB", mi_VFENE, { { 3, 0 } } },
    { "VFENEBS", mi_VFENE, { { 3, 0 }, { 4, 1 } } },
    { "VFENEF", mi_VFENE, { { 3, 2 } } },
    { "VFENEFS", mi_VFENE, { { 3, 2 }, { 4, 1 } } },
    { "VFENEH", mi_VFENE, { { 3, 1 } } },
    { "VFENEHS", mi_VFENE, { { 3, 1 }, { 4, 1 } } },
    { "VFENEZB", mi_VFENE, { { 3, 0 }, { 4, 2 } } },
    { "VFENEZBS", mi_VFENE, { { 3, 0 }, { 4, 3 } } },
    { "VFENEZF", mi_VFENE, { { 3, 2 }, { 4, 2 } } },
    { "VFENEZFS", mi_VFENE, { { 3, 2 }, { 4, 3 } } },
    { "VFENEZH", mi_VFENE, { { 3, 1 }, { 4, 2 } } },
    { "VFENEZHS", mi_VFENE, { { 3, 1 }, { 4, 3 } } },
    { "VFIDB", mi_VFI, { { 2, 3 } } },
    { "VFISB", mi_VFI, { { 2, 2 } } },
    { "VFKEDB", mi_VFCE, { { 3, 3 }, { 4, 4 }, { 5, 0 } } },
    { "VFKEDBS", mi_VFCE, { { 3, 3 }, { 4, 4 }, { 5, 1 } } },
    { "VFKESB", mi_VFCE, { { 3, 2 }, { 4, 4 }, { 5, 0 } } },
    { "VFKESBS", mi_VFCE, { { 3, 2 }, { 4, 4 }, { 5, 1 } } },
    { "VFKHDB", mi_VFCH, { { 3, 3 }, { 4, 4 }, { 5, 0 } } },
    { "VFKHDBS", mi_VFCH, { { 3, 3 }, { 4, 4 }, { 5, 1 } } },
    { "VFKHEDB", mi_VFCHE, { { 3, 3 }, { 4, 4 }, { 5, 0 } } },
    { "VFKHEDBS", mi_VFCHE, { { 3, 3 }, { 4, 4 }, { 5, 1 } } },
    { "VFKHESB", mi_VFCHE, { { 3, 2 }, { 4, 4 }, { 5, 0 } } },
    { "VFKHESBS", mi_VFCHE, { { 3, 2 }, { 4, 4 }, { 5, 1 } } },
    { "VFKHSB", mi_VFCH, { { 3, 2 }, { 4, 4 }, { 5, 0 } } },
    { "VFKHSBS", mi_VFCH, { { 3, 2 }, { 4, 4 }, { 5, 1 } } },
    { "VFLCDB", mi_VFPSO, { { 2, 3 }, { 3, 0 }, { 4, 0 } } },
    { "VFLCSB", mi_VFPSO, { { 2, 2 }, { 3, 0 }, { 4, 0 } } },
    { "VFLLS", mi_VFLL, { { 2, 2 }, { 3, 0 } } },
    { "VFLNDB", mi_VFPSO, { { 2, 3 }, { 3, 0 }, { 4, 1 } } },
    { "VFLNSB", mi_VFPSO, { { 2, 2 }, { 3, 0 }, { 4, 1 } } },
    { "VFLPDB", mi_VFPSO, { { 2, 3 }, { 3, 0 }, { 4, 2 } } },
    { "VFLPSB", mi_VFPSO, { { 2, 2 }, { 3, 0 }, { 4, 2 } } },
    { "VFLRD", mi_VFLR, { { 2, 3 } } },
    { "VFMADB", mi_VFMA, { { 4, 0 }, { 5, 3 } } },
    { "VFMASB", mi_VFMA, { { 4, 0 }, { 5, 2 } } },
    { "VFMAXDB", mi_VFMAX, { { 3, 3 }, { 4, 0 } } },
    { "VFMAXSB", mi_VFMAX, { { 3, 2 }, { 4, 0 } } },
    { "VFMDB", mi_VFM, { { 3, 3 }, { 4, 0 } } },
    { "VFMINDB", mi_VFMIN, { { 3, 3 }, { 4, 0 } } },
    { "VFMINSB", mi_VFMIN, { { 3, 2 }, { 4, 0 } } },
    { "VFMSB", mi_VFM, { { 3, 2 }, { 4, 0 } } },
    { "VFMSDB", mi_VFMS, { { 4, 0 }, { 5, 3 } } },
    { "VFMSSB", mi_VFMS, { { 4, 0 }, { 5, 2 } } },
    { "VFNMADB", mi_VFNMA, { { 4, 0 }, { 5, 3 } } },
    { "VFNMASB", mi_VFNMA, { { 4, 0 }, { 5, 2 } } },
    { "VFNMSDB", mi_VFNMS, { { 4, 0 }, { 5, 3 } } },
    { "VFNMSSB", mi_VFNMS, { { 4, 0 }, { 5, 2 } } },
    { "VFPSODB", mi_VFPSO, { { 2, 3 }, { 3, 0 } } },
    { "VFPSOSB", mi_VFPSO, { { 2, 2 }, { 3, 0 } } },
    { "VFSDB", mi_VFS, { { 2, 3 }, { 3, 0 } } },
    { "VFSQDB", mi_VFSQ, { { 2, 3 }, { 3, 0 } } },
    { "VFSQSB", mi_VFSQ, { { 2, 2 }, { 3, 0 } } },
    { "VFSSB", mi_VFS, { { 2, 2 }, { 3, 0 } } },
    { "VFTCIDB", mi_VFTCI, { { 3, 3 }, { 4, 0 } } },
    { "VFTCISB", mi_VFTCI, { { 3, 2 }, { 4, 0 } } },
    { "VGFMAB", mi_VGFMA, { { 4, 0 } } },
    { "VGFMAF", mi_VGFMA, { { 4, 2 } } },
    { "VGFMAG", mi_VGFMA, { { 4, 3 } } },
    { "VGFMAH", mi_VGFMA, { { 4, 1 } } },
    { "VGFMB", mi_VGFM, { { 3, 0 } } },
    { "VGFMF", mi_VGFM, { { 3, 2 } } },
    { "VGFMG", mi_VGFM, { { 3, 3 } } },
    { "VGFMH", mi_VGFM, { { 3, 1 } } },
    { "VGMB", mi_VGM, { { 3, 0 } } },
    { "VGMF", mi_VGM, { { 3, 2 } } },
    { "VGMG", mi_VGM, { { 3, 3 } } },
    { "VGMH", mi_VGM, { { 3, 1 } } },
    { "VISTRB", mi_VISTR, { { 3, 0 } } },
    { "VISTRBS", mi_VISTR, { { 3, 0 }, { 4, 1 } } },
    { "VISTRF", mi_VISTR, { { 3, 2 } } },
    { "VISTRFS", mi_VISTR, { { 3, 2 }, { 4, 1 } } },
    { "VISTRH", mi_VISTR, { { 3, 1 } } },
    { "VISTRHS", mi_VISTR, { { 3, 1 }, { 4, 1 } } },
    { "VLBRF", mi_VLBR, { { 2, 2 } } },
    { "VLBRG", mi_VLBR, { { 2, 3 } } },
    { "VLBRH", mi_VLBR, { { 2, 1 } } },
    { "VLBRQ", mi_VLBR, { { 2, 4 } } },
    { "VLBRREPF", mi_VLBRREP, { { 2, 2 } } },
    { "VLBRREPG", mi_VLBRREP, { { 2, 3 } } },
    { "VLBRREPH", mi_VLBRREP, { { 2, 1 } } },
    { "VLCB", mi_VLC, { { 2, 0 } } },
    { "VLCF", mi_VLC, { { 2, 2 } } },
    { "VLCG", mi_VLC, { { 2, 3 } } },
    { "VLCH", mi_VLC, { { 2, 1 } } },
    { "VLDE", mi_VFLL, {} },
    { "VLDEB", mi_VFLL, { { 2, 2 }, { 3, 0 } } },
    { "VLED", mi_VFLR, {} },
    { "VLEDB", mi_VFLR, { { 2, 3 } } },
    { "VLERF", mi_VLER, { { 2, 2 } } },
    { "VLERG", mi_VLER, { { 2, 3 } } },
    { "VLERH", mi_VLER, { { 2, 1 } } },
    { "VLGVB", mi_VLGV, { { 3, 0 } } },
    { "VLGVF", mi_VLGV, { { 3, 2 } } },
    { "VLGVG", mi_VLGV, { { 3, 3 } } },
    { "VLGVH", mi_VLGV, { { 3, 1 } } },
    { "VLLEBRZE", mi_VLLEBRZ, { { 2, 6 } } },
    { "VLLEBRZE", mi_VLLEBRZ, { { 2, 6 } } },
    { "VLLEBRZF", mi_VLLEBRZ, { { 2, 2 } } },
    { "VLLEBRZF", mi_VLLEBRZ, { { 2, 2 } } },
    { "VLLEBRZG", mi_VLLEBRZ, { { 2, 3 } } },
    { "VLLEBRZG", mi_VLLEBRZ, { { 2, 3 } } },
    { "VLLEBRZH", mi_VLLEBRZ, { { 2, 1 } } },
    { "VLLEBRZH", mi_VLLEBRZ, { { 2, 1 } } },
    { "VLLEZB", mi_VLLEZ, { { 2, 0 } } },
    { "VLLEZF", mi_VLLEZ, { { 2, 2 } } },
    { "VLLEZG", mi_VLLEZ, { { 2, 3 } } },
    { "VLLEZH", mi_VLLEZ, { { 2, 1 } } },
    { "VLLEZLF", mi_VLLEZ, { { 2, 6 } } },
    { "VLPB", mi_VLP, { { 2, 0 } } },
    { "VLPF", mi_VLP, { { 2, 2 } } },
    { "VLPG", mi_VLP, { { 2, 3 } } },
    { "VLPH", mi_VLP, { { 2, 1 } } },
    { "VLREPB", mi_VLREP, { { 2, 0 } } },
    { "VLREPF", mi_VLREP, { { 2, 2 } } },
    { "VLREPG", mi_VLREP, { { 2, 3 } } },
    { "VLREPH", mi_VLREP, { { 2, 1 } } },
    { "VLVGB", mi_VLVG, { { 3, 0 } } },
    { "VLVGF", mi_VLVG, { { 3, 2 } } },
    { "VLVGG", mi_VLVG, { { 3, 3 } } },
    { "VLVGH", mi_VLVG, { { 3, 1 } } },
    { "VMAEB", mi_VMAE, { { 4, 0 } } },
    { "VMAEF", mi_VMAE, { { 4, 2 } } },
    { "VMAEH", mi_VMAE, { { 4, 1 } } },
    { "VMAHB", mi_VMAH, { { 4, 0 } } },
    { "VMAHF", mi_VMAH, { { 4, 2 } } },
    { "VMAHH", mi_VMAH, { { 4, 1 } } },
    { "VMALB", mi_VMAL, { { 4, 0 } } },
    { "VMALEB", mi_VMALE, { { 4, 0 } } },
    { "VMALEF", mi_VMALE, { { 4, 2 } } },
    { "VMALEH", mi_VMALE, { { 4, 1 } } },
    { "VMALF", mi_VMAL, { { 4, 2 } } },
    { "VMALHB", mi_VMALH, { { 4, 0 } } },
    { "VMALHF", mi_VMALH, { { 4, 2 } } },
    { "VMALHH", mi_VMALH, { { 4, 1 } } },
    { "VMALHW", mi_VMAL, { { 4, 1 } } },
    { "VMALOB", mi_VMALO, { { 4, 0 } } },
    { "VMALOF", mi_VMALO, { { 4, 2 } } },
    { "VMALOH", mi_VMALO, { { 4, 1 } } },
    { "VMAOB", mi_VMAO, { { 4, 0 } } },
    { "VMAOF", mi_VMAO, { { 4, 2 } } },
    { "VMAOH", mi_VMAO, { { 4, 1 } } },
    { "VMEB", mi_VME, { { 3, 0 } } },
    { "VMEF", mi_VME, { { 3, 2 } } },
    { "VMEH", mi_VME, { { 3, 1 } } },
    { "VMHB", mi_VMH, { { 3, 0 } } },
    { "VMHF", mi_VMH, { { 3, 2 } } },
    { "VMHH", mi_VMH, { { 3, 1 } } },
    { "VMLB", mi_VML, { { 3, 0 } } },
    { "VMLEB", mi_VMLE, { { 3, 0 } } },
    { "VMLEF", mi_VMLE, { { 3, 2 } } },
    { "VMLEH", mi_VMLE, { { 3, 1 } } },
    { "VMLF", mi_VML, { { 3, 2 } } },
    { "VMLHB", mi_VMLH, { { 3, 0 } } },
    { "VMLHF", mi_VMLH, { { 3, 2 } } },
    { "VMLHH", mi_VMLH, { { 3, 1 } } },
    { "VMLHW", mi_VML, { { 3, 1 } } },
    { "VMLOB", mi_VMLO, { { 3, 0 } } },
    { "VMLOF", mi_VMLO, { { 3, 2 } } },
    { "VMLOH", mi_VMLO, { { 3, 1 } } },
    { "VMNB", mi_VMN, { { 3, 0 } } },
    { "VMNF", mi_VMN, { { 3, 2 } } },
    { "VMNG", mi_VMN, { { 3, 3 } } },
    { "VMNH", mi_VMN, { { 3, 1 } } },
    { "VMNLB", mi_VMNL, { { 3, 0 } } },
    { "VMNLF", mi_VMNL, { { 3, 2 } } },
    { "VMNLG", mi_VMNL, { { 3, 3 } } },
    { "VMNLH", mi_VMNL, { { 3, 1 } } },
    { "VMOB", mi_VMO, { { 3, 0 } } },
    { "VMOF", mi_VMO, { { 3, 2 } } },
    { "VMOH", mi_VMO, { { 3, 1 } } },
    { "VMRHB", mi_VMRH, { { 3, 0 } } },
    { "VMRHF", mi_VMRH, { { 3, 2 } } },
    { "VMRHG", mi_VMRH, { { 3, 3 } } },
    { "VMRHH", mi_VMRH, { { 3, 1 } } },
    { "VMRLB", mi_VMRL, { { 3, 0 } } },
    { "VMRLF", mi_VMRL, { { 3, 2 } } },
    { "VMRLG", mi_VMRL, { { 3, 3 } } },
    { "VMRLH", mi_VMRL, { { 3, 1 } } },
    { "VMSLG", mi_VMSL, { { 4, 3 } } },
    { "VMXB", mi_VMX, { { 3, 0 } } },
    { "VMXF", mi_VMX, { { 3, 2 } } },
    { "VMXG", mi_VMX, { { 3, 3 } } },
    { "VMXH", mi_VMX, { { 3, 1 } } },
    { "VMXLB", mi_VMXL, { { 3, 0 } } },
    { "VMXLF", mi_VMXL, { { 3, 2 } } },
    { "VMXLG", mi_VMXL, { { 3, 3 } } },
    { "VMXLH", mi_VMXL, { { 3, 1 } } },
    { "VNOT", mi_VNO, { { 2, 0 } } }, // VNO V1,V2,V2        (operand with index 2 replaced with 0 )
    { "VONE", mi_VGBM, { { 1, 65535 } } },
    { "VPKF", mi_VPK, { { 3, 2 } } },
    { "VPKG", mi_VPK, { { 3, 3 } } },
    { "VPKH", mi_VPK, { { 3, 1 } } },
    { "VPKLSF", mi_VPKLS, { { 3, 2 }, { 4, 0 } } },
    { "VPKLSFS", mi_VPKLS, { { 3, 2 }, { 4, 1 } } },
    { "VPKLSG", mi_VPKLS, { { 3, 3 }, { 4, 0 } } },
    { "VPKLSGS", mi_VPKLS, { { 3, 3 }, { 4, 1 } } },
    { "VPKLSH", mi_VPKLS, { { 3, 1 }, { 4, 0 } } },
    { "VPKLSHS", mi_VPKLS, { { 3, 1 }, { 4, 1 } } },
    { "VPKSF", mi_VPKS, { { 3, 2 }, { 4, 0 } } },
    { "VPKSFS", mi_VPKS, { { 3, 2 }, { 4, 1 } } },
    { "VPKSG", mi_VPKS, { { 3, 3 }, { 4, 0 } } },
    { "VPKSGS", mi_VPKS, { { 3, 3 }, { 4, 1 } } },
    { "VPKSH", mi_VPKS, { { 3, 1 }, { 4, 0 } } },
    { "VPKSHS", mi_VPKS, { { 3, 1 }, { 4, 1 } } },
    { "VPOPCTB", mi_VPOPCT, { { 2, 0 } } },
    { "VPOPCTF", mi_VPOPCT, { { 2, 2 } } },
    { "VPOPCTG", mi_VPOPCT, { { 2, 3 } } },
    { "VPOPCTH", mi_VPOPCT, { { 2, 1 } } },
    { "VREPB", mi_VREP, { { 3, 0 } } },
    { "VREPF", mi_VREP, { { 3, 2 } } },
    { "VREPG", mi_VREP, { { 3, 3 } } },
    { "VREPH", mi_VREP, { { 3, 1 } } },
    { "VREPIB", mi_VREPI, { { 2, 0 } } },
    { "VREPIF", mi_VREPI, { { 2, 2 } } },
    { "VREPIG", mi_VREPI, { { 2, 3 } } },
    { "VREPIH", mi_VREPI, { { 2, 1 } } },
    { "VSB", mi_VS, { { 3, 0 } } },
    { "VSBCBIQ", mi_VSBCBI, { { 4, 4 } } },
    { "VSBIQ", mi_VSBI, { { 4, 4 } } },
    { "VSCBIB", mi_VSCBI, { { 3, 0 } } },
    { "VSCBIF", mi_VSCBI, { { 3, 2 } } },
    { "VSCBIG", mi_VSCBI, { { 3, 3 } } },
    { "VSCBIH", mi_VSCBI, { { 3, 1 } } },
    { "VSCBIQ", mi_VSCBI, { { 3, 4 } } },
    { "VSEGB", mi_VSEG, { { 2, 0 } } },
    { "VSEGF", mi_VSEG, { { 2, 2 } } },
    { "VSEGH", mi_VSEG, { { 2, 1 } } },
    { "VSF", mi_VS, { { 3, 2 } } },
    { "VSG", mi_VS, { { 3, 3 } } },
    { "VSH", mi_VS, { { 3, 1 } } },
    { "VSQ", mi_VS, { { 3, 4 } } },
    { "VSTBRF", mi_VSTBR, { { 2, 2 } } },
    { "VSTBRG", mi_VSTBR, { { 2, 3 } } },
    { "VSTBRH", mi_VSTBR, { { 2, 1 } } },
    { "VSTBRQ", mi_VSTBR, { { 2, 4 } } },
    { "VSTERF", mi_VSTER, { { 2, 2 } } },
    { "VSTERG", mi_VSTER, { { 2, 3 } } },
    { "VSTERH", mi_VSTER, { { 2, 1 } } },
    { "VSTRCB", mi_VSTRC, { { 4, 0 } } },
    { "VSTRCBS", mi_VSTRC, { { 4, 0 } } }, // operand with index 5 ORed with 1
    { "VSTRCF", mi_VSTRC, { { 4, 2 } } },
    { "VSTRCFS", mi_VSTRC, { { 4, 2 } } }, // operand with index 5 ORed with 1
    { "VSTRCH", mi_VSTRC, { { 4, 1 } } },
    { "VSTRCHS", mi_VSTRC, { { 4, 1 } } }, // operand with index 5 ORed with 1
    { "VSTRCZB", mi_VSTRC, { { 4, 0 } } }, // operand with index 5 ORed with 2
    { "VSTRCZBS", mi_VSTRC, { { 4, 0 } } }, // operand with index 5 ORed with 3
    { "VSTRCZF", mi_VSTRC, { { 4, 2 } } }, // operand with index 5 ORed with 2
    { "VSTRCZFS", mi_VSTRC, { { 4, 2 } } }, // operand with index 5 ORed with 3 always OR
    { "VSTRCZH", mi_VSTRC, { { 4, 1 } } }, // operand with index 5 ORed with 2
    { "VSTRCZHS", mi_VSTRC, { { 4, 1 } } }, // operand with index 5 ORed with 3
    { "VSTRSB", mi_VSTRS, { { 4, 0 } } },
    { "VSTRSF", mi_VSTRS, { { 4, 2 } } },
    { "VSTRSH", mi_VSTRS, { { 4, 1 } } },
    { "VSTRSZB", mi_VSTRS, { { 4, 0 }, { 5, 2 } } },
    { "VSUMB", mi_VSUM, { { 3, 0 } } },
    { "VSUMGF", mi_VSUMG, { { 3, 2 } } },
    { "VSUMGH", mi_VSUMG, { { 3, 1 } } },
    { "VSUMH", mi_VSUM, { { 3, 1 } } },
    { "VSUMQF", mi_VSUMQ, { { 3, 2 } } },
    { "VSUMQG", mi_VSUMQ, { { 3, 3 } } },
    { "VUPHB", mi_VUPH, { { 2, 0 } } },
    { "VUPHF", mi_VUPH, { { 2, 2 } } },
    { "VUPHH", mi_VUPH, { { 2, 1 } } },
    { "VUPLB", mi_VUPL, { { 2, 0 } } },
    { "VUPLF", mi_VUPL, { { 2, 2 } } },
    { "VUPLHB", mi_VUPLH, { { 2, 0 } } },
    { "VUPLHF", mi_VUPLH, { { 2, 2 } } },
    { "VUPLHG", mi_VUPLH, { { 2, 1 } } },
    { "VUPLHW", mi_VUPL, { { 2, 1 } } },
    { "VUPLLB", mi_VUPLL, { { 2, 0 } } },
    { "VUPLLF", mi_VUPLL, { { 2, 2 } } },
    { "VUPLLH", mi_VUPLL, { { 2, 1 } } },
    { "VZERO", mi_VGBM, { { 0, 1 } } },
    { "WCDGB", mi_VCFPS, { { 2, 2 } } },
    { "WCDGB", mi_VCFPS, { { 2, 3 } } }, // operand with index 3 ORed with 8
    { "WCDLGB", mi_VCFPL, { { 2, 3 } } }, // operand with index 3 ORed with 8
    { "WCEFB", mi_VCFPS, { { 2, 2 } } }, // operand with index 3 ORed with 8
    { "WCELFB", mi_VCFPL, { { 2, 2 } } }, // operand with index 3 ORed with 8
    { "WCFEB", mi_VCSFP, { { 2, 2 } } }, // operand with index 3 ORed with 8
    { "WCGDB", mi_VCSFP, { { 2, 3 } } }, // operand with index 3 ORed with 8
    { "WCLFEB", mi_VCLFP, { { 2, 2 } } }, // operand with index 3 ORed with 8
    { "WCLGDB", mi_VCLGD, { { 2, 3 } } }, // operand with index 3 ORed with 8
    { "WFADB", mi_VFA, { { 3, 3 }, { 4, 8 } } },
    { "WFASB", mi_VFA, { { 3, 2 }, { 4, 8 } } },
    { "WFAXB", mi_VFA, { { 3, 4 }, { 4, 8 } } },
    { "WFCDB", mi_WFC, { { 3, 3 }, { 4, 0 } } },
    { "WFCEDB", mi_VFCE, { { 3, 3 }, { 4, 8 }, { 5, 0 } } },
    { "WFCEDBS", mi_VFCE, { { 3, 3 }, { 4, 8 }, { 5, 1 } } },
    { "WFCESB", mi_VFCE, { { 3, 2 }, { 4, 8 }, { 5, 0 } } },
    { "WFCESBS", mi_VFCE, { { 3, 2 }, { 4, 8 }, { 5, 1 } } },
    { "WFCEXB", mi_VFCE, { { 3, 4 }, { 4, 8 }, { 5, 0 } } },
    { "WFCEXBS", mi_VFCE, { { 3, 4 }, { 4, 8 }, { 5, 1 } } },
    { "WFCHDB", mi_VFCH, { { 3, 3 }, { 4, 8 }, { 5, 0 } } },
    { "WFCHDBS", mi_VFCH, { { 3, 3 }, { 4, 8 }, { 5, 1 } } },
    { "WFCHEDB", mi_VFCHE, { { 3, 3 }, { 4, 8 }, { 5, 0 } } },
    { "WFCHEDBS", mi_VFCHE, { { 3, 3 }, { 4, 8 }, { 5, 1 } } },
    { "WFCHESB", mi_VFCHE, { { 3, 2 }, { 4, 8 }, { 5, 0 } } },
    { "WFCHESBS", mi_VFCHE, { { 3, 2 }, { 4, 8 }, { 5, 1 } } },
    { "WFCHEXB", mi_VFCHE, { { 3, 4 }, { 4, 8 }, { 5, 0 } } },
    { "WFCHEXBS", mi_VFCHE, { { 3, 4 }, { 4, 8 }, { 5, 1 } } },
    { "WFCHSB", mi_VFCH, { { 3, 2 }, { 4, 8 }, { 5, 0 } } },
    { "WFCHSBS", mi_VFCH, { { 3, 2 }, { 4, 8 }, { 5, 1 } } },
    { "WFCHXB", mi_VFCH, { { 3, 4 }, { 4, 8 }, { 5, 0 } } },
    { "WFCHXBS", mi_VFCH, { { 3, 4 }, { 4, 8 }, { 5, 1 } } },
    { "WFCSB", mi_WFC, { { 3, 2 }, { 4, 0 } } },
    { "WFCXB", mi_WFC, { { 3, 4 }, { 4, 0 } } },
    { "WFDDB", mi_VFD, { { 3, 3 }, { 4, 8 } } },
    { "WFDSB", mi_VFD, { { 3, 2 }, { 4, 8 } } },
    { "WFDXB", mi_VFD, { { 3, 4 }, { 4, 8 } } },
    { "WFIDB", mi_VFI, { { 2, 3 } } }, // operand with index 3 ORed with 8
    { "WFISB", mi_VFI, { { 2, 2 } } }, // operand with index 3 ORed with 8
    { "WFIXB", mi_VFI, { { 2, 4 } } }, // operand with index 3 ORed with 8
    { "WFKDB", mi_WFK, { { 3, 3 }, { 4, 0 } } },
    { "WFKEDB", mi_VFCE, { { 3, 3 }, { 4, 12 }, { 5, 0 } } },
    { "WFKEDBS", mi_VFCE, { { 3, 3 }, { 4, 12 }, { 5, 1 } } },
    { "WFKESB", mi_VFCE, { { 3, 2 }, { 4, 12 }, { 5, 0 } } },
    { "WFKESBS", mi_VFCE, { { 3, 2 }, { 4, 12 }, { 5, 1 } } },
    { "WFKEXB", mi_VFCE, { { 3, 4 }, { 4, 12 }, { 5, 0 } } },
    { "WFKEXBS", mi_VFCE, { { 3, 4 }, { 4, 12 }, { 5, 1 } } },
    { "WFKHDB", mi_VFCH, { { 3, 3 }, { 4, 12 }, { 5, 0 } } },
    { "WFKHDBS", mi_VFCH, { { 3, 3 }, { 4, 12 }, { 5, 1 } } },
    { "WFKHEDB", mi_VFCHE, { { 3, 3 }, { 4, 12 }, { 5, 0 } } },
    { "WFKHEDBS", mi_VFCHE, { { 3, 3 }, { 4, 12 }, { 5, 1 } } },
    { "WFKHESB", mi_VFCHE, { { 3, 2 }, { 4, 12 }, { 5, 0 } } },
    { "WFKHESBS", mi_VFCHE, { { 3, 2 }, { 4, 12 }, { 5, 1 } } },
    { "WFKHEXB", mi_VFCHE, { { 3, 4 }, { 4, 12 }, { 5, 0 } } },
    { "WFKHEXBS", mi_VFCHE, { { 3, 4 }, { 4, 12 }, { 5, 1 } } },
    { "WFKHSB", mi_VFCH, { { 3, 2 }, { 4, 12 }, { 5, 0 } } },
    { "WFKHSBS", mi_VFCH, { { 3, 2 }, { 4, 12 }, { 5, 1 } } },
    { "WFKHXB", mi_VFCH, { { 3, 4 }, { 4, 12 }, { 5, 0 } } },
    { "WFKHXBS", mi_VFCH, { { 3, 4 }, { 4, 12 }, { 5, 1 } } },
    { "WFKSB", mi_WFK, { { 3, 2 }, { 4, 0 } } },
    { "WFKXB", mi_WFK, { { 3, 4 }, { 4, 0 } } },
    { "WFLCDB", mi_VFPSO, { { 2, 3 }, { 3, 8 }, { 4, 0 } } },
    { "WFLCSB", mi_VFPSO, { { 2, 2 }, { 3, 8 }, { 4, 0 } } },
    { "WFLCXB", mi_VFPSO, { { 2, 4 }, { 3, 8 }, { 4, 0 } } },
    { "WFLLD", mi_VFLL, { { 2, 3 }, { 3, 8 } } },
    { "WFLLS", mi_VFLL, { { 2, 2 }, { 3, 8 } } },
    { "WFLNDB", mi_VFPSO, { { 2, 3 }, { 3, 8 }, { 4, 1 } } },
    { "WFLNSB", mi_VFPSO, { { 2, 2 }, { 3, 8 }, { 4, 1 } } },
    { "WFLNXB", mi_VFPSO, { { 2, 4 }, { 3, 8 }, { 4, 1 } } },
    { "WFLPDB", mi_VFPSO, { { 2, 3 }, { 3, 8 }, { 4, 2 } } },
    { "WFLPSB", mi_VFPSO, { { 2, 2 }, { 3, 8 }, { 4, 2 } } },
    { "WFLPXB", mi_VFPSO, { { 2, 4 }, { 3, 8 }, { 4, 2 } } },
    { "WFLRD", mi_VFLR, { { 2, 3 } } }, // operand with index 3 ORed with 8
    { "WFLRX", mi_VFLR, { { 2, 4 } } }, // operand with index 3 ORed with 8
    { "WFMADB", mi_VFMA, { { 4, 8 }, { 5, 3 } } },
    { "WFMASB", mi_VFMA, { { 4, 8 }, { 5, 2 } } },
    { "WFMAXB", mi_VFMA, { { 4, 8 }, { 5, 4 } } },
    { "WFMAXDB", mi_VFMAX, { { 3, 3 }, { 4, 8 } } },
    { "WFMAXSB", mi_VFMAX, { { 3, 2 }, { 4, 8 } } },
    { "WFMAXXB", mi_VFMAX, { { 3, 4 }, { 4, 8 } } },
    { "WFMDB", mi_VFM, { { 3, 3 }, { 4, 8 } } },
    { "WFMINDB", mi_VFMIN, { { 3, 3 }, { 4, 8 } } },
    { "WFMINSB", mi_VFMIN, { { 3, 2 }, { 4, 8 } } },
    { "WFMINXB", mi_VFMIN, { { 3, 4 }, { 4, 8 } } },
    { "WFMSB", mi_VFM, { { 3, 2 }, { 4, 8 } } },
    { "WFMSDB", mi_VFMS, { { 4, 8 }, { 5, 3 } } },
    { "WFMSSB", mi_VFMS, { { 4, 8 }, { 5, 2 } } },
    { "WFMSXB", mi_VFMS, { { 4, 8 }, { 5, 4 } } },
    { "WFMXB", mi_VFM, { { 3, 4 }, { 4, 8 } } },
    { "WFNMADB", mi_VFNMA, { { 4, 8 }, { 5, 3 } } },
    { "WFNMASB", mi_VFNMA, { { 4, 8 }, { 5, 2 } } },
    { "WFNMAXB", mi_VFNMA, { { 4, 8 }, { 5, 4 } } },
    { "WFNMSDB", mi_VFNMS, { { 4, 8 }, { 5, 3 } } },
    { "WFNMSSB", mi_VFNMS, { { 4, 8 }, { 5, 2 } } },
    { "WFNMSXB", mi_VFNMS, { { 4, 8 }, { 5, 4 } } },
    { "WFPSODB", mi_VFPSO, { { 2, 3 }, { 3, 8 } } },
    { "WFPSOSB", mi_VFPSO, { { 2, 2 }, { 3, 8 } } },
    { "WFPSOXB", mi_VFPSO, { { 2, 4 }, { 3, 8 } } },
    { "WFSDB", mi_VFS, { { 2, 3 }, { 3, 8 } } },
    { "WFSQDB", mi_VFSQ, { { 2, 3 }, { 3, 8 } } },
    { "WFSQSB", mi_VFSQ, { { 2, 2 }, { 3, 8 } } },
    { "WFSQXB", mi_VFSQ, { { 2, 4 }, { 3, 8 } } },
    { "WFSSB", mi_VFS, { { 2, 2 }, { 3, 8 } } },
    { "WFSXB", mi_VFS, { { 2, 4 }, { 3, 8 } } },
    { "WFTCIDB", mi_VFTCI, { { 3, 3 }, { 4, 8 } } },
    { "WFTCISB", mi_VFTCI, { { 3, 2 }, { 4, 8 } } },
    { "WFTCIXB", mi_VFTCI, { { 3, 4 }, { 4, 8 } } },
    { "WLDEB", mi_VFLL, { { 2, 2 }, { 3, 8 } } },
    { "WLEDB", mi_VFLR, { { 2, 3 } } }, // operand with index 3 ORed with 8
    { "XHHR", mi_RXSBG, { { 2, 0 }, { 3, 31 } } },
    { "XHHR", mi_RXSBG, { { 2, 0 }, { 3, 31 } } },
    { "XHLR", mi_RXSBG, { { 2, 0 }, { 3, 31 }, { 4, 32 } } },
    { "XLHR", mi_RXSBG, { { 2, 32 }, { 3, 63 }, { 4, 32 } } },
    { "XLHR", mi_RXSBG, { { 2, 32 }, { 3, 63 }, { 4, 32 } } },
};

#ifdef __cpp_lib_ranges
static_assert(std::ranges::is_sorted(mnemonic_codes, {}, &mnemonic_code::name));

const mnemonic_code* instruction::find_mnemonic_codes(std::string_view name)
{
    auto it = std::ranges::lower_bound(mnemonic_codes, name, {}, &mnemonic_code::name);
    if (it == std::ranges::end(mnemonic_codes) || it->name() != name)
        return nullptr;
    return &*it;
}
#else
static_assert(std::is_sorted(std::begin(mnemonic_codes), std::end(mnemonic_codes), [](const auto& l, const auto& r) {
    return l.name() < r.name();
}));

const mnemonic_code* instruction::find_mnemonic_codes(std::string_view name)
{
    auto it =
        std::lower_bound(std::begin(mnemonic_codes), std::end(mnemonic_codes), name, [](const auto& l, const auto& r) {
            return l.name() < r;
        });
    if (it == std::end(mnemonic_codes) || it->name() != name)
        return nullptr;
    return &*it;
}
#endif

const mnemonic_code& instruction::get_mnemonic_codes(std::string_view name)
{
    auto result = find_mnemonic_codes(name);
    assert(result);
    return *result;
}
std::span<const mnemonic_code> instruction::all_mnemonic_codes() { return mnemonic_codes; }
