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

const std::map<mach_format, std::string> instruction::mach_format_to_string = {
    { mach_format::E, "E" },
    { mach_format::I, "I" },
    { mach_format::IE, "IE" },
    { mach_format::MII, "MII" },
    { mach_format::RI_a, "RI-a" },
    { mach_format::RI_b, "RI-b" },
    { mach_format::RI_c, "RI-c" },
    { mach_format::RIE_a, "RIE-a" },
    { mach_format::RIE_b, "RIE-b" },
    { mach_format::RIE_c, "RIE-c" },
    { mach_format::RIE_d, "RIE-d" },
    { mach_format::RIE_e, "RIE-e" },
    { mach_format::RIE_f, "RIE-f" },
    { mach_format::RIE_g, "RIE-g" },
    { mach_format::RIL_a, "RIL-a" },
    { mach_format::RIL_b, "RIL-b" },
    { mach_format::RIL_c, "RIL-c" },
    { mach_format::RIS, "RIS" },
    { mach_format::RR, "RR" },
    { mach_format::RRD, "RRD" },
    { mach_format::RRE, "RRE" },
    { mach_format::RRF_a, "RRF-a" },
    { mach_format::RRF_b, "RRF-b" },
    { mach_format::RRF_c, "RRF-c" },
    { mach_format::RRF_d, "RRF-d" },
    { mach_format::RRF_e, "RRF-e" },
    { mach_format::RRS, "RRS" },
    { mach_format::RS_a, "RS-a" },
    { mach_format::RS_b, "RS-b" },
    { mach_format::RSI, "RSI" },
    { mach_format::RSL_a, "RSL-a" },
    { mach_format::RSL_b, "RSL-b" },
    { mach_format::RSY_a, "RSY-a" },
    { mach_format::RSY_b, "RSY-b" },
    { mach_format::RX_a, "RX-a" },
    { mach_format::RX_b, "RX-b" },
    { mach_format::RXE, "RXE" },
    { mach_format::RXF, "RXF" },
    { mach_format::RXY_a, "RXY-a" },
    { mach_format::RXY_b, "RXY-b" },
    { mach_format::S, "S" },
    { mach_format::SI, "SI" },
    { mach_format::SIL, "SIL" },
    { mach_format::SIY, "SIY" },
    { mach_format::SMI, "SMI" },
    { mach_format::SS_a, "SS-a" },
    { mach_format::SS_b, "SS-b" },
    { mach_format::SS_c, "SS-c" },
    { mach_format::SS_d, "SS-d" },
    { mach_format::SS_e, "SS-e" },
    { mach_format::SS_f, "SS-f" },
    { mach_format::SSE, "SSE" },
    { mach_format::SSF, "SSF" },
    { mach_format::VRI_a, "VRI-a" },
    { mach_format::VRI_b, "VRI-b" },
    { mach_format::VRI_c, "VRI-c" },
    { mach_format::VRI_d, "VRI-d" },
    { mach_format::VRI_e, "VRI-e" },
    { mach_format::VRI_f, "VRI-f" },
    { mach_format::VRI_g, "VRI-g" },
    { mach_format::VRI_h, "VRI-h" },
    { mach_format::VRI_i, "VRI-i" },
    { mach_format::VRR_a, "VRR-a" },
    { mach_format::VRR_b, "VRR-b" },
    { mach_format::VRR_c, "VRR-c" },
    { mach_format::VRR_d, "VRR-d" },
    { mach_format::VRR_e, "VRR-e" },
    { mach_format::VRR_f, "VRR-f" },
    { mach_format::VRR_g, "VRR-g" },
    { mach_format::VRR_h, "VRR-h" },
    { mach_format::VRR_i, "VRR-i" },
    { mach_format::VRS_a, "VRS-a" },
    { mach_format::VRS_b, "VRS-b" },
    { mach_format::VRS_c, "VRS-c" },
    { mach_format::VRS_d, "VRS-d" },
    { mach_format::VSI, "VSI" },
    { mach_format::VRV, "VRV" },
    { mach_format::VRX, "VRX" },
};

const std::vector<ca_instruction> instruction::ca_instructions = {
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

const std::map<std::string, assembler_instruction> instruction::assembler_instructions = {
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
            "value,?<length_attribute_value>,?<type_attribute_value>,?<program_type_value>,?<assembler_type_"
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

    const auto add_machine_instr = [&result](std::string_view instruction_name,
                                       mach_format format,
                                       std::initializer_list<machine_operand_format> op_format,
                                       short page_no) { result.emplace(instruction_name, format, op_format, page_no); };

    add_machine_instr("AR", mach_format::RR, { reg_4_U, reg_4_U }, 510);
    add_machine_instr("ADDFRR", mach_format::RRE, { reg_4_U, reg_4_U }, 7);
    add_machine_instr("AGR", mach_format::RRE, { reg_4_U, reg_4_U }, 510);
    add_machine_instr("AGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 510);
    add_machine_instr("ARK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 510);
    add_machine_instr("AGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 510);
    add_machine_instr("A", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 510);
    add_machine_instr("AY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 511);
    add_machine_instr("AG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 511);
    add_machine_instr("AGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 511);
    add_machine_instr("AFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 511);
    add_machine_instr("AGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 511);
    add_machine_instr("AHIK", mach_format::RIE_d, { reg_4_U, reg_4_U, imm_16_S }, 511);
    add_machine_instr("AGHIK", mach_format::RIE_d, { reg_4_U, reg_4_U, imm_16_S }, 511);
    add_machine_instr("ASI", mach_format::SIY, { db_20_4_S, imm_8_S }, 511);
    add_machine_instr("AGSI", mach_format::SIY, { db_20_4_S, imm_8_S }, 511);
    add_machine_instr("AH", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 512);
    add_machine_instr("AHY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 512);
    add_machine_instr("AGH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 512);
    add_machine_instr("AHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 512);
    add_machine_instr("AGHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 513);
    add_machine_instr("AHHHR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 513);
    add_machine_instr("AHHLR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 513);
    add_machine_instr("AIH", mach_format::RIL_a, { reg_4_U, imm_32_S }, 513);
    add_machine_instr("ALR", mach_format::RR, { reg_4_U, reg_4_U }, 514);
    add_machine_instr("ALGR", mach_format::RRE, { reg_4_U, reg_4_U }, 514);
    add_machine_instr("ALGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 514);
    add_machine_instr("ALRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 514);
    add_machine_instr("ALGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 514);
    add_machine_instr("AL", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 514);
    add_machine_instr("ALY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 514);
    add_machine_instr("ALG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 514);
    add_machine_instr("ALGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 514);
    add_machine_instr("ALFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 514);
    add_machine_instr("ALGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 514);
    add_machine_instr("ALHHHR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 515);
    add_machine_instr("ALHHLR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 515);
    add_machine_instr("ALCR", mach_format::RRE, { reg_4_U, reg_4_U }, 515);
    add_machine_instr("ALCGR", mach_format::RRE, { reg_4_U, reg_4_U }, 515);
    add_machine_instr("ALC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 515);
    add_machine_instr("ALCG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 515);
    add_machine_instr("ALSI", mach_format::SIY, { db_20_4_S, imm_8_S }, 516);
    add_machine_instr("ALGSI", mach_format::SIY, { db_20_4_S, imm_8_S }, 516);
    add_machine_instr("ALHSIK", mach_format::RIE_d, { reg_4_U, reg_4_U, imm_16_S }, 516);
    add_machine_instr("ALGHSIK", mach_format::RIE_d, { reg_4_U, reg_4_U, imm_16_S }, 516);
    add_machine_instr("ALSIH", mach_format::RIL_a, { reg_4_U, imm_32_S }, 517);
    add_machine_instr("ALSIHN", mach_format::RIL_a, { reg_4_U, imm_32_S }, 517);
    add_machine_instr("NR", mach_format::RR, { reg_4_U, reg_4_U }, 517);
    add_machine_instr("NGR", mach_format::RRE, { reg_4_U, reg_4_U }, 517);
    add_machine_instr("NRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 517);
    add_machine_instr("NGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 517);
    add_machine_instr("N", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 517);
    add_machine_instr("NY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 517);
    add_machine_instr("NG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 517);
    add_machine_instr("NI", mach_format::SI, { db_12_4_U, imm_8_U }, 517);
    add_machine_instr("NIY", mach_format::SIY, { db_20_4_S, imm_8_U }, 518);
    add_machine_instr("NC", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 518);
    add_machine_instr("NIHF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 518);
    add_machine_instr("NIHH", mach_format::RI_a, { reg_4_U, imm_16_U }, 518);
    add_machine_instr("NIHL", mach_format::RI_a, { reg_4_U, imm_16_U }, 518);
    add_machine_instr("NILF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 519);
    add_machine_instr("NILH", mach_format::RI_a, { reg_4_U, imm_16_U }, 519);
    add_machine_instr("NILL", mach_format::RI_a, { reg_4_U, imm_16_U }, 519);
    add_machine_instr("BALR", mach_format::RR, { reg_4_U, reg_4_U }, 519);
    add_machine_instr("BAL", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 519);
    add_machine_instr("BASR", mach_format::RR, { reg_4_U, reg_4_U }, 520);
    add_machine_instr("BAS", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 520);
    add_machine_instr("BASSM", mach_format::RX_a, { reg_4_U, reg_4_U }, 520);
    add_machine_instr("BSM", mach_format::RR, { reg_4_U, reg_4_U }, 522);
    add_machine_instr("BIC", mach_format::RXY_b, { mask_4_U, dxb_20_4x4_S }, 523);
    add_machine_instr("BCR", mach_format::RR, { mask_4_U, reg_4_U }, 524);
    add_machine_instr("BC", mach_format::RX_b, { mask_4_U, dxb_12_4x4_U }, 524);
    add_machine_instr("BCTR", mach_format::RR, { reg_4_U, reg_4_U }, 525);
    add_machine_instr("BCTGR", mach_format::RRE, { reg_4_U, reg_4_U }, 525);
    add_machine_instr("BCT", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 525);
    add_machine_instr("BCTG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 525);
    add_machine_instr("BXH", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 526);
    add_machine_instr("BXHG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 526);
    add_machine_instr("BXLE", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 526);
    add_machine_instr("BXLEG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 526);
    add_machine_instr("BPP", mach_format::SMI, { mask_4_U, rel_addr_imm_16_S, db_12_4_U }, 527);
    add_machine_instr("BPRP", mach_format::MII, { mask_4_U, rel_addr_imm_12_S, rel_addr_imm_24_S }, 527);
    add_machine_instr("BRAS", mach_format::RI_b, { reg_4_U, rel_addr_imm_16_S }, 530);
    add_machine_instr("BRASL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 530);
    add_machine_instr("BRC", mach_format::RI_c, { mask_4_U, rel_addr_imm_16_S }, 530);
    add_machine_instr("BRCL", mach_format::RIL_c, { mask_4_U, rel_addr_imm_32_S }, 530);
    add_machine_instr("BRCT", mach_format::RI_b, { reg_4_U, rel_addr_imm_16_S }, 531);
    add_machine_instr("BRCTG", mach_format::RI_b, { reg_4_U, rel_addr_imm_16_S }, 531);
    add_machine_instr("BRCTH", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 531);
    add_machine_instr("BRXH", mach_format::RSI, { reg_4_U, reg_4_U, rel_addr_imm_16_S }, 532);
    add_machine_instr("BRXHG", mach_format::RIE_e, { reg_4_U, reg_4_U, rel_addr_imm_16_S }, 532);
    add_machine_instr("BRXLE", mach_format::RSI, { reg_4_U, reg_4_U, rel_addr_imm_16_S }, 532);
    add_machine_instr("BRXLG", mach_format::RIE_e, { reg_4_U, reg_4_U, rel_addr_imm_16_S }, 532);
    add_machine_instr("CKSM", mach_format::RRE, { reg_4_U, reg_4_U }, 533);
    add_machine_instr("KM", mach_format::RRE, { reg_4_U, reg_4_U }, 537);
    add_machine_instr("KMC", mach_format::RRE, { reg_4_U, reg_4_U }, 537);
    add_machine_instr("KMA", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U }, 562);
    add_machine_instr("KMF", mach_format::RRE, { reg_4_U, reg_4_U }, 576);
    add_machine_instr("KMCTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U }, 591);
    add_machine_instr("KMO", mach_format::RRE, { reg_4_U, reg_4_U }, 604);
    add_machine_instr("CR", mach_format::RR, { reg_4_U, reg_4_U }, 618);
    add_machine_instr("CGR", mach_format::RRE, { reg_4_U, reg_4_U }, 618);
    add_machine_instr("CGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 618);
    add_machine_instr("C", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 618);
    add_machine_instr("CY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 618);
    add_machine_instr("CG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 618);
    add_machine_instr("CGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 618);
    add_machine_instr("CFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 618);
    add_machine_instr("CGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 619);
    add_machine_instr("CRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 619);
    add_machine_instr("CGRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 619);
    add_machine_instr("CGFRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 619);
    add_machine_instr("CRB", mach_format::RRS, { reg_4_U, reg_4_U, mask_4_U, db_12_4_U }, 619);
    add_machine_instr("CGRB", mach_format::RRS, { reg_4_U, reg_4_U, mask_4_U, db_12_4_U }, 619);
    add_machine_instr("CRJ", mach_format::RIE_b, { reg_4_U, reg_4_U, mask_4_U, rel_addr_imm_16_S }, 619);
    add_machine_instr("CGRJ", mach_format::RIE_b, { reg_4_U, reg_4_U, mask_4_U, rel_addr_imm_16_S }, 620);
    add_machine_instr("CIB", mach_format::RIS, { reg_4_U, imm_8_S, mask_4_U, db_12_4_U }, 620);
    add_machine_instr("CGIB", mach_format::RIS, { reg_4_U, imm_8_S, mask_4_U, db_12_4_U }, 620);
    add_machine_instr("CIJ", mach_format::RIE_c, { reg_4_U, imm_8_S, mask_4_U, rel_addr_imm_16_S }, 620);
    add_machine_instr("CGIJ", mach_format::RIE_c, { reg_4_U, imm_8_S, mask_4_U, rel_addr_imm_16_S }, 620);
    add_machine_instr("CFC", mach_format::S, { db_12_4_U }, 621);
    add_machine_instr("CS", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 628);
    add_machine_instr("CSY", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 628);
    add_machine_instr("CSG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 628);
    add_machine_instr("CDS", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 628);
    add_machine_instr("CDSY", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 628);
    add_machine_instr("CDSG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 628);
    add_machine_instr("CSST", mach_format::SSF, { db_12_4_U, db_12_4_U, reg_4_U }, 630);
    add_machine_instr("CRT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 633);
    add_machine_instr("CGRT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 633);
    add_machine_instr("CIT", mach_format::RIE_a, { reg_4_U, imm_16_S, mask_4_U }, 633);
    add_machine_instr("CGIT", mach_format::RIE_a, { reg_4_U, imm_16_S, mask_4_U }, 633);
    add_machine_instr("CH", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 634);
    add_machine_instr("CHY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 634);
    add_machine_instr("CGH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 634);
    add_machine_instr("CHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 634);
    add_machine_instr("CGHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 634);
    add_machine_instr("CHHSI", mach_format::SIL, { db_12_4_U, imm_16_S }, 634);
    add_machine_instr("CHSI", mach_format::SIL, { db_12_4_U, imm_16_S }, 634);
    add_machine_instr("CGHSI", mach_format::SIL, { db_12_4_U, imm_16_S }, 634);
    add_machine_instr("CHRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 634);
    add_machine_instr("CGHRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 634);
    add_machine_instr("CHHR", mach_format::RRE, { reg_4_U, reg_4_U }, 635);
    add_machine_instr("CHLR", mach_format::RRE, { reg_4_U, reg_4_U }, 635);
    add_machine_instr("CHF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 635);
    add_machine_instr("CIH", mach_format::RIL_a, { reg_4_U, imm_32_S }, 635);
    add_machine_instr("CLR", mach_format::RR, { reg_4_U, reg_4_U }, 636);
    add_machine_instr("CLGR", mach_format::RRE, { reg_4_U, reg_4_U }, 636);
    add_machine_instr("CLGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 636);
    add_machine_instr("CL", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 636);
    add_machine_instr("CLY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 636);
    add_machine_instr("CLG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 636);
    add_machine_instr("CLGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 636);
    add_machine_instr("CLC", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 636);
    add_machine_instr("CLFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 636);
    add_machine_instr("CLGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 636);
    add_machine_instr("CLI", mach_format::SI, { db_12_4_U, imm_8_U }, 636);
    add_machine_instr("CLIY", mach_format::SIY, { db_12_4_U, imm_8_U }, 636);
    add_machine_instr("CLFHSI", mach_format::SIL, { db_12_4_U, imm_16_U }, 636);
    add_machine_instr("CLGHSI", mach_format::SIL, { db_12_4_U, imm_16_U }, 636);
    add_machine_instr("CLHHSI", mach_format::SIL, { db_12_4_U, imm_16_U }, 636);
    add_machine_instr("CLRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 637);
    add_machine_instr("CLGRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 637);
    add_machine_instr("CLGFRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 637);
    add_machine_instr("CLHRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 637);
    add_machine_instr("CLGHRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 637);
    add_machine_instr("CLRB", mach_format::RRS, { reg_4_U, reg_4_U, mask_4_U, db_12_4_U }, 638);
    add_machine_instr("CLGRB", mach_format::RRS, { reg_4_U, reg_4_U, mask_4_U, db_12_4_U }, 638);
    add_machine_instr("CLRJ", mach_format::RIE_b, { reg_4_U, reg_4_U, mask_4_U, rel_addr_imm_16_S }, 638);
    add_machine_instr("CLGRJ", mach_format::RIE_b, { reg_4_U, reg_4_U, mask_4_U, rel_addr_imm_16_S }, 638);
    add_machine_instr("CLIB", mach_format::RIS, { reg_4_U, imm_8_S, mask_4_U, db_12_4_U }, 638);
    add_machine_instr("CLGIB", mach_format::RIS, { reg_4_U, imm_8_S, mask_4_U, db_12_4_U }, 638);
    add_machine_instr("CLIJ", mach_format::RIE_c, { reg_4_U, imm_8_S, mask_4_U, rel_addr_imm_16_S }, 638);
    add_machine_instr("CLGIJ", mach_format::RIE_c, { reg_4_U, imm_8_S, mask_4_U, rel_addr_imm_16_S }, 638);
    add_machine_instr("CLRT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 639);
    add_machine_instr("CLGRT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 639);
    add_machine_instr("CLT", mach_format::RSY_b, { reg_4_U, mask_4_U, dxb_20_4x4_S }, 639);
    add_machine_instr("CLGT", mach_format::RSY_b, { reg_4_U, mask_4_U, dxb_20_4x4_S }, 639);
    add_machine_instr("CLFIT", mach_format::RIE_a, { reg_4_U, imm_16_S, mask_4_U }, 640);
    add_machine_instr("CLGIT", mach_format::RIE_a, { reg_4_U, imm_16_S, mask_4_U }, 640);
    add_machine_instr("CLM", mach_format::RS_b, { reg_4_U, mask_4_U, db_12_4_U }, 641);
    add_machine_instr("CLMY", mach_format::RSY_b, { reg_4_U, mask_4_U, db_20_4_S }, 641);
    add_machine_instr("CLMH", mach_format::RSY_b, { reg_4_U, mask_4_U, db_20_4_S }, 641);
    add_machine_instr("CLHHR", mach_format::RRE, { reg_4_U, reg_4_U }, 641);
    add_machine_instr("CLHLR", mach_format::RRE, { reg_4_U, reg_4_U }, 641);
    add_machine_instr("CLHF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 641);
    add_machine_instr("CLCL", mach_format::RR, { reg_4_U, reg_4_U }, 642);
    add_machine_instr("CLIH", mach_format::RIL_a, { reg_4_U, imm_32_S }, 642);
    add_machine_instr("CLCLE", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 644);
    add_machine_instr("CLCLU", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 647);
    add_machine_instr("CLST", mach_format::RRE, { reg_4_U, reg_4_U }, 650);
    add_machine_instr("CUSE", mach_format::RRE, { reg_4_U, reg_4_U }, 651);
    add_machine_instr("CMPSC", mach_format::RRE, { reg_4_U, reg_4_U }, 654);
    add_machine_instr("KIMD", mach_format::RRE, { reg_4_U, reg_4_U }, 672);
    add_machine_instr("KLMD", mach_format::RRE, { reg_4_U, reg_4_U }, 685);
    add_machine_instr("KMAC", mach_format::RRE, { reg_4_U, reg_4_U }, 703);
    add_machine_instr("CVB", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 714);
    add_machine_instr("CVBY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 714);
    add_machine_instr("CVBG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 714);
    add_machine_instr("CVD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 715);
    add_machine_instr("CVDY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 715);
    add_machine_instr("CVDG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 715);
    add_machine_instr("CU24", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 715);
    add_machine_instr("CUUTF", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 718);
    add_machine_instr("CU21", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 718);
    add_machine_instr("CU42", mach_format::RRE, { reg_4_U, reg_4_U }, 722);
    add_machine_instr("CU41", mach_format::RRE, { reg_4_U, reg_4_U }, 725);
    add_machine_instr("CUTFU", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 728);
    add_machine_instr("CU12", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 728);
    add_machine_instr("CU14", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 732);
    add_machine_instr("CPYA", mach_format::RRE, { reg_4_U, reg_4_U }, 736);
    add_machine_instr("DR", mach_format::RR, { reg_4_U, reg_4_U }, 736);
    add_machine_instr("D", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 736);
    add_machine_instr("DLR", mach_format::RRE, { reg_4_U, reg_4_U }, 737);
    add_machine_instr("DLGR", mach_format::RRE, { reg_4_U, reg_4_U }, 737);
    add_machine_instr("DL", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 737);
    add_machine_instr("DLG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 737);
    add_machine_instr("DSGR", mach_format::RRE, { reg_4_U, reg_4_U }, 738);
    add_machine_instr("DSGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 738);
    add_machine_instr("DSG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 738);
    add_machine_instr("DSGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 738);
    add_machine_instr("HIO", mach_format::S, { db_12_4_U }, 129);
    add_machine_instr("HDV", mach_format::S, { db_12_4_U }, 129);
    add_machine_instr("SIO", mach_format::S, { db_12_4_U }, 129);
    add_machine_instr("SIOF", mach_format::S, { db_12_4_U }, 129);
    add_machine_instr("STIDC", mach_format::S, { db_12_4_U }, 129);
    add_machine_instr("CLRCH", mach_format::S, { db_12_4_U }, 367);
    add_machine_instr("CLRIO", mach_format::S, { db_12_4_U }, 368);
    add_machine_instr("TCH", mach_format::S, { db_12_4_U }, 384);
    add_machine_instr("TIO", mach_format::S, { db_12_4_U }, 385);
    add_machine_instr("RRB", mach_format::S, { db_12_4_U }, 295);
    add_machine_instr("CONCS", mach_format::S, { db_12_4_U }, 263);
    add_machine_instr("DISCS", mach_format::S, { db_12_4_U }, 265);
    add_machine_instr("XR", mach_format::RR, { reg_4_U, reg_4_U }, 738);
    add_machine_instr("XGR", mach_format::RRE, { reg_4_U, reg_4_U }, 738);
    add_machine_instr("XRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 738);
    add_machine_instr("XGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 738);
    add_machine_instr("X", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 738);
    add_machine_instr("XY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 738);
    add_machine_instr("XG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 738);
    add_machine_instr("XI", mach_format::SI, { db_12_4_U, imm_8_U }, 739);
    add_machine_instr("XIY", mach_format::SIY, { db_20_4_S, imm_8_U }, 739);
    add_machine_instr("XC", mach_format::SS_a, { db_12_8x4L_U, db_20_4_S }, 739);
    add_machine_instr("EX", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 740);
    add_machine_instr("XIHF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 740);
    add_machine_instr("XILF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 740);
    add_machine_instr("EXRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 740);
    add_machine_instr("EAR", mach_format::RRE, { reg_4_U, reg_4_U }, 741);
    add_machine_instr("ECAG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 741);
    add_machine_instr("ECTG", mach_format::SSF, { db_12_4_U, db_12_4_U, reg_4_U }, 744);
    add_machine_instr("EPSW", mach_format::RRE, { reg_4_U, reg_4_U }, 745);
    add_machine_instr("ETND", mach_format::RRE, { reg_4_U }, 745);
    add_machine_instr("FLOGR", mach_format::RRE, { reg_4_U, reg_4_U }, 746);
    add_machine_instr("IC", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 746);
    add_machine_instr("ICY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 746);
    add_machine_instr("ICM", mach_format::RS_b, { reg_4_U, mask_4_U, db_12_4_U }, 746);
    add_machine_instr("ICMY", mach_format::RSY_b, { reg_4_U, mask_4_U, db_20_4_S }, 746);
    add_machine_instr("ICMH", mach_format::RSY_b, { reg_4_U, mask_4_U, db_20_4_S }, 746);
    add_machine_instr("IIHF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 747);
    add_machine_instr("IIHH", mach_format::RI_a, { reg_4_U, imm_16_U }, 747);
    add_machine_instr("IIHL", mach_format::RI_a, { reg_4_U, imm_16_U }, 747);
    add_machine_instr("IILF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 747);
    add_machine_instr("IILH", mach_format::RI_a, { reg_4_U, imm_16_U }, 747);
    add_machine_instr("IILL", mach_format::RI_a, { reg_4_U, imm_16_U }, 747);
    add_machine_instr("IPM", mach_format::RRE, { reg_4_U }, 748);
    add_machine_instr("LR", mach_format::RR, { reg_4_U, reg_4_U }, 748);
    add_machine_instr("LGR", mach_format::RRE, { reg_4_U, reg_4_U }, 748);
    add_machine_instr("LGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 748);
    add_machine_instr("L", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 748);
    add_machine_instr("LY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 748);
    add_machine_instr("LG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 748);
    add_machine_instr("LGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 748);
    add_machine_instr("LGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 748);
    add_machine_instr("LRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 748);
    add_machine_instr("LGRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 748);
    add_machine_instr("LGFRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 748);
    add_machine_instr("LAM", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 749);
    add_machine_instr("LAMY", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 749);
    add_machine_instr("LA", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 750);
    add_machine_instr("LAY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 750);
    add_machine_instr("LAE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 750);
    add_machine_instr("LAEY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 750);
    add_machine_instr("LARL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 751);
    add_machine_instr("LAA", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 752);
    add_machine_instr("LAAG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 752);
    add_machine_instr("LAAL", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 752);
    add_machine_instr("LAALG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 752);
    add_machine_instr("LAN", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 753);
    add_machine_instr("LANG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 753);
    add_machine_instr("LAX", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 753);
    add_machine_instr("LAXG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 753);
    add_machine_instr("LAO", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 754);
    add_machine_instr("LAOG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 754);
    add_machine_instr("LTR", mach_format::RR, { reg_4_U, reg_4_U }, 754);
    add_machine_instr("LTGR", mach_format::RRE, { reg_4_U, reg_4_U }, 754);
    add_machine_instr("LTGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 754);
    add_machine_instr("LT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 755);
    add_machine_instr("LTG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 755);
    add_machine_instr("LTGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 755);
    add_machine_instr("LAT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 755);
    add_machine_instr("LGAT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 755);
    add_machine_instr("LZRF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 755);
    add_machine_instr("LZRG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 755);
    add_machine_instr("LBR", mach_format::RRE, { reg_4_U, reg_4_U }, 756);
    add_machine_instr("LGBR", mach_format::RRE, { reg_4_U, reg_4_U }, 756);
    add_machine_instr("LB", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 756);
    add_machine_instr("LGB", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 756);
    add_machine_instr("LBH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 756);
    add_machine_instr("LCR", mach_format::RR, { reg_4_U, reg_4_U }, 756);
    add_machine_instr("LCGR", mach_format::RRE, { reg_4_U, reg_4_U }, 757);
    add_machine_instr("LCGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 757);
    add_machine_instr("LCBB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U, mask_4_U }, 757);
    add_machine_instr("LGG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 758);
    add_machine_instr("LLGFSG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 758);
    add_machine_instr("LGSC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 759);
    add_machine_instr("LHR", mach_format::RRE, { reg_4_U, reg_4_U }, 760);
    add_machine_instr("LGHR", mach_format::RRE, { reg_4_U, reg_4_U }, 760);
    add_machine_instr("LH", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 760);
    add_machine_instr("LHY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 760);
    add_machine_instr("LGH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 760);
    add_machine_instr("LHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 760);
    add_machine_instr("LGHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 760);
    add_machine_instr("LHRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 760);
    add_machine_instr("LGHRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 760);
    add_machine_instr("LHH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 761);
    add_machine_instr("LOCHI", mach_format::RIE_g, { reg_4_U, imm_16_S, mask_4_U }, 761);
    add_machine_instr("LOCGHI", mach_format::RIE_g, { reg_4_U, imm_16_S, mask_4_U }, 761);
    add_machine_instr("LOCHHI", mach_format::RIE_g, { reg_4_U, imm_16_S, mask_4_U }, 761);
    add_machine_instr("LFH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 762);
    add_machine_instr("LFHAT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 762);
    add_machine_instr("LLGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 762);
    add_machine_instr("LLGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 762);
    add_machine_instr("LLGFRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 762);
    add_machine_instr("LLGFAT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 763);
    add_machine_instr("LLCR", mach_format::RRE, { reg_4_U, reg_4_U }, 763);
    add_machine_instr("LLGCR", mach_format::RRE, { reg_4_U, reg_4_U }, 763);
    add_machine_instr("LLC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 763);
    add_machine_instr("LLGC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 763);
    add_machine_instr("LLZRGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 763);
    add_machine_instr("LLCH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 764);
    add_machine_instr("LLHR", mach_format::RRE, { reg_4_U, reg_4_U }, 764);
    add_machine_instr("LLGHR", mach_format::RRE, { reg_4_U, reg_4_U }, 764);
    add_machine_instr("LLH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 764);
    add_machine_instr("LLGH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 764);
    add_machine_instr("LLHRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 764);
    add_machine_instr("LLGHRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 764);
    add_machine_instr("LLHH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 765);
    add_machine_instr("LLIHF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 765);
    add_machine_instr("LLIHH", mach_format::RI_a, { reg_4_U, imm_16_U }, 765);
    add_machine_instr("LLIHL", mach_format::RI_a, { reg_4_U, imm_16_U }, 765);
    add_machine_instr("LLILF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 765);
    add_machine_instr("LLILH", mach_format::RI_a, { reg_4_U, imm_16_U }, 765);
    add_machine_instr("LLILL", mach_format::RI_a, { reg_4_U, imm_16_U }, 765);
    add_machine_instr("LLGTR", mach_format::RRE, { reg_4_U, reg_4_U }, 765);
    add_machine_instr("LLGT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 766);
    add_machine_instr("LLGTAT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 766);
    add_machine_instr("LM", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 766);
    add_machine_instr("LMY", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 766);
    add_machine_instr("LMG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 766);
    add_machine_instr("LMD", mach_format::SS_e, { reg_4_U, reg_4_U, db_12_4_U, db_12_4_U }, 767);
    add_machine_instr("LMH", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 767);
    add_machine_instr("LNR", mach_format::RR, { reg_4_U, reg_4_U }, 767);
    add_machine_instr("LNGR", mach_format::RRE, { reg_4_U, reg_4_U }, 767);
    add_machine_instr("LNGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 768);
    add_machine_instr("LOCFHR", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 768);
    add_machine_instr("LOCFH", mach_format::RSY_b, { reg_4_U, db_20_4_S, mask_4_U }, 768);
    add_machine_instr("LOCR", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 768);
    add_machine_instr("LOCGR", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 768);
    add_machine_instr("LOC", mach_format::RSY_b, { reg_4_U, db_20_4_S, mask_4_U }, 768);
    add_machine_instr("LOCG", mach_format::RSY_b, { reg_4_U, db_20_4_S, mask_4_U }, 768);
    add_machine_instr("LPD", mach_format::SSF, { reg_4_U, db_12_4_U, db_12_4_U }, 769);
    add_machine_instr("LPDG", mach_format::SSF, { reg_4_U, db_12_4_U, db_12_4_U }, 769);
    add_machine_instr("LPQ", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 770);
    add_machine_instr("LPR", mach_format::RR, { reg_4_U, reg_4_U }, 771);
    add_machine_instr("LPGR", mach_format::RRE, { reg_4_U, reg_4_U }, 771);
    add_machine_instr("LPGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 771);
    add_machine_instr("LRVR", mach_format::RRE, { reg_4_U, reg_4_U }, 771);
    add_machine_instr("LRVGR", mach_format::RRE, { reg_4_U, reg_4_U }, 771);
    add_machine_instr("LRVH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 771);
    add_machine_instr("LRV", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 771);
    add_machine_instr("LRVG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 771);
    add_machine_instr("MC", mach_format::SI, { db_12_4_U, imm_8_S }, 772);
    add_machine_instr("MVC", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 773);
    add_machine_instr("MVCRL", mach_format::SSE, { db_12_4_U, db_12_4_U }, 788);
    add_machine_instr("MVHHI", mach_format::SIL, { db_12_4_U, imm_16_S }, 773);
    add_machine_instr("MVHI", mach_format::SIL, { db_12_4_U, imm_16_S }, 773);
    add_machine_instr("MVGHI", mach_format::SIL, { db_12_4_U, imm_16_S }, 773);
    add_machine_instr("MVI", mach_format::SI, { db_12_4_U, imm_8_U }, 773);
    add_machine_instr("MVIY", mach_format::SIY, { db_12_4_U, imm_8_U }, 773);
    add_machine_instr("MVCIN", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 774);
    add_machine_instr("MVCL", mach_format::RR, { reg_4_U, reg_4_U }, 774);
    add_machine_instr("MVCLE", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 778);
    add_machine_instr("MVCLU", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 781);
    add_machine_instr("MVN", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 785);
    add_machine_instr("MVST", mach_format::RRE, { reg_4_U, reg_4_U }, 785);
    add_machine_instr("MVO", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 786);
    add_machine_instr("MVZ", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 787);
    add_machine_instr("MR", mach_format::RR, { reg_4_U, reg_4_U }, 788);
    add_machine_instr("MGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 788);
    add_machine_instr("M", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 788);
    add_machine_instr("MFY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 788);
    add_machine_instr("MG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 788);
    add_machine_instr("MH", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 789);
    add_machine_instr("MHY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 789);
    add_machine_instr("MGH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 789);
    add_machine_instr("MHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 789);
    add_machine_instr("MGHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 789);
    add_machine_instr("MLR", mach_format::RRE, { reg_4_U, reg_4_U }, 790);
    add_machine_instr("MLGR", mach_format::RRE, { reg_4_U, reg_4_U }, 790);
    add_machine_instr("ML", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 790);
    add_machine_instr("MLG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 790);
    add_machine_instr("MSR", mach_format::RRE, { reg_4_U, reg_4_U }, 791);
    add_machine_instr("MSRKC", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 791);
    add_machine_instr("MSGR", mach_format::RRE, { reg_4_U, reg_4_U }, 791);
    add_machine_instr("MSGRKC", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 791);
    add_machine_instr("MSGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 791);
    add_machine_instr("MS", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 791);
    add_machine_instr("MSC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 791);
    add_machine_instr("MSY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 791);
    add_machine_instr("MSG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 791);
    add_machine_instr("MSGC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 791);
    add_machine_instr("MSGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 791);
    add_machine_instr("MSFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 791);
    add_machine_instr("MSGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 791);
    add_machine_instr("NIAI", mach_format::IE, { imm_4_U, imm_4_U }, 792);
    add_machine_instr("NTSTG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 794);
    add_machine_instr("NCGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 522);
    add_machine_instr("NCRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 522);
    add_machine_instr("NNRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 796);
    add_machine_instr("NNGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 796);
    add_machine_instr("NOGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 799);
    add_machine_instr("NORK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 799);
    add_machine_instr("NXRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 799);
    add_machine_instr("NXGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 799);
    add_machine_instr("OR", mach_format::RR, { reg_4_U, reg_4_U }, 794);
    add_machine_instr("OGR", mach_format::RRE, { reg_4_U, reg_4_U }, 794);
    add_machine_instr("ORK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 794);
    add_machine_instr("OCGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 802);
    add_machine_instr("OCRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 802);
    add_machine_instr("OGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 794);
    add_machine_instr("O", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 794);
    add_machine_instr("OY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 794);
    add_machine_instr("OG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 795);
    add_machine_instr("OI", mach_format::SI, { db_12_4_U, imm_8_U }, 795);
    add_machine_instr("OIY", mach_format::SIY, { db_20_4_S, imm_8_U }, 795);
    add_machine_instr("OC", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 795);
    add_machine_instr("OIHF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 796);
    add_machine_instr("OIHH", mach_format::RI_a, { reg_4_U, imm_16_U }, 796);
    add_machine_instr("OIHL", mach_format::RI_a, { reg_4_U, imm_16_U }, 796);
    add_machine_instr("OILF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 796);
    add_machine_instr("OILH", mach_format::RI_a, { reg_4_U, imm_16_U }, 796);
    add_machine_instr("OILL", mach_format::RI_a, { reg_4_U, imm_16_U }, 796);
    add_machine_instr("PACK", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 796);
    add_machine_instr("PKA", mach_format::SS_f, { db_12_4_U, db_12_8x4L_U }, 797);
    add_machine_instr("PKU", mach_format::SS_f, { db_12_4_U, db_12_8x4L_U }, 798);
    add_machine_instr("PCC", mach_format::RRE, {}, 799);
    add_machine_instr("PLO", mach_format::SS_e, { reg_4_U, db_12_4_U, reg_4_U, db_12_4_U }, 815);
    add_machine_instr("PPA", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 829);
    add_machine_instr("PRNO", mach_format::RRE, { reg_4_U, reg_4_U }, 830);
    add_machine_instr("PPNO", mach_format::RRE, { reg_4_U, reg_4_U }, 830);
    add_machine_instr("POPCNT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 853);
    add_machine_instr("PFD", mach_format::RXY_b, { mask_4_U, dxb_20_4x4_S }, 843);
    add_machine_instr("PFDRL", mach_format::RIL_c, { mask_4_U, rel_addr_imm_32_S }, 843);
    add_machine_instr("RLL", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 845);
    add_machine_instr("RLLG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 845);
    add_machine_instr("RNSBG", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 845);
    add_machine_instr("RXSBG", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 846);
    add_machine_instr("ROSBG", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 846);
    add_machine_instr("RISBG", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 847);
    add_machine_instr("RISBGN", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 847);
    add_machine_instr("RISBHG", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 848);
    add_machine_instr("RISBLG", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 849);
    add_machine_instr("RNSBGT", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 845);
    add_machine_instr("RXSBGT", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 846);
    add_machine_instr("ROSBGT", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 858);
    add_machine_instr("RISBGZ", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 858);
    add_machine_instr("RISBGNZ", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 860);
    add_machine_instr("RISBHGZ", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 860);
    add_machine_instr("RISBLGZ", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S_opt }, 860);
    add_machine_instr("SRST", mach_format::RRE, { reg_4_U, reg_4_U }, 850);
    add_machine_instr("SRSTU", mach_format::RRE, { reg_4_U, reg_4_U }, 852);
    add_machine_instr("SAR", mach_format::RRE, { reg_4_U, reg_4_U }, 854);
    add_machine_instr("SAM24", mach_format::E, {}, 854);
    add_machine_instr("SAM31", mach_format::E, {}, 854);
    add_machine_instr("SAM64", mach_format::E, {}, 854);
    add_machine_instr("SPM", mach_format::RR, { reg_4_U }, 855);
    add_machine_instr("SLDA", mach_format::RS_a, { reg_4_U, db_12_4_U }, 855);
    add_machine_instr("SLA", mach_format::RS_a, { reg_4_U, db_12_4_U }, 856);
    add_machine_instr("SLAK", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 856);
    add_machine_instr("SLAG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 856);
    add_machine_instr("SLDL", mach_format::RS_a, { reg_4_U, db_12_4_U }, 856);
    add_machine_instr("SLL", mach_format::RS_a, { reg_4_U, db_12_4_U }, 857);
    add_machine_instr("SLLK", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 857);
    add_machine_instr("SLLG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 857);
    add_machine_instr("SRDA", mach_format::RS_a, { reg_4_U, db_12_4_U }, 858);
    add_machine_instr("SRDL", mach_format::RS_a, { reg_4_U, db_12_4_U }, 858);
    add_machine_instr("SRA", mach_format::RS_a, { reg_4_U, db_12_4_U }, 859);
    add_machine_instr("SRAK", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 859);
    add_machine_instr("SRAG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 859);
    add_machine_instr("SRL", mach_format::RS_a, { reg_4_U, db_12_4_U }, 860);
    add_machine_instr("SRLK", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 860);
    add_machine_instr("SRLG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 860);
    add_machine_instr("ST", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 860);
    add_machine_instr("STY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 861);
    add_machine_instr("STG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 861);
    add_machine_instr("STRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 861);
    add_machine_instr("STGRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 861);
    add_machine_instr("STAM", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 861);
    add_machine_instr("STAMY", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 861);
    add_machine_instr("STC", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 862);
    add_machine_instr("STCY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 862);
    add_machine_instr("STCH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 862);
    add_machine_instr("STCM", mach_format::RS_b, { reg_4_U, mask_4_U, db_12_4_U }, 862);
    add_machine_instr("STCMY", mach_format::RSY_b, { reg_4_U, mask_4_U, db_20_4_S }, 862);
    add_machine_instr("STCMH", mach_format::RSY_b, { reg_4_U, mask_4_U, db_20_4_S }, 862);
    add_machine_instr("STCK", mach_format::S, { db_12_4_U }, 863);
    add_machine_instr("STCKF", mach_format::S, { db_12_4_U }, 863);
    add_machine_instr("STCKE", mach_format::S, { db_12_4_U }, 864);
    add_machine_instr("STFLE", mach_format::S, { db_20_4_S }, 866);
    add_machine_instr("STGSC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 867);
    add_machine_instr("STH", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 867);
    add_machine_instr("STHY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 868);
    add_machine_instr("STHRL", mach_format::RIL_b, { reg_4_U, rel_addr_imm_32_S }, 868);
    add_machine_instr("STHH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 868);
    add_machine_instr("STFH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 868);
    add_machine_instr("STM", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 869);
    add_machine_instr("STMY", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 869);
    add_machine_instr("STMG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 869);
    add_machine_instr("STMH", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 869);
    add_machine_instr("STOC", mach_format::RSY_b, { reg_4_U, db_20_4_S, mask_4_U }, 869);
    add_machine_instr("STOCG", mach_format::RSY_b, { reg_4_U, db_20_4_S, mask_4_U }, 869);
    add_machine_instr("STOCFH", mach_format::RSY_b, { reg_4_U, db_20_4_S, mask_4_U }, 870);
    add_machine_instr("STPQ", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 870);
    add_machine_instr("STRVH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 871);
    add_machine_instr("STRV", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 871);
    add_machine_instr("STRVG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 871);
    add_machine_instr("SR", mach_format::RR, { reg_4_U, reg_4_U }, 871);
    add_machine_instr("SGR", mach_format::RRE, { reg_4_U, reg_4_U }, 871);
    add_machine_instr("SGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 871);
    add_machine_instr("SRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 871);
    add_machine_instr("SGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 872);
    add_machine_instr("S", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 872);
    add_machine_instr("SY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 872);
    add_machine_instr("SG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 872);
    add_machine_instr("SGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 872);
    add_machine_instr("SH", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 872);
    add_machine_instr("SHY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 872);
    add_machine_instr("SGH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 872);
    add_machine_instr("SHHHR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 873);
    add_machine_instr("SHHLR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 873);
    add_machine_instr("SLR", mach_format::RR, { reg_4_U, reg_4_U }, 873);
    add_machine_instr("SLGR", mach_format::RRE, { reg_4_U, reg_4_U }, 873);
    add_machine_instr("SLGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 873);
    add_machine_instr("SLRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 873);
    add_machine_instr("SLGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 873);
    add_machine_instr("SL", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 874);
    add_machine_instr("SLY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 874);
    add_machine_instr("SLG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 874);
    add_machine_instr("SLGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 874);
    add_machine_instr("SLFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 874);
    add_machine_instr("SLGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 874);
    add_machine_instr("SLHHHR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 875);
    add_machine_instr("SLHHLR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 875);
    add_machine_instr("SLBR", mach_format::RRE, { reg_4_U, reg_4_U }, 875);
    add_machine_instr("SLBGR", mach_format::RRE, { reg_4_U, reg_4_U }, 875);
    add_machine_instr("SLB", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 875);
    add_machine_instr("SLBG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 875);
    add_machine_instr("SVC", mach_format::I, { imm_8_U }, 876);
    add_machine_instr("TS", mach_format::SI, { db_12_4_U }, 876);
    add_machine_instr("TAM", mach_format::E, {}, 876);
    add_machine_instr("TM", mach_format::SI, { db_12_4_U, imm_8_U }, 877);
    add_machine_instr("TMY", mach_format::SIY, { db_20_4_S, imm_8_U }, 877);
    add_machine_instr("TMHH", mach_format::RI_a, { reg_4_U, imm_16_U }, 877);
    add_machine_instr("TMHL", mach_format::RI_a, { reg_4_U, imm_16_U }, 877);
    add_machine_instr("TMH", mach_format::RI_a, { reg_4_U, imm_16_U }, 877);
    add_machine_instr("TMLH", mach_format::RI_a, { reg_4_U, imm_16_U }, 877);
    add_machine_instr("TML", mach_format::RI_a, { reg_4_U, imm_16_U }, 877);
    add_machine_instr("TMLL", mach_format::RI_a, { reg_4_U, imm_16_U }, 877);
    add_machine_instr("TABORT", mach_format::S, { db_12_4_U }, 878);
    add_machine_instr("TBEGIN", mach_format::SIL, { db_12_4_U, imm_16_S }, 879);
    add_machine_instr("TBEGINC", mach_format::SIL, { db_12_4_U, imm_16_S }, 883);
    add_machine_instr("TEND", mach_format::S, {}, 885);
    add_machine_instr("TR", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 886);
    add_machine_instr("TRT", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 887);
    add_machine_instr("TRTE", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 887);
    add_machine_instr("TRTRE", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 888);
    add_machine_instr("TRTR", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 892);
    add_machine_instr("TRE", mach_format::RRE, { reg_4_U, reg_4_U }, 893);
    add_machine_instr("TROO", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 895);
    add_machine_instr("TROT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 895);
    add_machine_instr("TRTO", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 895);
    add_machine_instr("TRTT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 895);
    add_machine_instr("UNPK", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 900);
    add_machine_instr("UNPKA", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 901);
    add_machine_instr("UNPKU", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 902);
    add_machine_instr("UPT", mach_format::E, {}, 903);
    add_machine_instr("AP", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 920);
    add_machine_instr("CP", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 921);
    add_machine_instr("DP", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 921);
    add_machine_instr("DFLTCC", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 1714);
    add_machine_instr("ED", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 922);
    add_machine_instr("EDMK", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 925);
    add_machine_instr("SRP", mach_format::SS_c, { db_12_4x4L_U, db_12_4_U, imm_4_U }, 926);
    add_machine_instr("MP", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 926);
    add_machine_instr("SP", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 927);
    add_machine_instr("TP", mach_format::RSL_a, { db_12_4x4L_U }, 928);
    add_machine_instr("ZAP", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 928);
    add_machine_instr("THDR", mach_format::RRE, { reg_4_U, reg_4_U }, 955);
    add_machine_instr("THDER", mach_format::RRE, { reg_4_U, reg_4_U }, 955);
    add_machine_instr("TBEDR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 956);
    add_machine_instr("TBDR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 956);
    add_machine_instr("CPSDR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U }, 958);
    add_machine_instr("EFPC", mach_format::RRE, { reg_4_U }, 958);
    add_machine_instr("LER", mach_format::RR, { reg_4_U, reg_4_U }, 959);
    add_machine_instr("LDR", mach_format::RR, { reg_4_U, reg_4_U }, 959);
    add_machine_instr("LXR", mach_format::RRE, { reg_4_U, reg_4_U }, 959);
    add_machine_instr("LE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 959);
    add_machine_instr("LD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 959);
    add_machine_instr("LEY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 959);
    add_machine_instr("LDY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 959);
    add_machine_instr("LCDFR", mach_format::RRE, { reg_4_U, reg_4_U }, 959);
    add_machine_instr("LFPC", mach_format::S, { db_12_4_U }, 959);
    add_machine_instr("LFAS", mach_format::S, { db_12_4_U }, 960);
    add_machine_instr("LDGR", mach_format::RRE, { reg_4_U, reg_4_U }, 962);
    add_machine_instr("LGDR", mach_format::RRE, { reg_4_U, reg_4_U }, 962);
    add_machine_instr("LNDFR", mach_format::RRE, { reg_4_U, reg_4_U }, 962);
    add_machine_instr("LPDFR", mach_format::RRE, { reg_4_U, reg_4_U }, 962);
    add_machine_instr("LZER", mach_format::RRE, { reg_4_U }, 963);
    add_machine_instr("LZXR", mach_format::RRE, { reg_4_U }, 963);
    add_machine_instr("LZDR", mach_format::RRE, { reg_4_U }, 963);
    add_machine_instr("PFPO", mach_format::E, {}, 963);
    add_machine_instr("SRNM", mach_format::S, { db_12_4_U }, 975);
    add_machine_instr("SRNMB", mach_format::S, { db_12_4_U }, 975);
    add_machine_instr("SRNMT", mach_format::S, { db_12_4_U }, 975);
    add_machine_instr("SFPC", mach_format::RRE, { reg_4_U }, 975);
    add_machine_instr("SFASR", mach_format::RRE, { reg_4_U }, 976);
    add_machine_instr("STE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 976);
    add_machine_instr("STD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 976);
    add_machine_instr("STDY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 977);
    add_machine_instr("STEY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 977);
    add_machine_instr("STFPC", mach_format::S, { db_12_4_U }, 977);
    add_machine_instr("BSA", mach_format::RRE, { reg_4_U, reg_4_U }, 989);
    add_machine_instr("BAKR", mach_format::RRE, { reg_4_U, reg_4_U }, 993);
    add_machine_instr("BSG", mach_format::RRE, { reg_4_U, reg_4_U }, 995);
    add_machine_instr("CRDTE", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U_opt }, 999);
    add_machine_instr("CSP", mach_format::RRE, { reg_4_U, reg_4_U }, 1003);
    add_machine_instr("CSPG", mach_format::RRE, { reg_4_U, reg_4_U }, 1003);
    add_machine_instr("ESEA", mach_format::RRE, { reg_4_U }, 1006);
    add_machine_instr("EPAR", mach_format::RRE, { reg_4_U }, 1006);
    add_machine_instr("EPAIR", mach_format::RRE, { reg_4_U }, 1006);
    add_machine_instr("ESAR", mach_format::RRE, { reg_4_U }, 1006);
    add_machine_instr("ESAIR", mach_format::RRE, { reg_4_U }, 1007);
    add_machine_instr("EREG", mach_format::RRE, { reg_4_U, reg_4_U }, 1007);
    add_machine_instr("EREGG", mach_format::RRE, { reg_4_U, reg_4_U }, 1007);
    add_machine_instr("ESTA", mach_format::RRE, { reg_4_U, reg_4_U }, 1008);
    add_machine_instr("IAC", mach_format::RRE, { reg_4_U }, 1011);
    add_machine_instr("IPK", mach_format::S, {}, 1012);
    add_machine_instr("IRBM", mach_format::RRE, { reg_4_U, reg_4_U }, 1012);
    add_machine_instr("ISK", mach_format::RR, { reg_4_U, reg_4_U }, 268);
    add_machine_instr("ISKE", mach_format::RRE, { reg_4_U, reg_4_U }, 1012);
    add_machine_instr("IVSK", mach_format::RRE, { reg_4_U, reg_4_U }, 1013);
    add_machine_instr("IDTE", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U_opt }, 1014);
    add_machine_instr("IPTE", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U_opt, mask_4_U_opt }, 1019);
    add_machine_instr("LASP", mach_format::SSE, { db_12_4_U, db_12_4_U }, 1023);
    add_machine_instr("LCTL", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 1032);
    add_machine_instr("LCTLG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 1032);
    add_machine_instr("LPTEA", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1032);
    add_machine_instr("LPSW", mach_format::SI, { db_12_4_U }, 1036);
    add_machine_instr("LPSWE", mach_format::S, { db_12_4_U }, 1037);
    add_machine_instr("LRA", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1038);
    add_machine_instr("LRAY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 1038);
    add_machine_instr("LRAG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 1038);
    add_machine_instr("LURA", mach_format::RRE, { reg_4_U, reg_4_U }, 1042);
    add_machine_instr("LURAG", mach_format::RRE, { reg_4_U, reg_4_U }, 1042);
    add_machine_instr("MSTA", mach_format::RRE, { reg_4_U }, 1043);
    add_machine_instr("MVPG", mach_format::RRE, { reg_4_U, reg_4_U }, 1044);
    add_machine_instr("MVCP", mach_format::SS_d, { drb_12_4x4_U, db_12_4_U, reg_4_U }, 1046);
    add_machine_instr("MVCS", mach_format::SS_d, { drb_12_4x4_U, db_12_4_U, reg_4_U }, 1046);
    add_machine_instr("MVCDK", mach_format::SSE, { db_12_4_U, db_12_4_U }, 1048);
    add_machine_instr("MVCK", mach_format::SS_d, { drb_12_4x4_U, db_12_4_U, reg_4_U }, 1049);
    add_machine_instr("MVCOS", mach_format::SSF, { db_12_4_U, db_12_4_U, reg_4_U }, 1050);
    add_machine_instr("MVCSK", mach_format::SSE, { db_12_4_U, db_12_4_U }, 1053);
    add_machine_instr("PGIN", mach_format::RRE, { reg_4_U, reg_4_U }, 1054);
    add_machine_instr("PGOUT", mach_format::RRE, { reg_4_U, reg_4_U }, 1055);
    add_machine_instr("PCKMO", mach_format::RRE, {}, 1056);
    add_machine_instr("PFMF", mach_format::RRE, { reg_4_U, reg_4_U }, 1059);
    add_machine_instr("PTFF", mach_format::E, {}, 1063);
    add_machine_instr("PTF", mach_format::RRE, { reg_4_U }, 1071);
    add_machine_instr("PC", mach_format::S, { db_12_4_U }, 1072);
    add_machine_instr("PR", mach_format::E, {}, 1085);
    add_machine_instr("PTI", mach_format::RRE, { reg_4_U, reg_4_U }, 1089);
    add_machine_instr("PT", mach_format::RRE, { reg_4_U, reg_4_U }, 1089);
    add_machine_instr("PALB", mach_format::RRE, {}, 1098);
    add_machine_instr("PTLB", mach_format::S, {}, 1098);
    add_machine_instr("RRBE", mach_format::RRE, { reg_4_U, reg_4_U }, 1098);
    add_machine_instr("RRBM", mach_format::RRE, { reg_4_U, reg_4_U }, 1099);
    add_machine_instr("RP", mach_format::S, { db_12_4_U }, 1099);
    add_machine_instr("SAC", mach_format::S, { db_12_4_U }, 1102);
    add_machine_instr("SACF", mach_format::S, { db_12_4_U }, 1102);
    add_machine_instr("SCK", mach_format::S, { db_12_4_U }, 1103);
    add_machine_instr("SCKC", mach_format::S, { db_12_4_U }, 1104);
    add_machine_instr("SCKPF", mach_format::E, {}, 1105);
    add_machine_instr("SPX", mach_format::S, { db_12_4_U }, 1105);
    add_machine_instr("SPT", mach_format::S, { db_12_4_U }, 1105);
    add_machine_instr("SPKA", mach_format::S, { db_12_4_U }, 1106);
    add_machine_instr("SSAR", mach_format::RRE, { reg_4_U }, 1107);
    add_machine_instr("SSAIR", mach_format::RRE, { reg_4_U }, 1107);
    add_machine_instr("SSK", mach_format::RR, { reg_4_U, reg_4_U }, 304);
    add_machine_instr("SSKE", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U_opt }, 1112);
    add_machine_instr("SSM", mach_format::SI, { db_12_4_U }, 1115);
    add_machine_instr("SIGP", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 1115);
    add_machine_instr("STCKC", mach_format::S, { db_12_4_U }, 1117);
    add_machine_instr("STCTL", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 1117);
    add_machine_instr("STCTG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 1117);
    add_machine_instr("STAP", mach_format::S, { db_12_4_U }, 1118);
    add_machine_instr("STIDP", mach_format::S, { db_12_4_U }, 1118);
    add_machine_instr("STPT", mach_format::S, { db_12_4_U }, 1120);
    add_machine_instr("STFL", mach_format::S, { db_12_4_U }, 1120);
    add_machine_instr("STPX", mach_format::S, { db_12_4_U }, 1121);
    add_machine_instr("STRAG", mach_format::SSE, { db_12_4_U, db_12_4_U }, 1121);
    add_machine_instr("STSI", mach_format::S, { db_12_4_U }, 1122);
    add_machine_instr("STOSM", mach_format::SI, { db_12_4_U, imm_8_U }, 1146);
    add_machine_instr("STNSM", mach_format::SI, { db_12_4_U, imm_8_U }, 1146);
    add_machine_instr("STURA", mach_format::RRE, { reg_4_U, reg_4_U }, 1147);
    add_machine_instr("STURG", mach_format::RRE, { reg_4_U, reg_4_U }, 1147);
    add_machine_instr("TAR", mach_format::RRE, { reg_4_U, reg_4_U }, 1147);
    add_machine_instr("TB", mach_format::RRE, { reg_4_U, reg_4_U }, 1149);
    add_machine_instr("TPEI", mach_format::RRE, { reg_4_U, reg_4_U }, 1151);
    add_machine_instr("TPROT", mach_format::SSE, { db_12_4_U, db_12_4_U }, 1152);
    add_machine_instr("TRACE", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 1155);
    add_machine_instr("TRACG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 1155);
    add_machine_instr("TRAP2", mach_format::E, {}, 1156);
    add_machine_instr("TRAP4", mach_format::S, { db_12_4_U }, 1156);
    add_machine_instr("XSCH", mach_format::S, {}, 1215);
    add_machine_instr("CSCH", mach_format::S, {}, 1217);
    add_machine_instr("HSCH", mach_format::S, {}, 1218);
    add_machine_instr("MSCH", mach_format::S, { db_12_4_U }, 1219);
    add_machine_instr("RCHP", mach_format::S, {}, 1221);
    add_machine_instr("RSCH", mach_format::S, {}, 1222);
    add_machine_instr("SAL", mach_format::S, {}, 1224);
    add_machine_instr("SCHM", mach_format::S, {}, 1225);
    add_machine_instr("SSCH", mach_format::S, { db_12_4_U }, 1227);
    add_machine_instr("STCPS", mach_format::S, { db_12_4_U }, 1228);
    add_machine_instr("STCRW", mach_format::S, { db_12_4_U }, 1229);
    add_machine_instr("STSCH", mach_format::S, { db_12_4_U }, 1230);
    add_machine_instr("TPI", mach_format::S, { db_12_4_U }, 1231);
    add_machine_instr("TSCH", mach_format::S, { db_12_4_U }, 1232);

    add_machine_instr("AER", mach_format::RR, { reg_4_U, reg_4_U }, 1412);
    add_machine_instr("ADR", mach_format::RR, { reg_4_U, reg_4_U }, 1412);
    add_machine_instr("AXR", mach_format::RR, { reg_4_U, reg_4_U }, 1412);
    add_machine_instr("AE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1412);
    add_machine_instr("AD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1412);
    add_machine_instr("AWR", mach_format::RR, { reg_4_U, reg_4_U }, 1413);
    add_machine_instr("AUR", mach_format::RR, { reg_4_U, reg_4_U }, 1413);
    add_machine_instr("AU", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1413);
    add_machine_instr("AW", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1413);
    add_machine_instr("CER", mach_format::RR, { reg_4_U, reg_4_U }, 1414);
    add_machine_instr("CDR", mach_format::RR, { reg_4_U, reg_4_U }, 1414);
    add_machine_instr("CXR", mach_format::RRE, { reg_4_U, reg_4_U }, 1414);
    add_machine_instr("CE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1414);
    add_machine_instr("CD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1414);
    add_machine_instr("CEFR", mach_format::RRE, { reg_4_U, reg_4_U }, 1415);
    add_machine_instr("CDFR", mach_format::RRE, { reg_4_U, reg_4_U }, 1415);
    add_machine_instr("CXFR", mach_format::RRE, { reg_4_U, reg_4_U }, 1415);
    add_machine_instr("CEGR", mach_format::RRE, { reg_4_U, reg_4_U }, 1415);
    add_machine_instr("CDGR", mach_format::RRE, { reg_4_U, reg_4_U }, 1415);
    add_machine_instr("CXGR", mach_format::RRE, { reg_4_U, reg_4_U }, 1415);
    add_machine_instr("CFER", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1415);
    add_machine_instr("CFDR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1415);
    add_machine_instr("CFXR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1415);
    add_machine_instr("CGER", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1415);
    add_machine_instr("CGDR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1415);
    add_machine_instr("CGXR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1415);
    add_machine_instr("DDR", mach_format::RR, { reg_4_U, reg_4_U }, 1416);
    add_machine_instr("DER", mach_format::RR, { reg_4_U, reg_4_U }, 1416);
    add_machine_instr("DXR", mach_format::RRE, { reg_4_U, reg_4_U }, 1416);
    add_machine_instr("DD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1416);
    add_machine_instr("DE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1416);
    add_machine_instr("HDR", mach_format::RR, { reg_4_U, reg_4_U }, 1417);
    add_machine_instr("HER", mach_format::RR, { reg_4_U, reg_4_U }, 1417);
    add_machine_instr("LTER", mach_format::RR, { reg_4_U, reg_4_U }, 1417);
    add_machine_instr("LTDR", mach_format::RR, { reg_4_U, reg_4_U }, 1417);
    add_machine_instr("LTXR", mach_format::RRE, { reg_4_U, reg_4_U }, 1418);
    add_machine_instr("LCER", mach_format::RR, { reg_4_U, reg_4_U }, 1418);
    add_machine_instr("LCDR", mach_format::RR, { reg_4_U, reg_4_U }, 1418);
    add_machine_instr("LCXR", mach_format::RRE, { reg_4_U, reg_4_U }, 1418);
    add_machine_instr("FIER", mach_format::RRE, { reg_4_U, reg_4_U }, 1419);
    add_machine_instr("FIDR", mach_format::RRE, { reg_4_U, reg_4_U }, 1419);
    add_machine_instr("FIXR", mach_format::RRE, { reg_4_U, reg_4_U }, 1419);
    add_machine_instr("LDER", mach_format::RRE, { reg_4_U, reg_4_U }, 1419);
    add_machine_instr("LXDR", mach_format::RRE, { reg_4_U, reg_4_U }, 1419);
    add_machine_instr("LXER", mach_format::RRE, { reg_4_U, reg_4_U }, 1419);
    add_machine_instr("LDE", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1419);
    add_machine_instr("LXD", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1419);
    add_machine_instr("LXE", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1419);
    add_machine_instr("LNDR", mach_format::RR, { reg_4_U, reg_4_U }, 1420);
    add_machine_instr("LNER", mach_format::RR, { reg_4_U, reg_4_U }, 1420);
    add_machine_instr("LPDR", mach_format::RR, { reg_4_U, reg_4_U }, 1420);
    add_machine_instr("LPER", mach_format::RR, { reg_4_U, reg_4_U }, 1420);
    add_machine_instr("LNXR", mach_format::RRE, { reg_4_U, reg_4_U }, 1420);
    add_machine_instr("LPXR", mach_format::RRE, { reg_4_U, reg_4_U }, 1420);
    add_machine_instr("LEDR", mach_format::RR, { reg_4_U, reg_4_U }, 1421);
    add_machine_instr("LDXR", mach_format::RR, { reg_4_U, reg_4_U }, 1421);
    add_machine_instr("LRER", mach_format::RR, { reg_4_U, reg_4_U }, 1421);
    add_machine_instr("LRDR", mach_format::RR, { reg_4_U, reg_4_U }, 1421);
    add_machine_instr("LEXR", mach_format::RRE, { reg_4_U, reg_4_U }, 1421);
    add_machine_instr("MEER", mach_format::RRE, { reg_4_U, reg_4_U }, 1421);
    add_machine_instr("MDR", mach_format::RR, { reg_4_U, reg_4_U }, 1421);
    add_machine_instr("MXR", mach_format::RR, { reg_4_U, reg_4_U }, 1421);
    add_machine_instr("MDER", mach_format::RR, { reg_4_U, reg_4_U }, 1421);
    add_machine_instr("MER", mach_format::RR, { reg_4_U, reg_4_U }, 1421);
    add_machine_instr("MXDR", mach_format::RR, { reg_4_U, reg_4_U }, 1421);
    add_machine_instr("MEE", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1422);
    add_machine_instr("MD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1422);
    add_machine_instr("MDE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1422);
    add_machine_instr("MXD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1422);
    add_machine_instr("ME", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1422);
    add_machine_instr("MAER", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1423);
    add_machine_instr("MADR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1423);
    add_machine_instr("MAD", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1423);
    add_machine_instr("MAE", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1423);
    add_machine_instr("MSER", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1423);
    add_machine_instr("MSDR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1423);
    add_machine_instr("MSE", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1423);
    add_machine_instr("MSD", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1423);
    add_machine_instr("MAYR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1424);
    add_machine_instr("MAYHR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1424);
    add_machine_instr("MAYLR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1424);
    add_machine_instr("MAY", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1424);
    add_machine_instr("MAYH", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1424);
    add_machine_instr("MAYL", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1424);
    add_machine_instr("MYR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1426);
    add_machine_instr("MYHR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1426);
    add_machine_instr("MYLR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1426);
    add_machine_instr("MY", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1426);
    add_machine_instr("MYH", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1426);
    add_machine_instr("MYL", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1426);
    add_machine_instr("SQER", mach_format::RRE, { reg_4_U, reg_4_U }, 1427);
    add_machine_instr("SQDR", mach_format::RRE, { reg_4_U, reg_4_U }, 1427);
    add_machine_instr("SQXR", mach_format::RRE, { reg_4_U, reg_4_U }, 1427);
    add_machine_instr("SQE", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1427);
    add_machine_instr("SQD", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1427);
    add_machine_instr("SER", mach_format::RR, { reg_4_U, reg_4_U }, 1428);
    add_machine_instr("SDR", mach_format::RR, { reg_4_U, reg_4_U }, 1428);
    add_machine_instr("SXR", mach_format::RR, { reg_4_U, reg_4_U }, 1428);
    add_machine_instr("SE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1428);
    add_machine_instr("SD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1428);
    add_machine_instr("SUR", mach_format::RR, { reg_4_U, reg_4_U }, 1429);
    add_machine_instr("SWR", mach_format::RR, { reg_4_U, reg_4_U }, 1429);
    add_machine_instr("SU", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1429);
    add_machine_instr("SW", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 1429);
    add_machine_instr("AEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1445);
    add_machine_instr("ADBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1445);
    add_machine_instr("AXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1445);
    add_machine_instr("AEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1445);
    add_machine_instr("ADB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1445);
    add_machine_instr("CEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1447);
    add_machine_instr("CDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1447);
    add_machine_instr("CXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1447);
    add_machine_instr("CDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1447);
    add_machine_instr("CEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1447);
    add_machine_instr("KEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1448);
    add_machine_instr("KDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1448);
    add_machine_instr("KDSA", mach_format::RRE, { reg_4_U, reg_4_U }, 1700);
    add_machine_instr("KXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1448);
    add_machine_instr("KDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1448);
    add_machine_instr("KEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1448);
    add_machine_instr("CEFBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1449);
    add_machine_instr("CDFBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1449);
    add_machine_instr("CXFBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1449);
    add_machine_instr("CEGBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1449);
    add_machine_instr("CDGBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1449);
    add_machine_instr("CXGBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1449);
    add_machine_instr("CEFBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1449);
    add_machine_instr("CDFBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1449);
    add_machine_instr("CXFBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1449);
    add_machine_instr("CEGBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1449);
    add_machine_instr("CDGBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1449);
    add_machine_instr("CXGBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1449);
    add_machine_instr("CELFBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1451);
    add_machine_instr("CDLFBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1451);
    add_machine_instr("CXLFBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1451);
    add_machine_instr("CELGBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1451);
    add_machine_instr("CDLGBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1451);
    add_machine_instr("CXLGBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1451);
    add_machine_instr("CFEBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1452);
    add_machine_instr("CFDBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1452);
    add_machine_instr("CFXBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1452);
    add_machine_instr("CGEBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1452);
    add_machine_instr("CGDBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1452);
    add_machine_instr("CGXBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1452);
    add_machine_instr("CFEBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1452);
    add_machine_instr("CFDBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1452);
    add_machine_instr("CFXBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1452);
    add_machine_instr("CGEBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1452);
    add_machine_instr("CGDBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1452);
    add_machine_instr("CGXBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1452);
    add_machine_instr("CLFEBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1455);
    add_machine_instr("CLFDBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1455);
    add_machine_instr("CLFXBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1455);
    add_machine_instr("CLGEBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1455);
    add_machine_instr("CLGDBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1455);
    add_machine_instr("CLGXBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1455);
    add_machine_instr("DEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1457);
    add_machine_instr("DDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1457);
    add_machine_instr("DXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1457);
    add_machine_instr("DEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1457);
    add_machine_instr("DDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1457);
    add_machine_instr("DIEBR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1458);
    add_machine_instr("DIDBR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1458);
    add_machine_instr("LTEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1461);
    add_machine_instr("LTDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1461);
    add_machine_instr("LTXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1461);
    add_machine_instr("LCEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1461);
    add_machine_instr("LCDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1461);
    add_machine_instr("LCXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1461);
    add_machine_instr("ECCTR", mach_format::RRE, { reg_4_U, reg_4_U }, 39);
    add_machine_instr("EPCTR", mach_format::RRE, { reg_4_U, reg_4_U }, 39);
    add_machine_instr("ECPGA", mach_format::RRE, { reg_4_U, reg_4_U }, 39);
    add_machine_instr("FIEBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1462);
    add_machine_instr("FIDBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1462);
    add_machine_instr("FIXBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1462);
    add_machine_instr("FIEBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1462);
    add_machine_instr("FIDBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1462);
    add_machine_instr("FIXBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1462);
    add_machine_instr("LSCTL", mach_format::S, { db_12_4_U }, 42);
    add_machine_instr("LDEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1463);
    add_machine_instr("LXDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1463);
    add_machine_instr("LPCTL", mach_format::S, { db_12_4_U }, 41);
    add_machine_instr("LCCTL", mach_format::S, { db_12_4_U }, 40);
    add_machine_instr("LXEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1463);
    add_machine_instr("LDEB", mach_format::RRE, { reg_4_U, reg_4_U }, 1464);
    add_machine_instr("LXDB", mach_format::RRE, { reg_4_U, reg_4_U }, 1464);
    add_machine_instr("LXEB", mach_format::RRE, { reg_4_U, reg_4_U }, 1464);
    add_machine_instr("LNEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1464);
    add_machine_instr("LNDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1464);
    add_machine_instr("LNXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1464);
    add_machine_instr("LPEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1465);
    add_machine_instr("LPDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1465);
    add_machine_instr("LPXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1465);
    add_machine_instr("LEDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1465);
    add_machine_instr("LDXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1465);
    add_machine_instr("LEXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1465);
    add_machine_instr("LEDBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1465);
    add_machine_instr("LDXBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1465);
    add_machine_instr("LEXBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1465);
    add_machine_instr("LPP", mach_format::S, { db_12_4_U }, 11);
    add_machine_instr("MEEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1467);
    add_machine_instr("MDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1467);
    add_machine_instr("MXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1467);
    add_machine_instr("MDEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1467);
    add_machine_instr("MXDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1467);
    add_machine_instr("MEEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1467);
    add_machine_instr("MDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1467);
    add_machine_instr("MDEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1467);
    add_machine_instr("MXDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1467);
    add_machine_instr("MADBR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1468);
    add_machine_instr("MAEBR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1468);
    add_machine_instr("MAEB", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1468);
    add_machine_instr("MADB", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1468);
    add_machine_instr("MSEBR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1468);
    add_machine_instr("MSDBR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 1468);
    add_machine_instr("MSEB", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1468);
    add_machine_instr("MSDB", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1468);
    add_machine_instr("QCTRI", mach_format::S, { db_12_4_U }, 43);
    add_machine_instr("QSI", mach_format::S, { db_12_4_U }, 45);
    add_machine_instr("SCCTR", mach_format::RRE, { reg_4_U, reg_4_U }, 46);
    add_machine_instr("SPCTR", mach_format::RRE, { reg_4_U, reg_4_U }, 47);
    add_machine_instr("SQEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1470);
    add_machine_instr("SQDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1470);
    add_machine_instr("SQXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1470);
    add_machine_instr("SQEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1470);
    add_machine_instr("SQDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1470);
    add_machine_instr("SEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1470);
    add_machine_instr("SDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1470);
    add_machine_instr("SXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 1470);
    add_machine_instr("SEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1470);
    add_machine_instr("SDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1470);
    add_machine_instr("SORTL", mach_format::RRE, { reg_4_U, reg_4_U }, 19);
    add_machine_instr("TCEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1471);
    add_machine_instr("TCDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1471);
    add_machine_instr("TCXB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1471);
    add_machine_instr("ADTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 1491);
    add_machine_instr("AXTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 1491);
    add_machine_instr("ADTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1491);
    add_machine_instr("AXTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1491);
    add_machine_instr("CDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1494);
    add_machine_instr("CXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1494);
    add_machine_instr("KDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1495);
    add_machine_instr("KXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1495);
    add_machine_instr("CEDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1495);
    add_machine_instr("CEXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1495);
    add_machine_instr("CDGTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1496);
    add_machine_instr("CXGTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1496);
    add_machine_instr("CDGTRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1496);
    add_machine_instr("CXGTRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1496);
    add_machine_instr("CDFTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1496);
    add_machine_instr("CXFTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1496);
    add_machine_instr("CDLGTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1497);
    add_machine_instr("CXLGTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1497);
    add_machine_instr("CDLFTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1497);
    add_machine_instr("CXLFTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1497);
    add_machine_instr("CDPT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 1498);
    add_machine_instr("CXPT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 1498);
    add_machine_instr("CDSTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1500);
    add_machine_instr("CXSTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1500);
    add_machine_instr("CDUTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1500);
    add_machine_instr("CXUTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1500);
    add_machine_instr("CDZT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 1501);
    add_machine_instr("CXZT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 1501);
    add_machine_instr("CGDTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1501);
    add_machine_instr("CGXTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 1501);
    add_machine_instr("CGDTRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1502);
    add_machine_instr("CGXTRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1502);
    add_machine_instr("CFDTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1502);
    add_machine_instr("CFXTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1502);
    add_machine_instr("CLGDTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1504);
    add_machine_instr("CLGXTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1504);
    add_machine_instr("CLFDTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1504);
    add_machine_instr("CLFXTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1504);
    add_machine_instr("CPDT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 1505);
    add_machine_instr("CPXT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 1505);
    add_machine_instr("CSDTR", mach_format::RRF_d, { reg_4_U, reg_4_U, mask_4_U }, 1507);
    add_machine_instr("CSXTR", mach_format::RRF_d, { reg_4_U, reg_4_U, mask_4_U }, 1507);
    add_machine_instr("CUDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1507);
    add_machine_instr("CUXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1507);
    add_machine_instr("CZDT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 1508);
    add_machine_instr("CZXT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 1508);
    add_machine_instr("DDTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 1509);
    add_machine_instr("DXTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 1509);
    add_machine_instr("DDTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1509);
    add_machine_instr("DXTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1509);
    add_machine_instr("EEXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1511);
    add_machine_instr("EEDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1511);
    add_machine_instr("ESDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1511);
    add_machine_instr("ESXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1511);
    add_machine_instr("IEDTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U }, 1512);
    add_machine_instr("IEXTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U }, 1512);
    add_machine_instr("LTDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1513);
    add_machine_instr("LTXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 1513);
    add_machine_instr("FIDTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1514);
    add_machine_instr("FIXTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1514);
    add_machine_instr("LDETR", mach_format::RRF_d, { reg_4_U, reg_4_U, mask_4_U }, 1517);
    add_machine_instr("LXDTR", mach_format::RRF_d, { reg_4_U, reg_4_U, mask_4_U }, 1517);
    add_machine_instr("LEDTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1518);
    add_machine_instr("LDXTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 1518);
    add_machine_instr("MDTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 1519);
    add_machine_instr("MXTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 1519);
    add_machine_instr("MDTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1520);
    add_machine_instr("MXTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1520);
    add_machine_instr("QADTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1521);
    add_machine_instr("QAXTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1521);
    add_machine_instr("RRDTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1524);
    add_machine_instr("RRXTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1524);
    add_machine_instr("SELFHR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 864);
    add_machine_instr("SELGR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 864);
    add_machine_instr("SELR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 864);
    add_machine_instr("SLDT", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1526);
    add_machine_instr("SLXT", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1526);
    add_machine_instr("SRDT", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1526);
    add_machine_instr("SRXT", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 1526);
    add_machine_instr("SDTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 1527);
    add_machine_instr("SXTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 1527);
    add_machine_instr("SDTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1527);
    add_machine_instr("SXTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1527);
    add_machine_instr("TDCET", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1528);
    add_machine_instr("TDCDT", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1528);
    add_machine_instr("TDCXT", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1528);
    add_machine_instr("TDGET", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1529);
    add_machine_instr("TDGDT", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1529);
    add_machine_instr("TDGXT", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 1529);
    add_machine_instr("VBPERM", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1536);
    add_machine_instr("VGEF", mach_format::VRV, { vec_reg_5_U, dvb_12_5x4_U, mask_4_U }, 1536);
    add_machine_instr("VCFPS", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1641);
    add_machine_instr("VCLFP", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1611);
    add_machine_instr("VGEG", mach_format::VRV, { vec_reg_5_U, dvb_12_5x4_U, mask_4_U }, 1536);
    add_machine_instr("VGBM", mach_format::VRI_a, { vec_reg_5_U, imm_16_U }, 1537);
    add_machine_instr("VGM", mach_format::VRI_b, { vec_reg_5_U, imm_8_U, imm_8_U, mask_4_U }, 1537);
    add_machine_instr("VL", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U_opt }, 1538);
    add_machine_instr("VSTEBRF", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1576);
    add_machine_instr("VSTEBRG", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1576);
    add_machine_instr("VLLEBRZ", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1562);
    add_machine_instr("VLREP", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1538);
    add_machine_instr("VLR", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U }, 1538);
    add_machine_instr("VLEB", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1538);
    add_machine_instr("VLEBRH", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1561);
    add_machine_instr("VLEBRG", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1561);
    add_machine_instr("VLBRREP", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1562);
    add_machine_instr("VLER", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1564);
    add_machine_instr("VLBR", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1563);
    add_machine_instr("VLEH", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1539);
    add_machine_instr("VLEIH", mach_format::VRI_a, { vec_reg_5_U, imm_16_S, mask_4_U }, 1539);
    add_machine_instr("VLEF", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1539);
    add_machine_instr("VLEIF", mach_format::VRI_a, { vec_reg_5_U, imm_16_S, mask_4_U }, 1539);
    add_machine_instr("VLEG", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1539);
    add_machine_instr("VLEIG", mach_format::VRI_a, { vec_reg_5_U, imm_16_S, mask_4_U }, 1539);
    add_machine_instr("VLEIB", mach_format::VRI_a, { vec_reg_5_U, imm_16_S, mask_4_U }, 1539);
    add_machine_instr("VLGV", mach_format::VRS_c, { reg_4_U, vec_reg_5_U, db_12_4_U, mask_4_U }, 1539);
    add_machine_instr("VLLEZ", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1540);
    add_machine_instr("VLM", mach_format::VRS_a, { vec_reg_5_U, vec_reg_5_U, db_12_4_U, mask_4_U_opt }, 1541);
    add_machine_instr("VLRLR", mach_format::VRS_d, { vec_reg_5_U, reg_4_U, db_12_4_U }, 1541);
    add_machine_instr("VLRL", mach_format::VSI, { vec_reg_5_U, db_12_4_U, imm_8_U }, 1541);
    add_machine_instr("VLBB", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1542);
    add_machine_instr("VLVG", mach_format::VRS_b, { vec_reg_5_U, reg_4_U, db_12_4_U, mask_4_U }, 1543);
    add_machine_instr("VLVGP", mach_format::VRR_f, { vec_reg_5_U, reg_4_U, reg_4_U }, 1543);
    add_machine_instr("VLL", mach_format::VRS_b, { vec_reg_5_U, reg_4_U, db_12_4_U }, 1543);
    add_machine_instr("VMRH", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1544);
    add_machine_instr("VMRL", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1544);
    add_machine_instr("VPK", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1545);
    add_machine_instr("VPKS", mach_format::VRR_b, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1545);
    add_machine_instr("VPKLS", mach_format::VRR_b, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1546);
    add_machine_instr("VPERM", mach_format::VRR_e, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1547);
    add_machine_instr("VPDI", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1547);
    add_machine_instr("VREP", mach_format::VRI_c, { vec_reg_5_U, vec_reg_5_U, imm_16_U, mask_4_U }, 1547);
    add_machine_instr("VREPI", mach_format::VRI_a, { vec_reg_5_U, imm_16_S, mask_4_U }, 1548);
    add_machine_instr("VSCEF", mach_format::VRV, { vec_reg_5_U, dvb_12_5x4_U, mask_4_U }, 1548);
    add_machine_instr("VSCEG", mach_format::VRV, { vec_reg_5_U, dvb_12_5x4_U, mask_4_U }, 1548);
    add_machine_instr("VSEL", mach_format::VRR_e, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1549);
    add_machine_instr("VSEG", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1549);
    add_machine_instr("VSTBR", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1576);
    add_machine_instr("VST", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U_opt }, 1550);
    add_machine_instr("VSTEB", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1550);
    add_machine_instr("VSTEBRH", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1576);
    add_machine_instr("VSTEH", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1550);
    add_machine_instr("VSTEF", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1550);
    add_machine_instr("VSTEG", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1550);
    add_machine_instr("VSTER", mach_format::VRX, { vec_reg_5_U, dxb_12_4x4_U, mask_4_U }, 1578);
    add_machine_instr("VSTM", mach_format::VRS_a, { vec_reg_5_U, vec_reg_5_U, db_12_4_U, mask_4_U_opt }, 1551);
    add_machine_instr("VSTRLR", mach_format::VRS_d, { vec_reg_5_U, reg_4_U, db_12_4_U }, 1551);
    add_machine_instr("VSTRL", mach_format::VSI, { vec_reg_5_U, db_12_4_U, imm_8_U }, 1551);
    add_machine_instr("VSTL", mach_format::VRS_b, { vec_reg_5_U, reg_4_U, db_12_4_U }, 1552);
    add_machine_instr("VUPH", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1552);
    add_machine_instr("VUPL", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1553);
    add_machine_instr("VUPLH", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1553);
    add_machine_instr("VUPLL", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1554);
    add_machine_instr("VA", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1557);
    add_machine_instr("VACC", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1558);
    add_machine_instr(
        "VAC", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1558);
    add_machine_instr(
        "VACCC", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1559);
    add_machine_instr("VN", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1559);
    add_machine_instr("VNC", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1559);
    add_machine_instr("VAVG", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1560);
    add_machine_instr("VAVGL", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1560);
    add_machine_instr("VCKSM", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1560);
    add_machine_instr("VEC", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1561);
    add_machine_instr("VECL", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1561);
    add_machine_instr("VCEQ", mach_format::VRR_b, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1561);
    add_machine_instr("VCH", mach_format::VRR_b, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1562);
    add_machine_instr("VCHL", mach_format::VRR_b, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1563);
    add_machine_instr("VCLZ", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1564);
    add_machine_instr("VCTZ", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1564);
    add_machine_instr("VGFM", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1565);
    add_machine_instr("VX", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1565);
    add_machine_instr("VLC", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1566);
    add_machine_instr(
        "VGFMA", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1566);
    add_machine_instr("VLP", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1566);
    add_machine_instr("VMX", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1567);
    add_machine_instr("VMXL", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1567);
    add_machine_instr("VMN", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1567);
    add_machine_instr("VMNL", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1568);
    add_machine_instr(
        "VMAL", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1568);
    add_machine_instr(
        "VMAH", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1569);
    add_machine_instr(
        "VMALH", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1569);
    add_machine_instr(
        "VMAE", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1569);
    add_machine_instr(
        "VMALE", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1569);
    add_machine_instr(
        "VMAO", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1570);
    add_machine_instr(
        "VMALO", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1570);
    add_machine_instr("VMH", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1570);
    add_machine_instr("VMLH", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1571);
    add_machine_instr("VML", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1571);
    add_machine_instr("VME", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1572);
    add_machine_instr("VMLE", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1572);
    add_machine_instr("VMO", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1572);
    add_machine_instr("VMLO", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1572);
    add_machine_instr(
        "VMSL", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1573);
    add_machine_instr("VNN", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1574);
    add_machine_instr("VNO", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1574);
    add_machine_instr("VNX", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1574);
    add_machine_instr("VO", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1574);
    add_machine_instr("VOC", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1575);
    add_machine_instr("VPOPCT", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1575);
    add_machine_instr("VERLLV", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1575);
    add_machine_instr("VERLL", mach_format::VRS_a, { vec_reg_5_U, vec_reg_5_U, db_12_4_U, mask_4_U }, 1575);
    add_machine_instr("VERIM", mach_format::VRI_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U }, 1576);
    add_machine_instr("VESLV", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1577);
    add_machine_instr("VESL", mach_format::VRS_a, { vec_reg_5_U, vec_reg_5_U, db_12_4_U, mask_4_U }, 1577);
    add_machine_instr("VESRAV", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1577);
    add_machine_instr("VESRA", mach_format::VRS_a, { vec_reg_5_U, vec_reg_5_U, db_12_4_U, mask_4_U }, 1577);
    add_machine_instr("VESRLV", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1578);
    add_machine_instr("VESRL", mach_format::VRS_a, { vec_reg_5_U, vec_reg_5_U, db_12_4_U, mask_4_U }, 1578);
    add_machine_instr("VSLD", mach_format::VRI_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U }, 1607);
    add_machine_instr("VSRD", mach_format::VRI_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U }, 1608);
    add_machine_instr("VSLDB", mach_format::VRI_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U }, 1579);
    add_machine_instr("VSL", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1579);
    add_machine_instr("VSLB", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1579);
    add_machine_instr("VSRA", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1579);
    add_machine_instr("VSRAB", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1580);
    add_machine_instr("VSRL", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1580);
    add_machine_instr("VSRLB", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U }, 1580);
    add_machine_instr("VS", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1580);
    add_machine_instr("VSCBI", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1581);
    add_machine_instr("VCSFP", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1644);
    add_machine_instr(
        "VSBI", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1581);
    add_machine_instr(
        "VSBCBI", mach_format::VRR_d, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1582);
    add_machine_instr("VSUMG", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1582);
    add_machine_instr("VSUMQ", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1583);
    add_machine_instr("VSUM", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1583);
    add_machine_instr("VTM", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U }, 1584);
    add_machine_instr(
        "VFAE", mach_format::VRR_b, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U_opt }, 1585);
    add_machine_instr(
        "VFEE", mach_format::VRR_b, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U_opt }, 1587);
    add_machine_instr(
        "VFENE", mach_format::VRR_b, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U_opt }, 1588);
    add_machine_instr("VISTR", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U_opt }, 1589);
    add_machine_instr("VSTRC",
        mach_format::VRR_d,
        { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U_opt },
        1590);
    add_machine_instr("VSTRS",
        mach_format::VRR_d,
        { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U_opt },
        1622);
    add_machine_instr("VFA", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1595);
    add_machine_instr("WFC", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1599);
    add_machine_instr("WFK", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1600);
    add_machine_instr(
        "VFCE", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1601);
    add_machine_instr(
        "VFCH", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1603);
    add_machine_instr(
        "VFCHE", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1605);
    add_machine_instr("VCFPS", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1607);
    add_machine_instr("VCFPL", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1643);
    add_machine_instr("VCLGD", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1611);
    add_machine_instr("VFD", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1613);
    add_machine_instr("VFI", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1615);
    add_machine_instr("VFLL", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1617);
    add_machine_instr("VFLR", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1618);
    add_machine_instr(
        "VFMAX", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1619);
    add_machine_instr(
        "VFMIN", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1625);
    add_machine_instr("VFM", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1631);
    add_machine_instr(
        "VFMA", mach_format::VRR_e, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1633);
    add_machine_instr(
        "VFMS", mach_format::VRR_e, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1633);
    add_machine_instr(
        "VFNMA", mach_format::VRR_e, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1633);
    add_machine_instr(
        "VFNMS", mach_format::VRR_e, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1633);
    add_machine_instr("VFPSO", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U, mask_4_U }, 1635);
    add_machine_instr("VFSQ", mach_format::VRR_a, { vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1636);
    add_machine_instr("VFS", mach_format::VRR_c, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, mask_4_U, mask_4_U }, 1637);
    add_machine_instr("VFTCI", mach_format::VRI_e, { vec_reg_5_U, vec_reg_5_U, imm_12_S, mask_4_U, mask_4_U }, 1638);
    add_machine_instr("VAP", mach_format::VRI_f, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U }, 1643);
    add_machine_instr("VCP", mach_format::VRR_h, { vec_reg_5_U, vec_reg_5_U, mask_4_U }, 1644);
    add_machine_instr("VCVB", mach_format::VRR_i, { reg_4_U, vec_reg_5_U, mask_4_U }, 1645);
    add_machine_instr("VCVBG", mach_format::VRR_i, { reg_4_U, vec_reg_5_U, mask_4_U }, 1645);
    add_machine_instr("VCVD", mach_format::VRI_i, { vec_reg_5_U, reg_4_U, imm_8_S, mask_4_U }, 1646);
    add_machine_instr("VCVDG", mach_format::VRI_i, { vec_reg_5_U, reg_4_U, imm_8_S, mask_4_U }, 1646);
    add_machine_instr("VDP", mach_format::VRI_f, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U }, 1648);
    add_machine_instr("VMP", mach_format::VRI_f, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U }, 1650);
    add_machine_instr("VMSP", mach_format::VRI_f, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U }, 1651);
    add_machine_instr("VRP", mach_format::VRI_f, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U }, 1654);
    add_machine_instr("VSDP", mach_format::VRI_f, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U }, 1656);
    add_machine_instr("VSP", mach_format::VRI_f, { vec_reg_5_U, vec_reg_5_U, vec_reg_5_U, imm_8_U, mask_4_U }, 1658);
    add_machine_instr("VLIP", mach_format::VRI_h, { vec_reg_5_U, imm_16_S, imm_4_U }, 1649);
    add_machine_instr("VPKZ", mach_format::VSI, { vec_reg_5_U, db_12_4_U, imm_8_U }, 1652);
    add_machine_instr("VPSOP", mach_format::VRI_g, { vec_reg_5_U, vec_reg_5_U, imm_8_U, imm_8_U, mask_4_U }, 1653);
    add_machine_instr("VSRP", mach_format::VRI_g, { vec_reg_5_U, vec_reg_5_U, imm_8_U, imm_8_S, mask_4_U }, 1657);
    add_machine_instr("SIE", mach_format::S, { db_12_4_U }, 7);
    add_machine_instr("VAD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VSD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VADS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VSDS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VMDS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VDDS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VMADS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VMSDS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VCDS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VAS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VSS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VMS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VNS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VOS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VXS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VCS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VMD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VMSD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VMSE", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VMSES", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VLINT", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VDD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VDDS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VDE", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VDES", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VMXAD", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VMXAE", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VMXSE", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VNVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VOVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VLCVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VLELE", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VLELD", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VLI", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VLID", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VLBIX", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VLVCU", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VLVCA", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VLVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VMNSD", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VMNSE", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VMRRS", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VMRSV", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VACRS", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VACSV", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VSTVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VSTVP", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VSVMM", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VTVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VXELE", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VXELD", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VXVC", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VXVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VXVMM", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VSTI", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VSTID", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VRCL", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VRRS", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VRSV", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VRSVC", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VSLL", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VSPSD", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VZPSD", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VSRRS", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VSRSV", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VOVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VCVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VCZVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VCOVM", mach_format::RRE, { reg_4_U, reg_4_U }, 0);
    add_machine_instr("VTP", mach_format::VRR_g, { vec_reg_5_U }, 1660);
    add_machine_instr("VUPKZ", mach_format::VSI, { vec_reg_5_U, db_12_4_U, imm_8_U }, 1660);
    add_machine_instr("VSTK", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VSE", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VSES", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VSD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VSDS", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VSTD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VSTKD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VSTMD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VSTH", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VLD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VLH", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VLMD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VLY", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VLYD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VM", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VMAD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VMAES", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VMCD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VMCE", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VMD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VMES", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VACD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VACE", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VAE", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VAES", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VC", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VCD", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VCE", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);
    add_machine_instr("VCES", mach_format::RI_a, { reg_4_U, imm_16_U }, 0);

    return result;
}

const std::set<machine_instruction, machine_instruction_comparer> machine_instructions =
    generate_machine_instructions();

const machine_instruction* instruction::find_machine_instructions(std::string_view name)
{
    auto it = machine_instructions.find(name);
    if (it == machine_instructions.end())
        return nullptr;
    return &*it;
}
const machine_instruction& instruction::get_machine_instructions(std::string_view name)
{
    auto mi = find_machine_instructions(name);
    assert(mi);
    return *mi;
}

const std::set<machine_instruction, machine_instruction_comparer>& instruction::all_machine_instructions()
{
    return machine_instructions;
}

static std::map<std::string, mnemonic_code> generate_mnemonic_codes()
{
    std::map<std::string, mnemonic_code> result;

    const auto add_mnemonic_code = [&result](std::string_view mnemonic,
                                       std::string_view base_instr,
                                       std::initializer_list<std::pair<size_t, size_t>> replacement) {
        assert(std::is_sorted(
            replacement.begin(), replacement.end(), [](const auto& l, const auto& r) { return l.first < r.first; }));
        auto instr_p = machine_instructions.find(base_instr);
        assert(instr_p != machine_instructions.end());
        result.try_emplace(std::string(mnemonic), &*instr_p, replacement);
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

const std::map<std::string, mnemonic_code> instruction::mnemonic_codes = generate_mnemonic_codes();

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
