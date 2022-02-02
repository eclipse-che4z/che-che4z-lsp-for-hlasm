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

const std::vector<ca_instruction>* const ca_instructions = &instruction::all_ca_instructions();

const std::vector<ca_instruction>& instruction::all_ca_instructions()
{
    if (ca_instructions)
        return *ca_instructions;
    static const std::vector<ca_instruction> ca_instructions_ = {
        { "AIF", false },
        { "AGO", false },
        { "ACTR", false },
        { "SETA", false },
        { "SETB", false },
        { "SETC", false },
        { "ANOP", true },
        { "LCLA", false },
        { "LCLB", false },
        { "LCLC", false },
        { "GBLA", false },
        { "GBLB", false },
        { "GBLC", false },
        { "MACRO", true },
        { "MEND", true },
        { "MEXIT", true },
        { "MHELP", false },
        { "AEJECT", true },
        { "AREAD", false },
        { "ASPACE", false },
    };
    return ca_instructions_;
}

const ca_instruction* instruction::find_ca_instructions(std::string_view name)
{
    auto it = std::find_if(
        ca_instructions->begin(), ca_instructions->end(), [name](const auto& i) { return i.name == name; });

    if (it == ca_instructions->end())
        return nullptr;
    return &*it;
}

const ca_instruction& instruction::get_ca_instructions(std::string_view name)
{
    auto result = find_ca_instructions(name);
    assert(result);
    return *result;
}

const std::map<std::string_view, assembler_instruction>* const assembler_instructions =
    &instruction::all_assembler_instructions();

const assembler_instruction* instruction::find_assembler_instructions(std::string_view instr)
{
    auto it = assembler_instructions->find(instr);
    if (it == assembler_instructions->end())
        return nullptr;
    return &it->second;
}

const assembler_instruction& instruction::get_assembler_instructions(std::string_view instr)
{
    auto result = find_assembler_instructions(instr);
    assert(result);
    return *result;
}

const std::map<std::string_view, assembler_instruction>& instruction::all_assembler_instructions()
{
    if (assembler_instructions)
        return *assembler_instructions;
    static const std::map<std::string_view, assembler_instruction> assembler_instructions_ = {
        { "*PROCESS", assembler_instruction(1, -1, true, "") }, // TO DO
        { "ACONTROL", assembler_instruction(1, -1, false, "<selection>+") },
        { "ADATA", assembler_instruction(5, 5, false, "value1,value2,value3,value4,character_string") },
        { "AINSERT", assembler_instruction(2, 2, false, "'record',BACK|FRONT") },
        { "ALIAS", assembler_instruction(1, 1, false, "alias_string") },
        { "AMODE", assembler_instruction(1, 1, false, "amode_option") },
        { "CATTR", assembler_instruction(1, -1, false, "attribute+") },
        { "CCW", assembler_instruction(4, 4, true, "command_code,data_address,flags,data_count") },
        { "CCW0", assembler_instruction(4, 4, true, "command_code,data_address,flags,data_count") },
        { "CCW1", assembler_instruction(4, 4, true, "command_code,data_address,flags,data_count") },
        { "CEJECT", assembler_instruction(0, 1, true, "?number_of_lines") },
        { "CNOP", assembler_instruction(2, 2, true, "byte,boundary") },
        { "COM", assembler_instruction(0, 0, false, "") },
        { "COPY", assembler_instruction(1, 1, false, "member") },
        { "CSECT", assembler_instruction(0, 0, false, "") },
        { "CXD", assembler_instruction(0, 0, false, "") },
        { "DC", assembler_instruction(1, -1, true, "<operand>+") },
        { "DROP", assembler_instruction(0, -1, true, "?<<base_register|label>+>") },
        { "DS", assembler_instruction(1, -1, true, "<operand>+") },
        { "DSECT", assembler_instruction(0, 0, false, "") },
        { "DXD", assembler_instruction(1, -1, true, "<operand>+") },
        { "EJECT", assembler_instruction(0, 0, false, "") },
        { "END", assembler_instruction(0, 2, true, "?expression,?language") },
        { "ENTRY", assembler_instruction(1, -1, true, "entry_point+") },
        { "EQU",
            assembler_instruction(1,
                5,
                true,
                "value,?<length_attribute_value>,?<type_attribute_value>,?<program_type_value>,?<assembler_type>"
                "value>") },
        { "EXITCTL", assembler_instruction(2, 5, false, "exit_type,control_value+") },
        { "EXTRN", assembler_instruction(1, -1, false, "<external_symbol>+|PART(<external_symbol>+)") },
        { "ICTL", assembler_instruction(1, 3, false, "begin,?<end>,?<continue>") },
        { "ISEQ", assembler_instruction(0, 2, false, "?<left,right>") },
        { "LOCTR", assembler_instruction(0, 0, false, "") },
        { "LTORG", assembler_instruction(0, 0, false, "") },
        { "MNOTE", assembler_instruction(1, 2, false, "?<<severity|*|>,>message") },
        { "OPSYN", assembler_instruction(0, 1, false, "?operation_code_2") },
        { "ORG", assembler_instruction(0, 3, true, "expression?<,boundary?<,offset>>") },
        { "POP", assembler_instruction(1, 4, false, "<PRINT|USING|ACONTROL>+,?NOPRINT") },
        { "PRINT", assembler_instruction(1, -1, false, "operand+") },
        { "PUNCH", assembler_instruction(1, 1, false, "string") },
        { "PUSH", assembler_instruction(1, 4, false, "<PRINT|USING|ACONTROL>+,?NOPRINT") },
        { "REPRO", assembler_instruction(0, 0, false, "") },
        { "RMODE", assembler_instruction(1, 1, false, "rmode_option") },
        { "RSECT", assembler_instruction(0, 0, false, "") },
        { "SPACE", assembler_instruction(0, 1, true, "?number_of_lines") },
        { "START", assembler_instruction(0, 1, true, "?expression") },
        { "TITLE", assembler_instruction(1, 1, false, "title_string") },
        { "USING", assembler_instruction(2, -1, true, "operand+") },
        { "WXTRN", assembler_instruction(1, -1, false, "<external_symbol>+|PART(<external_symbol>+)") },
        { "XATTR", assembler_instruction(1, -1, false, "attribute+") },
    };
    return assembler_instructions_;
}

bool hlasm_plugin::parser_library::context::machine_instruction::check_nth_operand(
    size_t place, const checking::machine_operand* operand)
{
    diagnostic_op diag;
    const range stmt_range = range();
    if (operand->check(diag, operands[place], instr_name, stmt_range))
        return true;
    return false;
}

bool hlasm_plugin::parser_library::context::machine_instruction::check(std::string_view name_of_instruction,
    const std::vector<const checking::machine_operand*> to_check,
    const range& stmt_range,
    const diagnostic_collector& add_diagnostic) const
{
    // check size of operands
    int diff = (int)operands.size() - (int)to_check.size();
    if (diff > no_optional || diff < 0)
    {
        add_diagnostic(diagnostic_op::error_optional_number_of_operands(
            name_of_instruction, no_optional, (int)operands.size(), stmt_range));
        return false;
    }
    bool error = false;
    for (size_t i = 0; i < to_check.size(); i++)
    {
        assert(to_check[i] != nullptr);
        diagnostic_op diag;
        if (!(*to_check[i]).check(diag, operands[i], name_of_instruction, stmt_range))
        {
            add_diagnostic(diag);
            error = true;
        }
    };
    return (!error);
}

namespace {
struct instruction_format_definition
{
    const machine_operand_format* op_format;
    unsigned char op_format_size;

    mach_format format;
};

template<mach_format F, const machine_operand_format&... Ops>
class instruction_format_definition_factory
{
    static constexpr std::array<machine_operand_format, sizeof...(Ops)> format = { Ops... };

public:
    static constexpr instruction_format_definition def() { return { format.data(), sizeof...(Ops), F }; }
};
template<mach_format F>
class instruction_format_definition_factory<F>
{
public:
    static constexpr instruction_format_definition def() { return { nullptr, 0, F }; }
};
} // namespace

static auto generate_machine_instructions()
{
    std::set<machine_instruction, machine_instruction_comparer> result;

    // clang-format off
    static constexpr auto E_0 = instruction_format_definition_factory<mach_format::E>::def();
    static constexpr auto I_1 = instruction_format_definition_factory<mach_format::I, imm_8_U>::def();
    static constexpr auto IE_2 = instruction_format_definition_factory<mach_format::IE, imm_4_U, imm_4_U>::def();
    static constexpr auto MII_3 = instruction_format_definition_factory<mach_format::MII, mask_4_U, rel_addr_imm_12_S, rel_addr_imm_24_S>::def();
    static constexpr auto RI_a_2_s = instruction_format_definition_factory<mach_format::RI_a, reg_4_U, imm_16_S>::def();
    static constexpr auto RI_a_2_u = instruction_format_definition_factory<mach_format::RI_a, reg_4_U, imm_16_U>::def();
    static constexpr auto RI_b_2 = instruction_format_definition_factory<mach_format::RI_b, reg_4_U, rel_addr_imm_16_S>::def();
    static constexpr auto RI_c_2 = instruction_format_definition_factory<mach_format::RI_c, mask_4_U, rel_addr_imm_16_S>::def();
    static constexpr auto RIE_a_3 = instruction_format_definition_factory<mach_format::RIE_a, reg_4_U, imm_16_S, mask_4_U>::def();
    static constexpr auto RIE_b_4 = instruction_format_definition_factory<mach_format::RIE_b, reg_4_U, reg_4_U, mask_4_U, rel_addr_imm_16_S>::def();
    static constexpr auto RIE_c_4 = instruction_format_definition_factory<mach_format::RIE_c, reg_4_U, imm_8_S, mask_4_U, rel_addr_imm_16_S>::def();
    static constexpr auto RIE_d_3 = instruction_format_definition_factory<mach_format::RIE_d, reg_4_U, reg_4_U, imm_16_S>::def();
    static constexpr auto RIE_e_3 = instruction_format_definition_factory<mach_format::RIE_e, reg_4_U, reg_4_U, rel_addr_imm_16_S>::def();
    static constexpr auto RIE_f_5 = instruction_format_definition_factory<mach_format::RIE_f, reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt>::def();
    static constexpr auto RIE_g_3 = instruction_format_definition_factory<mach_format::RIE_g, reg_4_U, imm_16_S, mask_4_U>::def();
    static constexpr auto RIL_a_2 = instruction_format_definition_factory<mach_format::RIL_a, reg_4_U, imm_32_S>::def();
    static constexpr auto RIL_b_2 = instruction_format_definition_factory<mach_format::RIL_b, reg_4_U, rel_addr_imm_32_S>::def();
    static constexpr auto RIL_c_2 = instruction_format_definition_factory<mach_format::RIL_c, mask_4_U, rel_addr_imm_32_S>::def();
    static constexpr auto RIS_4 = instruction_format_definition_factory<mach_format::RIS, reg_4_U, imm_8_S, mask_4_U, db_12_4_U>::def();
    static constexpr auto RR_1 = instruction_format_definition_factory<mach_format::RR, reg_4_U>::def();
    static constexpr auto RR_2_m = instruction_format_definition_factory<mach_format::RR, mask_4_U, reg_4_U>::def();
    static constexpr auto RR_2 = instruction_format_definition_factory<mach_format::RR, reg_4_U, reg_4_U>::def();
    static constexpr auto RRD_3 = instruction_format_definition_factory<mach_format::RRD, reg_4_U, reg_4_U, reg_4_U>::def();
    static constexpr auto RRE_0 = instruction_format_definition_factory<mach_format::RRE>::def();
    static constexpr auto RRE_1 = instruction_format_definition_factory<mach_format::RRE, reg_4_U>::def();
    static constexpr auto RRE_2 = instruction_format_definition_factory<mach_format::RRE, reg_4_U, reg_4_U>::def();
    static constexpr auto RRF_a_3 = instruction_format_definition_factory<mach_format::RRF_a, reg_4_U, reg_4_U, reg_4_U>::def();
    static constexpr auto RRF_a_4 = instruction_format_definition_factory<mach_format::RRF_a, reg_4_U, reg_4_U, reg_4_U, mask_4_U>::def();
    static constexpr auto RRF_a_4_opt = instruction_format_definition_factory<mach_format::RRF_a, reg_4_U, reg_4_U, reg_4_U_opt, mask_4_U_opt>::def();
    static constexpr auto RRF_b_3 = instruction_format_definition_factory<mach_format::RRF_b, reg_4_U, reg_4_U, reg_4_U>::def();
    static constexpr auto RRF_b_4 = instruction_format_definition_factory<mach_format::RRF_b, reg_4_U, reg_4_U, reg_4_U, mask_4_U>::def();
    static constexpr auto RRF_b_4_opt = instruction_format_definition_factory<mach_format::RRF_b, reg_4_U, reg_4_U, reg_4_U, mask_4_U_opt>::def();
    static constexpr auto RRF_c_3 = instruction_format_definition_factory<mach_format::RRF_c, reg_4_U, reg_4_U, mask_4_U>::def();
    static constexpr auto RRF_c_3_opt = instruction_format_definition_factory<mach_format::RRF_c, reg_4_U, reg_4_U, mask_4_U_opt>::def();
    static constexpr auto RRF_d_3 = instruction_format_definition_factory<mach_format::RRF_d, reg_4_U, reg_4_U, mask_4_U>::def();
    static constexpr auto RRF_e_3 = instruction_format_definition_factory<mach_format::RRF_e, reg_4_U, mask_4_U, reg_4_U>::def();
    static constexpr auto RRF_e_4 = instruction_format_definition_factory<mach_format::RRF_e, reg_4_U, mask_4_U, reg_4_U, mask_4_U>::def();
    static constexpr auto RRS_4 = instruction_format_definition_factory<mach_format::RRS, reg_4_U, reg_4_U, mask_4_U, db_12_4_U>::def();
    static constexpr auto RS_a_2 = instruction_format_definition_factory<mach_format::RS_a, reg_4_U, db_12_4_U>::def();
    static constexpr auto RS_a_3 = instruction_format_definition_factory<mach_format::RS_a, reg_4_U, reg_4_U, db_12_4_U>::def();
    static constexpr auto RS_b_3 = instruction_format_definition_factory<mach_format::RS_b, reg_4_U, mask_4_U, db_12_4_U>::def();
    static constexpr auto RSI_3 = instruction_format_definition_factory<mach_format::RSI, reg_4_U, reg_4_U, rel_addr_imm_16_S>::def();
    static constexpr auto RSL_a_1 = instruction_format_definition_factory<mach_format::RSL_a, db_12_4x4L_U>::def();
    static constexpr auto RSL_b_3 = instruction_format_definition_factory<mach_format::RSL_b, reg_4_U, db_12_8x4L_U, mask_4_U>::def();
    static constexpr auto RSY_a_3 = instruction_format_definition_factory<mach_format::RSY_a, reg_4_U, reg_4_U, db_20_4_S>::def();
    static constexpr auto RSY_b_3_su = instruction_format_definition_factory<mach_format::RSY_b, reg_4_U, db_20_4_S, mask_4_U>::def();
    static constexpr auto RSY_b_3_us = instruction_format_definition_factory<mach_format::RSY_b, reg_4_U, mask_4_U, db_20_4_S>::def();
    static constexpr auto RSY_b_3_ux = instruction_format_definition_factory<mach_format::RSY_b, reg_4_U, mask_4_U, dxb_20_4x4_S>::def();
    static constexpr auto RX_a_2_ux = instruction_format_definition_factory<mach_format::RX_a, reg_4_U, dxb_12_4x4_U>::def();
    static constexpr auto RX_a_2 = instruction_format_definition_factory<mach_format::RX_a, reg_4_U, reg_4_U>::def();
    static constexpr auto RX_b_2 = instruction_format_definition_factory<mach_format::RX_b, mask_4_U, dxb_12_4x4_U>::def();
    static constexpr auto RXE_2 = instruction_format_definition_factory<mach_format::RXE, reg_4_U, dxb_12_4x4_U>::def();
    static constexpr auto RXE_3_xm = instruction_format_definition_factory<mach_format::RXE, reg_4_U, dxb_12_4x4_U, mask_4_U>::def();
    static constexpr auto RXF_3_x = instruction_format_definition_factory<mach_format::RXF, reg_4_U, reg_4_U, dxb_12_4x4_U>::def();
    static constexpr auto RXY_a_2 = instruction_format_definition_factory<mach_format::RXY_a, reg_4_U, dxb_20_4x4_S>::def();
    static constexpr auto RXY_b_2 = instruction_format_definition_factory<mach_format::RXY_b, mask_4_U, dxb_20_4x4_S>::def();
    static constexpr auto S_0 = instruction_format_definition_factory<mach_format::S>::def();
    static constexpr auto S_1_u = instruction_format_definition_factory<mach_format::S, db_12_4_U>::def();
    static constexpr auto S_1_s = instruction_format_definition_factory<mach_format::S, db_20_4_S>::def();
    static constexpr auto SI_1 = instruction_format_definition_factory<mach_format::SI, db_12_4_U>::def();
    static constexpr auto SI_2_s = instruction_format_definition_factory<mach_format::SI, db_12_4_U, imm_8_S>::def();
    static constexpr auto SI_2_u = instruction_format_definition_factory<mach_format::SI, db_12_4_U, imm_8_U>::def();
    static constexpr auto SIL_2_s = instruction_format_definition_factory<mach_format::SIL, db_12_4_U, imm_16_S>::def();
    static constexpr auto SIL_2_u = instruction_format_definition_factory<mach_format::SIL, db_12_4_U, imm_16_U>::def();
    static constexpr auto SIY_2_uu = instruction_format_definition_factory<mach_format::SIY, db_12_4_U, imm_8_U>::def();
    static constexpr auto SIY_2_ss = instruction_format_definition_factory<mach_format::SIY, db_20_4_S, imm_8_S>::def();
    static constexpr auto SIY_2_su = instruction_format_definition_factory<mach_format::SIY, db_20_4_S, imm_8_U>::def();
    static constexpr auto SMI_3 = instruction_format_definition_factory<mach_format::SMI, mask_4_U, rel_addr_imm_16_S, db_12_4_U>::def();
    static constexpr auto SS_a_2_u = instruction_format_definition_factory<mach_format::SS_a, db_12_8x4L_U, db_12_4_U>::def();
    static constexpr auto SS_a_2_s = instruction_format_definition_factory<mach_format::SS_a, db_12_8x4L_U, db_20_4_S>::def();
    static constexpr auto SS_b_2 = instruction_format_definition_factory<mach_format::SS_b, db_12_4x4L_U, db_12_4x4L_U>::def();
    static constexpr auto SS_c_3 = instruction_format_definition_factory<mach_format::SS_c, db_12_4x4L_U, db_12_4_U, imm_4_U>::def();
    static constexpr auto SS_d_3 = instruction_format_definition_factory<mach_format::SS_d, drb_12_4x4_U, db_12_4_U, reg_4_U>::def();
    static constexpr auto SS_e_4_br = instruction_format_definition_factory<mach_format::SS_e, reg_4_U, db_12_4_U, reg_4_U, db_12_4_U>::def();
    static constexpr auto SS_e_4_rb = instruction_format_definition_factory<mach_format::SS_e, reg_4_U, reg_4_U, db_12_4_U, db_12_4_U>::def();
    static constexpr auto SS_f_2 = instruction_format_definition_factory<mach_format::SS_f, db_12_4_U, db_12_8x4L_U>::def();
    static constexpr auto SSE_2 = instruction_format_definition_factory<mach_format::SSE, db_12_4_U, db_12_4_U>::def();
    static constexpr auto SSF_3_dr = instruction_format_definition_factory<mach_format::SSF, db_12_4_U, db_12_4_U, reg_4_U>::def();
    static constexpr auto SSF_3_rd = instruction_format_definition_factory<mach_format::SSF, reg_4_U, db_12_4_U, db_12_4_U>::def();
    static constexpr auto VRI_a_2 = instruction_format_definition_factory<mach_format::VRI_a, vec_reg_5_U, imm_16_U>::def();
    static constexpr auto VRI_a_3 = instruction_format_definition_factory<mach_format::VRI_a, vec_reg_5_U, imm_16_S, mask_4_U>::def();
    static constexpr auto VRI_b_4 = instruction_format_definition_factory<mach_format::VRI_b, vec_reg_5_U, imm_8_U, imm_8_U, mask_4_U>::def();
    static constexpr auto VRI_c_4 = instruction_format_definition_factory<mach_format::VRI_c, vec_reg_5_U, vec_reg_5_U, imm_16_U, mask_4_U>::def();
    static constexpr auto VRI_d_4 = instruction_format_definition_factory<mach_format::VRI_d, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U>::def();
    static constexpr auto VRI_d_5 = instruction_format_definition_factory<mach_format::VRI_d, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U>::def();
    static constexpr auto VRI_e_5 = instruction_format_definition_factory<mach_format::VRI_e, vec_reg_5_U, vec_reg_5_U, imm_12_S, mask_4_U, mask_4_U>::def();
    static constexpr auto VRI_f_5 = instruction_format_definition_factory<mach_format::VRI_f, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U>::def();
    static constexpr auto VRI_g_5_s = instruction_format_definition_factory<mach_format::VRI_g, vec_reg_5_U, vec_reg_5_U, imm_8_U, imm_8_S, mask_4_U>::def();
    static constexpr auto VRI_g_5_u = instruction_format_definition_factory<mach_format::VRI_g, vec_reg_5_U, vec_reg_5_U, imm_8_U, imm_8_U, mask_4_U>::def();
    static constexpr auto VRI_h_3 = instruction_format_definition_factory<mach_format::VRI_h, vec_reg_5_U, imm_16_S, imm_4_U>::def();
    static constexpr auto VRI_i_4 = instruction_format_definition_factory<mach_format::VRI_i, vec_reg_5_U, reg_4_U, imm_8_S, mask_4_U>::def();
    static constexpr auto VRR_a_2 = instruction_format_definition_factory<mach_format::VRR_a, vec_reg_5_U, vec_reg_5_U>::def();
    static constexpr auto VRR_a_3 = instruction_format_definition_factory<mach_format::VRR_a, vec_reg_5_U, vec_reg_5_U, mask_4_U>::def();
    static constexpr auto VRR_a_4 = instruction_format_definition_factory<mach_format::VRR_a, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U>::def();
    static constexpr auto VRR_a_4_opt = instruction_format_definition_factory<mach_format::VRR_a, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U_opt>::def();
    static constexpr auto VRR_a_5 = instruction_format_definition_factory<mach_format::VRR_a, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U>::def();
    static constexpr auto VRR_b_5 = instruction_format_definition_factory<mach_format::VRR_b, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U>::def();
    static constexpr auto VRR_b_5_opt = instruction_format_definition_factory<mach_format::VRR_b, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U_opt>::def();
    static constexpr auto VRR_c_3 = instruction_format_definition_factory<mach_format::VRR_c, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U>::def();
    static constexpr auto VRR_c_4 = instruction_format_definition_factory<mach_format::VRR_c, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U>::def();
    static constexpr auto VRR_c_5 = instruction_format_definition_factory<mach_format::VRR_c, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U>::def();
    static constexpr auto VRR_c_6 = instruction_format_definition_factory<mach_format::VRR_c, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U>::def();
    static constexpr auto VRR_d_5 = instruction_format_definition_factory<mach_format::VRR_d, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U>::def();
    static constexpr auto VRR_d_6 = instruction_format_definition_factory<mach_format::VRR_d, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U>::def();
    static constexpr auto VRR_d_6_opt = instruction_format_definition_factory<mach_format::VRR_d, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U_opt>::def();
    static constexpr auto VRR_e_4 = instruction_format_definition_factory<mach_format::VRR_e, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U>::def();
    static constexpr auto VRR_e_6 = instruction_format_definition_factory<mach_format::VRR_e, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U>::def();
    static constexpr auto VRR_f_3 = instruction_format_definition_factory<mach_format::VRR_f, vec_reg_5_U, reg_4_U, reg_4_U>::def();
    static constexpr auto VRR_g_1 = instruction_format_definition_factory<mach_format::VRR_g, vec_reg_5_U>::def();
    static constexpr auto VRR_h_3 = instruction_format_definition_factory<mach_format::VRR_h, vec_reg_5_U, vec_reg_5_U, mask_4_U>::def();
    static constexpr auto VRR_i_3 = instruction_format_definition_factory<mach_format::VRR_i, reg_4_U, vec_reg_5_U, mask_4_U>::def();
    static constexpr auto VRS_a_4 = instruction_format_definition_factory<mach_format::VRS_a, vec_reg_5_U, vec_reg_5_U, db_12_4_U, mask_4_U>::def();
    static constexpr auto VRS_a_4_opt = instruction_format_definition_factory<mach_format::VRS_a, vec_reg_5_U, vec_reg_5_U, db_12_4_U, mask_4_U_opt>::def();
    static constexpr auto VRS_b_3 = instruction_format_definition_factory<mach_format::VRS_b, vec_reg_5_U, reg_4_U, db_12_4_U>::def();
    static constexpr auto VRS_b_4 = instruction_format_definition_factory<mach_format::VRS_b, vec_reg_5_U, reg_4_U, db_12_4_U, mask_4_U>::def();
    static constexpr auto VRS_c_4 = instruction_format_definition_factory<mach_format::VRS_c, reg_4_U, vec_reg_5_U, db_12_4_U, mask_4_U>::def();
    static constexpr auto VRS_d_3 = instruction_format_definition_factory<mach_format::VRS_d, vec_reg_5_U, reg_4_U, db_12_4_U>::def();
    static constexpr auto VRV_3 = instruction_format_definition_factory<mach_format::VRV, vec_reg_5_U, dvb_12_5x4_U, mask_4_U>::def();
    static constexpr auto VRX_3 = instruction_format_definition_factory<mach_format::VRX, vec_reg_5_U, dxb_12_4x4_U, mask_4_U>::def();
    static constexpr auto VRX_3_opt = instruction_format_definition_factory<mach_format::VRX, vec_reg_5_U, dxb_12_4x4_U, mask_4_U_opt>::def();
    static constexpr auto VSI_3 = instruction_format_definition_factory<mach_format::VSI, vec_reg_5_U, db_12_4_U, imm_8_U>::def();
    // clang-format on

    struct iname
    {
        constexpr iname(std::string_view n)
            : len((unsigned char)n.size())
            , data {}
        {
            size_t i = 0;
            for (char c : n)
                data[i++] = c;
        }
        unsigned char len;
        std::array<char, 7> data;

        constexpr operator std::string_view() const { return std::string_view(data.data(), len); }
    };
    struct instruction_definition
    {
        constexpr instruction_definition(std::string_view name, instruction_format_definition def, short page_no)
            : name(name)
            , op_format(def.op_format)
            , op_format_size(def.op_format_size)
            , format(def.format)
            , page_no(page_no)
        {}
        iname name;

        const machine_operand_format* op_format;
        unsigned char op_format_size;

        mach_format format;

        short page_no;
    } static constexpr instructions[] = {
        { "AR", RR_2, 510 },
        { "ADDFRR", RRE_2, 7 },
        { "AGR", RRE_2, 510 },
        { "AGFR", RRE_2, 510 },
        { "ARK", RRF_a_3, 510 },
        { "AGRK", RRF_a_3, 510 },
        { "A", RX_a_2_ux, 510 },
        { "AY", RXY_a_2, 511 },
        { "AG", RXY_a_2, 511 },
        { "AGF", RXY_a_2, 511 },
        { "AFI", RIL_a_2, 511 },
        { "AGFI", RIL_a_2, 511 },
        { "AHIK", RIE_d_3, 511 },
        { "AGHIK", RIE_d_3, 511 },
        { "ASI", SIY_2_ss, 511 },
        { "AGSI", SIY_2_ss, 511 },
        { "AH", RX_a_2_ux, 512 },
        { "AHY", RXY_a_2, 512 },
        { "AGH", RXY_a_2, 512 },
        { "AHI", RI_a_2_s, 512 },
        { "AGHI", RI_a_2_s, 513 },
        { "AHHHR", RRF_a_3, 513 },
        { "AHHLR", RRF_a_3, 513 },
        { "AIH", RIL_a_2, 513 },
        { "ALR", RR_2, 514 },
        { "ALGR", RRE_2, 514 },
        { "ALGFR", RRE_2, 514 },
        { "ALRK", RRF_a_3, 514 },
        { "ALGRK", RRF_a_3, 514 },
        { "AL", RX_a_2_ux, 514 },
        { "ALY", RXY_a_2, 514 },
        { "ALG", RXY_a_2, 514 },
        { "ALGF", RXY_a_2, 514 },
        { "ALFI", RIL_a_2, 514 },
        { "ALGFI", RIL_a_2, 514 },
        { "ALHHHR", RRF_a_3, 515 },
        { "ALHHLR", RRF_a_3, 515 },
        { "ALCR", RRE_2, 515 },
        { "ALCGR", RRE_2, 515 },
        { "ALC", RXY_a_2, 515 },
        { "ALCG", RXY_a_2, 515 },
        { "ALSI", SIY_2_ss, 516 },
        { "ALGSI", SIY_2_ss, 516 },
        { "ALHSIK", RIE_d_3, 516 },
        { "ALGHSIK", RIE_d_3, 516 },
        { "ALSIH", RIL_a_2, 517 },
        { "ALSIHN", RIL_a_2, 517 },
        { "NR", RR_2, 517 },
        { "NGR", RRE_2, 517 },
        { "NRK", RRF_a_3, 517 },
        { "NGRK", RRF_a_3, 517 },
        { "N", RX_a_2_ux, 517 },
        { "NY", RXY_a_2, 517 },
        { "NG", RXY_a_2, 517 },
        { "NI", SI_2_u, 517 },
        { "NIY", SIY_2_su, 518 },
        { "NC", SS_a_2_u, 518 },
        { "NIHF", RIL_a_2, 518 },
        { "NIHH", RI_a_2_u, 518 },
        { "NIHL", RI_a_2_u, 518 },
        { "NILF", RIL_a_2, 519 },
        { "NILH", RI_a_2_u, 519 },
        { "NILL", RI_a_2_u, 519 },
        { "BALR", RR_2, 519 },
        { "BAL", RX_a_2_ux, 519 },
        { "BASR", RR_2, 520 },
        { "BAS", RX_a_2_ux, 520 },
        { "BASSM", RX_a_2, 520 },
        { "BSM", RR_2, 522 },
        { "BIC", RXY_b_2, 523 },
        { "BCR", RR_2_m, 524 },
        { "BC", RX_b_2, 524 },
        { "BCTR", RR_2, 525 },
        { "BCTGR", RRE_2, 525 },
        { "BCT", RX_a_2_ux, 525 },
        { "BCTG", RXY_a_2, 525 },
        { "BXH", RS_a_3, 526 },
        { "BXHG", RSY_a_3, 526 },
        { "BXLE", RS_a_3, 526 },
        { "BXLEG", RSY_a_3, 526 },
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
        { "CKSM", RRE_2, 533 },
        { "KM", RRE_2, 537 },
        { "KMC", RRE_2, 537 },
        { "KMA", RRF_b_3, 562 },
        { "KMF", RRE_2, 576 },
        { "KMCTR", RRF_b_3, 591 },
        { "KMO", RRE_2, 604 },
        { "CR", RR_2, 618 },
        { "CGR", RRE_2, 618 },
        { "CGFR", RRE_2, 618 },
        { "C", RX_a_2_ux, 618 },
        { "CY", RXY_a_2, 618 },
        { "CG", RXY_a_2, 618 },
        { "CGF", RXY_a_2, 618 },
        { "CFI", RIL_a_2, 618 },
        { "CGFI", RIL_a_2, 619 },
        { "CRL", RIL_b_2, 619 },
        { "CGRL", RIL_b_2, 619 },
        { "CGFRL", RIL_b_2, 619 },
        { "CRB", RRS_4, 619 },
        { "CGRB", RRS_4, 619 },
        { "CRJ", RIE_b_4, 619 },
        { "CGRJ", RIE_b_4, 620 },
        { "CIB", RIS_4, 620 },
        { "CGIB", RIS_4, 620 },
        { "CIJ", RIE_c_4, 620 },
        { "CGIJ", RIE_c_4, 620 },
        { "CFC", S_1_u, 621 },
        { "CS", RS_a_3, 628 },
        { "CSY", RSY_a_3, 628 },
        { "CSG", RSY_a_3, 628 },
        { "CDS", RS_a_3, 628 },
        { "CDSY", RSY_a_3, 628 },
        { "CDSG", RSY_a_3, 628 },
        { "CSST", SSF_3_dr, 630 },
        { "CRT", RRF_c_3, 633 },
        { "CGRT", RRF_c_3, 633 },
        { "CIT", RIE_a_3, 633 },
        { "CGIT", RIE_a_3, 633 },
        { "CH", RX_a_2_ux, 634 },
        { "CHY", RXY_a_2, 634 },
        { "CGH", RXY_a_2, 634 },
        { "CHI", RI_a_2_s, 634 },
        { "CGHI", RI_a_2_s, 634 },
        { "CHHSI", SIL_2_s, 634 },
        { "CHSI", SIL_2_s, 634 },
        { "CGHSI", SIL_2_s, 634 },
        { "CHRL", RIL_b_2, 634 },
        { "CGHRL", RIL_b_2, 634 },
        { "CHHR", RRE_2, 635 },
        { "CHLR", RRE_2, 635 },
        { "CHF", RXY_a_2, 635 },
        { "CIH", RIL_a_2, 635 },
        { "CLR", RR_2, 636 },
        { "CLGR", RRE_2, 636 },
        { "CLGFR", RRE_2, 636 },
        { "CL", RX_a_2_ux, 636 },
        { "CLY", RXY_a_2, 636 },
        { "CLG", RXY_a_2, 636 },
        { "CLGF", RXY_a_2, 636 },
        { "CLC", SS_a_2_u, 636 },
        { "CLFI", RIL_a_2, 636 },
        { "CLGFI", RIL_a_2, 636 },
        { "CLI", SI_2_u, 636 },
        { "CLIY", SIY_2_uu, 636 },
        { "CLFHSI", SIL_2_u, 636 },
        { "CLGHSI", SIL_2_u, 636 },
        { "CLHHSI", SIL_2_u, 636 },
        { "CLRL", RIL_b_2, 637 },
        { "CLGRL", RIL_b_2, 637 },
        { "CLGFRL", RIL_b_2, 637 },
        { "CLHRL", RIL_b_2, 637 },
        { "CLGHRL", RIL_b_2, 637 },
        { "CLRB", RRS_4, 638 },
        { "CLGRB", RRS_4, 638 },
        { "CLRJ", RIE_b_4, 638 },
        { "CLGRJ", RIE_b_4, 638 },
        { "CLIB", RIS_4, 638 },
        { "CLGIB", RIS_4, 638 },
        { "CLIJ", RIE_c_4, 638 },
        { "CLGIJ", RIE_c_4, 638 },
        { "CLRT", RRF_c_3, 639 },
        { "CLGRT", RRF_c_3, 639 },
        { "CLT", RSY_b_3_ux, 639 },
        { "CLGT", RSY_b_3_ux, 639 },
        { "CLFIT", RIE_a_3, 640 },
        { "CLGIT", RIE_a_3, 640 },
        { "CLM", RS_b_3, 641 },
        { "CLMY", RSY_b_3_us, 641 },
        { "CLMH", RSY_b_3_us, 641 },
        { "CLHHR", RRE_2, 641 },
        { "CLHLR", RRE_2, 641 },
        { "CLHF", RXY_a_2, 641 },
        { "CLCL", RR_2, 642 },
        { "CLIH", RIL_a_2, 642 },
        { "CLCLE", RS_a_3, 644 },
        { "CLCLU", RSY_a_3, 647 },
        { "CLST", RRE_2, 650 },
        { "CUSE", RRE_2, 651 },
        { "CMPSC", RRE_2, 654 },
        { "KIMD", RRE_2, 672 },
        { "KLMD", RRE_2, 685 },
        { "KMAC", RRE_2, 703 },
        { "CVB", RX_a_2_ux, 714 },
        { "CVBY", RXY_a_2, 714 },
        { "CVBG", RXY_a_2, 714 },
        { "CVD", RX_a_2_ux, 715 },
        { "CVDY", RXY_a_2, 715 },
        { "CVDG", RXY_a_2, 715 },
        { "CU24", RRF_c_3_opt, 715 },
        { "CUUTF", RRF_c_3_opt, 718 },
        { "CU21", RRF_c_3_opt, 718 },
        { "CU42", RRE_2, 722 },
        { "CU41", RRE_2, 725 },
        { "CUTFU", RRF_c_3_opt, 728 },
        { "CU12", RRF_c_3_opt, 728 },
        { "CU14", RRF_c_3_opt, 732 },
        { "CPYA", RRE_2, 736 },
        { "DR", RR_2, 736 },
        { "D", RX_a_2_ux, 736 },
        { "DLR", RRE_2, 737 },
        { "DLGR", RRE_2, 737 },
        { "DL", RXY_a_2, 737 },
        { "DLG", RXY_a_2, 737 },
        { "DSGR", RRE_2, 738 },
        { "DSGFR", RRE_2, 738 },
        { "DSG", RXY_a_2, 738 },
        { "DSGF", RXY_a_2, 738 },
        { "HIO", S_1_u, 129 },
        { "HDV", S_1_u, 129 },
        { "SIO", S_1_u, 129 },
        { "SIOF", S_1_u, 129 },
        { "STIDC", S_1_u, 129 },
        { "CLRCH", S_1_u, 367 },
        { "CLRIO", S_1_u, 368 },
        { "TCH", S_1_u, 384 },
        { "TIO", S_1_u, 385 },
        { "RRB", S_1_u, 295 },
        { "CONCS", S_1_u, 263 },
        { "DISCS", S_1_u, 265 },
        { "XR", RR_2, 738 },
        { "XGR", RRE_2, 738 },
        { "XRK", RRF_a_3, 738 },
        { "XGRK", RRF_a_3, 738 },
        { "X", RX_a_2_ux, 738 },
        { "XY", RXY_a_2, 738 },
        { "XG", RXY_a_2, 738 },
        { "XI", SI_2_u, 739 },
        { "XIY", SIY_2_su, 739 },
        { "XC", SS_a_2_s, 739 },
        { "EX", RX_a_2_ux, 740 },
        { "XIHF", RIL_a_2, 740 },
        { "XILF", RIL_a_2, 740 },
        { "EXRL", RIL_b_2, 740 },
        { "EAR", RRE_2, 741 },
        { "ECAG", RSY_a_3, 741 },
        { "ECTG", SSF_3_dr, 744 },
        { "EPSW", RRE_2, 745 },
        { "ETND", RRE_1, 745 },
        { "FLOGR", RRE_2, 746 },
        { "IC", RX_a_2_ux, 746 },
        { "ICY", RXY_a_2, 746 },
        { "ICM", RS_b_3, 746 },
        { "ICMY", RSY_b_3_us, 746 },
        { "ICMH", RSY_b_3_us, 746 },
        { "IIHF", RIL_a_2, 747 },
        { "IIHH", RI_a_2_u, 747 },
        { "IIHL", RI_a_2_u, 747 },
        { "IILF", RIL_a_2, 747 },
        { "IILH", RI_a_2_u, 747 },
        { "IILL", RI_a_2_u, 747 },
        { "IPM", RRE_1, 748 },
        { "LR", RR_2, 748 },
        { "LGR", RRE_2, 748 },
        { "LGFR", RRE_2, 748 },
        { "L", RX_a_2_ux, 748 },
        { "LY", RXY_a_2, 748 },
        { "LG", RXY_a_2, 748 },
        { "LGF", RXY_a_2, 748 },
        { "LGFI", RIL_a_2, 748 },
        { "LRL", RIL_b_2, 748 },
        { "LGRL", RIL_b_2, 748 },
        { "LGFRL", RIL_b_2, 748 },
        { "LAM", RS_a_3, 749 },
        { "LAMY", RSY_a_3, 749 },
        { "LA", RX_a_2_ux, 750 },
        { "LAY", RXY_a_2, 750 },
        { "LAE", RX_a_2_ux, 750 },
        { "LAEY", RXY_a_2, 750 },
        { "LARL", RIL_b_2, 751 },
        { "LAA", RSY_a_3, 752 },
        { "LAAG", RSY_a_3, 752 },
        { "LAAL", RSY_a_3, 752 },
        { "LAALG", RSY_a_3, 752 },
        { "LAN", RSY_a_3, 753 },
        { "LANG", RSY_a_3, 753 },
        { "LAX", RSY_a_3, 753 },
        { "LAXG", RSY_a_3, 753 },
        { "LAO", RSY_a_3, 754 },
        { "LAOG", RSY_a_3, 754 },
        { "LTR", RR_2, 754 },
        { "LTGR", RRE_2, 754 },
        { "LTGFR", RRE_2, 754 },
        { "LT", RXY_a_2, 755 },
        { "LTG", RXY_a_2, 755 },
        { "LTGF", RXY_a_2, 755 },
        { "LAT", RXY_a_2, 755 },
        { "LGAT", RXY_a_2, 755 },
        { "LZRF", RXY_a_2, 755 },
        { "LZRG", RXY_a_2, 755 },
        { "LBR", RRE_2, 756 },
        { "LGBR", RRE_2, 756 },
        { "LB", RXY_a_2, 756 },
        { "LGB", RXY_a_2, 756 },
        { "LBH", RXY_a_2, 756 },
        { "LCR", RR_2, 756 },
        { "LCGR", RRE_2, 757 },
        { "LCGFR", RRE_2, 757 },
        { "LCBB", RXE_3_xm, 757 },
        { "LGG", RXY_a_2, 758 },
        { "LLGFSG", RXY_a_2, 758 },
        { "LGSC", RXY_a_2, 759 },
        { "LHR", RRE_2, 760 },
        { "LGHR", RRE_2, 760 },
        { "LH", RX_a_2_ux, 760 },
        { "LHY", RXY_a_2, 760 },
        { "LGH", RXY_a_2, 760 },
        { "LHI", RI_a_2_s, 760 },
        { "LGHI", RI_a_2_s, 760 },
        { "LHRL", RIL_b_2, 760 },
        { "LGHRL", RIL_b_2, 760 },
        { "LHH", RXY_a_2, 761 },
        { "LOCHI", RIE_g_3, 761 },
        { "LOCGHI", RIE_g_3, 761 },
        { "LOCHHI", RIE_g_3, 761 },
        { "LFH", RXY_a_2, 762 },
        { "LFHAT", RXY_a_2, 762 },
        { "LLGFR", RRE_2, 762 },
        { "LLGF", RXY_a_2, 762 },
        { "LLGFRL", RIL_b_2, 762 },
        { "LLGFAT", RXY_a_2, 763 },
        { "LLCR", RRE_2, 763 },
        { "LLGCR", RRE_2, 763 },
        { "LLC", RXY_a_2, 763 },
        { "LLGC", RXY_a_2, 763 },
        { "LLZRGF", RXY_a_2, 763 },
        { "LLCH", RXY_a_2, 764 },
        { "LLHR", RRE_2, 764 },
        { "LLGHR", RRE_2, 764 },
        { "LLH", RXY_a_2, 764 },
        { "LLGH", RXY_a_2, 764 },
        { "LLHRL", RIL_b_2, 764 },
        { "LLGHRL", RIL_b_2, 764 },
        { "LLHH", RXY_a_2, 765 },
        { "LLIHF", RIL_a_2, 765 },
        { "LLIHH", RI_a_2_u, 765 },
        { "LLIHL", RI_a_2_u, 765 },
        { "LLILF", RIL_a_2, 765 },
        { "LLILH", RI_a_2_u, 765 },
        { "LLILL", RI_a_2_u, 765 },
        { "LLGTR", RRE_2, 765 },
        { "LLGT", RXY_a_2, 766 },
        { "LLGTAT", RXY_a_2, 766 },
        { "LM", RS_a_3, 766 },
        { "LMY", RSY_a_3, 766 },
        { "LMG", RSY_a_3, 766 },
        { "LMD", SS_e_4_rb, 767 },
        { "LMH", RSY_a_3, 767 },
        { "LNR", RR_2, 767 },
        { "LNGR", RRE_2, 767 },
        { "LNGFR", RRE_2, 768 },
        { "LOCFHR", RRF_c_3, 768 },
        { "LOCFH", RSY_b_3_su, 768 },
        { "LOCR", RRF_c_3, 768 },
        { "LOCGR", RRF_c_3, 768 },
        { "LOC", RSY_b_3_su, 768 },
        { "LOCG", RSY_b_3_su, 768 },
        { "LPD", SSF_3_rd, 769 },
        { "LPDG", SSF_3_rd, 769 },
        { "LPQ", RXY_a_2, 770 },
        { "LPR", RR_2, 771 },
        { "LPGR", RRE_2, 771 },
        { "LPGFR", RRE_2, 771 },
        { "LRVR", RRE_2, 771 },
        { "LRVGR", RRE_2, 771 },
        { "LRVH", RXY_a_2, 771 },
        { "LRV", RXY_a_2, 771 },
        { "LRVG", RXY_a_2, 771 },
        { "MC", SI_2_s, 772 },
        { "MVC", SS_a_2_u, 773 },
        { "MVCRL", SSE_2, 788 },
        { "MVHHI", SIL_2_s, 773 },
        { "MVHI", SIL_2_s, 773 },
        { "MVGHI", SIL_2_s, 773 },
        { "MVI", SI_2_u, 773 },
        { "MVIY", SIY_2_uu, 773 },
        { "MVCIN", SS_a_2_u, 774 },
        { "MVCL", RR_2, 774 },
        { "MVCLE", RS_a_3, 778 },
        { "MVCLU", RSY_a_3, 781 },
        { "MVN", SS_a_2_u, 785 },
        { "MVST", RRE_2, 785 },
        { "MVO", SS_b_2, 786 },
        { "MVZ", SS_a_2_u, 787 },
        { "MR", RR_2, 788 },
        { "MGRK", RRF_a_3, 788 },
        { "M", RX_a_2_ux, 788 },
        { "MFY", RXY_a_2, 788 },
        { "MG", RXY_a_2, 788 },
        { "MH", RX_a_2_ux, 789 },
        { "MHY", RXY_a_2, 789 },
        { "MGH", RXY_a_2, 789 },
        { "MHI", RI_a_2_s, 789 },
        { "MGHI", RI_a_2_s, 789 },
        { "MLR", RRE_2, 790 },
        { "MLGR", RRE_2, 790 },
        { "ML", RXY_a_2, 790 },
        { "MLG", RXY_a_2, 790 },
        { "MSR", RRE_2, 791 },
        { "MSRKC", RRF_a_3, 791 },
        { "MSGR", RRE_2, 791 },
        { "MSGRKC", RRF_a_3, 791 },
        { "MSGFR", RRE_2, 791 },
        { "MS", RX_a_2_ux, 791 },
        { "MSC", RXY_a_2, 791 },
        { "MSY", RXY_a_2, 791 },
        { "MSG", RXY_a_2, 791 },
        { "MSGC", RXY_a_2, 791 },
        { "MSGF", RXY_a_2, 791 },
        { "MSFI", RIL_a_2, 791 },
        { "MSGFI", RIL_a_2, 791 },
        { "NIAI", IE_2, 792 },
        { "NTSTG", RXY_a_2, 794 },
        { "NCGRK", RRF_a_3, 522 },
        { "NCRK", RRF_a_3, 522 },
        { "NNRK", RRF_a_3, 796 },
        { "NNGRK", RRF_a_3, 796 },
        { "NOGRK", RRF_a_3, 799 },
        { "NORK", RRF_a_3, 799 },
        { "NXRK", RRF_a_3, 799 },
        { "NXGRK", RRF_a_3, 799 },
        { "OR", RR_2, 794 },
        { "OGR", RRE_2, 794 },
        { "ORK", RRF_a_3, 794 },
        { "OCGRK", RRF_a_3, 802 },
        { "OCRK", RRF_a_3, 802 },
        { "OGRK", RRF_a_3, 794 },
        { "O", RX_a_2_ux, 794 },
        { "OY", RXY_a_2, 794 },
        { "OG", RXY_a_2, 795 },
        { "OI", SI_2_u, 795 },
        { "OIY", SIY_2_su, 795 },
        { "OC", SS_a_2_u, 795 },
        { "OIHF", RIL_a_2, 796 },
        { "OIHH", RI_a_2_u, 796 },
        { "OIHL", RI_a_2_u, 796 },
        { "OILF", RIL_a_2, 796 },
        { "OILH", RI_a_2_u, 796 },
        { "OILL", RI_a_2_u, 796 },
        { "PACK", SS_b_2, 796 },
        { "PKA", SS_f_2, 797 },
        { "PKU", SS_f_2, 798 },
        { "PCC", RRE_0, 799 },
        { "PLO", SS_e_4_br, 815 },
        { "PPA", RRF_c_3, 829 },
        { "PRNO", RRE_2, 830 },
        { "PPNO", RRE_2, 830 },
        { "POPCNT", RRF_c_3_opt, 853 },
        { "PFD", RXY_b_2, 843 },
        { "PFDRL", RIL_c_2, 843 },
        { "RLL", RSY_a_3, 845 },
        { "RLLG", RSY_a_3, 845 },
        { "RNSBG", RIE_f_5, 845 },
        { "RXSBG", RIE_f_5, 846 },
        { "ROSBG", RIE_f_5, 846 },
        { "RISBG", RIE_f_5, 847 },
        { "RISBGN", RIE_f_5, 847 },
        { "RISBHG", RIE_f_5, 848 },
        { "RISBLG", RIE_f_5, 849 },
        { "RNSBGT", RIE_f_5, 845 },
        { "RXSBGT", RIE_f_5, 846 },
        { "ROSBGT", RIE_f_5, 858 },
        { "RISBGZ", RIE_f_5, 858 },
        { "RISBGNZ", RIE_f_5, 860 },
        { "RISBHGZ", RIE_f_5, 860 },
        { "RISBLGZ", RIE_f_5, 860 },
        { "SRST", RRE_2, 850 },
        { "SRSTU", RRE_2, 852 },
        { "SAR", RRE_2, 854 },
        { "SAM24", E_0, 854 },
        { "SAM31", E_0, 854 },
        { "SAM64", E_0, 854 },
        { "SPM", RR_1, 855 },
        { "SLDA", RS_a_2, 855 },
        { "SLA", RS_a_2, 856 },
        { "SLAK", RSY_a_3, 856 },
        { "SLAG", RSY_a_3, 856 },
        { "SLDL", RS_a_2, 856 },
        { "SLL", RS_a_2, 857 },
        { "SLLK", RSY_a_3, 857 },
        { "SLLG", RSY_a_3, 857 },
        { "SRDA", RS_a_2, 858 },
        { "SRDL", RS_a_2, 858 },
        { "SRA", RS_a_2, 859 },
        { "SRAK", RSY_a_3, 859 },
        { "SRAG", RSY_a_3, 859 },
        { "SRL", RS_a_2, 860 },
        { "SRLK", RSY_a_3, 860 },
        { "SRLG", RSY_a_3, 860 },
        { "ST", RX_a_2_ux, 860 },
        { "STY", RXY_a_2, 861 },
        { "STG", RXY_a_2, 861 },
        { "STRL", RIL_b_2, 861 },
        { "STGRL", RIL_b_2, 861 },
        { "STAM", RS_a_3, 861 },
        { "STAMY", RSY_a_3, 861 },
        { "STC", RX_a_2_ux, 862 },
        { "STCY", RXY_a_2, 862 },
        { "STCH", RXY_a_2, 862 },
        { "STCM", RS_b_3, 862 },
        { "STCMY", RSY_b_3_us, 862 },
        { "STCMH", RSY_b_3_us, 862 },
        { "STCK", S_1_u, 863 },
        { "STCKF", S_1_u, 863 },
        { "STCKE", S_1_u, 864 },
        { "STFLE", S_1_s, 866 },
        { "STGSC", RXY_a_2, 867 },
        { "STH", RX_a_2_ux, 867 },
        { "STHY", RXY_a_2, 868 },
        { "STHRL", RIL_b_2, 868 },
        { "STHH", RXY_a_2, 868 },
        { "STFH", RXY_a_2, 868 },
        { "STM", RS_a_3, 869 },
        { "STMY", RSY_a_3, 869 },
        { "STMG", RSY_a_3, 869 },
        { "STMH", RSY_a_3, 869 },
        { "STOC", RSY_b_3_su, 869 },
        { "STOCG", RSY_b_3_su, 869 },
        { "STOCFH", RSY_b_3_su, 870 },
        { "STPQ", RXY_a_2, 870 },
        { "STRVH", RXY_a_2, 871 },
        { "STRV", RXY_a_2, 871 },
        { "STRVG", RXY_a_2, 871 },
        { "SR", RR_2, 871 },
        { "SGR", RRE_2, 871 },
        { "SGFR", RRE_2, 871 },
        { "SRK", RRF_a_3, 871 },
        { "SGRK", RRF_a_3, 872 },
        { "S", RX_a_2_ux, 872 },
        { "SY", RXY_a_2, 872 },
        { "SG", RXY_a_2, 872 },
        { "SGF", RXY_a_2, 872 },
        { "SH", RX_a_2_ux, 872 },
        { "SHY", RXY_a_2, 872 },
        { "SGH", RXY_a_2, 872 },
        { "SHHHR", RRF_a_3, 873 },
        { "SHHLR", RRF_a_3, 873 },
        { "SLR", RR_2, 873 },
        { "SLGR", RRE_2, 873 },
        { "SLGFR", RRE_2, 873 },
        { "SLRK", RRF_a_3, 873 },
        { "SLGRK", RRF_a_3, 873 },
        { "SL", RX_a_2_ux, 874 },
        { "SLY", RXY_a_2, 874 },
        { "SLG", RXY_a_2, 874 },
        { "SLGF", RXY_a_2, 874 },
        { "SLFI", RIL_a_2, 874 },
        { "SLGFI", RIL_a_2, 874 },
        { "SLHHHR", RRF_a_3, 875 },
        { "SLHHLR", RRF_a_3, 875 },
        { "SLBR", RRE_2, 875 },
        { "SLBGR", RRE_2, 875 },
        { "SLB", RXY_a_2, 875 },
        { "SLBG", RXY_a_2, 875 },
        { "SVC", I_1, 876 },
        { "TS", SI_1, 876 },
        { "TAM", E_0, 876 },
        { "TM", SI_2_u, 877 },
        { "TMY", SIY_2_su, 877 },
        { "TMHH", RI_a_2_u, 877 },
        { "TMHL", RI_a_2_u, 877 },
        { "TMH", RI_a_2_u, 877 },
        { "TMLH", RI_a_2_u, 877 },
        { "TML", RI_a_2_u, 877 },
        { "TMLL", RI_a_2_u, 877 },
        { "TABORT", S_1_u, 878 },
        { "TBEGIN", SIL_2_s, 879 },
        { "TBEGINC", SIL_2_s, 883 },
        { "TEND", S_0, 885 },
        { "TR", SS_a_2_u, 886 },
        { "TRT", SS_a_2_u, 887 },
        { "TRTE", RRF_c_3_opt, 887 },
        { "TRTRE", RRF_c_3_opt, 888 },
        { "TRTR", SS_a_2_u, 892 },
        { "TRE", RRE_2, 893 },
        { "TROO", RRF_c_3_opt, 895 },
        { "TROT", RRF_c_3_opt, 895 },
        { "TRTO", RRF_c_3_opt, 895 },
        { "TRTT", RRF_c_3_opt, 895 },
        { "UNPK", SS_b_2, 900 },
        { "UNPKA", SS_a_2_u, 901 },
        { "UNPKU", SS_a_2_u, 902 },
        { "UPT", E_0, 903 },
        { "AP", SS_b_2, 920 },
        { "CP", SS_b_2, 921 },
        { "DP", SS_b_2, 921 },
        { "DFLTCC", RRF_a_3, 1714 },
        { "ED", SS_a_2_u, 922 },
        { "EDMK", SS_a_2_u, 925 },
        { "SRP", SS_c_3, 926 },
        { "MP", SS_b_2, 926 },
        { "SP", SS_b_2, 927 },
        { "TP", RSL_a_1, 928 },
        { "ZAP", SS_b_2, 928 },
        { "THDR", RRE_2, 955 },
        { "THDER", RRE_2, 955 },
        { "TBEDR", RRF_e_3, 956 },
        { "TBDR", RRF_e_3, 956 },
        { "CPSDR", RRF_b_3, 958 },
        { "EFPC", RRE_1, 958 },
        { "LER", RR_2, 959 },
        { "LDR", RR_2, 959 },
        { "LXR", RRE_2, 959 },
        { "LE", RX_a_2_ux, 959 },
        { "LD", RX_a_2_ux, 959 },
        { "LEY", RXY_a_2, 959 },
        { "LDY", RXY_a_2, 959 },
        { "LCDFR", RRE_2, 959 },
        { "LFPC", S_1_u, 959 },
        { "LFAS", S_1_u, 960 },
        { "LDGR", RRE_2, 962 },
        { "LGDR", RRE_2, 962 },
        { "LNDFR", RRE_2, 962 },
        { "LPDFR", RRE_2, 962 },
        { "LZER", RRE_1, 963 },
        { "LZXR", RRE_1, 963 },
        { "LZDR", RRE_1, 963 },
        { "PFPO", E_0, 963 },
        { "SRNM", S_1_u, 975 },
        { "SRNMB", S_1_u, 975 },
        { "SRNMT", S_1_u, 975 },
        { "SFPC", RRE_1, 975 },
        { "SFASR", RRE_1, 976 },
        { "STE", RX_a_2_ux, 976 },
        { "STD", RX_a_2_ux, 976 },
        { "STDY", RXY_a_2, 977 },
        { "STEY", RXY_a_2, 977 },
        { "STFPC", S_1_u, 977 },
        { "BSA", RRE_2, 989 },
        { "BAKR", RRE_2, 993 },
        { "BSG", RRE_2, 995 },
        { "CRDTE", RRF_b_4_opt, 999 },
        { "CSP", RRE_2, 1003 },
        { "CSPG", RRE_2, 1003 },
        { "ESEA", RRE_1, 1006 },
        { "EPAR", RRE_1, 1006 },
        { "EPAIR", RRE_1, 1006 },
        { "ESAR", RRE_1, 1006 },
        { "ESAIR", RRE_1, 1007 },
        { "EREG", RRE_2, 1007 },
        { "EREGG", RRE_2, 1007 },
        { "ESTA", RRE_2, 1008 },
        { "IAC", RRE_1, 1011 },
        { "IPK", S_0, 1012 },
        { "IRBM", RRE_2, 1012 },
        { "ISK", RR_2, 268 },
        { "ISKE", RRE_2, 1012 },
        { "IVSK", RRE_2, 1013 },
        { "IDTE", RRF_b_4_opt, 1014 },
        { "IPTE", RRF_a_4_opt, 1019 },
        { "LASP", SSE_2, 1023 },
        { "LCTL", RS_a_3, 1032 },
        { "LCTLG", RSY_a_3, 1032 },
        { "LPTEA", RRF_b_4, 1032 },
        { "LPSW", SI_1, 1036 },
        { "LPSWE", S_1_u, 1037 },
        { "LRA", RX_a_2_ux, 1038 },
        { "LRAY", RXY_a_2, 1038 },
        { "LRAG", RXY_a_2, 1038 },
        { "LURA", RRE_2, 1042 },
        { "LURAG", RRE_2, 1042 },
        { "MSTA", RRE_1, 1043 },
        { "MVPG", RRE_2, 1044 },
        { "MVCP", SS_d_3, 1046 },
        { "MVCS", SS_d_3, 1046 },
        { "MVCDK", SSE_2, 1048 },
        { "MVCK", SS_d_3, 1049 },
        { "MVCOS", SSF_3_dr, 1050 },
        { "MVCSK", SSE_2, 1053 },
        { "PGIN", RRE_2, 1054 },
        { "PGOUT", RRE_2, 1055 },
        { "PCKMO", RRE_0, 1056 },
        { "PFMF", RRE_2, 1059 },
        { "PTFF", E_0, 1063 },
        { "PTF", RRE_1, 1071 },
        { "PC", S_1_u, 1072 },
        { "PR", E_0, 1085 },
        { "PTI", RRE_2, 1089 },
        { "PT", RRE_2, 1089 },
        { "PALB", RRE_0, 1098 },
        { "PTLB", S_0, 1098 },
        { "RRBE", RRE_2, 1098 },
        { "RRBM", RRE_2, 1099 },
        { "RP", S_1_u, 1099 },
        { "SAC", S_1_u, 1102 },
        { "SACF", S_1_u, 1102 },
        { "SCK", S_1_u, 1103 },
        { "SCKC", S_1_u, 1104 },
        { "SCKPF", E_0, 1105 },
        { "SPX", S_1_u, 1105 },
        { "SPT", S_1_u, 1105 },
        { "SPKA", S_1_u, 1106 },
        { "SSAR", RRE_1, 1107 },
        { "SSAIR", RRE_1, 1107 },
        { "SSK", RR_2, 304 },
        { "SSKE", RRF_c_3_opt, 1112 },
        { "SSM", SI_1, 1115 },
        { "SIGP", RS_a_3, 1115 },
        { "STCKC", S_1_u, 1117 },
        { "STCTL", RS_a_3, 1117 },
        { "STCTG", RSY_a_3, 1117 },
        { "STAP", S_1_u, 1118 },
        { "STIDP", S_1_u, 1118 },
        { "STPT", S_1_u, 1120 },
        { "STFL", S_1_u, 1120 },
        { "STPX", S_1_u, 1121 },
        { "STRAG", SSE_2, 1121 },
        { "STSI", S_1_u, 1122 },
        { "STOSM", SI_2_u, 1146 },
        { "STNSM", SI_2_u, 1146 },
        { "STURA", RRE_2, 1147 },
        { "STURG", RRE_2, 1147 },
        { "TAR", RRE_2, 1147 },
        { "TB", RRE_2, 1149 },
        { "TPEI", RRE_2, 1151 },
        { "TPROT", SSE_2, 1152 },
        { "TRACE", RS_a_3, 1155 },
        { "TRACG", RSY_a_3, 1155 },
        { "TRAP2", E_0, 1156 },
        { "TRAP4", S_1_u, 1156 },
        { "XSCH", S_0, 1215 },
        { "CSCH", S_0, 1217 },
        { "HSCH", S_0, 1218 },
        { "MSCH", S_1_u, 1219 },
        { "RCHP", S_0, 1221 },
        { "RSCH", S_0, 1222 },
        { "SAL", S_0, 1224 },
        { "SCHM", S_0, 1225 },
        { "SSCH", S_1_u, 1227 },
        { "STCPS", S_1_u, 1228 },
        { "STCRW", S_1_u, 1229 },
        { "STSCH", S_1_u, 1230 },
        { "TPI", S_1_u, 1231 },
        { "TSCH", S_1_u, 1232 },

        { "AER", RR_2, 1412 },
        { "ADR", RR_2, 1412 },
        { "AXR", RR_2, 1412 },
        { "AE", RX_a_2_ux, 1412 },
        { "AD", RX_a_2_ux, 1412 },
        { "AWR", RR_2, 1413 },
        { "AUR", RR_2, 1413 },
        { "AU", RX_a_2_ux, 1413 },
        { "AW", RX_a_2_ux, 1413 },
        { "CER", RR_2, 1414 },
        { "CDR", RR_2, 1414 },
        { "CXR", RRE_2, 1414 },
        { "CE", RX_a_2_ux, 1414 },
        { "CD", RX_a_2_ux, 1414 },
        { "CEFR", RRE_2, 1415 },
        { "CDFR", RRE_2, 1415 },
        { "CXFR", RRE_2, 1415 },
        { "CEGR", RRE_2, 1415 },
        { "CDGR", RRE_2, 1415 },
        { "CXGR", RRE_2, 1415 },
        { "CFER", RRF_e_3, 1415 },
        { "CFDR", RRF_e_3, 1415 },
        { "CFXR", RRF_e_3, 1415 },
        { "CGER", RRF_e_3, 1415 },
        { "CGDR", RRF_e_3, 1415 },
        { "CGXR", RRF_e_3, 1415 },
        { "DDR", RR_2, 1416 },
        { "DER", RR_2, 1416 },
        { "DXR", RRE_2, 1416 },
        { "DD", RX_a_2_ux, 1416 },
        { "DE", RX_a_2_ux, 1416 },
        { "HDR", RR_2, 1417 },
        { "HER", RR_2, 1417 },
        { "LTER", RR_2, 1417 },
        { "LTDR", RR_2, 1417 },
        { "LTXR", RRE_2, 1418 },
        { "LCER", RR_2, 1418 },
        { "LCDR", RR_2, 1418 },
        { "LCXR", RRE_2, 1418 },
        { "FIER", RRE_2, 1419 },
        { "FIDR", RRE_2, 1419 },
        { "FIXR", RRE_2, 1419 },
        { "LDER", RRE_2, 1419 },
        { "LXDR", RRE_2, 1419 },
        { "LXER", RRE_2, 1419 },
        { "LDE", RXE_2, 1419 },
        { "LXD", RXE_2, 1419 },
        { "LXE", RXE_2, 1419 },
        { "LNDR", RR_2, 1420 },
        { "LNER", RR_2, 1420 },
        { "LPDR", RR_2, 1420 },
        { "LPER", RR_2, 1420 },
        { "LNXR", RRE_2, 1420 },
        { "LPXR", RRE_2, 1420 },
        { "LEDR", RR_2, 1421 },
        { "LDXR", RR_2, 1421 },
        { "LRER", RR_2, 1421 },
        { "LRDR", RR_2, 1421 },
        { "LEXR", RRE_2, 1421 },
        { "MEER", RRE_2, 1421 },
        { "MDR", RR_2, 1421 },
        { "MXR", RR_2, 1421 },
        { "MDER", RR_2, 1421 },
        { "MER", RR_2, 1421 },
        { "MXDR", RR_2, 1421 },
        { "MEE", RXE_2, 1422 },
        { "MD", RX_a_2_ux, 1422 },
        { "MDE", RX_a_2_ux, 1422 },
        { "MXD", RX_a_2_ux, 1422 },
        { "ME", RX_a_2_ux, 1422 },
        { "MAER", RRD_3, 1423 },
        { "MADR", RRD_3, 1423 },
        { "MAD", RXF_3_x, 1423 },
        { "MAE", RXF_3_x, 1423 },
        { "MSER", RRD_3, 1423 },
        { "MSDR", RRD_3, 1423 },
        { "MSE", RXF_3_x, 1423 },
        { "MSD", RXF_3_x, 1423 },
        { "MAYR", RRD_3, 1424 },
        { "MAYHR", RRD_3, 1424 },
        { "MAYLR", RRD_3, 1424 },
        { "MAY", RXF_3_x, 1424 },
        { "MAYH", RXF_3_x, 1424 },
        { "MAYL", RXF_3_x, 1424 },
        { "MYR", RRD_3, 1426 },
        { "MYHR", RRD_3, 1426 },
        { "MYLR", RRD_3, 1426 },
        { "MY", RXF_3_x, 1426 },
        { "MYH", RXF_3_x, 1426 },
        { "MYL", RXF_3_x, 1426 },
        { "SQER", RRE_2, 1427 },
        { "SQDR", RRE_2, 1427 },
        { "SQXR", RRE_2, 1427 },
        { "SQE", RXE_2, 1427 },
        { "SQD", RXE_2, 1427 },
        { "SER", RR_2, 1428 },
        { "SDR", RR_2, 1428 },
        { "SXR", RR_2, 1428 },
        { "SE", RX_a_2_ux, 1428 },
        { "SD", RX_a_2_ux, 1428 },
        { "SUR", RR_2, 1429 },
        { "SWR", RR_2, 1429 },
        { "SU", RX_a_2_ux, 1429 },
        { "SW", RX_a_2_ux, 1429 },
        { "AEBR", RRE_2, 1445 },
        { "ADBR", RRE_2, 1445 },
        { "AXBR", RRE_2, 1445 },
        { "AEB", RXE_2, 1445 },
        { "ADB", RXE_2, 1445 },
        { "CEBR", RRE_2, 1447 },
        { "CDBR", RRE_2, 1447 },
        { "CXBR", RRE_2, 1447 },
        { "CDB", RXE_2, 1447 },
        { "CEB", RXE_2, 1447 },
        { "KEBR", RRE_2, 1448 },
        { "KDBR", RRE_2, 1448 },
        { "KDSA", RRE_2, 1700 },
        { "KXBR", RRE_2, 1448 },
        { "KDB", RXE_2, 1448 },
        { "KEB", RXE_2, 1448 },
        { "CEFBR", RRE_2, 1449 },
        { "CDFBR", RRE_2, 1449 },
        { "CXFBR", RRE_2, 1449 },
        { "CEGBR", RRE_2, 1449 },
        { "CDGBR", RRE_2, 1449 },
        { "CXGBR", RRE_2, 1449 },
        { "CEFBRA", RRF_e_4, 1449 },
        { "CDFBRA", RRF_e_4, 1449 },
        { "CXFBRA", RRF_e_4, 1449 },
        { "CEGBRA", RRF_e_4, 1449 },
        { "CDGBRA", RRF_e_4, 1449 },
        { "CXGBRA", RRF_e_4, 1449 },
        { "CELFBR", RRF_e_4, 1451 },
        { "CDLFBR", RRF_e_4, 1451 },
        { "CXLFBR", RRF_e_4, 1451 },
        { "CELGBR", RRF_e_4, 1451 },
        { "CDLGBR", RRF_e_4, 1451 },
        { "CXLGBR", RRF_e_4, 1451 },
        { "CFEBR", RRF_e_3, 1452 },
        { "CFDBR", RRF_e_3, 1452 },
        { "CFXBR", RRF_e_3, 1452 },
        { "CGEBR", RRF_e_3, 1452 },
        { "CGDBR", RRF_e_3, 1452 },
        { "CGXBR", RRF_e_3, 1452 },
        { "CFEBRA", RRF_e_4, 1452 },
        { "CFDBRA", RRF_e_4, 1452 },
        { "CFXBRA", RRF_e_4, 1452 },
        { "CGEBRA", RRF_e_4, 1452 },
        { "CGDBRA", RRF_e_4, 1452 },
        { "CGXBRA", RRF_e_4, 1452 },
        { "CLFEBR", RRF_e_4, 1455 },
        { "CLFDBR", RRF_e_4, 1455 },
        { "CLFXBR", RRF_e_4, 1455 },
        { "CLGEBR", RRF_e_4, 1455 },
        { "CLGDBR", RRF_e_4, 1455 },
        { "CLGXBR", RRF_e_4, 1455 },
        { "DEBR", RRE_2, 1457 },
        { "DDBR", RRE_2, 1457 },
        { "DXBR", RRE_2, 1457 },
        { "DEB", RXE_2, 1457 },
        { "DDB", RXE_2, 1457 },
        { "DIEBR", RRF_b_4, 1458 },
        { "DIDBR", RRF_b_4, 1458 },
        { "LTEBR", RRE_2, 1461 },
        { "LTDBR", RRE_2, 1461 },
        { "LTXBR", RRE_2, 1461 },
        { "LCEBR", RRE_2, 1461 },
        { "LCDBR", RRE_2, 1461 },
        { "LCXBR", RRE_2, 1461 },
        { "ECCTR", RRE_2, 39 },
        { "EPCTR", RRE_2, 39 },
        { "ECPGA", RRE_2, 39 },
        { "FIEBR", RRF_e_3, 1462 },
        { "FIDBR", RRF_e_3, 1462 },
        { "FIXBR", RRF_e_3, 1462 },
        { "FIEBRA", RRF_e_4, 1462 },
        { "FIDBRA", RRF_e_4, 1462 },
        { "FIXBRA", RRF_e_4, 1462 },
        { "LSCTL", S_1_u, 42 },
        { "LDEBR", RRE_2, 1463 },
        { "LXDBR", RRE_2, 1463 },
        { "LPCTL", S_1_u, 41 },
        { "LCCTL", S_1_u, 40 },
        { "LXEBR", RRE_2, 1463 },
        { "LDEB", RRE_2, 1464 },
        { "LXDB", RRE_2, 1464 },
        { "LXEB", RRE_2, 1464 },
        { "LNEBR", RRE_2, 1464 },
        { "LNDBR", RRE_2, 1464 },
        { "LNXBR", RRE_2, 1464 },
        { "LPEBR", RRE_2, 1465 },
        { "LPDBR", RRE_2, 1465 },
        { "LPXBR", RRE_2, 1465 },
        { "LEDBR", RRE_2, 1465 },
        { "LDXBR", RRE_2, 1465 },
        { "LEXBR", RRE_2, 1465 },
        { "LEDBRA", RRF_e_4, 1465 },
        { "LDXBRA", RRF_e_4, 1465 },
        { "LEXBRA", RRF_e_4, 1465 },
        { "LPP", S_1_u, 11 },
        { "MEEBR", RRE_2, 1467 },
        { "MDBR", RRE_2, 1467 },
        { "MXBR", RRE_2, 1467 },
        { "MDEBR", RRE_2, 1467 },
        { "MXDBR", RRE_2, 1467 },
        { "MEEB", RXE_2, 1467 },
        { "MDB", RXE_2, 1467 },
        { "MDEB", RXE_2, 1467 },
        { "MXDB", RXE_2, 1467 },
        { "MADBR", RRD_3, 1468 },
        { "MAEBR", RRD_3, 1468 },
        { "MAEB", RXF_3_x, 1468 },
        { "MADB", RXF_3_x, 1468 },
        { "MSEBR", RRD_3, 1468 },
        { "MSDBR", RRD_3, 1468 },
        { "MSEB", RXF_3_x, 1468 },
        { "MSDB", RXF_3_x, 1468 },
        { "QCTRI", S_1_u, 43 },
        { "QSI", S_1_u, 45 },
        { "SCCTR", RRE_2, 46 },
        { "SPCTR", RRE_2, 47 },
        { "SQEBR", RRE_2, 1470 },
        { "SQDBR", RRE_2, 1470 },
        { "SQXBR", RRE_2, 1470 },
        { "SQEB", RXE_2, 1470 },
        { "SQDB", RXE_2, 1470 },
        { "SEBR", RRE_2, 1470 },
        { "SDBR", RRE_2, 1470 },
        { "SXBR", RRE_2, 1470 },
        { "SEB", RXE_2, 1470 },
        { "SDB", RXE_2, 1470 },
        { "SORTL", RRE_2, 19 },
        { "TCEB", RXE_2, 1471 },
        { "TCDB", RXE_2, 1471 },
        { "TCXB", RXE_2, 1471 },
        { "ADTR", RRF_a_3, 1491 },
        { "AXTR", RRF_a_3, 1491 },
        { "ADTRA", RRF_a_4, 1491 },
        { "AXTRA", RRF_a_4, 1491 },
        { "CDTR", RRE_2, 1494 },
        { "CXTR", RRE_2, 1494 },
        { "KDTR", RRE_2, 1495 },
        { "KXTR", RRE_2, 1495 },
        { "CEDTR", RRE_2, 1495 },
        { "CEXTR", RRE_2, 1495 },
        { "CDGTR", RRE_2, 1496 },
        { "CXGTR", RRE_2, 1496 },
        { "CDGTRA", RRF_e_4, 1496 },
        { "CXGTRA", RRF_e_4, 1496 },
        { "CDFTR", RRF_e_4, 1496 },
        { "CXFTR", RRF_e_4, 1496 },
        { "CDLGTR", RRF_e_4, 1497 },
        { "CXLGTR", RRF_e_4, 1497 },
        { "CDLFTR", RRF_e_4, 1497 },
        { "CXLFTR", RRF_e_4, 1497 },
        { "CDPT", RSL_b_3, 1498 },
        { "CXPT", RSL_b_3, 1498 },
        { "CDSTR", RRE_2, 1500 },
        { "CXSTR", RRE_2, 1500 },
        { "CDUTR", RRE_2, 1500 },
        { "CXUTR", RRE_2, 1500 },
        { "CDZT", RSL_b_3, 1501 },
        { "CXZT", RSL_b_3, 1501 },
        { "CGDTR", RRF_e_3, 1501 },
        { "CGXTR", RRF_e_3, 1501 },
        { "CGDTRA", RRF_e_4, 1502 },
        { "CGXTRA", RRF_e_4, 1502 },
        { "CFDTR", RRF_e_4, 1502 },
        { "CFXTR", RRF_e_4, 1502 },
        { "CLGDTR", RRF_e_4, 1504 },
        { "CLGXTR", RRF_e_4, 1504 },
        { "CLFDTR", RRF_e_4, 1504 },
        { "CLFXTR", RRF_e_4, 1504 },
        { "CPDT", RSL_b_3, 1505 },
        { "CPXT", RSL_b_3, 1505 },
        { "CSDTR", RRF_d_3, 1507 },
        { "CSXTR", RRF_d_3, 1507 },
        { "CUDTR", RRE_2, 1507 },
        { "CUXTR", RRE_2, 1507 },
        { "CZDT", RSL_b_3, 1508 },
        { "CZXT", RSL_b_3, 1508 },
        { "DDTR", RRF_a_3, 1509 },
        { "DXTR", RRF_a_3, 1509 },
        { "DDTRA", RRF_a_4, 1509 },
        { "DXTRA", RRF_a_4, 1509 },
        { "EEXTR", RRE_2, 1511 },
        { "EEDTR", RRE_2, 1511 },
        { "ESDTR", RRE_2, 1511 },
        { "ESXTR", RRE_2, 1511 },
        { "IEDTR", RRF_b_3, 1512 },
        { "IEXTR", RRF_b_3, 1512 },
        { "LTDTR", RRE_2, 1513 },
        { "LTXTR", RRE_2, 1513 },
        { "FIDTR", RRF_e_4, 1514 },
        { "FIXTR", RRF_e_4, 1514 },
        { "LDETR", RRF_d_3, 1517 },
        { "LXDTR", RRF_d_3, 1517 },
        { "LEDTR", RRF_e_4, 1518 },
        { "LDXTR", RRF_e_4, 1518 },
        { "MDTR", RRF_a_3, 1519 },
        { "MXTR", RRF_a_3, 1519 },
        { "MDTRA", RRF_a_4, 1520 },
        { "MXTRA", RRF_a_4, 1520 },
        { "QADTR", RRF_b_4, 1521 },
        { "QAXTR", RRF_b_4, 1521 },
        { "RRDTR", RRF_b_4, 1524 },
        { "RRXTR", RRF_b_4, 1524 },
        { "SELFHR", RRF_a_4, 864 },
        { "SELGR", RRF_a_4, 864 },
        { "SELR", RRF_a_4, 864 },
        { "SLDT", RXF_3_x, 1526 },
        { "SLXT", RXF_3_x, 1526 },
        { "SRDT", RXF_3_x, 1526 },
        { "SRXT", RXF_3_x, 1526 },
        { "SDTR", RRF_a_3, 1527 },
        { "SXTR", RRF_a_3, 1527 },
        { "SDTRA", RRF_a_4, 1527 },
        { "SXTRA", RRF_a_4, 1527 },
        { "TDCET", RXE_2, 1528 },
        { "TDCDT", RXE_2, 1528 },
        { "TDCXT", RXE_2, 1528 },
        { "TDGET", RXE_2, 1529 },
        { "TDGDT", RXE_2, 1529 },
        { "TDGXT", RXE_2, 1529 },
        { "VBPERM", VRR_c_3, 1536 },
        { "VGEF", VRV_3, 1536 },
        { "VCFPS", VRR_a_5, 1641 },
        { "VCLFP", VRR_a_5, 1611 },
        { "VGEG", VRV_3, 1536 },
        { "VGBM", VRI_a_2, 1537 },
        { "VGM", VRI_b_4, 1537 },
        { "VL", VRX_3_opt, 1538 },
        { "VSTEBRF", VRX_3, 1576 },
        { "VSTEBRG", VRX_3, 1576 },
        { "VLLEBRZ", VRX_3, 1562 },
        { "VLREP", VRX_3, 1538 },
        { "VLR", VRR_a_2, 1538 },
        { "VLEB", VRX_3, 1538 },
        { "VLEBRH", VRX_3, 1561 },
        { "VLEBRG", VRX_3, 1561 },
        { "VLBRREP", VRX_3, 1562 },
        { "VLER", VRX_3, 1564 },
        { "VLBR", VRX_3, 1563 },
        { "VLEH", VRX_3, 1539 },
        { "VLEIH", VRI_a_3, 1539 },
        { "VLEF", VRX_3, 1539 },
        { "VLEIF", VRI_a_3, 1539 },
        { "VLEG", VRX_3, 1539 },
        { "VLEIG", VRI_a_3, 1539 },
        { "VLEIB", VRI_a_3, 1539 },
        { "VLGV", VRS_c_4, 1539 },
        { "VLLEZ", VRX_3, 1540 },
        { "VLM", VRS_a_4_opt, 1541 },
        { "VLRLR", VRS_d_3, 1541 },
        { "VLRL", VSI_3, 1541 },
        { "VLBB", VRX_3, 1542 },
        { "VLVG", VRS_b_4, 1543 },
        { "VLVGP", VRR_f_3, 1543 },
        { "VLL", VRS_b_3, 1543 },
        { "VMRH", VRR_c_4, 1544 },
        { "VMRL", VRR_c_4, 1544 },
        { "VPK", VRR_c_4, 1545 },
        { "VPKS", VRR_b_5, 1545 },
        { "VPKLS", VRR_b_5, 1546 },
        { "VPERM", VRR_e_4, 1547 },
        { "VPDI", VRR_c_4, 1547 },
        { "VREP", VRI_c_4, 1547 },
        { "VREPI", VRI_a_3, 1548 },
        { "VSCEF", VRV_3, 1548 },
        { "VSCEG", VRV_3, 1548 },
        { "VSEL", VRR_e_4, 1549 },
        { "VSEG", VRR_a_3, 1549 },
        { "VSTBR", VRX_3, 1576 },
        { "VST", VRX_3_opt, 1550 },
        { "VSTEB", VRX_3, 1550 },
        { "VSTEBRH", VRX_3, 1576 },
        { "VSTEH", VRX_3, 1550 },
        { "VSTEF", VRX_3, 1550 },
        { "VSTEG", VRX_3, 1550 },
        { "VSTER", VRX_3, 1578 },
        { "VSTM", VRS_a_4_opt, 1551 },
        { "VSTRLR", VRS_d_3, 1551 },
        { "VSTRL", VSI_3, 1551 },
        { "VSTL", VRS_b_3, 1552 },
        { "VUPH", VRR_a_3, 1552 },
        { "VUPL", VRR_a_3, 1553 },
        { "VUPLH", VRR_a_3, 1553 },
        { "VUPLL", VRR_a_3, 1554 },
        { "VA", VRR_c_4, 1557 },
        { "VACC", VRR_c_4, 1558 },
        { "VAC", VRR_d_5, 1558 },
        { "VACCC", VRR_d_5, 1559 },
        { "VN", VRR_c_3, 1559 },
        { "VNC", VRR_c_3, 1559 },
        { "VAVG", VRR_c_4, 1560 },
        { "VAVGL", VRR_c_4, 1560 },
        { "VCKSM", VRR_c_3, 1560 },
        { "VEC", VRR_a_3, 1561 },
        { "VECL", VRR_a_3, 1561 },
        { "VCEQ", VRR_b_5, 1561 },
        { "VCH", VRR_b_5, 1562 },
        { "VCHL", VRR_b_5, 1563 },
        { "VCLZ", VRR_a_3, 1564 },
        { "VCTZ", VRR_a_3, 1564 },
        { "VGFM", VRR_c_4, 1565 },
        { "VX", VRR_c_3, 1565 },
        { "VLC", VRR_a_3, 1566 },
        { "VGFMA", VRR_d_5, 1566 },
        { "VLP", VRR_a_3, 1566 },
        { "VMX", VRR_c_4, 1567 },
        { "VMXL", VRR_c_4, 1567 },
        { "VMN", VRR_c_4, 1567 },
        { "VMNL", VRR_c_4, 1568 },
        { "VMAL", VRR_d_5, 1568 },
        { "VMAH", VRR_d_5, 1569 },
        { "VMALH", VRR_d_5, 1569 },
        { "VMAE", VRR_d_5, 1569 },
        { "VMALE", VRR_d_5, 1569 },
        { "VMAO", VRR_d_5, 1570 },
        { "VMALO", VRR_d_5, 1570 },
        { "VMH", VRR_c_4, 1570 },
        { "VMLH", VRR_c_4, 1571 },
        { "VML", VRR_c_4, 1571 },
        { "VME", VRR_c_4, 1572 },
        { "VMLE", VRR_c_4, 1572 },
        { "VMO", VRR_c_4, 1572 },
        { "VMLO", VRR_c_4, 1572 },
        { "VMSL", VRR_d_6, 1573 },
        { "VNN", VRR_c_3, 1574 },
        { "VNO", VRR_c_3, 1574 },
        { "VNX", VRR_c_3, 1574 },
        { "VO", VRR_c_3, 1574 },
        { "VOC", VRR_c_3, 1575 },
        { "VPOPCT", VRR_a_3, 1575 },
        { "VERLLV", VRR_c_4, 1575 },
        { "VERLL", VRS_a_4, 1575 },
        { "VERIM", VRI_d_5, 1576 },
        { "VESLV", VRR_c_4, 1577 },
        { "VESL", VRS_a_4, 1577 },
        { "VESRAV", VRR_c_4, 1577 },
        { "VESRA", VRS_a_4, 1577 },
        { "VESRLV", VRR_c_4, 1578 },
        { "VESRL", VRS_a_4, 1578 },
        { "VSLD", VRI_d_4, 1607 },
        { "VSRD", VRI_d_4, 1608 },
        { "VSLDB", VRI_d_4, 1579 },
        { "VSL", VRR_c_3, 1579 },
        { "VSLB", VRR_c_3, 1579 },
        { "VSRA", VRR_c_3, 1579 },
        { "VSRAB", VRR_c_3, 1580 },
        { "VSRL", VRR_c_3, 1580 },
        { "VSRLB", VRR_c_3, 1580 },
        { "VS", VRR_c_4, 1580 },
        { "VSCBI", VRR_c_4, 1581 },
        { "VCSFP", VRR_a_5, 1644 },
        { "VSBI", VRR_d_5, 1581 },
        { "VSBCBI", VRR_d_5, 1582 },
        { "VSUMG", VRR_c_4, 1582 },
        { "VSUMQ", VRR_c_4, 1583 },
        { "VSUM", VRR_c_4, 1583 },
        { "VTM", VRR_a_2, 1584 },
        { "VFAE", VRR_b_5_opt, 1585 },
        { "VFEE", VRR_b_5_opt, 1587 },
        { "VFENE", VRR_b_5_opt, 1588 },
        { "VISTR", VRR_a_4_opt, 1589 },
        { "VSTRC", VRR_d_6_opt, 1590 },
        { "VSTRS", VRR_d_6_opt, 1622 },
        { "VFA", VRR_c_5, 1595 },
        { "WFC", VRR_a_4, 1599 },
        { "WFK", VRR_a_4, 1600 },
        { "VFCE", VRR_c_6, 1601 },
        { "VFCH", VRR_c_6, 1603 },
        { "VFCHE", VRR_c_6, 1605 },
        { "VCFPS", VRR_a_5, 1607 },
        { "VCFPL", VRR_a_5, 1643 },
        { "VCLGD", VRR_a_5, 1611 },
        { "VFD", VRR_c_5, 1613 },
        { "VFI", VRR_a_5, 1615 },
        { "VFLL", VRR_a_4, 1617 },
        { "VFLR", VRR_a_5, 1618 },
        { "VFMAX", VRR_c_6, 1619 },
        { "VFMIN", VRR_c_6, 1625 },
        { "VFM", VRR_c_5, 1631 },
        { "VFMA", VRR_e_6, 1633 },
        { "VFMS", VRR_e_6, 1633 },
        { "VFNMA", VRR_e_6, 1633 },
        { "VFNMS", VRR_e_6, 1633 },
        { "VFPSO", VRR_a_5, 1635 },
        { "VFSQ", VRR_a_4, 1636 },
        { "VFS", VRR_c_5, 1637 },
        { "VFTCI", VRI_e_5, 1638 },
        { "VAP", VRI_f_5, 1643 },
        { "VCP", VRR_h_3, 1644 },
        { "VCVB", VRR_i_3, 1645 },
        { "VCVBG", VRR_i_3, 1645 },
        { "VCVD", VRI_i_4, 1646 },
        { "VCVDG", VRI_i_4, 1646 },
        { "VDP", VRI_f_5, 1648 },
        { "VMP", VRI_f_5, 1650 },
        { "VMSP", VRI_f_5, 1651 },
        { "VRP", VRI_f_5, 1654 },
        { "VSDP", VRI_f_5, 1656 },
        { "VSP", VRI_f_5, 1658 },
        { "VLIP", VRI_h_3, 1649 },
        { "VPKZ", VSI_3, 1652 },
        { "VPSOP", VRI_g_5_u, 1653 },
        { "VSRP", VRI_g_5_s, 1657 },
        { "SIE", S_1_u, 7 },
        { "VAD", RI_a_2_u, 0 },
        { "VSD", RI_a_2_u, 0 },
        { "VADS", RI_a_2_u, 0 },
        { "VSDS", RI_a_2_u, 0 },
        { "VMDS", RI_a_2_u, 0 },
        { "VDDS", RI_a_2_u, 0 },
        { "VMADS", RI_a_2_u, 0 },
        { "VMSDS", RI_a_2_u, 0 },
        { "VCDS", RI_a_2_u, 0 },
        { "VAS", RI_a_2_u, 0 },
        { "VSS", RI_a_2_u, 0 },
        { "VMS", RI_a_2_u, 0 },
        { "VNS", RI_a_2_u, 0 },
        { "VOS", RI_a_2_u, 0 },
        { "VXS", RI_a_2_u, 0 },
        { "VCS", RI_a_2_u, 0 },
        { "VMD", RI_a_2_u, 0 },
        { "VMSD", RI_a_2_u, 0 },
        { "VMSE", RI_a_2_u, 0 },
        { "VMSES", RI_a_2_u, 0 },
        { "VLINT", RI_a_2_u, 0 },
        { "VDD", RI_a_2_u, 0 },
        { "VDDS", RI_a_2_u, 0 },
        { "VDE", RI_a_2_u, 0 },
        { "VDES", RI_a_2_u, 0 },
        { "VMXAD", RRE_2, 0 },
        { "VMXAE", RRE_2, 0 },
        { "VMXSE", RRE_2, 0 },
        { "VNVM", RRE_2, 0 },
        { "VOVM", RRE_2, 0 },
        { "VLCVM", RRE_2, 0 },
        { "VLELE", RRE_2, 0 },
        { "VLELD", RRE_2, 0 },
        { "VLI", RRE_2, 0 },
        { "VLID", RRE_2, 0 },
        { "VLBIX", RRE_2, 0 },
        { "VLVCU", RRE_2, 0 },
        { "VLVCA", RRE_2, 0 },
        { "VLVM", RRE_2, 0 },
        { "VMNSD", RRE_2, 0 },
        { "VMNSE", RRE_2, 0 },
        { "VMRRS", RRE_2, 0 },
        { "VMRSV", RRE_2, 0 },
        { "VACRS", RRE_2, 0 },
        { "VACSV", RRE_2, 0 },
        { "VSTVM", RRE_2, 0 },
        { "VSTVP", RRE_2, 0 },
        { "VSVMM", RRE_2, 0 },
        { "VTVM", RRE_2, 0 },
        { "VXELE", RRE_2, 0 },
        { "VXELD", RRE_2, 0 },
        { "VXVC", RRE_2, 0 },
        { "VXVM", RRE_2, 0 },
        { "VXVMM", RRE_2, 0 },
        { "VSTI", RRE_2, 0 },
        { "VSTID", RRE_2, 0 },
        { "VRCL", RRE_2, 0 },
        { "VRRS", RRE_2, 0 },
        { "VRSV", RRE_2, 0 },
        { "VRSVC", RRE_2, 0 },
        { "VSLL", RRE_2, 0 },
        { "VSPSD", RRE_2, 0 },
        { "VZPSD", RRE_2, 0 },
        { "VSRRS", RRE_2, 0 },
        { "VSRSV", RRE_2, 0 },
        { "VOVM", RRE_2, 0 },
        { "VCVM", RRE_2, 0 },
        { "VCZVM", RRE_2, 0 },
        { "VCOVM", RRE_2, 0 },
        { "VTP", VRR_g_1, 1660 },
        { "VUPKZ", VSI_3, 1660 },
        { "VSTK", RI_a_2_u, 0 },
        { "VSE", RI_a_2_u, 0 },
        { "VSES", RI_a_2_u, 0 },
        { "VSD", RI_a_2_u, 0 },
        { "VSDS", RI_a_2_u, 0 },
        { "VSTD", RI_a_2_u, 0 },
        { "VSTKD", RI_a_2_u, 0 },
        { "VSTMD", RI_a_2_u, 0 },
        { "VSTH", RI_a_2_u, 0 },
        { "VLD", RI_a_2_u, 0 },
        { "VLH", RI_a_2_u, 0 },
        { "VLMD", RI_a_2_u, 0 },
        { "VLY", RI_a_2_u, 0 },
        { "VLYD", RI_a_2_u, 0 },
        { "VM", RI_a_2_u, 0 },
        { "VMAD", RI_a_2_u, 0 },
        { "VMAES", RI_a_2_u, 0 },
        { "VMCD", RI_a_2_u, 0 },
        { "VMCE", RI_a_2_u, 0 },
        { "VMD", RI_a_2_u, 0 },
        { "VMES", RI_a_2_u, 0 },
        { "VACD", RI_a_2_u, 0 },
        { "VACE", RI_a_2_u, 0 },
        { "VAE", RI_a_2_u, 0 },
        { "VAES", RI_a_2_u, 0 },
        { "VC", RI_a_2_u, 0 },
        { "VCD", RI_a_2_u, 0 },
        { "VCE", RI_a_2_u, 0 },
        { "VCES", RI_a_2_u, 0 },
    };

    for (const auto& i : instructions)
        result.emplace(i.name, i.format, std::vector(i.op_format, i.op_format + i.op_format_size), i.page_no);

    return result;
}

const std::set<machine_instruction, machine_instruction_comparer>* const machine_instructions =
    &instruction::all_machine_instructions();

const std::set<machine_instruction, machine_instruction_comparer>& instruction::all_machine_instructions()
{
    if (machine_instructions)
        return *machine_instructions;
    static const std::set<machine_instruction, machine_instruction_comparer> machine_instructions_ =
        generate_machine_instructions();
    return machine_instructions_;
}

const machine_instruction* instruction::find_machine_instructions(std::string_view name)
{
    auto it = machine_instructions->find(name);
    if (it == machine_instructions->end())
        return nullptr;
    return &*it;
}
const machine_instruction& instruction::get_machine_instructions(std::string_view name)
{
    auto mi = find_machine_instructions(name);
    assert(mi);
    return *mi;
}

static auto generate_mnemonic_codes()
{
    std::map<std::string_view, mnemonic_code> result;

    const auto add_mnemonic_code = [&result, &mi = instruction::all_machine_instructions()](std::string_view mnemonic,
                                       std::string_view base_instr,
                                       std::initializer_list<std::pair<size_t, size_t>> replacement) {
        assert(std::is_sorted(
            replacement.begin(), replacement.end(), [](const auto& l, const auto& r) { return l.first < r.first; }));
        auto bi = mi.find(base_instr);
        assert(bi != mi.end());
        result.try_emplace(mnemonic, &*bi, replacement);
    };

    add_mnemonic_code("B", "BC", { { 0, 15 } });
    add_mnemonic_code("BR", "BCR", { { 0, 15 } });
    add_mnemonic_code("J", "BRC", { { 0, 15 } });
    add_mnemonic_code("NOP", "BC", { { 0, 0 } });
    add_mnemonic_code("NOPR", "BCR", { { 0, 0 } });
    add_mnemonic_code("JNOP", "BRC", { { 0, 0 } });
    add_mnemonic_code("BH", "BC", { { 0, 2 } });
    add_mnemonic_code("BHR", "BCR", { { 0, 2 } });
    add_mnemonic_code("JH", "BRC", { { 0, 2 } });
    add_mnemonic_code("BL", "BC", { { 0, 4 } });
    add_mnemonic_code("BLR", "BCR", { { 0, 4 } });
    add_mnemonic_code("JL", "BRC", { { 0, 4 } });
    add_mnemonic_code("BE", "BC", { { 0, 8 } });
    add_mnemonic_code("BER", "BCR", { { 0, 8 } });
    add_mnemonic_code("JE", "BRC", { { 0, 8 } });
    add_mnemonic_code("BNH", "BC", { { 0, 13 } });
    add_mnemonic_code("BNHR", "BCR", { { 0, 13 } });
    add_mnemonic_code("JNH", "BRC", { { 0, 13 } });
    add_mnemonic_code("BNL", "BC", { { 0, 11 } });
    add_mnemonic_code("BNLR", "BCR", { { 0, 11 } });
    add_mnemonic_code("JNL", "BRC", { { 0, 11 } });
    add_mnemonic_code("BNE", "BC", { { 0, 7 } });
    add_mnemonic_code("BNER", "BCR", { { 0, 7 } });
    add_mnemonic_code("JNE", "BRC", { { 0, 7 } });
    add_mnemonic_code("BP", "BC", { { 0, 2 } });
    add_mnemonic_code("BPR", "BCR", { { 0, 2 } });
    add_mnemonic_code("JP", "BRC", { { 0, 2 } });
    add_mnemonic_code("JM", "BRC", { { 0, 4 } });
    add_mnemonic_code("JZ", "BRC", { { 0, 8 } });
    add_mnemonic_code("JO", "BRC", { { 0, 1 } });
    add_mnemonic_code("BNP", "BC", { { 0, 13 } });
    add_mnemonic_code("BNPR", "BCR", { { 0, 13 } });
    add_mnemonic_code("JNP", "BRC", { { 0, 13 } });
    add_mnemonic_code("BNM", "BC", { { 0, 11 } });
    add_mnemonic_code("JNM", "BRC", { { 0, 11 } });
    add_mnemonic_code("BNZ", "BC", { { 0, 7 } });
    add_mnemonic_code("JNZ", "BRC", { { 0, 7 } });
    add_mnemonic_code("BNO", "BC", { { 0, 14 } });
    add_mnemonic_code("JNO", "BRC", { { 0, 14 } });
    add_mnemonic_code("XHLR", "RXSBG", { { 2, 0 }, { 3, 31 }, { 4, 32 } });
    add_mnemonic_code("XHHR", "RXSBG", { { 2, 0 }, { 3, 31 } });
    add_mnemonic_code("XLHR", "RXSBG", { { 2, 32 }, { 3, 63 }, { 4, 32 } });
    add_mnemonic_code("OHLR", "ROSBG", { { 2, 0 }, { 3, 31 }, { 4, 32 } });
    add_mnemonic_code("OHHR", "ROSBG", { { 2, 0 }, { 3, 31 } });
    add_mnemonic_code("OLHR", "ROSBG", { { 2, 32 }, { 3, 63 }, { 4, 32 } });
    add_mnemonic_code("NHLR", "RNSBG", { { 2, 0 }, { 3, 31 }, { 4, 32 } });
    add_mnemonic_code("NHHR", "RNSBG", { { 2, 0 }, { 3, 31 } });
    add_mnemonic_code("NLHR", "RNSBG", { { 2, 32 }, { 3, 63 }, { 4, 32 } });
    add_mnemonic_code("BO", "BC", { { 0, 1 } });
    add_mnemonic_code("BOR", "BCR", { { 0, 1 } });
    add_mnemonic_code("BM", "BC", { { 0, 4 } });
    add_mnemonic_code("BMR", "BCR", { { 0, 4 } });
    add_mnemonic_code("BZ", "BC", { { 0, 8 } });
    add_mnemonic_code("BZR", "BCR", { { 0, 8 } });
    add_mnemonic_code("BNOR", "BCR", { { 0, 14 } });
    add_mnemonic_code("BNMR", "BCR", { { 0, 11 } });
    add_mnemonic_code("BNZR", "BCR", { { 0, 7 } });
    add_mnemonic_code("BRUL", "BRCL", { { 0, 15 } });
    add_mnemonic_code("BRHL", "BRCL", { { 0, 2 } });
    add_mnemonic_code("BRLL", "BRCL", { { 0, 4 } });
    add_mnemonic_code("BREL", "BRCL", { { 0, 8 } });
    add_mnemonic_code("BRNHL", "BRCL", { { 0, 13 } });
    add_mnemonic_code("BRNLL", "BRCL", { { 0, 11 } });
    add_mnemonic_code("BRNEL", "BRCL", { { 0, 7 } });
    add_mnemonic_code("BRPL", "BRCL", { { 0, 2 } });
    add_mnemonic_code("BRML", "BRCL", { { 0, 4 } });
    add_mnemonic_code("BRZL", "BRCL", { { 0, 8 } });
    add_mnemonic_code("BROL", "BRCL", { { 0, 1 } });
    add_mnemonic_code("BRNPL", "BRCL", { { 0, 13 } });
    add_mnemonic_code("BRNML", "BRCL", { { 0, 11 } });
    add_mnemonic_code("BRNZL", "BRCL", { { 0, 7 } });
    add_mnemonic_code("BRNOL", "BRCL", { { 0, 14 } });
    add_mnemonic_code("BRO", "BRC", { { 0, 1 } });
    add_mnemonic_code("BRP", "BRC", { { 0, 2 } });
    add_mnemonic_code("BRH", "BRC", { { 0, 2 } });
    add_mnemonic_code("BRL", "BRC", { { 0, 4 } });
    add_mnemonic_code("BRM", "BRC", { { 0, 4 } });
    add_mnemonic_code("BRNE", "BRC", { { 0, 7 } });
    add_mnemonic_code("BRNZ", "BRC", { { 0, 7 } });
    add_mnemonic_code("BRE", "BRC", { { 0, 8 } });
    add_mnemonic_code("BRZ", "BRC", { { 0, 8 } });
    add_mnemonic_code("BRNL", "BRC", { { 0, 11 } });
    add_mnemonic_code("BRNM", "BRC", { { 0, 11 } });
    add_mnemonic_code("BRNH", "BRC", { { 0, 13 } });
    add_mnemonic_code("BRNP", "BRC", { { 0, 13 } });
    add_mnemonic_code("BRNO", "BRC", { { 0, 14 } });
    add_mnemonic_code("BRU", "BRC", { { 0, 15 } });
    add_mnemonic_code("JLU", "BRCL", { { 0, 15 } });
    add_mnemonic_code("JLNOP", "BRCL", { { 0, 0 } });
    add_mnemonic_code("JLH", "BRCL", { { 0, 2 } });
    add_mnemonic_code("JLL", "BRCL", { { 0, 4 } });
    add_mnemonic_code("JLE", "BRCL", { { 0, 8 } });
    add_mnemonic_code("JLNH", "BRCL", { { 0, 13 } });
    add_mnemonic_code("JLNL", "BRCL", { { 0, 11 } });
    add_mnemonic_code("JLNE", "BRCL", { { 0, 7 } });
    add_mnemonic_code("JLP", "BRCL", { { 0, 2 } });
    add_mnemonic_code("JLM", "BRCL", { { 0, 4 } });
    add_mnemonic_code("JLZ", "BRCL", { { 0, 8 } });
    add_mnemonic_code("JLO", "BRCL", { { 0, 1 } });
    add_mnemonic_code("JLNP", "BRCL", { { 0, 13 } });
    add_mnemonic_code("JLNM", "BRCL", { { 0, 11 } });
    add_mnemonic_code("JLNZ", "BRCL", { { 0, 7 } });
    add_mnemonic_code("JLNO", "BRCL", { { 0, 14 } });
    add_mnemonic_code("JAS", "BRAS", {});
    add_mnemonic_code("JASL", "BRASL", {});
    add_mnemonic_code("JC", "BRC", {});
    add_mnemonic_code("JCT", "BRCT", {});
    add_mnemonic_code("JCTG", "BRCTG", {});
    add_mnemonic_code("JXH", "BRXH", {});
    add_mnemonic_code("JXHG", "BRXHG", {});
    add_mnemonic_code("JXLE", "BRXLE", {});
    add_mnemonic_code("JXLEG", "BRXLG", {});
    add_mnemonic_code("VCDG", "VCFPS", {});
    add_mnemonic_code("VCGD", "VCSFP", {});
    add_mnemonic_code("BIO", "BIC", { { 0, 1 } });
    add_mnemonic_code("BIP", "BIC", { { 0, 2 } });
    add_mnemonic_code("BIH", "BIC", { { 0, 2 } });
    add_mnemonic_code("BIM", "BIC", { { 0, 4 } });
    add_mnemonic_code("BIL", "BIC", { { 0, 4 } });
    add_mnemonic_code("BINZ", "BIC", { { 0, 7 } });
    add_mnemonic_code("BINE", "BIC", { { 0, 7 } });
    add_mnemonic_code("BIZ", "BIC", { { 0, 8 } });
    add_mnemonic_code("BIE", "BIC", { { 0, 8 } });
    add_mnemonic_code("BINM", "BIC", { { 0, 11 } });
    add_mnemonic_code("BINL", "BIC", { { 0, 11 } });
    add_mnemonic_code("BINP", "BIC", { { 0, 13 } });
    add_mnemonic_code("BINH", "BIC", { { 0, 13 } });
    add_mnemonic_code("BINO", "BIC", { { 0, 14 } });
    add_mnemonic_code("BI", "BIC", { { 0, 15 } });
    add_mnemonic_code("VSTBRH", "VSTBR", { { 2, 1 } });
    add_mnemonic_code("VSTBRF", "VSTBR", { { 2, 2 } });
    add_mnemonic_code("VSTBRG", "VSTBR", { { 2, 3 } });
    add_mnemonic_code("VSTBRQ", "VSTBR", { { 2, 4 } });
    add_mnemonic_code("VSTERH", "VSTER", { { 2, 1 } });
    add_mnemonic_code("VSTERF", "VSTER", { { 2, 2 } });
    add_mnemonic_code("VSTERG", "VSTER", { { 2, 3 } });
    add_mnemonic_code("STERV", "VSTEBRF", { { 2, 0 } });
    add_mnemonic_code("STDRV", "VSTEBRG", { { 2, 0 } });
    add_mnemonic_code("SELFHRE", "SELFHR", { { 3, 8 } });
    add_mnemonic_code("SELFHRH", "SELFHR", { { 3, 2 } });
    add_mnemonic_code("SELFHRL", "SELFHR", { { 3, 4 } });
    add_mnemonic_code("SELFHRNE", "SELFHR", { { 3, 7 } });
    add_mnemonic_code("SELFHRNH", "SELFHR", { { 3, 13 } });
    add_mnemonic_code("SELFHRNL", "SELFHR", { { 3, 11 } });
    add_mnemonic_code("SELFHRNO", "SELFHR", { { 3, 14 } });
    add_mnemonic_code("SELFHRO", "SELFHR", { { 3, 1 } });
    add_mnemonic_code("LHHR", "RISBHGZ", { { 2, 0 }, { 3, 31 } });
    add_mnemonic_code("LHLR", "RISBHGZ", { { 2, 0 }, { 3, 31 }, { 4, 32 } });
    add_mnemonic_code("LLHHHR", "RISBHGZ", { { 2, 16 }, { 3, 31 } });
    add_mnemonic_code("LLHHLR", "RISBHGZ", { { 2, 16 }, { 3, 31 }, { 4, 32 } });
    add_mnemonic_code("LLCHHR", "RISBHGZ", { { 2, 24 }, { 3, 31 } });
    add_mnemonic_code("LLCHLR", "RISBHGZ", { { 2, 24 }, { 3, 31 }, { 4, 32 } });
    add_mnemonic_code("LLHFR", "RISBLGZ", { { 2, 0 }, { 3, 31 }, { 4, 32 } });
    add_mnemonic_code("LLHLHR", "RISBLGZ", { { 2, 16 }, { 3, 31 }, { 4, 32 } });
    add_mnemonic_code("LLCLHR", "RISBLGZ", { { 2, 24 }, { 3, 31 }, { 4, 32 } });
    add_mnemonic_code("LOCO", "LOC", { { 2, 1 } });
    add_mnemonic_code("LOCNO", "LOC", { { 2, 14 } });
    add_mnemonic_code("LOCGO", "LOCG", { { 2, 1 } });
    add_mnemonic_code("LOCGNO", "LOCG", { { 2, 14 } });
    add_mnemonic_code("LOCGHIH", "LOCGHI", { { 2, 2 } });
    add_mnemonic_code("LOCGHIL", "LOCGHI", { { 2, 4 } });
    add_mnemonic_code("LOCGHIE", "LOCGHI", { { 2, 8 } });
    add_mnemonic_code("LOCGHINE", "LOCGHI", { { 2, 7 } });
    add_mnemonic_code("LOCGHINL", "LOCGHI", { { 2, 11 } });
    add_mnemonic_code("LOCGHINH", "LOCGHI", { { 2, 13 } });
    add_mnemonic_code("LOCGHINO", "LOCGHI", { { 2, 14 } });
    add_mnemonic_code("LOCGHIO", "LOCGHI", { { 2, 1 } });
    add_mnemonic_code("LOCGRO", "LOCGR", { { 2, 1 } });
    add_mnemonic_code("LOCGRNO", "LOCGR", { { 2, 14 } });
    add_mnemonic_code("LOCHHIE", "LOCHHI", { { 2, 8 } });
    add_mnemonic_code("LOCHHIH", "LOCHHI", { { 2, 2 } });
    add_mnemonic_code("LOCHHIL", "LOCHHI", { { 2, 4 } });
    add_mnemonic_code("LOCHHINE", "LOCHHI", { { 2, 7 } });
    add_mnemonic_code("LOCHHINH", "LOCHHI", { { 2, 13 } });
    add_mnemonic_code("LOCHHINL", "LOCHHI", { { 2, 11 } });
    add_mnemonic_code("LOCHHINO", "LOCHHI", { { 2, 14 } });
    add_mnemonic_code("LOCHHIO", "LOCHHI", { { 2, 1 } });
    add_mnemonic_code("LOCHIE", "LOCHI", { { 2, 8 } });
    add_mnemonic_code("LOCHIH", "LOCHI", { { 2, 2 } });
    add_mnemonic_code("LOCHIL", "LOCHI", { { 2, 4 } });
    add_mnemonic_code("LOCHINE", "LOCHI", { { 2, 7 } });
    add_mnemonic_code("LOCHINH", "LOCHI", { { 2, 13 } });
    add_mnemonic_code("LOCHINL", "LOCHI", { { 2, 11 } });
    add_mnemonic_code("LOCHINO", "LOCHI", { { 2, 14 } });
    add_mnemonic_code("LOCHIO", "LOCHI", { { 2, 1 } });
    add_mnemonic_code("LOCRNO", "LOCR", { { 2, 14 } });
    add_mnemonic_code("LOCRO", "LOCR", { { 2, 1 } });
    add_mnemonic_code("LOCFHE", "LOCFH", { { 2, 8 } });
    add_mnemonic_code("LOCFHH", "LOCFH", { { 2, 2 } });
    add_mnemonic_code("LOCFHL", "LOCFH", { { 2, 4 } });
    add_mnemonic_code("LOCFHNE", "LOCFH", { { 2, 7 } });
    add_mnemonic_code("LOCFHNH", "LOCFH", { { 2, 13 } });
    add_mnemonic_code("LOCFHNL", "LOCFH", { { 2, 11 } });
    add_mnemonic_code("LOCFHNO", "LOCFH", { { 2, 14 } });
    add_mnemonic_code("LOCFHO", "LOCFH", { { 2, 1 } });
    add_mnemonic_code("LOCFHRH", "LOCFHR", { { 2, 2 } });
    add_mnemonic_code("LOCFHRL", "LOCFHR", { { 2, 4 } });
    add_mnemonic_code("LOCFHRE", "LOCFHR", { { 2, 8 } });
    add_mnemonic_code("LOCFHRNE", "LOCFHR", { { 2, 7 } });
    add_mnemonic_code("LOCFHRNH", "LOCFHR", { { 2, 13 } });
    add_mnemonic_code("LOCFHRNL", "LOCFHR", { { 2, 11 } });
    add_mnemonic_code("LOCFHRNO", "LOCFHR", { { 2, 14 } });
    add_mnemonic_code("LOCFHRO", "LOCFHR", { { 2, 1 } });
    add_mnemonic_code("STOCFHE", "STOCFH", { { 2, 8 } });
    add_mnemonic_code("STOCFHH", "STOCFH", { { 2, 2 } });
    add_mnemonic_code("STOCFHL", "STOCFH", { { 2, 4 } });
    add_mnemonic_code("STOCFHNE", "STOCFH", { { 2, 7 } });
    add_mnemonic_code("STOCFHNH", "STOCFH", { { 2, 13 } });
    add_mnemonic_code("STOCFHNL", "STOCFH", { { 2, 11 } });
    add_mnemonic_code("STOCFHNO", "STOCFH", { { 2, 14 } });
    add_mnemonic_code("STOCFHO", "STOCFH", { { 2, 1 } });
    add_mnemonic_code("STOCGNO", "STOCG", { { 2, 14 } });
    add_mnemonic_code("STOCGO", "STOCG", { { 2, 1 } });
    add_mnemonic_code("STOCNO", "STOC", { { 2, 14 } });
    add_mnemonic_code("STOCO", "STOC", { { 2, 1 } });
    add_mnemonic_code("SELGRE", "SELGR", { { 3, 8 } });
    add_mnemonic_code("SELGRH", "SELGR", { { 3, 2 } });
    add_mnemonic_code("SELGRL", "SELGR", { { 3, 4 } });
    add_mnemonic_code("SELGRNE", "SELGR", { { 3, 7 } });
    add_mnemonic_code("SELGRNH", "SELGR", { { 3, 13 } });
    add_mnemonic_code("SELGRNL", "SELGR", { { 3, 11 } });
    add_mnemonic_code("SELGRNO", "SELGR", { { 3, 14 } });
    add_mnemonic_code("SELGRO", "SELGR", { { 3, 1 } });
    add_mnemonic_code("SELRE", "SELR", { { 3, 8 } });
    add_mnemonic_code("SELRH", "SELR", { { 3, 2 } });
    add_mnemonic_code("SELRL", "SELR", { { 3, 4 } });
    add_mnemonic_code("SELRNE", "SELR", { { 3, 7 } });
    add_mnemonic_code("SELRNH", "SELR", { { 3, 13 } });
    add_mnemonic_code("SELRNL", "SELR", { { 3, 11 } });
    add_mnemonic_code("SELRNO", "SELR", { { 3, 14 } });
    add_mnemonic_code("SELRO", "SELR", { { 3, 1 } });
    add_mnemonic_code("VZERO", "VGBM", { { 0, 1 } });
    add_mnemonic_code("VONE", "VGBM", { { 1, 65535 } });
    add_mnemonic_code("VGMB", "VGM", { { 3, 0 } });
    add_mnemonic_code("VGMH", "VGM", { { 3, 1 } });
    add_mnemonic_code("VGMF", "VGM", { { 3, 2 } });
    add_mnemonic_code("VGMG", "VGM", { { 3, 3 } });
    add_mnemonic_code("VLREPB", "VLREP", { { 2, 0 } });
    add_mnemonic_code("VLREPH", "VLREP", { { 2, 1 } });
    add_mnemonic_code("VLREPF", "VLREP", { { 2, 2 } });
    add_mnemonic_code("VLREPG", "VLREP", { { 2, 3 } });
    add_mnemonic_code("VLGVB", "VLGV", { { 3, 0 } });
    add_mnemonic_code("VLGVH", "VLGV", { { 3, 1 } });
    add_mnemonic_code("VLGVF", "VLGV", { { 3, 2 } });
    add_mnemonic_code("VLGVG", "VLGV", { { 3, 3 } });
    add_mnemonic_code("VLLEZB", "VLLEZ", { { 2, 0 } });
    add_mnemonic_code("VLLEZH", "VLLEZ", { { 2, 1 } });
    add_mnemonic_code("VLLEZF", "VLLEZ", { { 2, 2 } });
    add_mnemonic_code("VLLEZG", "VLLEZ", { { 2, 3 } });
    add_mnemonic_code("VLLEZLF", "VLLEZ", { { 2, 6 } });
    add_mnemonic_code("VLLEBRZE", "VLLEBRZ", { { 2, 6 } });
    add_mnemonic_code("VLLEBRZG", "VLLEBRZ", { { 2, 3 } });
    add_mnemonic_code("VLLEBRZF", "VLLEBRZ", { { 2, 2 } });
    add_mnemonic_code("VLLEBRZH", "VLLEBRZ", { { 2, 1 } });
    add_mnemonic_code("VLVGB", "VLVG", { { 3, 0 } });
    add_mnemonic_code("VLVGH", "VLVG", { { 3, 1 } });
    add_mnemonic_code("VLVGF", "VLVG", { { 3, 2 } });
    add_mnemonic_code("VLVGG", "VLVG", { { 3, 3 } });
    add_mnemonic_code("VMRHB", "VMRH", { { 3, 0 } });
    add_mnemonic_code("VMRHH", "VMRH", { { 3, 1 } });
    add_mnemonic_code("VMRHF", "VMRH", { { 3, 2 } });
    add_mnemonic_code("VMRHG", "VMRH", { { 3, 3 } });
    add_mnemonic_code("VMRLB", "VMRL", { { 3, 0 } });
    add_mnemonic_code("VMRLH", "VMRL", { { 3, 1 } });
    add_mnemonic_code("VMRLF", "VMRL", { { 3, 2 } });
    add_mnemonic_code("VMRLG", "VMRL", { { 3, 3 } });
    add_mnemonic_code("VPKSH", "VPKS", { { 3, 1 }, { 4, 0 } });
    add_mnemonic_code("VPKSF", "VPKS", { { 3, 2 }, { 4, 0 } });
    add_mnemonic_code("VPKSG", "VPKS", { { 3, 3 }, { 4, 0 } });
    add_mnemonic_code("VPKSHS", "VPKS", { { 3, 1 }, { 4, 1 } });
    add_mnemonic_code("VPKSFS", "VPKS", { { 3, 2 }, { 4, 1 } });
    add_mnemonic_code("VPKSGS", "VPKS", { { 3, 3 }, { 4, 1 } });
    add_mnemonic_code("VPKLSH", "VPKLS", { { 3, 1 }, { 4, 0 } });
    add_mnemonic_code("VPKLSF", "VPKLS", { { 3, 2 }, { 4, 0 } });
    add_mnemonic_code("VPKLSG", "VPKLS", { { 3, 3 }, { 4, 0 } });
    add_mnemonic_code("VPKLSHS", "VPKLS", { { 3, 1 }, { 4, 1 } });
    add_mnemonic_code("VPKLSFS", "VPKLS", { { 3, 2 }, { 4, 1 } });
    add_mnemonic_code("VPKLSGS", "VPKLS", { { 3, 3 }, { 4, 1 } });
    add_mnemonic_code("VREPB", "VREP", { { 3, 0 } });
    add_mnemonic_code("VREPH", "VREP", { { 3, 1 } });
    add_mnemonic_code("VREPF", "VREP", { { 3, 2 } });
    add_mnemonic_code("VREPG", "VREP", { { 3, 3 } });
    add_mnemonic_code("VREPIB", "VREPI", { { 2, 0 } });
    add_mnemonic_code("VREPIH", "VREPI", { { 2, 1 } });
    add_mnemonic_code("VREPIF", "VREPI", { { 2, 2 } });
    add_mnemonic_code("VREPIG", "VREPI", { { 2, 3 } });
    add_mnemonic_code("VSEGB", "VSEG", { { 2, 0 } });
    add_mnemonic_code("VSEGH", "VSEG", { { 2, 1 } });
    add_mnemonic_code("VSEGF", "VSEG", { { 2, 2 } });
    add_mnemonic_code("VUPHB", "VUPH", { { 2, 0 } });
    add_mnemonic_code("VUPHH", "VUPH", { { 2, 1 } });
    add_mnemonic_code("VUPHF", "VUPH", { { 2, 2 } });
    add_mnemonic_code("VUPLHB", "VUPLH", { { 2, 0 } });
    add_mnemonic_code("VUPLHG", "VUPLH", { { 2, 1 } });
    add_mnemonic_code("VUPLHF", "VUPLH", { { 2, 2 } });
    add_mnemonic_code("VUPLB", "VUPL", { { 2, 0 } });
    add_mnemonic_code("VUPLHW", "VUPL", { { 2, 1 } });
    add_mnemonic_code("VUPLF", "VUPL", { { 2, 2 } });
    add_mnemonic_code("VUPLLB", "VUPLL", { { 2, 0 } });
    add_mnemonic_code("VUPLLH", "VUPLL", { { 2, 1 } });
    add_mnemonic_code("VUPLLF", "VUPLL", { { 2, 2 } });
    add_mnemonic_code("VAB", "VA", { { 3, 0 } });
    add_mnemonic_code("VAH", "VA", { { 3, 1 } });
    add_mnemonic_code("VAF", "VA", { { 3, 2 } });
    add_mnemonic_code("VAG", "VA", { { 3, 3 } });
    add_mnemonic_code("VAQ", "VA", { { 3, 4 } });
    add_mnemonic_code("VACCB", "VACC", { { 3, 0 } });
    add_mnemonic_code("VACCH", "VACC", { { 3, 1 } });
    add_mnemonic_code("VACCF", "VACC", { { 3, 2 } });
    add_mnemonic_code("VACCG", "VACC", { { 3, 3 } });
    add_mnemonic_code("VACCQ", "VACC", { { 3, 4 } });
    add_mnemonic_code("VACQ", "VAC", { { 3, 4 } });
    add_mnemonic_code("VACCCQ", "VACCC", { { 3, 4 } });
    add_mnemonic_code("VAVGB", "VAVG", { { 3, 0 } });
    add_mnemonic_code("VAVGH", "VAVG", { { 3, 1 } });
    add_mnemonic_code("VAVGF", "VAVG", { { 3, 2 } });
    add_mnemonic_code("VAVGG", "VAVG", { { 3, 3 } });
    add_mnemonic_code("VAVGLB", "VAVGL", { { 3, 0 } });
    add_mnemonic_code("VAVGLH", "VAVGL", { { 3, 1 } });
    add_mnemonic_code("VAVGLF", "VAVGL", { { 3, 2 } });
    add_mnemonic_code("VAVGLG", "VAVGL", { { 3, 3 } });
    add_mnemonic_code("VECB", "VEC", { { 2, 0 } });
    add_mnemonic_code("VECH", "VEC", { { 2, 1 } });
    add_mnemonic_code("VECF", "VEC", { { 2, 2 } });
    add_mnemonic_code("VECG", "VEC", { { 2, 3 } });
    add_mnemonic_code("VECLB", "VECL", { { 2, 0 } });
    add_mnemonic_code("VECLH", "VECL", { { 2, 1 } });
    add_mnemonic_code("VECLF", "VECL", { { 2, 2 } });
    add_mnemonic_code("VECLG", "VECL", { { 2, 3 } });
    add_mnemonic_code("VCEQB", "VCEQ", { { 3, 0 }, { 4, 0 } });
    add_mnemonic_code("VCEQH", "VCEQ", { { 3, 1 }, { 4, 0 } });
    add_mnemonic_code("VCEQF", "VCEQ", { { 3, 2 }, { 4, 0 } });
    add_mnemonic_code("VCEQG", "VCEQ", { { 3, 3 }, { 4, 0 } });
    add_mnemonic_code("VCEQBS", "VCEQ", { { 3, 0 }, { 4, 1 } });
    add_mnemonic_code("VCEQHS", "VCEQ", { { 3, 1 }, { 4, 1 } });
    add_mnemonic_code("VCEQFS", "VCEQ", { { 3, 2 }, { 4, 1 } });
    add_mnemonic_code("VCEQGS", "VCEQ", { { 3, 3 }, { 4, 1 } });
    add_mnemonic_code("VCHB", "VCH", { { 3, 0 }, { 4, 0 } });
    add_mnemonic_code("VCHH", "VCH", { { 3, 1 }, { 4, 0 } });
    add_mnemonic_code("VCHF", "VCH", { { 3, 2 }, { 4, 0 } });
    add_mnemonic_code("VCHG", "VCH", { { 3, 3 }, { 4, 0 } });
    add_mnemonic_code("VCHBS", "VCH", { { 3, 0 }, { 4, 1 } });
    add_mnemonic_code("VCHHS", "VCH", { { 3, 1 }, { 4, 1 } });
    add_mnemonic_code("VCHFS", "VCH", { { 3, 2 }, { 4, 1 } });
    add_mnemonic_code("VCHGS", "VCH", { { 3, 3 }, { 4, 1 } });
    add_mnemonic_code("VCHLB", "VCHL", { { 3, 0 }, { 4, 0 } });
    add_mnemonic_code("VCHLH", "VCHL", { { 3, 1 }, { 4, 0 } });
    add_mnemonic_code("VCHLF", "VCHL", { { 3, 2 }, { 4, 0 } });
    add_mnemonic_code("VCHLG", "VCHL", { { 3, 3 }, { 4, 0 } });
    add_mnemonic_code("VCHLBS", "VCHL", { { 3, 0 }, { 4, 1 } });
    add_mnemonic_code("VCHLHS", "VCHL", { { 3, 1 }, { 4, 1 } });
    add_mnemonic_code("VCHLFS", "VCHL", { { 3, 2 }, { 4, 1 } });
    add_mnemonic_code("VCHLGS", "VCHL", { { 3, 3 }, { 4, 1 } });
    add_mnemonic_code("VCLZB", "VCLZ", { { 2, 0 } });
    add_mnemonic_code("VCLZH", "VCLZ", { { 2, 1 } });
    add_mnemonic_code("VCLZF", "VCLZ", { { 2, 2 } });
    add_mnemonic_code("VCLZG", "VCLZ", { { 2, 3 } });
    add_mnemonic_code("VGFMB", "VGFM", { { 3, 0 } });
    add_mnemonic_code("VGFMH", "VGFM", { { 3, 1 } });
    add_mnemonic_code("VGFMF", "VGFM", { { 3, 2 } });
    add_mnemonic_code("VGFMG", "VGFM", { { 3, 3 } });
    add_mnemonic_code("VGFMAB", "VGFMA", { { 4, 0 } });
    add_mnemonic_code("VGFMAH", "VGFMA", { { 4, 1 } });
    add_mnemonic_code("VGFMAF", "VGFMA", { { 4, 2 } });
    add_mnemonic_code("VGFMAG", "VGFMA", { { 4, 3 } });
    add_mnemonic_code("VLCB", "VLC", { { 2, 0 } });
    add_mnemonic_code("VLCH", "VLC", { { 2, 1 } });
    add_mnemonic_code("VLCF", "VLC", { { 2, 2 } });
    add_mnemonic_code("VLCG", "VLC", { { 2, 3 } });
    add_mnemonic_code("VLPB", "VLP", { { 2, 0 } });
    add_mnemonic_code("VLPH", "VLP", { { 2, 1 } });
    add_mnemonic_code("VLPF", "VLP", { { 2, 2 } });
    add_mnemonic_code("VLPG", "VLP", { { 2, 3 } });
    add_mnemonic_code("VMXB", "VMX", { { 3, 0 } });
    add_mnemonic_code("VMXH", "VMX", { { 3, 1 } });
    add_mnemonic_code("VMXF", "VMX", { { 3, 2 } });
    add_mnemonic_code("VMXG", "VMX", { { 3, 3 } });
    add_mnemonic_code("VMXLB", "VMXL", { { 3, 0 } });
    add_mnemonic_code("VMXLH", "VMXL", { { 3, 1 } });
    add_mnemonic_code("VMXLF", "VMXL", { { 3, 2 } });
    add_mnemonic_code("VMXLG", "VMXL", { { 3, 3 } });
    add_mnemonic_code("VMNB", "VMN", { { 3, 0 } });
    add_mnemonic_code("VMNH", "VMN", { { 3, 1 } });
    add_mnemonic_code("VMNF", "VMN", { { 3, 2 } });
    add_mnemonic_code("VMNG", "VMN", { { 3, 3 } });
    add_mnemonic_code("VMNLB", "VMNL", { { 3, 0 } });
    add_mnemonic_code("VMNLH", "VMNL", { { 3, 1 } });
    add_mnemonic_code("VMNLF", "VMNL", { { 3, 2 } });
    add_mnemonic_code("VMNLG", "VMNL", { { 3, 3 } });
    add_mnemonic_code("VMALB", "VMAL", { { 4, 0 } });
    add_mnemonic_code("VMALHW", "VMAL", { { 4, 1 } });
    add_mnemonic_code("VMALF", "VMAL", { { 4, 2 } });
    add_mnemonic_code("VMAHB", "VMAH", { { 4, 0 } });
    add_mnemonic_code("VMAHH", "VMAH", { { 4, 1 } });
    add_mnemonic_code("VMAHF", "VMAH", { { 4, 2 } });
    add_mnemonic_code("VMALHB", "VMALH", { { 4, 0 } });
    add_mnemonic_code("VMALHH", "VMALH", { { 4, 1 } });
    add_mnemonic_code("VMALHF", "VMALH", { { 4, 2 } });
    add_mnemonic_code("VMAEB", "VMAE", { { 4, 0 } });
    add_mnemonic_code("VMAEH", "VMAE", { { 4, 1 } });
    add_mnemonic_code("VMAEF", "VMAE", { { 4, 2 } });
    add_mnemonic_code("VMALEB", "VMALE", { { 4, 0 } });
    add_mnemonic_code("VMALEH", "VMALE", { { 4, 1 } });
    add_mnemonic_code("VMALEF", "VMALE", { { 4, 2 } });
    add_mnemonic_code("VMAOB", "VMAO", { { 4, 0 } });
    add_mnemonic_code("VMAOH", "VMAO", { { 4, 1 } });
    add_mnemonic_code("VMAOF", "VMAO", { { 4, 2 } });
    add_mnemonic_code("VMALOB", "VMALO", { { 4, 0 } });
    add_mnemonic_code("VMALOH", "VMALO", { { 4, 1 } });
    add_mnemonic_code("VMALOF", "VMALO", { { 4, 2 } });
    add_mnemonic_code("VMHB", "VMH", { { 3, 0 } });
    add_mnemonic_code("VMHH", "VMH", { { 3, 1 } });
    add_mnemonic_code("VMHF", "VMH", { { 3, 2 } });
    add_mnemonic_code("VMLHB", "VMLH", { { 3, 0 } });
    add_mnemonic_code("VMLHH", "VMLH", { { 3, 1 } });
    add_mnemonic_code("VMLHF", "VMLH", { { 3, 2 } });
    add_mnemonic_code("VMLB", "VML", { { 3, 0 } });
    add_mnemonic_code("VMLHW", "VML", { { 3, 1 } });
    add_mnemonic_code("VMLF", "VML", { { 3, 2 } });
    add_mnemonic_code("VMEB", "VME", { { 3, 0 } });
    add_mnemonic_code("VMEH", "VME", { { 3, 1 } });
    add_mnemonic_code("VMEF", "VME", { { 3, 2 } });
    add_mnemonic_code("VMLEB", "VMLE", { { 3, 0 } });
    add_mnemonic_code("VMLEH", "VMLE", { { 3, 1 } });
    add_mnemonic_code("VMLEF", "VMLE", { { 3, 2 } });
    add_mnemonic_code("VMSLG", "VMSL", { { 4, 3 } });
    add_mnemonic_code("VMOB", "VMO", { { 3, 0 } });
    add_mnemonic_code("VMOH", "VMO", { { 3, 1 } });
    add_mnemonic_code("VMOF", "VMO", { { 3, 2 } });
    add_mnemonic_code("VMLOB", "VMLO", { { 3, 0 } });
    add_mnemonic_code("VMLOH", "VMLO", { { 3, 1 } });
    add_mnemonic_code("VMLOF", "VMLO", { { 3, 2 } });
    add_mnemonic_code("VPOPCTB", "VPOPCT", { { 2, 0 } });
    add_mnemonic_code("VPOPCTH", "VPOPCT", { { 2, 1 } });
    add_mnemonic_code("VPOPCTF", "VPOPCT", { { 2, 2 } });
    add_mnemonic_code("VPOPCTG", "VPOPCT", { { 2, 3 } });
    add_mnemonic_code("VERLLVB", "VERLLV", { { 3, 0 } });
    add_mnemonic_code("VERLLVH", "VERLLV", { { 3, 1 } });
    add_mnemonic_code("VERLLVF", "VERLLV", { { 3, 2 } });
    add_mnemonic_code("VERLLVG", "VERLLV", { { 3, 3 } });
    add_mnemonic_code("VERLLB", "VERLL", { { 3, 0 } });
    add_mnemonic_code("VERLLH", "VERLL", { { 3, 1 } });
    add_mnemonic_code("VERLLF", "VERLL", { { 3, 2 } });
    add_mnemonic_code("VERLLG", "VERLL", { { 3, 3 } });
    add_mnemonic_code("VERIMB", "VERIM", { { 4, 0 } });
    add_mnemonic_code("VERIMH", "VERIM", { { 4, 1 } });
    add_mnemonic_code("VERIMF", "VERIM", { { 4, 2 } });
    add_mnemonic_code("VERIMG", "VERIM", { { 4, 3 } });
    add_mnemonic_code("VESLVB", "VESLV", { { 3, 0 } });
    add_mnemonic_code("VESLVH", "VESLV", { { 3, 1 } });
    add_mnemonic_code("VESLVF", "VESLV", { { 3, 2 } });
    add_mnemonic_code("VESLVG", "VESLV", { { 3, 3 } });
    add_mnemonic_code("VESLB", "VESL", { { 3, 0 } });
    add_mnemonic_code("VESLH", "VESL", { { 3, 1 } });
    add_mnemonic_code("VESLF", "VESL", { { 3, 2 } });
    add_mnemonic_code("VESLG", "VESL", { { 3, 3 } });
    add_mnemonic_code("VESRAVB", "VESRAV", { { 3, 0 } });
    add_mnemonic_code("VESRAVH", "VESRAV", { { 3, 1 } });
    add_mnemonic_code("VESRAVF", "VESRAV", { { 3, 2 } });
    add_mnemonic_code("VESRAVG", "VESRAV", { { 3, 3 } });
    add_mnemonic_code("VESRAB", "VESRA", { { 3, 0 } });
    add_mnemonic_code("VESRAH", "VESRA", { { 3, 1 } });
    add_mnemonic_code("VESRAF", "VESRA", { { 3, 2 } });
    add_mnemonic_code("VESRAG", "VESRA", { { 3, 3 } });
    add_mnemonic_code("VESRLVB", "VESRLV", { { 3, 0 } });
    add_mnemonic_code("VESRLVH", "VESRLV", { { 3, 1 } });
    add_mnemonic_code("VESRLVF", "VESRLV", { { 3, 2 } });
    add_mnemonic_code("VESRLVG", "VESRLV", { { 3, 3 } });
    add_mnemonic_code("VESRLB", "VESRL", { { 3, 0 } });
    add_mnemonic_code("VESRLH", "VESRL", { { 3, 1 } });
    add_mnemonic_code("VESRLF", "VESRL", { { 3, 2 } });
    add_mnemonic_code("VESRLG", "VESRL", { { 3, 3 } });
    add_mnemonic_code("VCEFB", "VCFPS", { { 2, 0 } });
    add_mnemonic_code("VSB", "VS", { { 3, 0 } });
    add_mnemonic_code("VSH", "VS", { { 3, 1 } });
    add_mnemonic_code("VSF", "VS", { { 3, 2 } });
    add_mnemonic_code("VSG", "VS", { { 3, 3 } });
    add_mnemonic_code("VSQ", "VS", { { 3, 4 } });
    add_mnemonic_code("VLERH", "VLER", { { 2, 1 } });
    add_mnemonic_code("VLERF", "VLER", { { 2, 2 } });
    add_mnemonic_code("VLERG", "VLER", { { 2, 3 } });
    add_mnemonic_code("VSCBIB", "VSCBI", { { 3, 0 } });
    add_mnemonic_code("VCFEB", "VCSFP", { { 2, 2 } });
    add_mnemonic_code("VSCBIH", "VSCBI", { { 3, 1 } });
    add_mnemonic_code("VSCBIF", "VSCBI", { { 3, 2 } });
    add_mnemonic_code("VSCBIG", "VSCBI", { { 3, 3 } });
    add_mnemonic_code("VSCBIQ", "VSCBI", { { 3, 4 } });
    add_mnemonic_code("VSBIQ", "VSBI", { { 4, 4 } });
    add_mnemonic_code("VSBCBIQ", "VSBCBI", { { 4, 4 } });
    add_mnemonic_code("VSUMQF", "VSUMQ", { { 3, 2 } });
    add_mnemonic_code("VSUMQG", "VSUMQ", { { 3, 3 } });
    add_mnemonic_code("VSUMGH", "VSUMG", { { 3, 1 } });
    add_mnemonic_code("VSUMGF", "VSUMG", { { 3, 2 } });
    add_mnemonic_code("VSUMB", "VSUM", { { 3, 0 } });
    add_mnemonic_code("VSUMH", "VSUM", { { 3, 1 } });
    add_mnemonic_code("VFAEB", "VFAE", { { 3, 0 } });
    add_mnemonic_code("VFAEH", "VFAE", { { 3, 1 } });
    add_mnemonic_code("VFAEF", "VFAE", { { 3, 2 } });
    add_mnemonic_code("VFEEB", "VFEE", { { 3, 0 } });
    add_mnemonic_code("VFEEH", "VFEE", { { 3, 1 } });
    add_mnemonic_code("VFEEF", "VFEE", { { 3, 2 } });
    add_mnemonic_code("VLBRH", "VLBR", { { 2, 1 } });
    add_mnemonic_code("VLBRF", "VLBR", { { 2, 2 } });
    add_mnemonic_code("VLBRG", "VLBR", { { 2, 3 } });
    add_mnemonic_code("VLBRQ", "VLBR", { { 2, 4 } });
    add_mnemonic_code("VFEEBS", "VFEE", { { 3, 0 }, { 4, 1 } });
    add_mnemonic_code("VFEEGS", "VFEE", { { 3, 1 }, { 4, 1 } });
    add_mnemonic_code("VFEEFS", "VFEE", { { 3, 2 }, { 4, 1 } });
    add_mnemonic_code("VFEEZB", "VFEE", { { 3, 0 }, { 4, 2 } });
    add_mnemonic_code("VFEEZH", "VFEE", { { 3, 1 }, { 4, 2 } });
    add_mnemonic_code("VFEEZF", "VFEE", { { 3, 2 }, { 4, 2 } });
    add_mnemonic_code("VFEEZBS", "VFEE", { { 3, 0 }, { 4, 3 } });
    add_mnemonic_code("VFEEZHS", "VFEE", { { 3, 1 }, { 4, 3 } });
    add_mnemonic_code("VFEEZFS", "VFEE", { { 3, 2 }, { 4, 3 } });
    add_mnemonic_code("VFENEB", "VFENE", { { 3, 0 } });
    add_mnemonic_code("VFENEH", "VFENE", { { 3, 1 } });
    add_mnemonic_code("VFENEF", "VFENE", { { 3, 2 } });
    add_mnemonic_code("VFENEBS", "VFENE", { { 3, 0 }, { 4, 1 } });
    add_mnemonic_code("VFENEHS", "VFENE", { { 3, 1 }, { 4, 1 } });
    add_mnemonic_code("VFENEFS", "VFENE", { { 3, 2 }, { 4, 1 } });
    add_mnemonic_code("VFENEZB", "VFENE", { { 3, 0 }, { 4, 2 } });
    add_mnemonic_code("VFENEZH", "VFENE", { { 3, 1 }, { 4, 2 } });
    add_mnemonic_code("VFENEZF", "VFENE", { { 3, 2 }, { 4, 2 } });
    add_mnemonic_code("VFENEZBS", "VFENE", { { 3, 0 }, { 4, 3 } });
    add_mnemonic_code("VFENEZHS", "VFENE", { { 3, 1 }, { 4, 3 } });
    add_mnemonic_code("VFENEZFS", "VFENE", { { 3, 2 }, { 4, 3 } });
    add_mnemonic_code("VISTRB", "VISTR", { { 3, 0 } });
    add_mnemonic_code("VISTRH", "VISTR", { { 3, 1 } });
    add_mnemonic_code("VISTRF", "VISTR", { { 3, 2 } });
    add_mnemonic_code("VISTRBS", "VISTR", { { 3, 0 }, { 4, 1 } });
    add_mnemonic_code("VISTRHS", "VISTR", { { 3, 1 }, { 4, 1 } });
    add_mnemonic_code("VISTRFS", "VISTR", { { 3, 2 }, { 4, 1 } });
    add_mnemonic_code("VSTRCB", "VSTRC", { { 4, 0 } });
    add_mnemonic_code("VSTRCH", "VSTRC", { { 4, 1 } });
    add_mnemonic_code("VSTRCF", "VSTRC", { { 4, 2 } });
    add_mnemonic_code("VLBRREPH", "VLBRREP", { { 2, 1 } });
    add_mnemonic_code("VLBRREPF", "VLBRREP", { { 2, 2 } });
    add_mnemonic_code("VLBRREPG", "VLBRREP", { { 2, 3 } });
    add_mnemonic_code("VFASB", "VFA", { { 3, 2 }, { 4, 0 } });
    add_mnemonic_code("VFADB", "VFA", { { 3, 3 }, { 4, 0 } });
    add_mnemonic_code("WCDGB", "VCFPS", { { 2, 2 } });
    add_mnemonic_code("WFASB", "VFA", { { 3, 2 }, { 4, 8 } });
    add_mnemonic_code("WFADB", "VFA", { { 3, 3 }, { 4, 8 } });
    add_mnemonic_code("WFAXB", "VFA", { { 3, 4 }, { 4, 8 } });
    add_mnemonic_code("WFCSB", "WFC", { { 3, 2 }, { 4, 0 } });
    add_mnemonic_code("WFCDB", "WFC", { { 3, 3 }, { 4, 0 } });
    add_mnemonic_code("WFCXB", "WFC", { { 3, 4 }, { 4, 0 } });
    add_mnemonic_code("WFKSB", "WFK", { { 3, 2 }, { 4, 0 } });
    add_mnemonic_code("WFKDB", "WFK", { { 3, 3 }, { 4, 0 } });
    add_mnemonic_code("WFKXB", "WFK", { { 3, 4 }, { 4, 0 } });
    add_mnemonic_code("VFCESB", "VFCE", { { 3, 2 }, { 4, 0 }, { 5, 0 } });
    add_mnemonic_code("VFCESBS", "VFCE", { { 3, 2 }, { 4, 0 }, { 5, 1 } });
    add_mnemonic_code("VFCEDB", "VFCE", { { 3, 3 }, { 4, 0 }, { 5, 0 } });
    add_mnemonic_code("VFCEDBS", "VFCE", { { 3, 3 }, { 4, 0 }, { 5, 1 } });
    add_mnemonic_code("WFCESB", "VFCE", { { 3, 2 }, { 4, 8 }, { 5, 0 } });
    add_mnemonic_code("WFCESBS", "VFCE", { { 3, 2 }, { 4, 8 }, { 5, 1 } });
    add_mnemonic_code("WFCEDB", "VFCE", { { 3, 3 }, { 4, 8 }, { 5, 0 } });
    add_mnemonic_code("WFCEDBS", "VFCE", { { 3, 3 }, { 4, 8 }, { 5, 1 } });
    add_mnemonic_code("WFCEXB", "VFCE", { { 3, 4 }, { 4, 8 }, { 5, 0 } });
    add_mnemonic_code("WFCEXBS", "VFCE", { { 3, 4 }, { 4, 8 }, { 5, 1 } });
    add_mnemonic_code("VFKESB", "VFCE", { { 3, 2 }, { 4, 4 }, { 5, 0 } });
    add_mnemonic_code("VFKESBS", "VFCE", { { 3, 2 }, { 4, 4 }, { 5, 1 } });
    add_mnemonic_code("VFKEDB", "VFCE", { { 3, 3 }, { 4, 4 }, { 5, 0 } });
    add_mnemonic_code("VFKEDBS", "VFCE", { { 3, 3 }, { 4, 4 }, { 5, 1 } });
    add_mnemonic_code("WFKESB", "VFCE", { { 3, 2 }, { 4, 12 }, { 5, 0 } });
    add_mnemonic_code("WFKESBS", "VFCE", { { 3, 2 }, { 4, 12 }, { 5, 1 } });
    add_mnemonic_code("WFKEDB", "VFCE", { { 3, 3 }, { 4, 12 }, { 5, 0 } });
    add_mnemonic_code("WFKEDBS", "VFCE", { { 3, 3 }, { 4, 12 }, { 5, 1 } });
    add_mnemonic_code("WFKEXB", "VFCE", { { 3, 4 }, { 4, 12 }, { 5, 0 } });
    add_mnemonic_code("WFKEXBS", "VFCE", { { 3, 4 }, { 4, 12 }, { 5, 1 } });
    add_mnemonic_code("VSTRSB", "VSTRS", { { 4, 0 } });
    add_mnemonic_code("VSTRSH", "VSTRS", { { 4, 1 } });
    add_mnemonic_code("VSTRSF", "VSTRS", { { 4, 2 } });
    add_mnemonic_code("VSTRSZB", "VSTRS", { { 4, 0 }, { 5, 2 } });
    add_mnemonic_code("VFCHSB", "VFCH", { { 3, 2 }, { 4, 0 }, { 5, 0 } });
    add_mnemonic_code("VFCHSBS", "VFCH", { { 3, 2 }, { 4, 0 }, { 5, 1 } });
    add_mnemonic_code("VFCHDB", "VFCH", { { 3, 3 }, { 4, 0 }, { 5, 0 } });
    add_mnemonic_code("VFCHDBS", "VFCH", { { 3, 3 }, { 4, 0 }, { 5, 1 } });
    add_mnemonic_code("WFCHSB", "VFCH", { { 3, 2 }, { 4, 8 }, { 5, 0 } });
    add_mnemonic_code("WFCHSBS", "VFCH", { { 3, 2 }, { 4, 8 }, { 5, 1 } });
    add_mnemonic_code("WFCHDB", "VFCH", { { 3, 3 }, { 4, 8 }, { 5, 0 } });
    add_mnemonic_code("WFCHDBS", "VFCH", { { 3, 3 }, { 4, 8 }, { 5, 1 } });
    add_mnemonic_code("WFCHXB", "VFCH", { { 3, 4 }, { 4, 8 }, { 5, 0 } });
    add_mnemonic_code("WFCHXBS", "VFCH", { { 3, 4 }, { 4, 8 }, { 5, 1 } });
    add_mnemonic_code("VFKHSB", "VFCH", { { 3, 2 }, { 4, 4 }, { 5, 0 } });
    add_mnemonic_code("VFKHSBS", "VFCH", { { 3, 2 }, { 4, 4 }, { 5, 1 } });
    add_mnemonic_code("VFKHDB", "VFCH", { { 3, 3 }, { 4, 4 }, { 5, 0 } });
    add_mnemonic_code("VFKHDBS", "VFCH", { { 3, 3 }, { 4, 4 }, { 5, 1 } });
    add_mnemonic_code("WFKHSB", "VFCH", { { 3, 2 }, { 4, 12 }, { 5, 0 } });
    add_mnemonic_code("WFKHSBS", "VFCH", { { 3, 2 }, { 4, 12 }, { 5, 1 } });
    add_mnemonic_code("WFKHDB", "VFCH", { { 3, 3 }, { 4, 12 }, { 5, 0 } });
    add_mnemonic_code("WFKHDBS", "VFCH", { { 3, 3 }, { 4, 12 }, { 5, 1 } });
    add_mnemonic_code("WFKHXB", "VFCH", { { 3, 4 }, { 4, 12 }, { 5, 0 } });
    add_mnemonic_code("WFKHXBS", "VFCH", { { 3, 4 }, { 4, 12 }, { 5, 1 } });
    add_mnemonic_code("VFCHESB", "VFCHE", { { 3, 2 }, { 4, 0 }, { 5, 0 } });
    add_mnemonic_code("VFCHESBS", "VFCHE", { { 3, 2 }, { 4, 0 }, { 5, 1 } });
    add_mnemonic_code("VFCHEDB", "VFCHE", { { 3, 3 }, { 4, 0 }, { 5, 0 } });
    add_mnemonic_code("VFCHEDBS", "VFCHE", { { 3, 3 }, { 4, 0 }, { 5, 1 } });
    add_mnemonic_code("WFCHESB", "VFCHE", { { 3, 2 }, { 4, 8 }, { 5, 0 } });
    add_mnemonic_code("WFCHESBS", "VFCHE", { { 3, 2 }, { 4, 8 }, { 5, 1 } });
    add_mnemonic_code("WFCHEDB", "VFCHE", { { 3, 3 }, { 4, 8 }, { 5, 0 } });
    add_mnemonic_code("WFCHEDBS", "VFCHE", { { 3, 3 }, { 4, 8 }, { 5, 1 } });
    add_mnemonic_code("WFCHEXB", "VFCHE", { { 3, 4 }, { 4, 8 }, { 5, 0 } });
    add_mnemonic_code("WFCHEXBS", "VFCHE", { { 3, 4 }, { 4, 8 }, { 5, 1 } });
    add_mnemonic_code("VFKHESB", "VFCHE", { { 3, 2 }, { 4, 4 }, { 5, 0 } });
    add_mnemonic_code("VFKHESBS", "VFCHE", { { 3, 2 }, { 4, 4 }, { 5, 1 } });
    add_mnemonic_code("VFKHEDB", "VFCHE", { { 3, 3 }, { 4, 4 }, { 5, 0 } });
    add_mnemonic_code("VFKHEDBS", "VFCHE", { { 3, 3 }, { 4, 4 }, { 5, 1 } });
    add_mnemonic_code("WFKHESB", "VFCHE", { { 3, 2 }, { 4, 12 }, { 5, 0 } });
    add_mnemonic_code("WFKHESBS", "VFCHE", { { 3, 2 }, { 4, 12 }, { 5, 1 } });
    add_mnemonic_code("WFKHEDB", "VFCHE", { { 3, 3 }, { 4, 12 }, { 5, 0 } });
    add_mnemonic_code("WFKHEDBS", "VFCHE", { { 3, 3 }, { 4, 12 }, { 5, 1 } });
    add_mnemonic_code("WFKHEXB", "VFCHE", { { 3, 4 }, { 4, 12 }, { 5, 0 } });
    add_mnemonic_code("WFKHEXBS", "VFCHE", { { 3, 4 }, { 4, 12 }, { 5, 1 } });
    add_mnemonic_code("VCDGB", "VCFPS", { { 2, 3 } });
    add_mnemonic_code("VCDLG", "VCFPL", {});
    add_mnemonic_code("VCDLGB", "VCFPL", { { 2, 3 } });
    add_mnemonic_code("VCGDB", "VCSFP", { { 2, 3 } });
    add_mnemonic_code("VCLGDB", "VCLGD", { { 2, 3 } });
    add_mnemonic_code("VCLFEB", "VCLFP", { { 2, 0 } });
    add_mnemonic_code("VFDSB", "VFD", { { 3, 2 }, { 4, 0 } });
    add_mnemonic_code("WFDSB", "VFD", { { 3, 2 }, { 4, 8 } });
    add_mnemonic_code("VFDDB", "VFD", { { 3, 3 }, { 4, 0 } });
    add_mnemonic_code("WFDDB", "VFD", { { 3, 3 }, { 4, 8 } });
    add_mnemonic_code("WFDXB", "VFD", { { 3, 4 }, { 4, 8 } });
    add_mnemonic_code("VFISB", "VFI", { { 2, 2 } });
    add_mnemonic_code("VFIDB", "VFI", { { 2, 3 } });
    add_mnemonic_code("VLDE", "VFLL", {});
    add_mnemonic_code("VLDEB", "VFLL", { { 2, 2 }, { 3, 0 } });
    add_mnemonic_code("WLDEB", "VFLL", { { 2, 2 }, { 3, 8 } });
    add_mnemonic_code("VFLLS", "VFLL", { { 2, 2 }, { 3, 0 } });
    add_mnemonic_code("WFLLS", "VFLL", { { 2, 2 }, { 3, 8 } });
    add_mnemonic_code("WFLLD", "VFLL", { { 2, 3 }, { 3, 8 } });
    add_mnemonic_code("VLED", "VFLR", {});
    add_mnemonic_code("VLEDB", "VFLR", { { 2, 3 } });
    add_mnemonic_code("VFLRD", "VFLR", { { 2, 3 } });
    add_mnemonic_code("VFMAXSB", "VFMAX", { { 3, 2 }, { 4, 0 } });
    add_mnemonic_code("VFMAXDB", "VFMAX", { { 3, 3 }, { 4, 0 } });
    add_mnemonic_code("WFMAXSB", "VFMAX", { { 3, 2 }, { 4, 8 } });
    add_mnemonic_code("WFMAXDB", "VFMAX", { { 3, 3 }, { 4, 8 } });
    add_mnemonic_code("WFMAXXB", "VFMAX", { { 3, 4 }, { 4, 8 } });
    add_mnemonic_code("VFMINSB", "VFMIN", { { 3, 2 }, { 4, 0 } });
    add_mnemonic_code("VFMINDB", "VFMIN", { { 3, 3 }, { 4, 0 } });
    add_mnemonic_code("WFMINSB", "VFMIN", { { 3, 2 }, { 4, 8 } });
    add_mnemonic_code("WFMINDB", "VFMIN", { { 3, 3 }, { 4, 8 } });
    add_mnemonic_code("WFMINXB", "VFMIN", { { 3, 4 }, { 4, 8 } });
    add_mnemonic_code("VFMSB", "VFM", { { 3, 2 }, { 4, 0 } });
    add_mnemonic_code("VFMDB", "VFM", { { 3, 3 }, { 4, 0 } });
    add_mnemonic_code("WFMSB", "VFM", { { 3, 2 }, { 4, 8 } });
    add_mnemonic_code("WFMDB", "VFM", { { 3, 3 }, { 4, 8 } });
    add_mnemonic_code("WFMXB", "VFM", { { 3, 4 }, { 4, 8 } });
    add_mnemonic_code("VFMASB", "VFMA", { { 4, 0 }, { 5, 2 } });
    add_mnemonic_code("VFMADB", "VFMA", { { 4, 0 }, { 5, 3 } });
    add_mnemonic_code("WFMASB", "VFMA", { { 4, 8 }, { 5, 2 } });
    add_mnemonic_code("WFMADB", "VFMA", { { 4, 8 }, { 5, 3 } });
    add_mnemonic_code("WFMAXB", "VFMA", { { 4, 8 }, { 5, 4 } });
    add_mnemonic_code("VFMSSB", "VFMS", { { 4, 0 }, { 5, 2 } });
    add_mnemonic_code("VFMSDB", "VFMS", { { 4, 0 }, { 5, 3 } });
    add_mnemonic_code("WFMSSB", "VFMS", { { 4, 8 }, { 5, 2 } });
    add_mnemonic_code("WFMSDB", "VFMS", { { 4, 8 }, { 5, 3 } });
    add_mnemonic_code("WFMSXB", "VFMS", { { 4, 8 }, { 5, 4 } });
    add_mnemonic_code("VFNMASB", "VFNMA", { { 4, 0 }, { 5, 2 } });
    add_mnemonic_code("VFNMADB", "VFNMA", { { 4, 0 }, { 5, 3 } });
    add_mnemonic_code("WFNMASB", "VFNMA", { { 4, 8 }, { 5, 2 } });
    add_mnemonic_code("WFNMADB", "VFNMA", { { 4, 8 }, { 5, 3 } });
    add_mnemonic_code("WFNMAXB", "VFNMA", { { 4, 8 }, { 5, 4 } });
    add_mnemonic_code("VFNMSSB", "VFNMS", { { 4, 0 }, { 5, 2 } });
    add_mnemonic_code("VFNMSDB", "VFNMS", { { 4, 0 }, { 5, 3 } });
    add_mnemonic_code("WFNMSSB", "VFNMS", { { 4, 8 }, { 5, 2 } });
    add_mnemonic_code("WFNMSDB", "VFNMS", { { 4, 8 }, { 5, 3 } });
    add_mnemonic_code("WFNMSXB", "VFNMS", { { 4, 8 }, { 5, 4 } });
    add_mnemonic_code("VFPSOSB", "VFPSO", { { 2, 2 }, { 3, 0 } });
    add_mnemonic_code("WFPSOSB", "VFPSO", { { 2, 2 }, { 3, 8 } });
    add_mnemonic_code("VFLCSB", "VFPSO", { { 2, 2 }, { 3, 0 }, { 4, 0 } });
    add_mnemonic_code("VCELFB", "VCFPL", { { 2, 0 } });
    add_mnemonic_code("VLLEBRZH", "VLLEBRZ", { { 2, 1 } });
    add_mnemonic_code("VLLEBRZF", "VLLEBRZ", { { 2, 2 } });
    add_mnemonic_code("VLLEBRZG", "VLLEBRZ", { { 2, 3 } });
    add_mnemonic_code("VLLEBRZE", "VLLEBRZ", { { 2, 6 } });
    add_mnemonic_code("LDRV", "VLLEBRZ", { { 2, 3 } });
    add_mnemonic_code("LERV", "VLLEBRZ", { { 2, 6 } });
    add_mnemonic_code("VPKF", "VPK", { { 3, 2 } });
    add_mnemonic_code("VPKG", "VPK", { { 3, 3 } });
    add_mnemonic_code("VPKH", "VPK", { { 3, 1 } });
    add_mnemonic_code("WFLCSB", "VFPSO", { { 2, 2 }, { 3, 8 }, { 4, 0 } });
    add_mnemonic_code("VFLNSB", "VFPSO", { { 2, 2 }, { 3, 0 }, { 4, 1 } });
    add_mnemonic_code("WFLNSB", "VFPSO", { { 2, 2 }, { 3, 8 }, { 4, 1 } });
    add_mnemonic_code("VFLPSB", "VFPSO", { { 2, 2 }, { 3, 0 }, { 4, 2 } });
    add_mnemonic_code("WFLPSB", "VFPSO", { { 2, 2 }, { 3, 8 }, { 4, 2 } });
    add_mnemonic_code("VFPSODB", "VFPSO", { { 2, 3 }, { 3, 0 } });
    add_mnemonic_code("WFPSODB", "VFPSO", { { 2, 3 }, { 3, 8 } });
    add_mnemonic_code("VFLCDB", "VFPSO", { { 2, 3 }, { 3, 0 }, { 4, 0 } });
    add_mnemonic_code("WFLCDB", "VFPSO", { { 2, 3 }, { 3, 8 }, { 4, 0 } });
    add_mnemonic_code("VFLNDB", "VFPSO", { { 2, 3 }, { 3, 0 }, { 4, 1 } });
    add_mnemonic_code("WFLNDB", "VFPSO", { { 2, 3 }, { 3, 8 }, { 4, 1 } });
    add_mnemonic_code("VFLPDB", "VFPSO", { { 2, 3 }, { 3, 0 }, { 4, 2 } });
    add_mnemonic_code("WFLPDB", "VFPSO", { { 2, 3 }, { 3, 8 }, { 4, 2 } });
    add_mnemonic_code("WFPSOXB", "VFPSO", { { 2, 4 }, { 3, 8 } });
    add_mnemonic_code("WFLCXB", "VFPSO", { { 2, 4 }, { 3, 8 }, { 4, 0 } });
    add_mnemonic_code("WFLNXB", "VFPSO", { { 2, 4 }, { 3, 8 }, { 4, 1 } });
    add_mnemonic_code("WFLPXB", "VFPSO", { { 2, 4 }, { 3, 8 }, { 4, 2 } });
    add_mnemonic_code("VFSQSB", "VFSQ", { { 2, 2 }, { 3, 0 } });
    add_mnemonic_code("VFSQDB", "VFSQ", { { 2, 3 }, { 3, 0 } });
    add_mnemonic_code("WFSQSB", "VFSQ", { { 2, 2 }, { 3, 8 } });
    add_mnemonic_code("WFSQDB", "VFSQ", { { 2, 3 }, { 3, 8 } });
    add_mnemonic_code("WFSQXB", "VFSQ", { { 2, 4 }, { 3, 8 } });
    add_mnemonic_code("VFSSB", "VFS", { { 2, 2 }, { 3, 0 } });
    add_mnemonic_code("VFSDB", "VFS", { { 2, 3 }, { 3, 0 } });
    add_mnemonic_code("WFSSB", "VFS", { { 2, 2 }, { 3, 8 } });
    add_mnemonic_code("WFSDB", "VFS", { { 2, 3 }, { 3, 8 } });
    add_mnemonic_code("WFSXB", "VFS", { { 2, 4 }, { 3, 8 } });
    add_mnemonic_code("VFTCISB", "VFTCI", { { 3, 2 }, { 4, 0 } });
    add_mnemonic_code("VFTCIDB", "VFTCI", { { 3, 3 }, { 4, 0 } });
    add_mnemonic_code("WFTCISB", "VFTCI", { { 3, 2 }, { 4, 8 } });
    add_mnemonic_code("WFTCIDB", "VFTCI", { { 3, 3 }, { 4, 8 } });
    add_mnemonic_code("WFTCIXB", "VFTCI", { { 3, 4 }, { 4, 8 } });
    add_mnemonic_code("XHHR", "RXSBG", { { 2, 0 }, { 3, 31 } });
    add_mnemonic_code("XLHR", "RXSBG", { { 2, 32 }, { 3, 63 }, { 4, 32 } });
    // instruction under this position contain an OR operation not marked in this list

    // in case the operand is ommited, the OR number should be assigned to the value of the ommited operand
    add_mnemonic_code("VFAEBS", "VFAE", { { 3, 0 } }); // operand with index 4 ORed with 1
    add_mnemonic_code("VFAEHS", "VFAE", { { 3, 1 } }); // operand with index 4 ORed with 1
    add_mnemonic_code("VFAEFS", "VFAE", { { 3, 2 } }); // operand with index 4 ORed with 1
    add_mnemonic_code("VFAEZB", "VFAE", { { 3, 0 } }); // operand with index 4 ORed with 2
    add_mnemonic_code("VFAEZH", "VFAE", { { 3, 1 } }); // operand with index 4 ORed with 2
    add_mnemonic_code("VFAEZF", "VFAE", { { 3, 2 } }); // operand with index 4 ORed with 2
    add_mnemonic_code("VFAEZBS", "VFAE", { { 3, 0 } }); // operand with index 4 ORed with 3
    add_mnemonic_code("VFAEZHS", "VFAE", { { 3, 1 } }); // operand with index 4 ORed with 3
    add_mnemonic_code("VFAEZFS", "VFAE", { { 3, 2 } }); // operand with index 4 ORed with 3
    add_mnemonic_code("VSTRCBS", "VSTRC", { { 4, 0 } }); // operand with index 5 ORed with 1
    add_mnemonic_code("VSTRCHS", "VSTRC", { { 4, 1 } }); // operand with index 5 ORed with 1
    add_mnemonic_code("VSTRCFS", "VSTRC", { { 4, 2 } }); // operand with index 5 ORed with 1
    add_mnemonic_code("VSTRCZB", "VSTRC", { { 4, 0 } }); // operand with index 5 ORed with 2
    add_mnemonic_code("VSTRCZH", "VSTRC", { { 4, 1 } }); // operand with index 5 ORed with 2
    add_mnemonic_code("VSTRCZF", "VSTRC", { { 4, 2 } }); // operand with index 5 ORed with 2
    add_mnemonic_code("VSTRCZBS", "VSTRC", { { 4, 0 } }); // operand with index 5 ORed with 3
    add_mnemonic_code("VSTRCZHS", "VSTRC", { { 4, 1 } }); // operand with index 5 ORed with 3
    add_mnemonic_code("VSTRCZFS", "VSTRC", { { 4, 2 } }); // operand with index 5 ORed
                                                          // with 3 always OR
    add_mnemonic_code("WFISB", "VFI", { { 2, 2 } }); // operand with index 3 ORed with 8
    add_mnemonic_code("WFIDB", "VFI", { { 2, 3 } }); // operand with index 3 ORed with 8
    add_mnemonic_code("WFIXB", "VFI", { { 2, 4 } }); // operand with index 3 ORed with 8
    add_mnemonic_code("WCDLGB", "VCFPL", { { 2, 3 } }); // operand with index 3 ORed with 8
    add_mnemonic_code("WCGDB", "VCSFP", { { 2, 3 } }); // operand with index 3 ORed with 8
    add_mnemonic_code("WCELFB", "VCFPL", { { 2, 2 } }); // operand with index 3 ORed with 8
    add_mnemonic_code("WCLFEB", "VCLFP", { { 2, 2 } }); // operand with index 3 ORed with 8
    add_mnemonic_code("WCEFB", "VCFPS", { { 2, 2 } }); // operand with index 3 ORed with 8
    add_mnemonic_code("WCDGB", "VCFPS", { { 2, 3 } }); // operand with index 3 ORed with 8
    add_mnemonic_code("WCFEB", "VCSFP", { { 2, 2 } }); // operand with index 3 ORed with 8
    add_mnemonic_code("WCLGDB", "VCLGD", { { 2, 3 } }); // operand with index 3 ORed with 8
    add_mnemonic_code("WLEDB", "VFLR", { { 2, 3 } }); // operand with index 3 ORed with 8
    add_mnemonic_code("WFLRD", "VFLR", { { 2, 3 } }); // operand with index 3 ORed with 8
    add_mnemonic_code("WFLRX", "VFLR", { { 2, 4 } }); // operand with index 3 ORed with 8

    // mnemonics not in principles
    add_mnemonic_code("CIJE", "CIJ", { { 2, 8 } });
    add_mnemonic_code("CIJH", "CIJ", { { 2, 2 } });
    add_mnemonic_code("CIJL", "CIJ", { { 2, 4 } });
    add_mnemonic_code("CIJNE", "CIJ", { { 2, 6 } });
    add_mnemonic_code("CIJNH", "CIJ", { { 2, 12 } });
    add_mnemonic_code("CIJNL", "CIJ", { { 2, 10 } });
    add_mnemonic_code("CGIBE", "CGIB", { { 2, 8 } });
    add_mnemonic_code("CGIBH", "CGIB", { { 2, 2 } });
    add_mnemonic_code("CGIBL", "CGIB", { { 2, 4 } });
    add_mnemonic_code("CGIBNE", "CGIB", { { 2, 6 } });
    add_mnemonic_code("CGIBNH", "CGIB", { { 2, 12 } });
    add_mnemonic_code("CGIBNL", "CGIB", { { 2, 10 } });
    add_mnemonic_code("CGIJE", "CGIJ", { { 2, 8 } });
    add_mnemonic_code("CGIJH", "CGIJ", { { 2, 2 } });
    add_mnemonic_code("CGIJL", "CGIJ", { { 2, 4 } });
    add_mnemonic_code("CGIJNE", "CGIJ", { { 2, 6 } });
    add_mnemonic_code("CGIJNH", "CGIJ", { { 2, 12 } });
    add_mnemonic_code("CGIJNL", "CGIJ", { { 2, 10 } });
    add_mnemonic_code("CGITE", "CGIT", { { 2, 8 } });
    add_mnemonic_code("CGITH", "CGIT", { { 2, 2 } });
    add_mnemonic_code("CGITL", "CGIT", { { 2, 4 } });
    add_mnemonic_code("CGITNE", "CGIT", { { 2, 6 } });
    add_mnemonic_code("CGITNH", "CGIT", { { 2, 12 } });
    add_mnemonic_code("CGITNL", "CGIT", { { 2, 10 } });
    add_mnemonic_code("CGRBE", "CGRB", { { 2, 8 } });
    add_mnemonic_code("CGRBH", "CGRB", { { 2, 2 } });
    add_mnemonic_code("CGRBL", "CGRB", { { 2, 4 } });
    add_mnemonic_code("CGRBNE", "CGRB", { { 2, 6 } });
    add_mnemonic_code("CGRBNH", "CGRB", { { 2, 12 } });
    add_mnemonic_code("CGRBNL", "CGRB", { { 2, 10 } });
    add_mnemonic_code("CGRJE", "CGRJ", { { 2, 8 } });
    add_mnemonic_code("CGRJH", "CGRJ", { { 2, 2 } });
    add_mnemonic_code("CGRJL", "CGRJ", { { 2, 4 } });
    add_mnemonic_code("CGRJNE", "CGRJ", { { 2, 6 } });
    add_mnemonic_code("CGRJNH", "CGRJ", { { 2, 12 } });
    add_mnemonic_code("CGRJNL", "CGRJ", { { 2, 10 } });
    add_mnemonic_code("CGRTE", "CGRT", { { 2, 8 } });
    add_mnemonic_code("CGRTH", "CGRT", { { 2, 2 } });
    add_mnemonic_code("CGRTL", "CGRT", { { 2, 4 } });
    add_mnemonic_code("CGRTNE", "CGRT", { { 2, 6 } });
    add_mnemonic_code("CGRTNH", "CGRT", { { 2, 12 } });
    add_mnemonic_code("CGRTNL", "CGRT", { { 2, 10 } });
    add_mnemonic_code("CIBE", "CIB", { { 2, 8 } });
    add_mnemonic_code("CIBH", "CIB", { { 2, 2 } });
    add_mnemonic_code("CIBL", "CIB", { { 2, 4 } });
    add_mnemonic_code("CIBNE", "CIB", { { 2, 6 } });
    add_mnemonic_code("CIBNH", "CIB", { { 2, 12 } });
    add_mnemonic_code("CIBNL", "CIB", { { 2, 10 } });
    add_mnemonic_code("CITE", "CIT", { { 2, 8 } });
    add_mnemonic_code("CITH", "CIT", { { 2, 2 } });
    add_mnemonic_code("CITL", "CIT", { { 2, 4 } });
    add_mnemonic_code("CITNE", "CIT", { { 2, 6 } });
    add_mnemonic_code("CITNH", "CIT", { { 2, 12 } });
    add_mnemonic_code("CITNL", "CIT", { { 2, 10 } });
    add_mnemonic_code("CLFITE", "CLFIT", { { 2, 8 } });
    add_mnemonic_code("CLFITH", "CLFIT", { { 2, 2 } });
    add_mnemonic_code("CLFITL", "CLFIT", { { 2, 4 } });
    add_mnemonic_code("CLFITNE", "CLFIT", { { 2, 6 } });
    add_mnemonic_code("CLFITNH", "CLFIT", { { 2, 12 } });
    add_mnemonic_code("CLFITNL", "CLFIT", { { 2, 10 } });
    add_mnemonic_code("CLGIBE", "CLGIB", { { 2, 8 } });
    add_mnemonic_code("CLGIBH", "CLGIB", { { 2, 2 } });
    add_mnemonic_code("CLGIBL", "CLGIB", { { 2, 4 } });
    add_mnemonic_code("CLGIBNE", "CLGIB", { { 2, 6 } });
    add_mnemonic_code("CLGIBNH", "CLGIB", { { 2, 12 } });
    add_mnemonic_code("CLGIBNL", "CLGIB", { { 2, 10 } });
    add_mnemonic_code("CLGIJE", "CLGIJ", { { 2, 8 } });
    add_mnemonic_code("CLGIJH", "CLGIJ", { { 2, 2 } });
    add_mnemonic_code("CLGIJL", "CLGIJ", { { 2, 4 } });
    add_mnemonic_code("CLGIJNE", "CLGIJ", { { 2, 6 } });
    add_mnemonic_code("CLGIJNH", "CLGIJ", { { 2, 12 } });
    add_mnemonic_code("CLGIJNL", "CLGIJ", { { 2, 10 } });
    add_mnemonic_code("CLGITE", "CLGIT", { { 2, 8 } });
    add_mnemonic_code("CLGITH", "CLGIT", { { 2, 2 } });
    add_mnemonic_code("CLGITL", "CLGIT", { { 2, 4 } });
    add_mnemonic_code("CLGITNE", "CLGIT", { { 2, 6 } });
    add_mnemonic_code("CLGITNH", "CLGIT", { { 2, 12 } });
    add_mnemonic_code("CLGITNL", "CLGIT", { { 2, 10 } });
    add_mnemonic_code("CLGRBE", "CLGRB", { { 2, 8 } });
    add_mnemonic_code("CLGRBH", "CLGRB", { { 2, 2 } });
    add_mnemonic_code("CLGRBL", "CLGRB", { { 2, 4 } });
    add_mnemonic_code("CLGRBNE", "CLGRB", { { 2, 6 } });
    add_mnemonic_code("CLGRBNH", "CLGRB", { { 2, 12 } });
    add_mnemonic_code("CLGRBNL", "CLGRB", { { 2, 10 } });
    add_mnemonic_code("CLGRJE", "CLGRJ", { { 2, 8 } });
    add_mnemonic_code("CLGRJH", "CLGRJ", { { 2, 2 } });
    add_mnemonic_code("CLGRJL", "CLGRJ", { { 2, 4 } });
    add_mnemonic_code("CLGRJNE", "CLGRJ", { { 2, 6 } });
    add_mnemonic_code("CLGRJNH", "CLGRJ", { { 2, 12 } });
    add_mnemonic_code("CLGRJNL", "CLGRJ", { { 2, 10 } });
    add_mnemonic_code("CLGRTE", "CLGRT", { { 2, 8 } });
    add_mnemonic_code("CLGRTH", "CLGRT", { { 2, 2 } });
    add_mnemonic_code("CLGRTL", "CLGRT", { { 2, 4 } });
    add_mnemonic_code("CLGRTNE", "CLGRT", { { 2, 6 } });
    add_mnemonic_code("CLGRTNH", "CLGRT", { { 2, 12 } });
    add_mnemonic_code("CLGRTNL", "CLGRT", { { 2, 10 } });
    add_mnemonic_code("CLGTE", "CLGT", { { 1, 8 } });
    add_mnemonic_code("CLGTH", "CLGT", { { 1, 2 } });
    add_mnemonic_code("CLGTL", "CLGT", { { 1, 4 } });
    add_mnemonic_code("CLGTNE", "CLGT", { { 1, 6 } });
    add_mnemonic_code("CLGTNH", "CLGT", { { 1, 12 } });
    add_mnemonic_code("CLGTNL", "CLGT", { { 1, 10 } });
    add_mnemonic_code("CLIBE", "CLIB", { { 2, 8 } });
    add_mnemonic_code("CLIBH", "CLIB", { { 2, 2 } });
    add_mnemonic_code("CLIBL", "CLIB", { { 2, 4 } });
    add_mnemonic_code("CLIBNE", "CLIB", { { 2, 6 } });
    add_mnemonic_code("CLIBNH", "CLIB", { { 2, 12 } });
    add_mnemonic_code("CLIBNL", "CLIB", { { 2, 10 } });
    add_mnemonic_code("CLIJE", "CLIJ", { { 2, 8 } });
    add_mnemonic_code("CLIJH", "CLIJ", { { 2, 2 } });
    add_mnemonic_code("CLIJL", "CLIJ", { { 2, 4 } });
    add_mnemonic_code("CLIJNE", "CLIJ", { { 2, 6 } });
    add_mnemonic_code("CLIJNH", "CLIJ", { { 2, 12 } });
    add_mnemonic_code("CLIJNL", "CLIJ", { { 2, 10 } });
    add_mnemonic_code("CLRBE", "CLRB", { { 2, 8 } });
    add_mnemonic_code("CLRBH", "CLRB", { { 2, 2 } });
    add_mnemonic_code("CLRBL", "CLRB", { { 2, 4 } });
    add_mnemonic_code("CLRBNE", "CLRB", { { 2, 6 } });
    add_mnemonic_code("CLRBNH", "CLRB", { { 2, 12 } });
    add_mnemonic_code("CLRBNL", "CLRB", { { 2, 10 } });
    add_mnemonic_code("CLRJE", "CLRJ", { { 2, 8 } });
    add_mnemonic_code("CLRJH", "CLRJ", { { 2, 2 } });
    add_mnemonic_code("CLRJL", "CLRJ", { { 2, 4 } });
    add_mnemonic_code("CLRJNE", "CLRJ", { { 2, 6 } });
    add_mnemonic_code("CLRJNH", "CLRJ", { { 2, 12 } });
    add_mnemonic_code("CLRJNL", "CLRJ", { { 2, 10 } });
    add_mnemonic_code("CLRTE", "CLRT", { { 2, 8 } });
    add_mnemonic_code("CLRTH", "CLRT", { { 2, 2 } });
    add_mnemonic_code("CLRTL", "CLRT", { { 2, 4 } });
    add_mnemonic_code("CLRTNE", "CLRT", { { 2, 6 } });
    add_mnemonic_code("CLRTNH", "CLRT", { { 2, 12 } });
    add_mnemonic_code("CLRTNL", "CLRT", { { 2, 10 } });
    add_mnemonic_code("CLTE", "CLT", { { 1, 8 } });
    add_mnemonic_code("CLTH", "CLT", { { 1, 2 } });
    add_mnemonic_code("CLTL", "CLT", { { 1, 4 } });
    add_mnemonic_code("CLTNE", "CLT", { { 1, 6 } });
    add_mnemonic_code("CLTNH", "CLT", { { 1, 12 } });
    add_mnemonic_code("CLTNL", "CLT", { { 1, 10 } });
    add_mnemonic_code("CRBE", "CRB", { { 2, 8 } });
    add_mnemonic_code("CRBH", "CRB", { { 2, 2 } });
    add_mnemonic_code("CRBL", "CRB", { { 2, 4 } });
    add_mnemonic_code("CRBNE", "CRB", { { 2, 6 } });
    add_mnemonic_code("CRBNH", "CRB", { { 2, 12 } });
    add_mnemonic_code("CRBNL", "CRB", { { 2, 10 } });
    add_mnemonic_code("CRJE", "CRJ", { { 2, 8 } });
    add_mnemonic_code("CRJH", "CRJ", { { 2, 2 } });
    add_mnemonic_code("CRJL", "CRJ", { { 2, 4 } });
    add_mnemonic_code("CRJNE", "CRJ", { { 2, 6 } });
    add_mnemonic_code("CRJNH", "CRJ", { { 2, 12 } });
    add_mnemonic_code("CRJNL", "CRJ", { { 2, 10 } });
    add_mnemonic_code("CRTE", "CRT", { { 2, 8 } });
    add_mnemonic_code("CRTH", "CRT", { { 2, 2 } });
    add_mnemonic_code("CRTL", "CRT", { { 2, 4 } });
    add_mnemonic_code("CRTNE", "CRT", { { 2, 6 } });
    add_mnemonic_code("CRTNH", "CRT", { { 2, 12 } });
    add_mnemonic_code("CRTNL", "CRT", { { 2, 10 } });
    // operand with index 2 was omitted for the below instruction
    add_mnemonic_code("NOTR", "NORK", { { 2, 0 } });
    // operand with index 2 was omitted for the below instruction
    add_mnemonic_code("NOTGR", "NOGRK", { { 2, 0 } });
    add_mnemonic_code("LOCGE", "LOCG", { { 2, 8 } });
    add_mnemonic_code("LOCGH", "LOCG", { { 2, 2 } });
    add_mnemonic_code("LOCGL", "LOCG", { { 2, 4 } });
    add_mnemonic_code("LOCGNE", "LOCG", { { 2, 6 } });
    add_mnemonic_code("LOCGNH", "LOCG", { { 2, 12 } });
    add_mnemonic_code("LOCGNL", "LOCG", { { 2, 10 } });
    add_mnemonic_code("LOCRE", "LOCR", { { 2, 8 } });
    add_mnemonic_code("LOCRH", "LOCR", { { 2, 2 } });
    add_mnemonic_code("LOCRL", "LOCR", { { 2, 4 } });
    add_mnemonic_code("LOCRNE", "LOCR", { { 2, 6 } });
    add_mnemonic_code("LOCRNH", "LOCR", { { 2, 12 } });
    add_mnemonic_code("LOCRNL", "LOCR", { { 2, 10 } });
    add_mnemonic_code("LOCGRE", "LOCGR", { { 2, 8 } });
    add_mnemonic_code("LOCGRH", "LOCGR", { { 2, 2 } });
    add_mnemonic_code("LOCGRL", "LOCGR", { { 2, 4 } });
    add_mnemonic_code("LOCGRNE", "LOCGR", { { 2, 6 } });
    add_mnemonic_code("LOCGRNH", "LOCGR", { { 2, 12 } });
    add_mnemonic_code("LOCGRNL", "LOCGR", { { 2, 10 } });
    add_mnemonic_code("LOCE", "LOC", { { 2, 8 } });
    add_mnemonic_code("LOCH", "LOC", { { 2, 2 } });
    add_mnemonic_code("LOCL", "LOC", { { 2, 4 } });
    add_mnemonic_code("LOCNE", "LOC", { { 2, 6 } });
    add_mnemonic_code("LOCNH", "LOC", { { 2, 12 } });
    add_mnemonic_code("LOCNL", "LOC", { { 2, 10 } });
    add_mnemonic_code("STOCGE", "STOCG", { { 2, 8 } });
    add_mnemonic_code("STOCGH", "STOCG", { { 2, 2 } });
    add_mnemonic_code("STOCGL", "STOCG", { { 2, 4 } });
    add_mnemonic_code("STOCGNE", "STOCG", { { 2, 6 } });
    add_mnemonic_code("STOCGNH", "STOCG", { { 2, 12 } });
    add_mnemonic_code("STOCGNL", "STOCG", { { 2, 10 } });
    add_mnemonic_code("STOCE", "STOC", { { 2, 8 } });
    add_mnemonic_code("STOCH", "STOC", { { 2, 2 } });
    add_mnemonic_code("STOCL", "STOC", { { 2, 4 } });
    add_mnemonic_code("STOCNE", "STOC", { { 2, 6 } });
    add_mnemonic_code("STOCNH", "STOC", { { 2, 12 } });
    add_mnemonic_code("STOCNL", "STOC", { { 2, 10 } });
    // VNO V1,V2,V2        (operand with index 2 replaced with 0 )
    add_mnemonic_code("VNOT", "VNO", { { 2, 0 } });
    return result;
}

const std::map<std::string_view, mnemonic_code>* const mnemonic_codes = &instruction::all_mnemonic_codes();

const mnemonic_code* instruction::find_mnemonic_codes(std::string_view name)
{
    auto it = mnemonic_codes->find(name);
    if (it == mnemonic_codes->end())
        return nullptr;
    return &it->second;
}
const mnemonic_code& instruction::get_mnemonic_codes(std::string_view name)
{
    auto result = find_mnemonic_codes(name);
    assert(result);
    return *result;
}
const std::map<std::string_view, mnemonic_code>& instruction::all_mnemonic_codes()
{
    if (mnemonic_codes)
        return *mnemonic_codes;
    static const std::map<std::string_view, mnemonic_code> mnemonic_codes_ = generate_mnemonic_codes();
    return mnemonic_codes_;
}

// Generates a bitmask for an arbitrary machine instruction indicating which operands
// are of the RI type (and therefore are modified by transform_reloc_imm_operands)
unsigned char machine_instruction::generate_reladdr_bitmask(
    const std::vector<checking::machine_operand_format>& operands)
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

// Generates a bitmask for an arbitrary mnemonit indicating which operands
// are of the RI type (and therefore are modified by transform_reloc_imm_operands)
unsigned char mnemonic_code::generate_reladdr_bitmask(
    const machine_instruction* instruction, const std::vector<std::pair<size_t, size_t>>& replaced)
{
    unsigned char result = 0;

    decltype(result) top_bit = 1 << (std::numeric_limits<decltype(result)>::digits - 1);

    const std::pair<size_t, size_t>* replaced_b = replaced.data();
    const std::pair<size_t, size_t>* const replaced_e = replaced.data() + replaced.size();

    size_t position = 0;
    for (const auto& op : instruction->operands)
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
