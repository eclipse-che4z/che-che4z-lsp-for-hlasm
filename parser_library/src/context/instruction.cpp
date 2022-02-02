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

static auto generate_machine_instructions()
{
    std::set<machine_instruction, machine_instruction_comparer> result;

    static constexpr machine_operand_format empty_format(empty, empty, empty);
    struct instruction_definition
    {
        constexpr instruction_definition(std::string_view name,
            mach_format format,
            std::initializer_list<machine_operand_format> op_format_list,
            short page_no)
            : name(name)
            , format(format)
            , op_format { empty_format, empty_format, empty_format, empty_format, empty_format, empty_format }
            , op_format_size(op_format_list.size())
            , page_no(page_no)
        {
            size_t j = 0;
            for (const auto& f : op_format_list)
                op_format[j++] = f;
        }
        std::string_view name;
        mach_format format;
        std::array<machine_operand_format, 6> op_format;
        size_t op_format_size;
        short page_no;
    } static constexpr instructions[] = {
        { "AR", mach_format::RR, { reg_4_U, reg_4_U }, 510 },
        { "ADDFRR", mach_format::RRE, { reg_4_U, reg_4_U }, 7 },
        { "AGR", mach_format::RRE, { reg_4_U, reg_4_U }, 510 },
        { "AGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 510 },
        { "ARK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 510 },
        { "AGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 510 },
        { "A", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 510 },
        { "AY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 511 },
        { "AG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 511 },
        { "AGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 511 },
        { "AFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 511 },
        { "AGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 511 },
        { "AHIK", mach_format::RIE_d, { reg_4_U, reg_4_U, imm_16_S }, 511 },
        { "AGHIK", mach_format::RIE_d, { reg_4_U, reg_4_U, imm_16_S }, 511 },
        { "ASI", mach_format::SIY, { db_20_4_S, imm_8_S }, 511 },
        { "AGSI", mach_format::SIY, { db_20_4_S, imm_8_S }, 511 },
        { "AH", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 512 },
        { "AHY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 512 },
        { "AGH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 512 },
        { "AHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 512 },
        { "AGHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 513 },
        { "AHHHR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 513 },
        { "AHHLR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 513 },
        { "AIH", mach_format::RIL_a, { reg_4_U, imm_32_S }, 513 },
        { "ALR", mach_format::RR, { reg_4_U, reg_4_U }, 514 },
        { "ALGR", mach_format::RRE, { reg_4_U, reg_4_U }, 514 },
        { "ALGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 514 },
        { "ALRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 514 },
        { "ALGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 514 },
        { "AL", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 514 },
        { "ALY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 514 },
        { "ALG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 514 },
        { "ALGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 514 },
        { "ALFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 514 },
        { "ALGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 514 },
        { "ALHHHR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 515 },
        { "ALHHLR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 515 },
        { "ALCR", mach_format::RRE, { reg_4_U, reg_4_U }, 515 },
        { "ALCGR", mach_format::RRE, { reg_4_U, reg_4_U }, 515 },
        { "ALC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 515 },
        { "ALCG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 515 },
        { "ALSI", mach_format::SIY, { db_20_4_S, imm_8_S }, 516 },
        { "ALGSI", mach_format::SIY, { db_20_4_S, imm_8_S }, 516 },
        { "ALHSIK", mach_format::RIE_d, { reg_4_U, reg_4_U, imm_16_S }, 516 },
        { "ALGHSIK", mach_format::RIE_d, { reg_4_U, reg_4_U, imm_16_S }, 516 },
        { "ALSIH", mach_format::RIL_a, { reg_4_U, imm_32_S }, 517 },
        { "ALSIHN", mach_format::RIL_a, { reg_4_U, imm_32_S }, 517 },
        { "NR", mach_format::RR, { reg_4_U, reg_4_U }, 517 },
        { "NGR", mach_format::RRE, { reg_4_U, reg_4_U }, 517 },
        { "NRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 517 },
        { "NGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 517 },
        { "N", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 517 },
        { "NY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 517 },
        { "NG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 517 },
        { "NI", mach_format::SI, { db_12_4_U, imm_8_U }, 517 },
        { "NIY", mach_format::SIY, { db_20_4_S, imm_8_U }, 518 },
        { "NC", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 518 },
        { "NIHF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 518 },
        { "NIHH", mach_format::RI_a, { reg_4_U, imm_16_U }, 518 },
        { "NIHL", mach_format::RI_a, { reg_4_U, imm_16_U }, 518 },
        { "NILF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 519 },
        { "NILH", mach_format::RI_a, { reg_4_U, imm_16_U }, 519 },
        { "NILL", mach_format::RI_a, { reg_4_U, imm_16_U }, 519 },
        { "BALR", mach_format::RR, { reg_4_U, reg_4_U }, 519 },
        { "BAL", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 519 },
        { "BASR", mach_format::RR, { reg_4_U, reg_4_U }, 520 },
        { "BAS", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 520 },
        { "BASSM", mach_format::RX_a, { reg_4_U, reg_4_U }, 520 },
        { "BSM", mach_format::RR, { reg_4_U, reg_4_U }, 522 },
        { "BIC", mach_format::RXY_b, { mask_4_U, dxb_20_4x4_S }, 523 },
        { "BCR", mach_format::RR, { mask_4_U, reg_4_U }, 524 },
        { "BC", mach_format::RX_b, { mask_4_U, dxb_12_4x4_U }, 524 },
        { "BCTR", mach_format::RR, { reg_4_U, reg_4_U }, 525 },
        { "BCTGR", mach_format::RRE, { reg_4_U, reg_4_U }, 525 },
        { "BCT", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 525 },
        { "BCTG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 525 },
        { "BXH", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 526 },
        { "BXHG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 526 },
        { "BXLE", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 526 },
        { "BXLEG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 526 },
        { "BPP", mach_format::SMI, { mask_4_U, rel_addr_imm_16_S, db_12_4_U }, 527 },
        { "BPRP", mach_format::MII, { mask_4_U, rel_addr_imm_12_S, rel_addr_imm_24_S }, 527 },
        { "BRAS", mach_format::RI_b, { reg_4_U, rel_addr_imm_16_S }, 530 },
        { "BRASL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 530 },
        { "BRC", mach_format::RI_c, { mask_4_U, rel_addr_imm_16_S }, 530 },
        { "BRCL", mach_format::RIL_c, { mask_4_U, rel_addr_imm_32_S }, 530 },
        { "BRCT", mach_format::RI_b, { reg_4_U, rel_addr_imm_16_S }, 531 },
        { "BRCTG", mach_format::RI_b, { reg_4_U, rel_addr_imm_16_S }, 531 },
        { "BRCTH", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 531 },
        { "BRXH", mach_format::RSI, { reg_4_U, reg_4_U, rel_addr_imm_16_S }, 532 },
        { "BRXHG", mach_format::RIE_e, { reg_4_U, reg_4_U, rel_addr_imm_16_S }, 532 },
        { "BRXLE", mach_format::RSI, { reg_4_U, reg_4_U, rel_addr_imm_16_S }, 532 },
        { "BRXLG", mach_format::RIE_e, { reg_4_U, reg_4_U, rel_addr_imm_16_S }, 532 },
        { "CKSM", mach_format::RRE, { reg_4_U, reg_4_U }, 533 },
        { "KM", mach_format::RRE, { reg_4_U, reg_4_U }, 537 },
        { "KMC", mach_format::RRE, { reg_4_U, reg_4_U }, 537 },
        { "KMA", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U }, 562 },
        { "KMF", mach_format::RRE, { reg_4_U, reg_4_U }, 576 },
        { "KMCTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U }, 591 },
        { "KMO", mach_format::RRE, { reg_4_U, reg_4_U }, 604 },
        { "CR", mach_format::RR, { reg_4_U, reg_4_U }, 618 },
        { "CGR", mach_format::RRE, { reg_4_U, reg_4_U }, 618 },
        { "CGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 618 },
        { "C", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 618 },
        { "CY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 618 },
        { "CG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 618 },
        { "CGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 618 },
        { "CFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 618 },
        { "CGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 619 },
        { "CRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 619 },
        { "CGRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 619 },
        { "CGFRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 619 },
        { "CRB", mach_format::RRS, { reg_4_U, reg_4_U, mask_4_U, db_12_4_U }, 619 },
        { "CGRB", mach_format::RRS, { reg_4_U, reg_4_U, mask_4_U, db_12_4_U }, 619 },
        { "CRJ", mach_format::RIE_b, { reg_4_U, reg_4_U, mask_4_U, rel_addr_imm_16_S }, 619 },
        { "CGRJ", mach_format::RIE_b, { reg_4_U, reg_4_U, mask_4_U, rel_addr_imm_16_S }, 620 },
        { "CIB", mach_format::RIS, { reg_4_U, imm_8_S, mask_4_U, db_12_4_U }, 620 },
        { "CGIB", mach_format::RIS, { reg_4_U, imm_8_S, mask_4_U, db_12_4_U }, 620 },
        { "CIJ", mach_format::RIE_c, { reg_4_U, imm_8_S, mask_4_U, rel_addr_imm_16_S }, 620 },
        { "CGIJ", mach_format::RIE_c, { reg_4_U, imm_8_S, mask_4_U, rel_addr_imm_16_S }, 620 },
        { "CFC", mach_format::S, { db_12_4_U }, 621 },
        { "CS", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 628 },
        { "CSY", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 628 },
        { "CSG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 628 },
        { "CDS", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 628 },
        { "CDSY", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 628 },
        { "CDSG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 628 },
        { "CSST", mach_format::SSF, { db_12_4_U, db_12_4_U, reg_4_U }, 630 },
        { "CRT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 633 },
        { "CGRT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 633 },
        { "CIT", mach_format::RIE_a, { reg_4_U, imm_16_S, mask_4_U }, 633 },
        { "CGIT", mach_format::RIE_a, { reg_4_U, imm_16_S, mask_4_U }, 633 },
        { "CH", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 634 },
        { "CHY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 634 },
        { "CGH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 634 },
        { "CHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 634 },
        { "CGHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 634 },
        { "CHHSI", mach_format::SIL, { db_12_4_U, imm_16_S }, 634 },
        { "CHSI", mach_format::SIL, { db_12_4_U, imm_16_S }, 634 },
        { "CGHSI", mach_format::SIL, { db_12_4_U, imm_16_S }, 634 },
        { "CHRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 634 },
        { "CGHRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 634 },
        { "CHHR", mach_format::RRE, { reg_4_U, reg_4_U }, 635 },
        { "CHLR", mach_format::RRE, { reg_4_U, reg_4_U }, 635 },
        { "CHF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 635 },
        { "CIH", mach_format::RIL_a, { reg_4_U, imm_32_S }, 635 },
        { "CLR", mach_format::RR, { reg_4_U, reg_4_U }, 636 },
        { "CLGR", mach_format::RRE, { reg_4_U, reg_4_U }, 636 },
        { "CLGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 636 },
        { "CL", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 636 },
        { "CLY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 636 },
        { "CLG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 636 },
        { "CLGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 636 },
        { "CLC", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 636 },
        { "CLFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 636 },
        { "CLGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 636 },
        { "CLI", mach_format::SI, { db_12_4_U, imm_8_U }, 636 },
        { "CLIY", mach_format::SIY, { db_12_4_U, imm_8_U }, 636 },
        { "CLFHSI", mach_format::SIL, { db_12_4_U, imm_16_U }, 636 },
        { "CLGHSI", mach_format::SIL, { db_12_4_U, imm_16_U }, 636 },
        { "CLHHSI", mach_format::SIL, { db_12_4_U, imm_16_U }, 636 },
        { "CLRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 637 },
        { "CLGRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 637 },
        { "CLGFRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 637 },
        { "CLHRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 637 },
        { "CLGHRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 637 },
        { "CLRB", mach_format::RRS, { reg_4_U, reg_4_U, mask_4_U, db_12_4_U }, 638 },
        { "CLGRB", mach_format::RRS, { reg_4_U, reg_4_U, mask_4_U, db_12_4_U }, 638 },
        { "CLRJ", mach_format::RIE_b, { reg_4_U, reg_4_U, mask_4_U, rel_addr_imm_16_S }, 638 },
        { "CLGRJ", mach_format::RIE_b, { reg_4_U, reg_4_U, mask_4_U, rel_addr_imm_16_S }, 638 },
        { "CLIB", mach_format::RIS, { reg_4_U, imm_8_S, mask_4_U, db_12_4_U }, 638 },
        { "CLGIB", mach_format::RIS, { reg_4_U, imm_8_S, mask_4_U, db_12_4_U }, 638 },
        { "CLIJ", mach_format::RIE_c, { reg_4_U, imm_8_S, mask_4_U, rel_addr_imm_16_S }, 638 },
        { "CLGIJ", mach_format::RIE_c, { reg_4_U, imm_8_S, mask_4_U, rel_addr_imm_16_S }, 638 },
        { "CLRT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 639 },
        { "CLGRT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 639 },
        { "CLT", mach_format::RSY_b, { reg_4_U, mask_4_U, dxb_20_4x4_S }, 639 },
        { "CLGT", mach_format::RSY_b, { reg_4_U, mask_4_U, dxb_20_4x4_S }, 639 },
        { "CLFIT", mach_format::RIE_a, { reg_4_U, imm_16_S, mask_4_U }, 640 },
        { "CLGIT", mach_format::RIE_a, { reg_4_U, imm_16_S, mask_4_U }, 640 },
        { "CLM", mach_format::RS_b, { reg_4_U, mask_4_U, db_12_4_U }, 641 },
        { "CLMY", mach_format::RSY_b, { reg_4_U, mask_4_U, db_20_4_S }, 641 },
        { "CLMH", mach_format::RSY_b, { reg_4_U, mask_4_U, db_20_4_S }, 641 },
        { "CLHHR", mach_format::RRE, { reg_4_U, reg_4_U }, 641 },
        { "CLHLR", mach_format::RRE, { reg_4_U, reg_4_U }, 641 },
        { "CLHF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 641 },
        { "CLCL", mach_format::RR, { reg_4_U, reg_4_U }, 642 },
        { "CLIH", mach_format::RIL_a, { reg_4_U, imm_32_S }, 642 },
        { "CLCLE", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 644 },
        { "CLCLU", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 647 },
        { "CLST", mach_format::RRE, { reg_4_U, reg_4_U }, 650 },
        { "CUSE", mach_format::RRE, { reg_4_U, reg_4_U }, 651 },
        { "CMPSC", mach_format::RRE, { reg_4_U, reg_4_U }, 654 },
        { "KIMD", mach_format::RRE, { reg_4_U, reg_4_U }, 672 },
        { "KLMD", mach_format::RRE, { reg_4_U, reg_4_U }, 685 },
        { "KMAC", mach_format::RRE, { reg_4_U, reg_4_U }, 703 },
        { "CVB", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 714 },
        { "CVBY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 714 },
        { "CVBG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 714 },
        { "CVD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 715 },
        { "CVDY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 715 },
        { "CVDG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 715 },
        { "CU24", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 715 },
        { "CUUTF", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 718 },
        { "CU21", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 718 },
        { "CU42", mach_format::RRE, { reg_4_U, reg_4_U }, 722 },
        { "CU41", mach_format::RRE, { reg_4_U, reg_4_U }, 725 },
        { "CUTFU", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 728 },
        { "CU12", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 728 },
        { "CU14", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 732 },
        { "CPYA", mach_format::RRE, { reg_4_U, reg_4_U }, 736 },
        { "DR", mach_format::RR, { reg_4_U, reg_4_U }, 736 },
        { "D", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 736 },
        { "DLR", mach_format::RRE, { reg_4_U, reg_4_U }, 737 },
        { "DLGR", mach_format::RRE, { reg_4_U, reg_4_U }, 737 },
        { "DL", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 737 },
        { "DLG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 737 },
        { "DSGR", mach_format::RRE, { reg_4_U, reg_4_U }, 738 },
        { "DSGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 738 },
        { "DSG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 738 },
        { "DSGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 738 },
        { "HIO", mach_format::S, { db_12_4_U }, 129 },
        { "HDV", mach_format::S, { db_12_4_U }, 129 },
        { "SIO", mach_format::S, { db_12_4_U }, 129 },
        { "SIOF", mach_format::S, { db_12_4_U }, 129 },
        { "STIDC", mach_format::S, { db_12_4_U }, 129 },
        { "CLRCH", mach_format::S, { db_12_4_U }, 367 },
        { "CLRIO", mach_format::S, { db_12_4_U }, 368 },
        { "TCH", mach_format::S, { db_12_4_U }, 384 },
        { "TIO", mach_format::S, { db_12_4_U }, 385 },
        { "RRB", mach_format::S, { db_12_4_U }, 295 },
        { "CONCS", mach_format::S, { db_12_4_U }, 263 },
        { "DISCS", mach_format::S, { db_12_4_U }, 265 },
        { "XR", mach_format::RR, { reg_4_U, reg_4_U }, 738 },
        { "XGR", mach_format::RRE, { reg_4_U, reg_4_U }, 738 },
        { "XRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 738 },
        { "XGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 738 },
        { "X", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 738 },
        { "XY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 738 },
        { "XG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 738 },
        { "XI", mach_format::SI, { db_12_4_U, imm_8_U }, 739 },
        { "XIY", mach_format::SIY, { db_20_4_S, imm_8_U }, 739 },
        { "XC", mach_format::SS_a, { db_12_8x4L_U, db_20_4_S }, 739 },
        { "EX", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 740 },
        { "XIHF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 740 },
        { "XILF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 740 },
        { "EXRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 740 },
        { "EAR", mach_format::RRE, { reg_4_U, reg_4_U }, 741 },
        { "ECAG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 741 },
        { "ECTG", mach_format::SSF, { db_12_4_U, db_12_4_U, reg_4_U }, 744 },
        { "EPSW", mach_format::RRE, { reg_4_U, reg_4_U }, 745 },
        { "ETND", mach_format::RRE, { reg_4_U }, 745 },
        { "FLOGR", mach_format::RRE, { reg_4_U, reg_4_U }, 746 },
        { "IC", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 746 },
        { "ICY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 746 },
        { "ICM", mach_format::RS_b, { reg_4_U, mask_4_U, db_12_4_U }, 746 },
        { "ICMY", mach_format::RSY_b, { reg_4_U, mask_4_U, db_20_4_S }, 746 },
        { "ICMH", mach_format::RSY_b, { reg_4_U, mask_4_U, db_20_4_S }, 746 },
        { "IIHF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 747 },
        { "IIHH", mach_format::RI_a, { reg_4_U, imm_16_U }, 747 },
        { "IIHL", mach_format::RI_a, { reg_4_U, imm_16_U }, 747 },
        { "IILF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 747 },
        { "IILH", mach_format::RI_a, { reg_4_U, imm_16_U }, 747 },
        { "IILL", mach_format::RI_a, { reg_4_U, imm_16_U }, 747 },
        { "IPM", mach_format::RRE, { reg_4_U }, 748 },
        { "LR", mach_format::RR, { reg_4_U, reg_4_U }, 748 },
        { "LGR", mach_format::RRE, { reg_4_U, reg_4_U }, 748 },
        { "LGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 748 },
        { "L", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 748 },
        { "LY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 748 },
        { "LG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 748 },
        { "LGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 748 },
        { "LGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 748 },
        { "LRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 748 },
        { "LGRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 748 },
        { "LGFRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 748 },
        { "LAM", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 749 },
        { "LAMY", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 749 },
        { "LA", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 750 },
        { "LAY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 750 },
        { "LAE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 750 },
        { "LAEY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 750 },
        { "LARL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 751 },
        { "LAA", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 752 },
        { "LAAG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 752 },
        { "LAAL", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 752 },
        { "LAALG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 752 },
        { "LAN", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 753 },
        { "LANG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 753 },
        { "LAX", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 753 },
        { "LAXG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 753 },
        { "LAO", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 754 },
        { "LAOG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 754 },
        { "LTR", mach_format::RR, { reg_4_U, reg_4_U }, 754 },
        { "LTGR", mach_format::RRE, { reg_4_U, reg_4_U }, 754 },
        { "LTGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 754 },
        { "LT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 755 },
        { "LTG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 755 },
        { "LTGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 755 },
        { "LAT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 755 },
        { "LGAT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 755 },
        { "LZRF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 755 },
        { "LZRG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 755 },
        { "LBR", mach_format::RRE, { reg_4_U, reg_4_U }, 756 },
        { "LGBR", mach_format::RRE, { reg_4_U, reg_4_U }, 756 },
        { "LB", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 756 },
        { "LGB", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 756 },
        { "LBH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 756 },
        { "LCR", mach_format::RR, { reg_4_U, reg_4_U }, 756 },
        { "LCGR", mach_format::RRE, { reg_4_U, reg_4_U }, 757 },
        { "LCGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 757 },
        { "LCBB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U, mask_4_U }, 757 },
        { "LGG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 758 },
        { "LLGFSG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 758 },
        { "LGSC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 759 },
        { "LHR", mach_format::RRE, { reg_4_U, reg_4_U }, 760 },
        { "LGHR", mach_format::RRE, { reg_4_U, reg_4_U }, 760 },
        { "LH", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 760 },
        { "LHY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 760 },
        { "LGH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 760 },
        { "LHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 760 },
        { "LGHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 760 },
        { "LHRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 760 },
        { "LGHRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 760 },
        { "LHH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 761 },
        { "LOCHI", mach_format::RIE_g, { reg_4_U, imm_16_S, mask_4_U }, 761 },
        { "LOCGHI", mach_format::RIE_g, { reg_4_U, imm_16_S, mask_4_U }, 761 },
        { "LOCHHI", mach_format::RIE_g, { reg_4_U, imm_16_S, mask_4_U }, 761 },
        { "LFH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 762 },
        { "LFHAT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 762 },
        { "LLGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 762 },
        { "LLGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 762 },
        { "LLGFRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 762 },
        { "LLGFAT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 763 },
        { "LLCR", mach_format::RRE, { reg_4_U, reg_4_U }, 763 },
        { "LLGCR", mach_format::RRE, { reg_4_U, reg_4_U }, 763 },
        { "LLC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 763 },
        { "LLGC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 763 },
        { "LLZRGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 763 },
        { "LLCH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 764 },
        { "LLHR", mach_format::RRE, { reg_4_U, reg_4_U }, 764 },
        { "LLGHR", mach_format::RRE, { reg_4_U, reg_4_U }, 764 },
        { "LLH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 764 },
        { "LLGH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 764 },
        { "LLHRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 764 },
        { "LLGHRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 764 },
        { "LLHH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 765 },
        { "LLIHF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 765 },
        { "LLIHH", mach_format::RI_a, { reg_4_U, imm_16_U }, 765 },
        { "LLIHL", mach_format::RI_a, { reg_4_U, imm_16_U }, 765 },
        { "LLILF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 765 },
        { "LLILH", mach_format::RI_a, { reg_4_U, imm_16_U }, 765 },
        { "LLILL", mach_format::RI_a, { reg_4_U, imm_16_U }, 765 },
        { "LLGTR", mach_format::RRE, { reg_4_U, reg_4_U }, 765 },
        { "LLGT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 766 },
        { "LLGTAT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 766 },
        { "LM", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 766 },
        { "LMY", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 766 },
        { "LMG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 766 },
        { "LMD", mach_format::SS_e, { reg_4_U, reg_4_U, db_12_4_U, db_12_4_U }, 767 },
        { "LMH", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 767 },
        { "LNR", mach_format::RR, { reg_4_U, reg_4_U }, 767 },
        { "LNGR", mach_format::RRE, { reg_4_U, reg_4_U }, 767 },
        { "LNGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 768 },
        { "LOCFHR", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 768 },
        { "LOCFH", mach_format::RSY_b, { reg_4_U, db_20_4_S, mask_4_U }, 768 },
        { "LOCR", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 768 },
        { "LOCGR", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 768 },
        { "LOC", mach_format::RSY_b, { reg_4_U, db_20_4_S, mask_4_U }, 768 },
        { "LOCG", mach_format::RSY_b, { reg_4_U, db_20_4_S, mask_4_U }, 768 },
        { "LPD", mach_format::SSF, { reg_4_U, db_12_4_U, db_12_4_U }, 769 },
        { "LPDG", mach_format::SSF, { reg_4_U, db_12_4_U, db_12_4_U }, 769 },
        { "LPQ", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 770 },
        { "LPR", mach_format::RR, { reg_4_U, reg_4_U }, 771 },
        { "LPGR", mach_format::RRE, { reg_4_U, reg_4_U }, 771 },
        { "LPGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 771 },
        { "LRVR", mach_format::RRE, { reg_4_U, reg_4_U }, 771 },
        { "LRVGR", mach_format::RRE, { reg_4_U, reg_4_U }, 771 },
        { "LRVH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 771 },
        { "LRV", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 771 },
        { "LRVG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 771 },
        { "MC", mach_format::SI, { db_12_4_U, imm_8_S }, 772 },
        { "MVC", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 773 },
        { "MVCRL", mach_format::SSE, { db_12_4_U, db_12_4_U }, 788 },
        { "MVHHI", mach_format::SIL, { db_12_4_U, imm_16_S }, 773 },
        { "MVHI", mach_format::SIL, { db_12_4_U, imm_16_S }, 773 },
        { "MVGHI", mach_format::SIL, { db_12_4_U, imm_16_S }, 773 },
        { "MVI", mach_format::SI, { db_12_4_U, imm_8_U }, 773 },
        { "MVIY", mach_format::SIY, { db_12_4_U, imm_8_U }, 773 },
        { "MVCIN", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 774 },
        { "MVCL", mach_format::RR, { reg_4_U, reg_4_U }, 774 },
        { "MVCLE", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 778 },
        { "MVCLU", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 781 },
        { "MVN", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 785 },
        { "MVST", mach_format::RRE, { reg_4_U, reg_4_U }, 785 },
        { "MVO", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 786 },
        { "MVZ", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 787 },
        { "MR", mach_format::RR, { reg_4_U, reg_4_U }, 788 },
        { "MGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 788 },
        { "M", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 788 },
        { "MFY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 788 },
        { "MG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 788 },
        { "MH", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 789 },
        { "MHY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 789 },
        { "MGH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 789 },
        { "MHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 789 },
        { "MGHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 789 },
        { "MLR", mach_format::RRE, { reg_4_U, reg_4_U }, 790 },
        { "MLGR", mach_format::RRE, { reg_4_U, reg_4_U }, 790 },
        { "ML", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 790 },
        { "MLG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 790 },
        { "MSR", mach_format::RRE, { reg_4_U, reg_4_U }, 791 },
        { "MSRKC", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 791 },
        { "MSGR", mach_format::RRE, { reg_4_U, reg_4_U }, 791 },
        { "MSGRKC", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 791 },
        { "MSGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 791 },
        { "MS", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 791 },
        { "MSC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 791 },
        { "MSY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 791 },
        { "MSG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 791 },
        { "MSGC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 791 },
        { "MSGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 791 },
        { "MSFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 791 },
        { "MSGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 791 },
        { "NIAI", mach_format::IE, { imm_4_U, imm_4_U }, 792 },
        { "NTSTG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 794 },
        { "NCGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 522 },
        { "NCRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 522 },
        { "NNRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 796 },
        { "NNGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 796 },
        { "NOGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 799 },
        { "NORK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 799 },
        { "NXRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 799 },
        { "NXGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 799 },
        { "OR", mach_format::RR, { reg_4_U, reg_4_U }, 794 },
        { "OGR", mach_format::RRE, { reg_4_U, reg_4_U }, 794 },
        { "ORK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 794 },
        { "OCGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 802 },
        { "OCRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 802 },
        { "OGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 794 },
        { "O", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 794 },
        { "OY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 794 },
        { "OG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 795 },
        { "OI", mach_format::SI, { db_12_4_U, imm_8_U }, 795 },
        { "OIY", mach_format::SIY, { db_20_4_S, imm_8_U }, 795 },
        { "OC", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 795 },
        { "OIHF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 796 },
        { "OIHH", mach_format::RI_a, { reg_4_U, imm_16_U }, 796 },
        { "OIHL", mach_format::RI_a, { reg_4_U, imm_16_U }, 796 },
        { "OILF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 796 },
        { "OILH", mach_format::RI_a, { reg_4_U, imm_16_U }, 796 },
        { "OILL", mach_format::RI_a, { reg_4_U, imm_16_U }, 796 },
        { "PACK", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 796 },
        { "PKA", mach_format::SS_f, { db_12_4_U, db_12_8x4L_U }, 797 },
        { "PKU", mach_format::SS_f, { db_12_4_U, db_12_8x4L_U }, 798 },
        { "PCC", mach_format::RRE, {}, 799 },
        { "PLO", mach_format::SS_e, { reg_4_U, db_12_4_U, reg_4_U, db_12_4_U }, 815 },
        { "PPA", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 829 },
        { "PRNO", mach_format::RRE, { reg_4_U, reg_4_U }, 830 },
        { "PPNO", mach_format::RRE, { reg_4_U, reg_4_U }, 830 },
        { "POPCNT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 853 },
        { "PFD", mach_format::RXY_b, { mask_4_U, dxb_20_4x4_S }, 843 },
        { "PFDRL", mach_format::RIL_c, { mask_4_U, rel_addr_imm_32_S }, 843 },
        { "RLL", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 845 },
        { "RLLG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 845 },
        { "RNSBG", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 845 },
        { "RXSBG", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 846 },
        { "ROSBG", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 846 },
        { "RISBG", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 847 },
        { "RISBGN", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 847 },
        { "RISBHG", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 848 },
        { "RISBLG", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 849 },
        { "RNSBGT", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 845 },
        { "RXSBGT", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 846 },
        { "ROSBGT", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 858 },
        { "RISBGZ", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 858 },
        { "RISBGNZ", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 860 },
        { "RISBHGZ", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 860 },
        { "RISBLGZ", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 860 },
        { "SRST", mach_format::RRE, { reg_4_U, reg_4_U }, 850 },
        { "SRSTU", mach_format::RRE, { reg_4_U, reg_4_U }, 852 },
        { "SAR", mach_format::RRE, { reg_4_U, reg_4_U }, 854 },
        { "SAM24", mach_format::E, {}, 854 },
        { "SAM31", mach_format::E, {}, 854 },
        { "SAM64", mach_format::E, {}, 854 },
        { "SPM", mach_format::RR, { reg_4_U }, 855 },
        { "SLDA", mach_format::RS_a, { reg_4_U, db_12_4_U }, 855 },
        { "SLA", mach_format::RS_a, { reg_4_U, db_12_4_U }, 856 },
        { "SLAK", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 856 },
        { "SLAG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 856 },
        { "SLDL", mach_format::RS_a, { reg_4_U, db_12_4_U }, 856 },
        { "SLL", mach_format::RS_a, { reg_4_U, db_12_4_U }, 857 },
        { "SLLK", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 857 },
        { "SLLG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 857 },
        { "SRDA", mach_format::RS_a, { reg_4_U, db_12_4_U }, 858 },
        { "SRDL", mach_format::RS_a, { reg_4_U, db_12_4_U }, 858 },
        { "SRA", mach_format::RS_a, { reg_4_U, db_12_4_U }, 859 },
        { "SRAK", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 859 },
        { "SRAG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 859 },
        { "SRL", mach_format::RS_a, { reg_4_U, db_12_4_U }, 860 },
        { "SRLK", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 860 },
        { "SRLG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 860 },
        { "ST", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 860 },
        { "STY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 861 },
        { "STG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 861 },
        { "STRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 861 },
        { "STGRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 861 },
        { "STAM", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 861 },
        { "STAMY", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 861 },
        { "STC", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 862 },
        { "STCY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 862 },
        { "STCH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 862 },
        { "STCM", mach_format::RS_b, { reg_4_U, mask_4_U, db_12_4_U }, 862 },
        { "STCMY", mach_format::RSY_b, { reg_4_U, mask_4_U, db_20_4_S }, 862 },
        { "STCMH", mach_format::RSY_b, { reg_4_U, mask_4_U, db_20_4_S }, 862 },
        { "STCK", mach_format::S, { db_12_4_U }, 863 },
        { "STCKF", mach_format::S, { db_12_4_U }, 863 },
        { "STCKE", mach_format::S, { db_12_4_U }, 864 },
        { "STFLE", mach_format::S, { db_20_4_S }, 866 },
        { "STGSC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 867 },
        { "STH", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 867 },
        { "STHY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 868 },
        { "STHRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 868 },
        { "STHH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 868 },
        { "STFH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 868 },
        { "STM", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 869 },
        { "STMY", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 869 },
        { "STMG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 869 },
        { "STMH", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 869 },
        { "STOC", mach_format::RSY_b, { reg_4_U, db_20_4_S, mask_4_U }, 869 },
        { "STOCG", mach_format::RSY_b, { reg_4_U, db_20_4_S, mask_4_U }, 869 },
        { "STOCFH", mach_format::RSY_b, { reg_4_U, db_20_4_S, mask_4_U }, 870 },
        { "STPQ", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 870 },
        { "STRVH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 871 },
        { "STRV", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 871 },
        { "STRVG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 871 },
        { "SR", mach_format::RR, { reg_4_U, reg_4_U }, 871 },
        { "SGR", mach_format::RRE, { reg_4_U, reg_4_U }, 871 },
        { "SGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 871 },
        { "SRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 871 },
        { "SGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 872 },
        { "S", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 872 },
        { "SY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 872 },
        { "SG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 872 },
        { "SGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 872 },
        { "SH", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 872 },
        { "SHY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 872 },
        { "SGH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 872 },
        { "SHHHR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 873 },
        { "SHHLR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 873 },
        { "SLR", mach_format::RR, { reg_4_U, reg_4_U }, 873 },
        { "SLGR", mach_format::RRE, { reg_4_U, reg_4_U }, 873 },
        { "SLGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 873 },
        { "SLRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 873 },
        { "SLGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 873 },
        { "SL", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 874 },
        { "SLY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 874 },
        { "SLG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 874 },
        { "SLGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 874 },
        { "SLFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 874 },
        { "SLGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 874 },
        { "SLHHHR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 875 },
        { "SLHHLR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 875 },
        { "SLBR", mach_format::RRE, { reg_4_U, reg_4_U }, 875 },
        { "SLBGR", mach_format::RRE, { reg_4_U, reg_4_U }, 875 },
        { "SLB", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 875 },
        { "SLBG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 875 },
        { "SVC", mach_format::I, { imm_8_U }, 876 },
        { "TS", mach_format::SI, { db_12_4_U }, 876 },
        { "TAM", mach_format::E, {}, 876 },
        { "TM", mach_format::SI, { db_12_4_U, imm_8_U }, 877 },
        { "TMY", mach_format::SIY, { db_20_4_S, imm_8_U }, 877 },
        { "TMHH", mach_format::RI_a, { reg_4_U, imm_16_U }, 877 },
        { "TMHL", mach_format::RI_a, { reg_4_U, imm_16_U }, 877 },
        { "TMH", mach_format::RI_a, { reg_4_U, imm_16_U }, 877 },
        { "TMLH", mach_format::RI_a, { reg_4_U, imm_16_U }, 877 },
        { "TML", mach_format::RI_a, { reg_4_U, imm_16_U }, 877 },
        { "TMLL", mach_format::RI_a, { reg_4_U, imm_16_U }, 877 },
        { "TABORT", mach_format::S, { db_12_4_U }, 878 },
        { "TBEGIN", mach_format::SIL, { db_12_4_U, imm_16_S }, 879 },
        { "TBEGINC", mach_format::SIL, { db_12_4_U, imm_16_S }, 883 },
        { "TEND", mach_format::S, {}, 885 },
        { "TR", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 886 },
        { "TRT", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 887 },
        { "TRTE", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 887 },
        { "TRTRE", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 888 },
        { "TRTR", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 892 },
        { "TRE", mach_format::RRE, { reg_4_U, reg_4_U }, 893 },
        { "TROO", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 895 },
        { "TROT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 895 },
        { "TRTO", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 895 },
        { "TRTT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 895 },
        { "UNPK", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 900 },
        { "UNPKA", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 901 },
        { "UNPKU", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 902 },
        { "UPT", mach_format::E, {}, 903 },
        { "AP", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 920 },
        { "CP", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 921 },
        { "DP", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 921 },
        { "DFLTCC", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 1714 },
        { "ED", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 922 },
        { "EDMK", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 925 },
        { "SRP", mach_format::SS_c, { db_12_4x4L_U, db_12_4_U, imm_4_U }, 926 },
        { "MP", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 926 },
        { "SP", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 927 },
        { "TP", mach_format::RSL_a, { db_12_4x4L_U }, 928 },
        { "ZAP", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 928 },
        { "THDR", mach_format::RRE, { reg_4_U, reg_4_U }, 955 },
        { "THDER", mach_format::RRE, { reg_4_U, reg_4_U }, 955 },
        { "TBEDR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 956 },
        { "TBDR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 956 },
        { "CPSDR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U }, 958 },
        { "EFPC", mach_format::RRE, { reg_4_U }, 958 },
        { "LER", mach_format::RR, { reg_4_U, reg_4_U }, 959 },
        { "LDR", mach_format::RR, { reg_4_U, reg_4_U }, 959 },
        { "LXR", mach_format::RRE, { reg_4_U, reg_4_U }, 959 },
        { "LE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 959 },
        { "LD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 959 },
        { "LEY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 959 },
        { "LDY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 959 },
        { "LCDFR", mach_format::RRE, { reg_4_U, reg_4_U }, 959 },
        { "LFPC", mach_format::S, { db_12_4_U }, 959 },
        { "LFAS", mach_format::S, { db_12_4_U }, 960 },
        { "LDGR", mach_format::RRE, { reg_4_U, reg_4_U }, 962 },
        { "LGDR", mach_format::RRE, { reg_4_U, reg_4_U }, 962 },
        { "LNDFR", mach_format::RRE, { reg_4_U, reg_4_U }, 962 },
        { "LPDFR", mach_format::RRE, { reg_4_U, reg_4_U }, 962 },
        { "LZER", mach_format::RRE, { reg_4_U }, 963 },
        { "LZXR", mach_format::RRE, { reg_4_U }, 963 },
        { "LZDR", mach_format::RRE, { reg_4_U }, 963 },
        { "PFPO", mach_format::E, {}, 963 },
        { "SRNM", mach_format::S, { db_12_4_U }, 975 },
        { "SRNMB", mach_format::S, { db_12_4_U }, 975 },
        { "SRNMT", mach_format::S, { db_12_4_U }, 975 },
        { "SFPC", mach_format::RRE, { reg_4_U }, 975 },
        { "SFASR", mach_format::RRE, { reg_4_U }, 976 },
        { "STE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 976 },
        { "STD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 976 },
        { "STDY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 977 },
        { "STEY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 977 },
        { "STFPC", mach_format::S, { db_12_4_U }, 977 },
        { "BSA", mach_format::RRE, { reg_4_U, reg_4_U }, 989 },
        { "BAKR", mach_format::RRE, { reg_4_U, reg_4_U }, 993 },
        { "BSG", mach_format::RRE, { reg_4_U, reg_4_U }, 995 },
        { "CRDTE", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U_opt }, 999 },
        { "CSP", mach_format::RRE, { reg_4_U, reg_4_U }, 1003 },
        { "CSPG", mach_format::RRE, { reg_4_U, reg_4_U }, 1003 },
        { "ESEA", mach_format::RRE, { reg_4_U }, 1006 },
        { "EPAR", mach_format::RRE, { reg_4_U }, 1006 },
        { "EPAIR", mach_format::RRE, { reg_4_U }, 1006 },
        { "ESAR", mach_format::RRE, { reg_4_U }, 1006 },
        { "ESAIR", mach_format::RRE, { reg_4_U }, 1007 },
        { "EREG", mach_format::RRE, { reg_4_U, reg_4_U }, 1007 },
        { "EREGG", mach_format::RRE, { reg_4_U, reg_4_U }, 1007 },
        { "ESTA", mach_format::RRE, { reg_4_U, reg_4_U }, 1008 },
        { "IAC", mach_format::RRE, { reg_4_U }, 1011 },
        { "IPK", mach_format::S, {}, 1012 },
        { "IRBM", mach_format::RRE, { reg_4_U, reg_4_U }, 1012 },
        { "ISK", mach_format::RR, { reg_4_U, reg_4_U }, 268 },
        { "ISKE", mach_format::RRE, { reg_4_U, reg_4_U }, 1012 },
        { "IVSK", mach_format::RRE, { reg_4_U, reg_4_U }, 1013 },
        { "IDTE", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U_opt }, 1014 },
        { "IPTE", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U_opt, mask_4_U_opt }, 1019 },
        { "LASP", mach_format::SSE, { db_12_4_U, db_12_4_U }, 1023 },
        { "LCTL", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 1032 },
        { "LCTLG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 1032 },
        { "LPTEA", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1032 },
        { "LPSW", mach_format::SI, { db_12_4_U }, 1036 },
        { "LPSWE", mach_format::S, { db_12_4_U }, 1037 },
        { "LRA", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1038 },
        { "LRAY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 1038 },
        { "LRAG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 1038 },
        { "LURA", mach_format::RRE, { reg_4_U, reg_4_U }, 1042 },
        { "LURAG", mach_format::RRE, { reg_4_U, reg_4_U }, 1042 },
        { "MSTA", mach_format::RRE, { reg_4_U }, 1043 },
        { "MVPG", mach_format::RRE, { reg_4_U, reg_4_U }, 1044 },
        { "MVCP", mach_format::SS_d, { drb_12_4x4_U, db_12_4_U, reg_4_U }, 1046 },
        { "MVCS", mach_format::SS_d, { drb_12_4x4_U, db_12_4_U, reg_4_U }, 1046 },
        { "MVCDK", mach_format::SSE, { db_12_4_U, db_12_4_U }, 1048 },
        { "MVCK", mach_format::SS_d, { drb_12_4x4_U, db_12_4_U, reg_4_U }, 1049 },
        { "MVCOS", mach_format::SSF, { db_12_4_U, db_12_4_U, reg_4_U }, 1050 },
        { "MVCSK", mach_format::SSE, { db_12_4_U, db_12_4_U }, 1053 },
        { "PGIN", mach_format::RRE, { reg_4_U, reg_4_U }, 1054 },
        { "PGOUT", mach_format::RRE, { reg_4_U, reg_4_U }, 1055 },
        { "PCKMO", mach_format::RRE, {}, 1056 },
        { "PFMF", mach_format::RRE, { reg_4_U, reg_4_U }, 1059 },
        { "PTFF", mach_format::E, {}, 1063 },
        { "PTF", mach_format::RRE, { reg_4_U }, 1071 },
        { "PC", mach_format::S, { db_12_4_U }, 1072 },
        { "PR", mach_format::E, {}, 1085 },
        { "PTI", mach_format::RRE, { reg_4_U, reg_4_U }, 1089 },
        { "PT", mach_format::RRE, { reg_4_U, reg_4_U }, 1089 },
        { "PALB", mach_format::RRE, {}, 1098 },
        { "PTLB", mach_format::S, {}, 1098 },
        { "RRBE", mach_format::RRE, { reg_4_U, reg_4_U }, 1098 },
        { "RRBM", mach_format::RRE, { reg_4_U, reg_4_U }, 1099 },
        { "RP", mach_format::S, { db_12_4_U }, 1099 },
        { "SAC", mach_format::S, { db_12_4_U }, 1102 },
        { "SACF", mach_format::S, { db_12_4_U }, 1102 },
        { "SCK", mach_format::S, { db_12_4_U }, 1103 },
        { "SCKC", mach_format::S, { db_12_4_U }, 1104 },
        { "SCKPF", mach_format::E, {}, 1105 },
        { "SPX", mach_format::S, { db_12_4_U }, 1105 },
        { "SPT", mach_format::S, { db_12_4_U }, 1105 },
        { "SPKA", mach_format::S, { db_12_4_U }, 1106 },
        { "SSAR", mach_format::RRE, { reg_4_U }, 1107 },
        { "SSAIR", mach_format::RRE, { reg_4_U }, 1107 },
        { "SSK", mach_format::RR, { reg_4_U, reg_4_U }, 304 },
        { "SSKE", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 1112 },
        { "SSM", mach_format::SI, { db_12_4_U }, 1115 },
        { "SIGP", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 1115 },
        { "STCKC", mach_format::S, { db_12_4_U }, 1117 },
        { "STCTL", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 1117 },
        { "STCTG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 1117 },
        { "STAP", mach_format::S, { db_12_4_U }, 1118 },
        { "STIDP", mach_format::S, { db_12_4_U }, 1118 },
        { "STPT", mach_format::S, { db_12_4_U }, 1120 },
        { "STFL", mach_format::S, { db_12_4_U }, 1120 },
        { "STPX", mach_format::S, { db_12_4_U }, 1121 },
        { "STRAG", mach_format::SSE, { db_12_4_U, db_12_4_U }, 1121 },
        { "STSI", mach_format::S, { db_12_4_U }, 1122 },
        { "STOSM", mach_format::SI, { db_12_4_U, imm_8_U }, 1146 },
        { "STNSM", mach_format::SI, { db_12_4_U, imm_8_U }, 1146 },
        { "STURA", mach_format::RRE, { reg_4_U, reg_4_U }, 1147 },
        { "STURG", mach_format::RRE, { reg_4_U, reg_4_U }, 1147 },
        { "TAR", mach_format::RRE, { reg_4_U, reg_4_U }, 1147 },
        { "TB", mach_format::RRE, { reg_4_U, reg_4_U }, 1149 },
        { "TPEI", mach_format::RRE, { reg_4_U, reg_4_U }, 1151 },
        { "TPROT", mach_format::SSE, { db_12_4_U, db_12_4_U }, 1152 },
        { "TRACE", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 1155 },
        { "TRACG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 1155 },
        { "TRAP2", mach_format::E, {}, 1156 },
        { "TRAP4", mach_format::S, { db_12_4_U }, 1156 },
        { "XSCH", mach_format::S, {}, 1215 },
        { "CSCH", mach_format::S, {}, 1217 },
        { "HSCH", mach_format::S, {}, 1218 },
        { "MSCH", mach_format::S, { db_12_4_U }, 1219 },
        { "RCHP", mach_format::S, {}, 1221 },
        { "RSCH", mach_format::S, {}, 1222 },
        { "SAL", mach_format::S, {}, 1224 },
        { "SCHM", mach_format::S, {}, 1225 },
        { "SSCH", mach_format::S, { db_12_4_U }, 1227 },
        { "STCPS", mach_format::S, { db_12_4_U }, 1228 },
        { "STCRW", mach_format::S, { db_12_4_U }, 1229 },
        { "STSCH", mach_format::S, { db_12_4_U }, 1230 },
        { "TPI", mach_format::S, { db_12_4_U }, 1231 },
        { "TSCH", mach_format::S, { db_12_4_U }, 1232 },

        { "AER", mach_format::RR, { reg_4_U, reg_4_U }, 1412 },
        { "ADR", mach_format::RR, { reg_4_U, reg_4_U }, 1412 },
        { "AXR", mach_format::RR, { reg_4_U, reg_4_U }, 1412 },
        { "AE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1412 },
        { "AD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1412 },
        { "AWR", mach_format::RR, { reg_4_U, reg_4_U }, 1413 },
        { "AUR", mach_format::RR, { reg_4_U, reg_4_U }, 1413 },
        { "AU", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1413 },
        { "AW", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1413 },
        { "CER", mach_format::RR, { reg_4_U, reg_4_U }, 1414 },
        { "CDR", mach_format::RR, { reg_4_U, reg_4_U }, 1414 },
        { "CXR", mach_format::RRE, { reg_4_U, reg_4_U }, 1414 },
        { "CE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1414 },
        { "CD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1414 },
        { "CEFR", mach_format::RRE, { reg_4_U, reg_4_U }, 1415 },
        { "CDFR", mach_format::RRE, { reg_4_U, reg_4_U }, 1415 },
        { "CXFR", mach_format::RRE, { reg_4_U, reg_4_U }, 1415 },
        { "CEGR", mach_format::RRE, { reg_4_U, reg_4_U }, 1415 },
        { "CDGR", mach_format::RRE, { reg_4_U, reg_4_U }, 1415 },
        { "CXGR", mach_format::RRE, { reg_4_U, reg_4_U }, 1415 },
        { "CFER", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1415 },
        { "CFDR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1415 },
        { "CFXR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1415 },
        { "CGER", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1415 },
        { "CGDR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1415 },
        { "CGXR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1415 },
        { "DDR", mach_format::RR, { reg_4_U, reg_4_U }, 1416 },
        { "DER", mach_format::RR, { reg_4_U, reg_4_U }, 1416 },
        { "DXR", mach_format::RRE, { reg_4_U, reg_4_U }, 1416 },
        { "DD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1416 },
        { "DE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1416 },
        { "HDR", mach_format::RR, { reg_4_U, reg_4_U }, 1417 },
        { "HER", mach_format::RR, { reg_4_U, reg_4_U }, 1417 },
        { "LTER", mach_format::RR, { reg_4_U, reg_4_U }, 1417 },
        { "LTDR", mach_format::RR, { reg_4_U, reg_4_U }, 1417 },
        { "LTXR", mach_format::RRE, { reg_4_U, reg_4_U }, 1418 },
        { "LCER", mach_format::RR, { reg_4_U, reg_4_U }, 1418 },
        { "LCDR", mach_format::RR, { reg_4_U, reg_4_U }, 1418 },
        { "LCXR", mach_format::RRE, { reg_4_U, reg_4_U }, 1418 },
        { "FIER", mach_format::RRE, { reg_4_U, reg_4_U }, 1419 },
        { "FIDR", mach_format::RRE, { reg_4_U, reg_4_U }, 1419 },
        { "FIXR", mach_format::RRE, { reg_4_U, reg_4_U }, 1419 },
        { "LDER", mach_format::RRE, { reg_4_U, reg_4_U }, 1419 },
        { "LXDR", mach_format::RRE, { reg_4_U, reg_4_U }, 1419 },
        { "LXER", mach_format::RRE, { reg_4_U, reg_4_U }, 1419 },
        { "LDE", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1419 },
        { "LXD", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1419 },
        { "LXE", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1419 },
        { "LNDR", mach_format::RR, { reg_4_U, reg_4_U }, 1420 },
        { "LNER", mach_format::RR, { reg_4_U, reg_4_U }, 1420 },
        { "LPDR", mach_format::RR, { reg_4_U, reg_4_U }, 1420 },
        { "LPER", mach_format::RR, { reg_4_U, reg_4_U }, 1420 },
        { "LNXR", mach_format::RRE, { reg_4_U, reg_4_U }, 1420 },
        { "LPXR", mach_format::RRE, { reg_4_U, reg_4_U }, 1420 },
        { "LEDR", mach_format::RR, { reg_4_U, reg_4_U }, 1421 },
        { "LDXR", mach_format::RR, { reg_4_U, reg_4_U }, 1421 },
        { "LRER", mach_format::RR, { reg_4_U, reg_4_U }, 1421 },
        { "LRDR", mach_format::RR, { reg_4_U, reg_4_U }, 1421 },
        { "LEXR", mach_format::RRE, { reg_4_U, reg_4_U }, 1421 },
        { "MEER", mach_format::RRE, { reg_4_U, reg_4_U }, 1421 },
        { "MDR", mach_format::RR, { reg_4_U, reg_4_U }, 1421 },
        { "MXR", mach_format::RR, { reg_4_U, reg_4_U }, 1421 },
        { "MDER", mach_format::RR, { reg_4_U, reg_4_U }, 1421 },
        { "MER", mach_format::RR, { reg_4_U, reg_4_U }, 1421 },
        { "MXDR", mach_format::RR, { reg_4_U, reg_4_U }, 1421 },
        { "MEE", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1422 },
        { "MD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1422 },
        { "MDE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1422 },
        { "MXD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1422 },
        { "ME", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1422 },
        { "MAER", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1423 },
        { "MADR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1423 },
        { "MAD", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1423 },
        { "MAE", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1423 },
        { "MSER", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1423 },
        { "MSDR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1423 },
        { "MSE", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1423 },
        { "MSD", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1423 },
        { "MAYR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1424 },
        { "MAYHR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1424 },
        { "MAYLR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1424 },
        { "MAY", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1424 },
        { "MAYH", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1424 },
        { "MAYL", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1424 },
        { "MYR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1426 },
        { "MYHR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1426 },
        { "MYLR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1426 },
        { "MY", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1426 },
        { "MYH", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1426 },
        { "MYL", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1426 },
        { "SQER", mach_format::RRE, { reg_4_U, reg_4_U }, 1427 },
        { "SQDR", mach_format::RRE, { reg_4_U, reg_4_U }, 1427 },
        { "SQXR", mach_format::RRE, { reg_4_U, reg_4_U }, 1427 },
        { "SQE", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1427 },
        { "SQD", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1427 },
        { "SER", mach_format::RR, { reg_4_U, reg_4_U }, 1428 },
        { "SDR", mach_format::RR, { reg_4_U, reg_4_U }, 1428 },
        { "SXR", mach_format::RR, { reg_4_U, reg_4_U }, 1428 },
        { "SE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1428 },
        { "SD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1428 },
        { "SUR", mach_format::RR, { reg_4_U, reg_4_U }, 1429 },
        { "SWR", mach_format::RR, { reg_4_U, reg_4_U }, 1429 },
        { "SU", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1429 },
        { "SW", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1429 },
        { "AEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1445 },
        { "ADBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1445 },
        { "AXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1445 },
        { "AEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1445 },
        { "ADB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1445 },
        { "CEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1447 },
        { "CDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1447 },
        { "CXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1447 },
        { "CDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1447 },
        { "CEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1447 },
        { "KEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1448 },
        { "KDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1448 },
        { "KDSA", mach_format::RRE, { reg_4_U, reg_4_U }, 1700 },
        { "KXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1448 },
        { "KDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1448 },
        { "KEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1448 },
        { "CEFBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1449 },
        { "CDFBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1449 },
        { "CXFBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1449 },
        { "CEGBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1449 },
        { "CDGBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1449 },
        { "CXGBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1449 },
        { "CEFBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1449 },
        { "CDFBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1449 },
        { "CXFBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1449 },
        { "CEGBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1449 },
        { "CDGBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1449 },
        { "CXGBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1449 },
        { "CELFBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1451 },
        { "CDLFBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1451 },
        { "CXLFBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1451 },
        { "CELGBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1451 },
        { "CDLGBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1451 },
        { "CXLGBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1451 },
        { "CFEBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1452 },
        { "CFDBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1452 },
        { "CFXBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1452 },
        { "CGEBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1452 },
        { "CGDBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1452 },
        { "CGXBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1452 },
        { "CFEBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1452 },
        { "CFDBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1452 },
        { "CFXBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1452 },
        { "CGEBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1452 },
        { "CGDBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1452 },
        { "CGXBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1452 },
        { "CLFEBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1455 },
        { "CLFDBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1455 },
        { "CLFXBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1455 },
        { "CLGEBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1455 },
        { "CLGDBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1455 },
        { "CLGXBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1455 },
        { "DEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1457 },
        { "DDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1457 },
        { "DXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1457 },
        { "DEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1457 },
        { "DDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1457 },
        { "DIEBR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1458 },
        { "DIDBR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1458 },
        { "LTEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1461 },
        { "LTDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1461 },
        { "LTXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1461 },
        { "LCEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1461 },
        { "LCDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1461 },
        { "LCXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1461 },
        { "ECCTR", mach_format::RRE, { reg_4_U, reg_4_U }, 39 },
        { "EPCTR", mach_format::RRE, { reg_4_U, reg_4_U }, 39 },
        { "ECPGA", mach_format::RRE, { reg_4_U, reg_4_U }, 39 },
        { "FIEBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1462 },
        { "FIDBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1462 },
        { "FIXBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1462 },
        { "FIEBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1462 },
        { "FIDBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1462 },
        { "FIXBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1462 },
        { "LSCTL", mach_format::S, { db_12_4_U }, 42 },
        { "LDEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1463 },
        { "LXDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1463 },
        { "LPCTL", mach_format::S, { db_12_4_U }, 41 },
        { "LCCTL", mach_format::S, { db_12_4_U }, 40 },
        { "LXEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1463 },
        { "LDEB", mach_format::RRE, { reg_4_U, reg_4_U }, 1464 },
        { "LXDB", mach_format::RRE, { reg_4_U, reg_4_U }, 1464 },
        { "LXEB", mach_format::RRE, { reg_4_U, reg_4_U }, 1464 },
        { "LNEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1464 },
        { "LNDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1464 },
        { "LNXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1464 },
        { "LPEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1465 },
        { "LPDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1465 },
        { "LPXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1465 },
        { "LEDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1465 },
        { "LDXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1465 },
        { "LEXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1465 },
        { "LEDBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1465 },
        { "LDXBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1465 },
        { "LEXBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1465 },
        { "LPP", mach_format::S, { db_12_4_U }, 11 },
        { "MEEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1467 },
        { "MDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1467 },
        { "MXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1467 },
        { "MDEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1467 },
        { "MXDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1467 },
        { "MEEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1467 },
        { "MDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1467 },
        { "MDEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1467 },
        { "MXDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1467 },
        { "MADBR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1468 },
        { "MAEBR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1468 },
        { "MAEB", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1468 },
        { "MADB", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1468 },
        { "MSEBR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1468 },
        { "MSDBR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1468 },
        { "MSEB", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1468 },
        { "MSDB", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1468 },
        { "QCTRI", mach_format::S, { db_12_4_U }, 43 },
        { "QSI", mach_format::S, { db_12_4_U }, 45 },
        { "SCCTR", mach_format::RRE, { reg_4_U, reg_4_U }, 46 },
        { "SPCTR", mach_format::RRE, { reg_4_U, reg_4_U }, 47 },
        { "SQEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1470 },
        { "SQDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1470 },
        { "SQXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1470 },
        { "SQEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1470 },
        { "SQDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1470 },
        { "SEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1470 },
        { "SDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1470 },
        { "SXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1470 },
        { "SEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1470 },
        { "SDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1470 },
        { "SORTL", mach_format::RRE, { reg_4_U, reg_4_U }, 19 },
        { "TCEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1471 },
        { "TCDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1471 },
        { "TCXB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1471 },
        { "ADTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 1491 },
        { "AXTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 1491 },
        { "ADTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1491 },
        { "AXTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1491 },
        { "CDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1494 },
        { "CXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1494 },
        { "KDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1495 },
        { "KXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1495 },
        { "CEDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1495 },
        { "CEXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1495 },
        { "CDGTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1496 },
        { "CXGTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1496 },
        { "CDGTRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1496 },
        { "CXGTRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1496 },
        { "CDFTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1496 },
        { "CXFTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1496 },
        { "CDLGTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1497 },
        { "CXLGTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1497 },
        { "CDLFTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1497 },
        { "CXLFTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1497 },
        { "CDPT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 1498 },
        { "CXPT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 1498 },
        { "CDSTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1500 },
        { "CXSTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1500 },
        { "CDUTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1500 },
        { "CXUTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1500 },
        { "CDZT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 1501 },
        { "CXZT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 1501 },
        { "CGDTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1501 },
        { "CGXTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1501 },
        { "CGDTRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1502 },
        { "CGXTRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1502 },
        { "CFDTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1502 },
        { "CFXTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1502 },
        { "CLGDTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1504 },
        { "CLGXTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1504 },
        { "CLFDTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1504 },
        { "CLFXTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1504 },
        { "CPDT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 1505 },
        { "CPXT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 1505 },
        { "CSDTR", mach_format::RRF_d, { reg_4_U, reg_4_U, mask_4_U }, 1507 },
        { "CSXTR", mach_format::RRF_d, { reg_4_U, reg_4_U, mask_4_U }, 1507 },
        { "CUDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1507 },
        { "CUXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1507 },
        { "CZDT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 1508 },
        { "CZXT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 1508 },
        { "DDTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 1509 },
        { "DXTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 1509 },
        { "DDTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1509 },
        { "DXTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1509 },
        { "EEXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1511 },
        { "EEDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1511 },
        { "ESDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1511 },
        { "ESXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1511 },
        { "IEDTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U }, 1512 },
        { "IEXTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U }, 1512 },
        { "LTDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1513 },
        { "LTXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1513 },
        { "FIDTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1514 },
        { "FIXTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1514 },
        { "LDETR", mach_format::RRF_d, { reg_4_U, reg_4_U, mask_4_U }, 1517 },
        { "LXDTR", mach_format::RRF_d, { reg_4_U, reg_4_U, mask_4_U }, 1517 },
        { "LEDTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1518 },
        { "LDXTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1518 },
        { "MDTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 1519 },
        { "MXTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 1519 },
        { "MDTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1520 },
        { "MXTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1520 },
        { "QADTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1521 },
        { "QAXTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1521 },
        { "RRDTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1524 },
        { "RRXTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1524 },
        { "SELFHR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 864 },
        { "SELGR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 864 },
        { "SELR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 864 },
        { "SLDT", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1526 },
        { "SLXT", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1526 },
        { "SRDT", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1526 },
        { "SRXT", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1526 },
        { "SDTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 1527 },
        { "SXTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 1527 },
        { "SDTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1527 },
        { "SXTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1527 },
        { "TDCET", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1528 },
        { "TDCDT", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1528 },
        { "TDCXT", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1528 },
        { "TDGET", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1529 },
        { "TDGDT", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1529 },
        { "TDGXT", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1529 },
        { "VBPERM", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1536 },
        { "VGEF", mach_format::VRV, { vec_reg_5_U, dvb_12_5x4_U, mask_4_U }, 1536 },
        { "VCFPS", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1641 },
        { "VCLFP", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1611 },
        { "VGEG", mach_format::VRV, { vec_reg_5_U, dvb_12_5x4_U, mask_4_U }, 1536 },
        { "VGBM", mach_format::VRI_a, { vec_reg_5_U, imm_16_U }, 1537 },
        { "VGM", mach_format::VRI_b, { vec_reg_5_U, imm_8_U, imm_8_U, mask_4_U }, 1537 },
        { "VL", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U_opt }, 1538 },
        { "VSTEBRF", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1576 },
        { "VSTEBRG", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1576 },
        { "VLLEBRZ", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1562 },
        { "VLREP", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1538 },
        { "VLR", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U }, 1538 },
        { "VLEB", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1538 },
        { "VLEBRH", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1561 },
        { "VLEBRG", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1561 },
        { "VLBRREP", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1562 },
        { "VLER", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1564 },
        { "VLBR", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1563 },
        { "VLEH", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1539 },
        { "VLEIH", mach_format::VRI_a, { vec_reg_5_U, imm_16_S, mask_4_U }, 1539 },
        { "VLEF", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1539 },
        { "VLEIF", mach_format::VRI_a, { vec_reg_5_U, imm_16_S, mask_4_U }, 1539 },
        { "VLEG", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1539 },
        { "VLEIG", mach_format::VRI_a, { vec_reg_5_U, imm_16_S, mask_4_U }, 1539 },
        { "VLEIB", mach_format::VRI_a, { vec_reg_5_U, imm_16_S, mask_4_U }, 1539 },
        { "VLGV", mach_format::VRS_c, { reg_4_U, vec_reg_5_U, db_12_4_U, mask_4_U }, 1539 },
        { "VLLEZ", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1540 },
        { "VLM", mach_format::VRS_a, { vec_reg_5_U, vec_reg_5_U, db_12_4_U, mask_4_U_opt }, 1541 },
        { "VLRLR", mach_format::VRS_d, { vec_reg_5_U, reg_4_U, db_12_4_U }, 1541 },
        { "VLRL", mach_format::VSI, { vec_reg_5_U, db_12_4_U, imm_8_U }, 1541 },
        { "VLBB", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1542 },
        { "VLVG", mach_format::VRS_b, { vec_reg_5_U, reg_4_U, db_12_4_U, mask_4_U }, 1543 },
        { "VLVGP", mach_format::VRR_f, { vec_reg_5_U, reg_4_U, reg_4_U }, 1543 },
        { "VLL", mach_format::VRS_b, { vec_reg_5_U, reg_4_U, db_12_4_U }, 1543 },
        { "VMRH", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1544 },
        { "VMRL", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1544 },
        { "VPK", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1545 },
        { "VPKS", mach_format::VRR_b, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1545 },
        { "VPKLS", mach_format::VRR_b, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1546 },
        { "VPERM", mach_format::VRR_e, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1547 },
        { "VPDI", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1547 },
        { "VREP", mach_format::VRI_c, { vec_reg_5_U, vec_reg_5_U, imm_16_U, mask_4_U }, 1547 },
        { "VREPI", mach_format::VRI_a, { vec_reg_5_U, imm_16_S, mask_4_U }, 1548 },
        { "VSCEF", mach_format::VRV, { vec_reg_5_U, dvb_12_5x4_U, mask_4_U }, 1548 },
        { "VSCEG", mach_format::VRV, { vec_reg_5_U, dvb_12_5x4_U, mask_4_U }, 1548 },
        { "VSEL", mach_format::VRR_e, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1549 },
        { "VSEG", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1549 },
        { "VSTBR", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1576 },
        { "VST", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U_opt }, 1550 },
        { "VSTEB", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1550 },
        { "VSTEBRH", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1576 },
        { "VSTEH", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1550 },
        { "VSTEF", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1550 },
        { "VSTEG", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1550 },
        { "VSTER", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1578 },
        { "VSTM", mach_format::VRS_a, { vec_reg_5_U, vec_reg_5_U, db_12_4_U, mask_4_U_opt }, 1551 },
        { "VSTRLR", mach_format::VRS_d, { vec_reg_5_U, reg_4_U, db_12_4_U }, 1551 },
        { "VSTRL", mach_format::VSI, { vec_reg_5_U, db_12_4_U, imm_8_U }, 1551 },
        { "VSTL", mach_format::VRS_b, { vec_reg_5_U, reg_4_U, db_12_4_U }, 1552 },
        { "VUPH", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1552 },
        { "VUPL", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1553 },
        { "VUPLH", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1553 },
        { "VUPLL", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1554 },
        { "VA", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1557 },
        { "VACC", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1558 },
        { "VAC", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1558 },
        { "VACCC", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1559 },
        { "VN", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1559 },
        { "VNC", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1559 },
        { "VAVG", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1560 },
        { "VAVGL", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1560 },
        { "VCKSM", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1560 },
        { "VEC", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1561 },
        { "VECL", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1561 },
        { "VCEQ", mach_format::VRR_b, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1561 },
        { "VCH", mach_format::VRR_b, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1562 },
        { "VCHL", mach_format::VRR_b, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1563 },
        { "VCLZ", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1564 },
        { "VCTZ", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1564 },
        { "VGFM", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1565 },
        { "VX", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1565 },
        { "VLC", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1566 },
        { "VGFMA", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1566 },
        { "VLP", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1566 },
        { "VMX", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1567 },
        { "VMXL", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1567 },
        { "VMN", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1567 },
        { "VMNL", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1568 },
        { "VMAL", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1568 },
        { "VMAH", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1569 },
        { "VMALH", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1569 },
        { "VMAE", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1569 },
        { "VMALE", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1569 },
        { "VMAO", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1570 },
        { "VMALO", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1570 },
        { "VMH", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1570 },
        { "VMLH", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1571 },
        { "VML", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1571 },
        { "VME", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1572 },
        { "VMLE", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1572 },
        { "VMO", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1572 },
        { "VMLO", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1572 },
        { "VMSL",
            mach_format::VRR_d,
            { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U },
            1573 },
        { "VNN", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1574 },
        { "VNO", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1574 },
        { "VNX", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1574 },
        { "VO", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1574 },
        { "VOC", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1575 },
        { "VPOPCT", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1575 },
        { "VERLLV", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1575 },
        { "VERLL", mach_format::VRS_a, { vec_reg_5_U, vec_reg_5_U, db_12_4_U, mask_4_U }, 1575 },
        { "VERIM", mach_format::VRI_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U }, 1576 },
        { "VESLV", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1577 },
        { "VESL", mach_format::VRS_a, { vec_reg_5_U, vec_reg_5_U, db_12_4_U, mask_4_U }, 1577 },
        { "VESRAV", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1577 },
        { "VESRA", mach_format::VRS_a, { vec_reg_5_U, vec_reg_5_U, db_12_4_U, mask_4_U }, 1577 },
        { "VESRLV", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1578 },
        { "VESRL", mach_format::VRS_a, { vec_reg_5_U, vec_reg_5_U, db_12_4_U, mask_4_U }, 1578 },
        { "VSLD", mach_format::VRI_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U }, 1607 },
        { "VSRD", mach_format::VRI_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U }, 1608 },
        { "VSLDB", mach_format::VRI_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U }, 1579 },
        { "VSL", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1579 },
        { "VSLB", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1579 },
        { "VSRA", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1579 },
        { "VSRAB", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1580 },
        { "VSRL", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1580 },
        { "VSRLB", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1580 },
        { "VS", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1580 },
        { "VSCBI", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1581 },
        { "VCSFP", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1644 },
        { "VSBI", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1581 },
        { "VSBCBI", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1582 },
        { "VSUMG", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1582 },
        { "VSUMQ", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1583 },
        { "VSUM", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1583 },
        { "VTM", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U }, 1584 },
        { "VFAE", mach_format::VRR_b, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U_opt }, 1585 },
        { "VFEE", mach_format::VRR_b, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U_opt }, 1587 },
        { "VFENE", mach_format::VRR_b, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U_opt }, 1588 },
        { "VISTR", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U_opt }, 1589 },
        { "VSTRC",
            mach_format::VRR_d,
            { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U_opt },
            1590 },
        { "VSTRS",
            mach_format::VRR_d,
            { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U_opt },
            1622 },
        { "VFA", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1595 },
        { "WFC", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1599 },
        { "WFK", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1600 },
        { "VFCE", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1601 },
        { "VFCH", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1603 },
        { "VFCHE", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1605 },
        { "VCFPS", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1607 },
        { "VCFPL", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1643 },
        { "VCLGD", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1611 },
        { "VFD", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1613 },
        { "VFI", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1615 },
        { "VFLL", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1617 },
        { "VFLR", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1618 },
        { "VFMAX", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1619 },
        { "VFMIN", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1625 },
        { "VFM", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1631 },
        { "VFMA",
            mach_format::VRR_e,
            { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U },
            1633 },
        { "VFMS",
            mach_format::VRR_e,
            { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U },
            1633 },
        { "VFNMA",
            mach_format::VRR_e,
            { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U },
            1633 },
        { "VFNMS",
            mach_format::VRR_e,
            { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U },
            1633 },
        { "VFPSO", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1635 },
        { "VFSQ", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1636 },
        { "VFS", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1637 },
        { "VFTCI", mach_format::VRI_e, { vec_reg_5_U, vec_reg_5_U, imm_12_S, mask_4_U, mask_4_U }, 1638 },
        { "VAP", mach_format::VRI_f, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U }, 1643 },
        { "VCP", mach_format::VRR_h, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1644 },
        { "VCVB", mach_format::VRR_i, { reg_4_U, vec_reg_5_U, mask_4_U }, 1645 },
        { "VCVBG", mach_format::VRR_i, { reg_4_U, vec_reg_5_U, mask_4_U }, 1645 },
        { "VCVD", mach_format::VRI_i, { vec_reg_5_U, reg_4_U, imm_8_S, mask_4_U }, 1646 },
        { "VCVDG", mach_format::VRI_i, { vec_reg_5_U, reg_4_U, imm_8_S, mask_4_U }, 1646 },
        { "VDP", mach_format::VRI_f, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U }, 1648 },
        { "VMP", mach_format::VRI_f, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U }, 1650 },
        { "VMSP", mach_format::VRI_f, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U }, 1651 },
        { "VRP", mach_format::VRI_f, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U }, 1654 },
        { "VSDP", mach_format::VRI_f, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U }, 1656 },
        { "VSP", mach_format::VRI_f, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U }, 1658 },
        { "VLIP", mach_format::VRI_h, { vec_reg_5_U, imm_16_S, imm_4_U }, 1649 },
        { "VPKZ", mach_format::VSI, { vec_reg_5_U, db_12_4_U, imm_8_U }, 1652 },
        { "VPSOP", mach_format::VRI_g, { vec_reg_5_U, vec_reg_5_U, imm_8_U, imm_8_U, mask_4_U }, 1653 },
        { "VSRP", mach_format::VRI_g, { vec_reg_5_U, vec_reg_5_U, imm_8_U, imm_8_S, mask_4_U }, 1657 },
        { "SIE", mach_format::S, { db_12_4_U }, 7 },
        { "VAD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VSD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VADS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VSDS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VMDS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VDDS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VMADS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VMSDS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VCDS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VAS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VSS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VMS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VNS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VOS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VXS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VCS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VMD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VMSD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VMSE", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VMSES", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VLINT", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VDD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VDDS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VDE", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VDES", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VMXAD", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VMXAE", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VMXSE", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VNVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VOVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VLCVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VLELE", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VLELD", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VLI", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VLID", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VLBIX", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VLVCU", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VLVCA", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VLVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VMNSD", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VMNSE", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VMRRS", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VMRSV", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VACRS", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VACSV", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VSTVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VSTVP", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VSVMM", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VTVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VXELE", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VXELD", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VXVC", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VXVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VXVMM", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VSTI", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VSTID", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VRCL", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VRRS", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VRSV", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VRSVC", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VSLL", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VSPSD", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VZPSD", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VSRRS", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VSRSV", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VOVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VCVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VCZVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VCOVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0 },
        { "VTP", mach_format::VRR_g, { vec_reg_5_U }, 1660 },
        { "VUPKZ", mach_format::VSI, { vec_reg_5_U, db_12_4_U, imm_8_U }, 1660 },
        { "VSTK", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VSE", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VSES", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VSD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VSDS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VSTD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VSTKD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VSTMD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VSTH", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VLD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VLH", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VLMD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VLY", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VLYD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VM", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VMAD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VMAES", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VMCD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VMCE", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VMD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VMES", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VACD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VACE", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VAE", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VAES", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VC", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VCD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VCE", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
        { "VCES", mach_format::RI_a, { reg_4_U, imm_16_U }, 0 },
    };

    for (const auto& i : instructions)
        result.emplace(
            i.name, i.format, std::vector(i.op_format.data(), i.op_format.data() + i.op_format_size), i.page_no);

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
