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

using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library::checking;
using namespace hlasm_plugin::parser_library;

const std::map<mach_format, const std::string> instruction::mach_format_to_string = { { mach_format::E, "E" },
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
    { mach_format::VRX, "VRX" } };

const std::vector<ca_instruction> instruction::ca_instructions = { { "AIF", false },
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
    { "AEJECT", true },
    { "AREAD", false },
    { "ASPACE", false } };

const std::map<const std::string, assembler_instruction> instruction::assembler_instructions = {
    { { "*PROCESS", assembler_instruction(1, -1, true, "") }, // TO DO
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
        { "XATTR", assembler_instruction(1, -1, false, "attribute+") } }
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

bool hlasm_plugin::parser_library::context::machine_instruction::check(const std::string& name_of_instruction,
    const std::vector<const checking::machine_operand*> to_check,
    const range& stmt_range,
    const diagnostic_collector& add_diagnostic)
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

void hlasm_plugin::parser_library::context::machine_instruction::clear_diagnostics() { diagnostics.clear(); }

class vnot_instruction : public machine_instruction
{
public:
    vnot_instruction(const std::string& name,
        mach_format format,
        std::vector<machine_operand_format> operands,
        size_t size,
        size_t page_no)
        : machine_instruction(name, format, operands, (int)size, page_no, (size_t)0)
    {}

    virtual bool check(const std::string& name_of_instruction,
        const std::vector<const hlasm_plugin::parser_library::checking::machine_operand*> to_check,
        const range& stmt_range,
        const diagnostic_collector& add_diagnostic) override
    {
        if (!machine_instruction::check(name_of_instruction, to_check, stmt_range, add_diagnostic))
            return false;
        if (to_check.size() == 3)
        {
            try
            {
                const one_operand& second = dynamic_cast<const one_operand&>(*to_check[1]);
                const one_operand& third = dynamic_cast<const one_operand&>(*to_check[2]);
                if (second.value == third.value)
                    return true;
                else
                {
                    add_diagnostic(diagnostic_op::error_M200(name_of_instruction, stmt_range));
                    return false;
                }
            }
            catch (...)
            {
                assert(false);
            }
        }
        add_diagnostic(diagnostic_op::error_M000(name_of_instruction, (int)operands.size(), stmt_range));
        return false;
    }
};

//#define(instr_name, format, )

void add_machine_instr(std::map<const std::string, machine_instruction_ptr>& result,
    const std::string& instruction_name,
    mach_format format,
    std::vector<machine_operand_format> op_format,
    size_t size,
    size_t page_no)
{
    result.insert(std::pair<const std::string, machine_instruction_ptr>(
        instruction_name, std::make_unique<machine_instruction>(instruction_name, format, op_format, size, page_no)));
}
void add_machine_instr(std::map<const std::string, machine_instruction_ptr>& result,
    const std::string& instruction_name,
    mach_format format,
    std::vector<machine_operand_format> op_format,
    int optional,
    size_t size,
    size_t page_no)
{
    result.insert(std::pair<const std::string, machine_instruction_ptr>(instruction_name,
        std::make_unique<machine_instruction>(
            instruction_name, format, std::move(op_format), optional, size, page_no)));
}

std::map<const std::string, machine_instruction_ptr>
hlasm_plugin::parser_library::context::instruction::get_machine_instructions()
{
    std::map<const std::string, machine_instruction_ptr> result;
    add_machine_instr(result, "AR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 510);
    add_machine_instr(result, "AGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 510);
    add_machine_instr(result, "AGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 510);
    add_machine_instr(result, "ARK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 510);
    add_machine_instr(result, "AGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 510);
    add_machine_instr(result, "A", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 510);
    add_machine_instr(result, "AY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 511);
    add_machine_instr(result, "AG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 511);
    add_machine_instr(result, "AGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 511);
    add_machine_instr(result, "AFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 511);
    add_machine_instr(result, "AGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 511);
    add_machine_instr(result, "AHIK", mach_format::RIE_d, { reg_4_U, reg_4_U, imm_16_S }, 48, 511);
    add_machine_instr(result, "AGHIK", mach_format::RIE_d, { reg_4_U, reg_4_U, imm_16_S }, 48, 511);
    add_machine_instr(result, "ASI", mach_format::SIY, { db_20_4_S, imm_8_S }, 48, 511);
    add_machine_instr(result, "AGSI", mach_format::SIY, { db_20_4_S, imm_8_S }, 48, 511);
    add_machine_instr(result, "AH", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 512);
    add_machine_instr(result, "AHY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 512);
    add_machine_instr(result, "AGH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 512);
    add_machine_instr(result, "AHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 32, 512);
    add_machine_instr(result, "AGHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 32, 513);
    add_machine_instr(result,
        "AHHHR",
        mach_format::RRF_a,
        {
            reg_4_U,
            reg_4_U,
            reg_4_U,
        },
        32,
        513);
    add_machine_instr(result, "AHHLR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 513);
    add_machine_instr(result, "AIH", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 513);
    add_machine_instr(result, "ALR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 514);
    add_machine_instr(result, "ALGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 514);
    add_machine_instr(result, "ALGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 514);
    add_machine_instr(result, "ALRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 514);
    add_machine_instr(result, "ALGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 514);
    add_machine_instr(result, "AL", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 514);
    add_machine_instr(result, "ALY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 514);
    add_machine_instr(result, "ALG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 514);
    add_machine_instr(result, "ALGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 514);
    add_machine_instr(result, "ALFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 514);
    add_machine_instr(result, "ALGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 514);
    add_machine_instr(result, "ALHHHR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 515);
    add_machine_instr(result, "ALHHLR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 515);
    add_machine_instr(result, "ALCR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 515);
    add_machine_instr(result, "ALCGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 515);
    add_machine_instr(result, "ALC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 515);
    add_machine_instr(result, "ALCG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 515);
    add_machine_instr(result, "ALSI", mach_format::SIY, { db_20_4_S, imm_8_S }, 48, 516);
    add_machine_instr(result, "ALGSI", mach_format::SIY, { db_20_4_S, imm_8_S }, 48, 516);
    add_machine_instr(result, "ALHSIK", mach_format::RIE_d, { reg_4_U, reg_4_U, imm_16_S }, 48, 516);
    add_machine_instr(result, "ALGHSIK", mach_format::RIE_d, { reg_4_U, reg_4_U, imm_16_S }, 48, 516);
    add_machine_instr(result, "ALSIH", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 517);
    add_machine_instr(result, "ALSIHN", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 517);
    add_machine_instr(result, "NR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 517);
    add_machine_instr(result, "NGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 517);
    add_machine_instr(result, "NRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 517);
    add_machine_instr(result, "NGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 517);
    add_machine_instr(result, "N", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 517);
    add_machine_instr(result, "NY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 517);
    add_machine_instr(result, "NG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 517);
    add_machine_instr(result, "NI", mach_format::SI, { db_12_4_U, imm_8_U }, 32, 517);
    add_machine_instr(result, "NIY", mach_format::SIY, { db_20_4_S, imm_8_U }, 48, 518);
    add_machine_instr(result, "NC", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 48, 518);
    add_machine_instr(result, "NIHF", mach_format::RIL_a, { reg_4_U, imm_32_U }, 48, 518);
    add_machine_instr(result, "NIHH", mach_format::RI_a, { reg_4_U, imm_16_U }, 32, 518);
    add_machine_instr(result, "NIHL", mach_format::RI_a, { reg_4_U, imm_16_U }, 32, 518);
    add_machine_instr(result, "NILF", mach_format::RIL_a, { reg_4_U, imm_32_U }, 48, 519);
    add_machine_instr(result, "NILH", mach_format::RI_a, { reg_4_U, imm_16_U }, 32, 519);
    add_machine_instr(result, "NILL", mach_format::RI_a, { reg_4_U, imm_16_U }, 32, 519);
    add_machine_instr(result, "BALR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 519);
    add_machine_instr(result, "BAL", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 519);
    add_machine_instr(result, "BASR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 520);
    add_machine_instr(result, "BAS", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 520);
    add_machine_instr(result, "BASSM", mach_format::RX_a, { reg_4_U, reg_4_U }, 16, 520);
    add_machine_instr(result, "BSM", mach_format::RR, { reg_4_U, reg_4_U }, 16, 522);
    add_machine_instr(result, "BIC", mach_format::RXY_b, { mask_4_U, dxb_20_4x4_S }, 48, 523);
    add_machine_instr(result, "BCR", mach_format::RR, { mask_4_U, reg_4_U }, 16, 524);
    add_machine_instr(result, "BC", mach_format::RX_b, { mask_4_U, dxb_12_4x4_U }, 32, 524);
    add_machine_instr(result, "BCTR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 525);
    add_machine_instr(result, "BCTGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 525);
    add_machine_instr(result, "BCT", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 525);
    add_machine_instr(result, "BCTG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 525);
    add_machine_instr(result, "BXH", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 32, 526);
    add_machine_instr(result,
        "BXHG",
        mach_format::RSY_a,
        {
            reg_4_U,
            reg_4_U,
            db_20_4_S,
        },
        48,
        526);
    add_machine_instr(result, "BXLE", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 32, 526);
    add_machine_instr(result,
        "BXLEG",
        mach_format::RSY_a,
        {
            reg_4_U,
            reg_4_U,
            db_20_4_S,
        },
        48,
        526);
    add_machine_instr(result, "BPP", mach_format::SMI, { mask_4_U, reg_imm_16_S, db_12_4_U }, 48, 527);
    add_machine_instr(result, "BPRP", mach_format::MII, { mask_4_U, reg_imm_12_S, reg_imm_24_S }, 48, 527);
    add_machine_instr(result, "BRAS", mach_format::RI_b, { reg_4_U, reg_imm_16_S }, 32, 530);
    add_machine_instr(result, "BRASL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 48, 530);
    add_machine_instr(result, "BRC", mach_format::RI_c, { mask_4_U, reg_imm_16_S }, 32, 530);
    add_machine_instr(result, "BRCL", mach_format::RIL_c, { mask_4_U, reg_imm_32_S }, 48, 530);
    add_machine_instr(result, "BRCT", mach_format::RI_b, { reg_4_U, reg_imm_16_S }, 32, 531);
    add_machine_instr(result, "BRCTG", mach_format::RI_b, { reg_4_U, reg_imm_16_S }, 32, 531);
    add_machine_instr(result, "BRCTH", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 48, 531);
    add_machine_instr(result, "BRXH", mach_format::RSI, { reg_4_U, reg_4_U, reg_imm_16_S }, 32, 532);
    add_machine_instr(result, "BRXHG", mach_format::RIE_e, { reg_4_U, reg_4_U, reg_imm_16_S }, 48, 532);
    add_machine_instr(result, "BRXLE", mach_format::RSI, { reg_4_U, reg_4_U, reg_imm_16_S }, 32, 532);
    add_machine_instr(result, "BRXLG", mach_format::RIE_e, { reg_4_U, reg_4_U, reg_imm_16_S }, 48, 532);
    add_machine_instr(result, "CKSM", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 533);
    add_machine_instr(result, "KM", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 537);
    add_machine_instr(result, "KMC", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 537);
    add_machine_instr(result, "KMA", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U }, 32, 562);
    add_machine_instr(result, "KMF", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 576);
    add_machine_instr(result, "KMCTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U }, 32, 591);
    add_machine_instr(result, "KMO", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 604);
    add_machine_instr(result, "CR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 618);
    add_machine_instr(result, "CGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 618);
    add_machine_instr(result, "CGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 618);
    add_machine_instr(result, "C", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 618);
    add_machine_instr(result, "CY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 618);
    add_machine_instr(result, "CG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 618);
    add_machine_instr(result, "CGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 618);
    add_machine_instr(result, "CFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 618);
    add_machine_instr(result, "CGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 619);
    add_machine_instr(result, "CRL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 48, 619);
    add_machine_instr(result, "CGRL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 48, 619);
    add_machine_instr(result, "CGFRL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 48, 619);
    add_machine_instr(result, "CRB", mach_format::RRS, { reg_4_U, reg_4_U, mask_4_U, db_12_4_U }, 48, 619);
    add_machine_instr(result, "CGRB", mach_format::RRS, { reg_4_U, reg_4_U, mask_4_U, db_12_4_U }, 48, 619);
    add_machine_instr(result, "CRJ", mach_format::RIE_b, { reg_4_U, reg_4_U, mask_4_U, reg_imm_16_S }, 48, 619);
    add_machine_instr(result, "CGRJ", mach_format::RIE_b, { reg_4_U, reg_4_U, mask_4_U, reg_imm_16_S }, 48, 620);
    add_machine_instr(result,
        "CIB",
        mach_format::RIS,
        {
            reg_4_U,
            imm_8_S,
            mask_4_U,
            db_12_4_U,
        },
        48,
        620);
    add_machine_instr(result,
        "CGIB",
        mach_format::RIS,
        {
            reg_4_U,
            imm_8_S,
            mask_4_U,
            db_12_4_U,
        },
        48,
        620);
    add_machine_instr(result, "CIJ", mach_format::RIE_c, { reg_4_U, imm_8_S, mask_4_U, reg_imm_16_S }, 48, 620);
    add_machine_instr(result, "CGIJ", mach_format::RIE_c, { reg_4_U, imm_8_S, mask_4_U, reg_imm_16_S }, 48, 620);
    add_machine_instr(result, "CFC", mach_format::S, { db_12_4_U }, 32, 621);
    add_machine_instr(result, "CS", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 32, 628);
    add_machine_instr(result, "CSY", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 628);
    add_machine_instr(result, "CSG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 628);
    add_machine_instr(result, "CDS", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 32, 628);
    add_machine_instr(result, "CDSY", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 628);
    add_machine_instr(result, "CDSG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 628);
    add_machine_instr(result, "CSST", mach_format::SSF, { db_12_4_U, db_12_4_U, reg_4_U }, 48, 630);
    add_machine_instr(result, "CRT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 32, 633);
    add_machine_instr(result, "CGRT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 32, 633);
    add_machine_instr(result, "CIT", mach_format::RIE_a, { reg_4_U, imm_16_S, mask_4_U }, 48, 633);
    add_machine_instr(result, "CGIT", mach_format::RIE_a, { reg_4_U, imm_16_S, mask_4_U }, 48, 633);
    add_machine_instr(result, "CH", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 634);
    add_machine_instr(result, "CHY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 634);
    add_machine_instr(result, "CGH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 634);
    add_machine_instr(result, "CHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 32, 634);
    add_machine_instr(result, "CGHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 32, 634);
    add_machine_instr(result, "CHHSI", mach_format::SIL, { db_12_4_U, imm_16_S }, 48, 634);
    add_machine_instr(result, "CHSI", mach_format::SIL, { db_12_4_U, imm_16_S }, 48, 634);
    add_machine_instr(result, "CGHSI", mach_format::SIL, { db_12_4_U, imm_16_S }, 48, 634);
    add_machine_instr(result, "CHRL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 48, 634);
    add_machine_instr(result, "CGHRL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 48, 634);
    add_machine_instr(result, "CHHR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 635);
    add_machine_instr(result, "CHLR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 635);
    add_machine_instr(result, "CHF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 635);
    add_machine_instr(result, "CIH", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 635);
    add_machine_instr(result, "CLR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 636);
    add_machine_instr(result, "CLGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 636);
    add_machine_instr(result, "CLGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 636);
    add_machine_instr(result, "CL", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 636);
    add_machine_instr(result, "CLY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 636);
    add_machine_instr(result, "CLG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 636);
    add_machine_instr(result, "CLGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 636);
    add_machine_instr(result,
        "CLC",
        mach_format::SS_a,
        {
            db_12_8x4L_U,
            db_12_4_U,
        },
        48,
        636);
    add_machine_instr(result, "CLFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 636);
    add_machine_instr(result, "CLGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 32, 636);
    add_machine_instr(result, "CLI", mach_format::SI, { db_12_4_U, imm_8_U }, 48, 636);
    add_machine_instr(result, "CLIY", mach_format::SIY, { db_12_4_U, imm_8_U }, 48, 636);
    add_machine_instr(result, "CLFHSI", mach_format::SIL, { db_12_4_U, imm_16_U }, 48, 636);
    add_machine_instr(result, "CLGHSI", mach_format::SIL, { db_12_4_U, imm_16_U }, 48, 636);
    add_machine_instr(result, "CLHHSI", mach_format::SIL, { db_12_4_U, imm_16_U }, 48, 636);
    add_machine_instr(result, "CLRL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 48, 637);
    add_machine_instr(result, "CLGRL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 48, 637);
    add_machine_instr(result, "CLGFRL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 48, 637);
    add_machine_instr(result, "CLHRL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 48, 637);
    add_machine_instr(result, "CLGHRL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 48, 637);
    add_machine_instr(result, "CLRB", mach_format::RRS, { reg_4_U, reg_4_U, mask_4_U, db_12_4_U }, 48, 638);
    add_machine_instr(result, "CLGRB", mach_format::RRS, { reg_4_U, reg_4_U, mask_4_U, db_12_4_U }, 48, 638);
    add_machine_instr(result, "CLRJ", mach_format::RIE_b, { reg_4_U, reg_4_U, mask_4_U, reg_imm_16_S }, 48, 638);
    add_machine_instr(result, "CLGRJ", mach_format::RIE_b, { reg_4_U, reg_4_U, mask_4_U, reg_imm_16_S }, 48, 638);
    add_machine_instr(result, "CLIB", mach_format::RIS, { reg_4_U, imm_8_S, mask_4_U, db_12_4_U }, 48, 638);
    add_machine_instr(result, "CLGIB", mach_format::RIS, { reg_4_U, imm_8_S, mask_4_U, db_12_4_U }, 48, 638);
    add_machine_instr(result, "CLIJ", mach_format::RIE_c, { reg_4_U, imm_8_S, mask_4_U, reg_imm_16_S }, 48, 638);
    add_machine_instr(result, "CLGIJ", mach_format::RIE_c, { reg_4_U, imm_8_S, mask_4_U, reg_imm_16_S }, 48, 638);
    add_machine_instr(result, "CLRT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 32, 639);
    add_machine_instr(result, "CLGRT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 32, 639);
    add_machine_instr(result, "CLT", mach_format::RSY_b, { reg_4_U, mask_4_U, dxb_20_4x4_S }, 48, 639);
    add_machine_instr(result, "CLGT", mach_format::RSY_b, { reg_4_U, mask_4_U, dxb_20_4x4_S }, 48, 639);
    add_machine_instr(result, "CLFIT", mach_format::RIE_a, { reg_4_U, imm_16_S, mask_4_U }, 48, 640);
    add_machine_instr(result, "CLGIT", mach_format::RIE_a, { reg_4_U, imm_16_S, mask_4_U }, 48, 640);
    add_machine_instr(result, "CLM", mach_format::RS_b, { reg_4_U, mask_4_U, db_12_4_U }, 32, 641);
    add_machine_instr(result, "CLMY", mach_format::RSY_b, { reg_4_U, mask_4_U, db_20_4_S }, 48, 641);
    add_machine_instr(result, "CLMH", mach_format::RSY_b, { reg_4_U, mask_4_U, db_20_4_S }, 48, 641);
    add_machine_instr(result, "CLHHR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 641);
    add_machine_instr(result, "CLHLR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 641);
    add_machine_instr(result, "CLHF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 641);
    add_machine_instr(result, "CLCL", mach_format::RR, { reg_4_U, reg_4_U }, 16, 642);
    add_machine_instr(result, "CLIH", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 642);
    add_machine_instr(result, "CLCLE", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 32, 644);
    add_machine_instr(result, "CLCLU", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 647);
    add_machine_instr(result, "CLST", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 650);
    add_machine_instr(result, "CUSE", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 651);
    add_machine_instr(result, "CMPSC", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 654);
    add_machine_instr(result, "KIMD", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 672);
    add_machine_instr(result, "KLMD", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 685);
    add_machine_instr(result, "KMAC", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 703);
    add_machine_instr(result, "CVB", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 714);
    add_machine_instr(result, "CVBY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 714);
    add_machine_instr(result, "CVBG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 714);
    add_machine_instr(result, "CVD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 715);
    add_machine_instr(result, "CVDY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 715);
    add_machine_instr(result, "CVDG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 715);
    add_machine_instr(result, "CU24", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 1, 32, 715);
    add_machine_instr(result, "CUUTF", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 32, 718);
    add_machine_instr(result, "CU21", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 32, 718);
    add_machine_instr(result, "CU42", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 722);
    add_machine_instr(result, "CU41", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 725);
    add_machine_instr(result, "CUTFU", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 1, 32, 728);
    add_machine_instr(result, "CU12", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 1, 32, 728);
    add_machine_instr(result, "CU14", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 1, 32, 732);
    add_machine_instr(result, "CPYA", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 736);
    add_machine_instr(result, "DR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 736);
    add_machine_instr(result, "D", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 736);
    add_machine_instr(result, "DLR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 737);
    add_machine_instr(result, "DLGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 737);
    add_machine_instr(result, "DL", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 737);
    add_machine_instr(result, "DLG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 737);
    add_machine_instr(result, "DSGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 738);
    add_machine_instr(result, "DSGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 738);
    add_machine_instr(result, "DSG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 738);
    add_machine_instr(result, "DSGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 738);
    add_machine_instr(result, "XR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 738);
    add_machine_instr(result, "XGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 738);
    add_machine_instr(result, "XRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 738);
    add_machine_instr(result, "XGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 738);
    add_machine_instr(result, "X", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 738);
    add_machine_instr(result, "XY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 738);
    add_machine_instr(result, "XG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 738);
    add_machine_instr(result, "XI", mach_format::SI, { db_12_4_U, imm_8_S }, 32, 739);
    add_machine_instr(result, "XIY", mach_format::SIY, { db_20_4_S, imm_8_S }, 48, 739);
    add_machine_instr(result, "XC", mach_format::SS_a, { db_12_8x4L_U, db_20_4_S }, 48, 739);
    add_machine_instr(result, "EX", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 740);
    add_machine_instr(result, "XIHF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 740);
    add_machine_instr(result, "XILF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 740);
    add_machine_instr(result, "EXRL", mach_format::RIL_b, { reg_4_U, imm_32_S }, 48, 740);
    add_machine_instr(result, "EAR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 741);
    add_machine_instr(result, "ECAG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 741);
    add_machine_instr(result, "ECTG", mach_format::SSF, { db_12_4_U, db_12_4_U, reg_4_U }, 48, 744);
    add_machine_instr(result, "EPSW", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 745);
    add_machine_instr(result, "ETND", mach_format::RRE, { reg_4_U }, 32, 745);
    add_machine_instr(result, "FLOGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 746);
    add_machine_instr(result, "IC", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 746);
    add_machine_instr(result, "ICY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 746);
    add_machine_instr(result, "ICM", mach_format::RS_b, { reg_4_U, mask_4_U, db_12_4_U }, 32, 746);
    add_machine_instr(result, "ICMY", mach_format::RSY_b, { reg_4_U, mask_4_U, db_20_4_S }, 48, 746);
    add_machine_instr(result, "ICMH", mach_format::RSY_b, { reg_4_U, mask_4_U, db_20_4_S }, 48, 746);
    add_machine_instr(result, "IIHF", mach_format::RIL_a, { reg_4_U, imm_32_U }, 48, 747);
    add_machine_instr(result, "IIHH", mach_format::RI_a, { reg_4_U, imm_16_U }, 32, 747);
    add_machine_instr(result, "IIHL", mach_format::RI_a, { reg_4_U, imm_16_U }, 32, 747);
    add_machine_instr(result, "IILF", mach_format::RIL_a, { reg_4_U, imm_32_U }, 48, 747);
    add_machine_instr(result, "IILH", mach_format::RI_a, { reg_4_U, imm_16_U }, 32, 747);
    add_machine_instr(result, "IILL", mach_format::RI_a, { reg_4_U, imm_16_U }, 32, 747);
    add_machine_instr(result, "IPM", mach_format::RRE, { reg_4_U }, 32, 748);
    add_machine_instr(result, "LR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 748);
    add_machine_instr(result, "LGR", mach_format::RRE, { reg_4_U, reg_4_U }, 16, 748);
    add_machine_instr(result, "LGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 16, 748);
    add_machine_instr(result, "L", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 16, 748);
    add_machine_instr(result, "LY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 16, 748);
    add_machine_instr(result, "LG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 16, 748);
    add_machine_instr(result, "LGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 16, 748);
    add_machine_instr(result, "LGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 16, 748);
    add_machine_instr(result, "LRL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 16, 748);
    add_machine_instr(result, "LGRL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 16, 748);
    add_machine_instr(result, "LGFRL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 16, 748);
    add_machine_instr(result, "LAM", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 32, 749);
    add_machine_instr(result, "LAMY", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 749);
    add_machine_instr(result, "LA", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 750);
    add_machine_instr(result, "LAY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 750);
    add_machine_instr(result, "LAE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 750);
    add_machine_instr(result, "LAEY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 750);
    add_machine_instr(result, "LARL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 48, 751);
    add_machine_instr(result, "LAA", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 752);
    add_machine_instr(result, "LAAG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 752);
    add_machine_instr(result, "LAAL", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 752);
    add_machine_instr(result, "LAALG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 752);
    add_machine_instr(result, "LAN", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 753);
    add_machine_instr(result, "LANG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 753);
    add_machine_instr(result, "LAX", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 753);
    add_machine_instr(result, "LAXG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 753);
    add_machine_instr(result, "LAO", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 754);
    add_machine_instr(result, "LAOG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 754);
    add_machine_instr(result, "LTR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 754);
    add_machine_instr(result, "LTGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 754);
    add_machine_instr(result, "LTGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 754);
    add_machine_instr(result, "LT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 755);
    add_machine_instr(result, "LTG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 755);
    add_machine_instr(result, "LTGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 755);
    add_machine_instr(result, "LAT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 755);
    add_machine_instr(result, "LGAT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 755);
    add_machine_instr(result, "LZRF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 755);
    add_machine_instr(result, "LZRG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 755);
    add_machine_instr(result, "LBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 756);
    add_machine_instr(result, "LGBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 756);
    add_machine_instr(result, "LB", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 756);
    add_machine_instr(result, "LGB", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 756);
    add_machine_instr(result, "LBH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 756);
    add_machine_instr(result, "LCR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 756);
    add_machine_instr(result, "LCGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 757);
    add_machine_instr(result, "LCGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 757);
    add_machine_instr(result, "LCBB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 757);
    add_machine_instr(result, "LGG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 758);
    add_machine_instr(result, "LLGFSG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 758);
    add_machine_instr(result, "LGSC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 759);
    add_machine_instr(result, "LHR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 760);
    add_machine_instr(result, "LGHR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 760);
    add_machine_instr(result, "LH", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 760);
    add_machine_instr(result, "LHY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 760);
    add_machine_instr(result, "LGH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 760);
    add_machine_instr(result, "LHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 32, 760);
    add_machine_instr(result, "LGHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 32, 760);
    add_machine_instr(result, "LHRL", mach_format::RIL_b, { reg_4_U, imm_32_S }, 48, 760);
    add_machine_instr(result, "LGHRL", mach_format::RIL_b, { reg_4_U, imm_32_S }, 48, 760);
    add_machine_instr(result, "LHH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 761);
    add_machine_instr(result, "LOCHI", mach_format::RIE_g, { reg_4_U, imm_16_S, mask_4_U }, 48, 761);
    add_machine_instr(result, "LOCGHI", mach_format::RIE_g, { reg_4_U, imm_16_S, mask_4_U }, 48, 761);
    add_machine_instr(result, "LOCHHI", mach_format::RIE_g, { reg_4_U, imm_16_S, mask_4_U }, 48, 761);
    add_machine_instr(result, "LFH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 762);
    add_machine_instr(result, "LFHAT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 762);
    add_machine_instr(result, "LLGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 762);
    add_machine_instr(result, "LLGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 762);
    add_machine_instr(result, "LLGFRL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 48, 762);
    add_machine_instr(result, "LLGFAT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 763);
    add_machine_instr(result, "LLCR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 763);
    add_machine_instr(result, "LLGCR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 763);
    add_machine_instr(result, "LLC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 763);
    add_machine_instr(result, "LLGC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 763);
    add_machine_instr(result, "LLZRGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 763);
    add_machine_instr(result, "LLCH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 764);
    add_machine_instr(result, "LLHR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 764);
    add_machine_instr(result, "LLGHR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 764);
    add_machine_instr(result, "LLH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 764);
    add_machine_instr(result, "LLGH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 764);
    add_machine_instr(result, "LLHRL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 48, 764);
    add_machine_instr(result, "LLGHRL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 48, 764);
    add_machine_instr(result, "LLHH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 765);
    add_machine_instr(result, "LLIHF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 765);
    add_machine_instr(result, "LLIHH", mach_format::RI_a, { reg_4_U, imm_16_S }, 32, 765);
    add_machine_instr(result, "LLIHL", mach_format::RI_a, { reg_4_U, imm_16_S }, 32, 765);
    add_machine_instr(result, "LLILF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 765);
    add_machine_instr(result, "LLILH", mach_format::RI_a, { reg_4_U, imm_16_S }, 32, 765);
    add_machine_instr(result, "LLILL", mach_format::RI_a, { reg_4_U, imm_16_S }, 32, 765);
    add_machine_instr(result, "LLGTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 765);
    add_machine_instr(result, "LLGT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 766);
    add_machine_instr(result, "LLGTAT", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 766);
    add_machine_instr(result, "LM", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 32, 766);
    add_machine_instr(result, "LMY", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 766);
    add_machine_instr(result, "LMG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 766);
    add_machine_instr(result, "LMD", mach_format::SS_e, { reg_4_U, reg_4_U, db_12_4_U, db_12_4_U }, 48, 767);
    add_machine_instr(result, "LMH", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 767);
    add_machine_instr(result, "LNR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 767);
    add_machine_instr(result, "LNGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 767);
    add_machine_instr(result, "LNGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 768);
    add_machine_instr(result, "LOCFHR", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 32, 768);
    add_machine_instr(result, "LOCFH", mach_format::RSY_b, { reg_4_U, db_20_4_S, mask_4_U }, 48, 768);
    add_machine_instr(result, "LOCR", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 32, 768);
    add_machine_instr(result, "LOCGR", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 32, 768);
    add_machine_instr(result, "LOC", mach_format::RSY_b, { reg_4_U, db_20_4_S, mask_4_U }, 48, 768);
    add_machine_instr(result, "LOCG", mach_format::RSY_b, { reg_4_U, db_20_4_S, mask_4_U }, 48, 768);
    add_machine_instr(result, "LPD", mach_format::SSF, { reg_4_U, db_12_4_U, db_12_4_U }, 48, 769);
    add_machine_instr(result, "LPDG", mach_format::SSF, { reg_4_U, db_12_4_U, db_12_4_U }, 48, 769);
    add_machine_instr(result, "LPQ", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 770);
    add_machine_instr(result, "LPR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 771);
    add_machine_instr(result, "LPGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 771);
    add_machine_instr(result, "LPGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 771);
    add_machine_instr(result, "LRVR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 771);
    add_machine_instr(result, "LRVGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 771);
    add_machine_instr(result, "LRVH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 32, 771);
    add_machine_instr(result, "LRV", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 32, 771);
    add_machine_instr(result, "LRVG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 32, 771);
    add_machine_instr(result, "MC", mach_format::SI, { db_12_4_U, imm_8_S }, 32, 772);
    add_machine_instr(result, "MVC", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 48, 773);
    add_machine_instr(result, "MVHHI", mach_format::SIL, { db_12_4_U, imm_16_S }, 48, 773);
    add_machine_instr(result, "MVHI", mach_format::SIL, { db_12_4_U, imm_16_S }, 48, 773);
    add_machine_instr(result, "MVGHI", mach_format::SIL, { db_12_4_U, imm_16_S }, 48, 773);
    add_machine_instr(result, "MVI", mach_format::SI, { db_12_4_U, imm_8_U }, 32, 773);
    add_machine_instr(result, "MVIY", mach_format::SIY, { db_12_4_U, imm_8_U }, 48, 773);
    add_machine_instr(result, "MVCIN", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 48, 774);
    add_machine_instr(result, "MVCL", mach_format::RR, { reg_4_U, reg_4_U }, 16, 774);
    add_machine_instr(result, "MVCLE", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 32, 778);
    add_machine_instr(result, "MVCLU", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 781);
    add_machine_instr(result, "MVN", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 48, 785);
    add_machine_instr(result, "MVST", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 785);
    add_machine_instr(result, "MVO", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 48, 786);
    add_machine_instr(result, "MVZ", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 48, 787);
    add_machine_instr(result, "MR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 788);
    add_machine_instr(result, "MGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 788);
    add_machine_instr(result, "M", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 788);
    add_machine_instr(result, "MFY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 788);
    add_machine_instr(result, "MG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 788);
    add_machine_instr(result, "MH", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 789);
    add_machine_instr(result, "MHY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 789);
    add_machine_instr(result, "MGH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 789);
    add_machine_instr(result, "MHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 32, 789);
    add_machine_instr(result, "MGHI", mach_format::RI_a, { reg_4_U, imm_16_S }, 32, 789);
    add_machine_instr(result, "MLR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 790);
    add_machine_instr(result, "MLGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 790);
    add_machine_instr(result, "ML", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 790);
    add_machine_instr(result, "MLG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 790);
    add_machine_instr(result, "MSR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 791);
    add_machine_instr(result, "MSRKC", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 791);
    add_machine_instr(result, "MSGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 791);
    add_machine_instr(result, "MSGRKC", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 791);
    add_machine_instr(result, "MSGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 791);
    add_machine_instr(result, "MS", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 791);
    add_machine_instr(result, "MSC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 791);
    add_machine_instr(result, "MSY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 791);
    add_machine_instr(result, "MSG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 791);
    add_machine_instr(result, "MSGC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 791);
    add_machine_instr(result, "MSGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 791);
    add_machine_instr(result, "MSFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 791);
    add_machine_instr(result, "MSGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 791);
    add_machine_instr(result, "NIAI", mach_format::IE, { imm_4_U, imm_4_U }, 32, 792);
    add_machine_instr(result, "NTSTG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 794);
    add_machine_instr(result, "OR", mach_format::RR, { reg_4_U, reg_4_U }, 32, 794);
    add_machine_instr(result, "OGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 794);
    add_machine_instr(result, "ORK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 794);
    add_machine_instr(result, "OGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 794);
    add_machine_instr(result, "O", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 794);
    add_machine_instr(result, "OY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 32, 794);
    add_machine_instr(result, "OG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 795);
    add_machine_instr(result, "OI", mach_format::SI, { db_12_4_U, imm_8_U }, 48, 795);
    add_machine_instr(result, "OIY", mach_format::SIY, { db_20_4_S, imm_8_U }, 48, 795);
    add_machine_instr(result, "OC", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 48, 795);
    add_machine_instr(result, "OIHF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 32, 796);
    add_machine_instr(result, "OIHH", mach_format::RI_a, { reg_4_U, imm_16_S }, 32, 796);
    add_machine_instr(result, "OIHL", mach_format::RI_a, { reg_4_U, imm_16_S }, 32, 796);
    add_machine_instr(result, "OILF", mach_format::RIL_a, { reg_4_U, imm_32_S }, 32, 796);
    add_machine_instr(result, "OILH", mach_format::RI_a, { reg_4_U, imm_16_S }, 32, 796);
    add_machine_instr(result, "OILL", mach_format::RI_a, { reg_4_U, imm_16_S }, 32, 796);
    add_machine_instr(result, "PACK", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 32, 796);
    add_machine_instr(result, "PKA", mach_format::SS_f, { db_12_4_U, db_12_8x4L_U }, 48, 797);
    add_machine_instr(result, "PKU", mach_format::SS_f, { db_12_4_U, db_12_8x4L_U }, 48, 798);
    add_machine_instr(result, "PCC", mach_format::RRE, {}, 32, 799);
    add_machine_instr(result, "PLO", mach_format::SS_e, { reg_4_U, db_12_4_U, reg_4_U, db_12_4_U }, 48, 815);
    add_machine_instr(result, "PPA", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 32, 829);
    add_machine_instr(result, "PRNO", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 830);
    add_machine_instr(result, "PPNO", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 830);
    add_machine_instr(result, "POPCNT", mach_format::RRE, { reg_4_U, reg_4_U }, 48, 843);
    add_machine_instr(result, "PFD", mach_format::RXY_b, { mask_4_U, dxb_20_4x4_S }, 48, 843);
    add_machine_instr(result, "PFDRL", mach_format::RIL_c, { mask_4_U, reg_imm_32_S }, 48, 843);
    add_machine_instr(result, "RLL", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 845);
    add_machine_instr(result, "RLLG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 845);
    add_machine_instr(result, "RNSBG", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S }, 1, 48, 845);
    add_machine_instr(result, "RXSBG", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S }, 1, 48, 846);
    add_machine_instr(result, "ROSBG", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S }, 1, 48, 846);
    add_machine_instr(result, "RISBG", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S }, 1, 48, 847);
    add_machine_instr(
        result, "RISBGN", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S }, 1, 48, 847);
    add_machine_instr(
        result, "RISBHG", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S }, 1, 48, 848);
    add_machine_instr(
        result, "RISBLG", mach_format::RIE_f, { reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S }, 1, 48, 849);
    add_machine_instr(result, "SRST", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 850);
    add_machine_instr(result, "SRSTU", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 852);
    add_machine_instr(result, "SAR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 854);
    add_machine_instr(result, "SAM24", mach_format::E, {}, 16, 854);
    add_machine_instr(result, "SAM31", mach_format::E, {}, 16, 854);
    add_machine_instr(result, "SAM64", mach_format::E, {}, 16, 854);
    add_machine_instr(result, "SPM", mach_format::RR, { reg_4_U }, 16, 855);
    add_machine_instr(result, "SLDA", mach_format::RS_a, { reg_4_U, db_12_4_U }, 32, 855);
    add_machine_instr(result, "SLA", mach_format::RS_a, { reg_4_U, db_12_4_U }, 32, 856);
    add_machine_instr(result, "SLAK", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 856);
    add_machine_instr(result, "SLAG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 856);
    add_machine_instr(result, "SLDL", mach_format::RS_a, { reg_4_U, db_12_4_U }, 32, 856);
    add_machine_instr(result, "SLL", mach_format::RS_a, { reg_4_U, db_12_4_U }, 32, 857);
    add_machine_instr(result, "SLLK", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 857);
    add_machine_instr(result, "SLLG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 857);
    add_machine_instr(result, "SRDA", mach_format::RS_a, { reg_4_U, db_12_4_U }, 32, 858);
    add_machine_instr(result, "SRDL", mach_format::RS_a, { reg_4_U, db_12_4_U }, 32, 858);
    add_machine_instr(result, "SRA", mach_format::RS_a, { reg_4_U, db_12_4_U }, 32, 859);
    add_machine_instr(result, "SRAK", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 859);
    add_machine_instr(result, "SRAG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 859);
    add_machine_instr(result, "SRL", mach_format::RS_a, { reg_4_U, db_12_4_U }, 32, 860);
    add_machine_instr(result, "SRLK", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 860);
    add_machine_instr(result, "SRLG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 860);
    add_machine_instr(result, "ST", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 860);
    add_machine_instr(result, "STY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 861);
    add_machine_instr(result, "STG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 861);
    add_machine_instr(result, "STRL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 48, 861);
    add_machine_instr(result, "STGRL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 48, 861);
    add_machine_instr(result, "STAM", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 48, 861);
    add_machine_instr(result, "STAMY", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 861);
    add_machine_instr(result, "STC", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 862);
    add_machine_instr(result, "STCY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 862);
    add_machine_instr(result, "STCH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 862);
    add_machine_instr(result, "STCM", mach_format::RS_b, { reg_4_U, mask_4_U, db_12_4_U }, 32, 862);
    add_machine_instr(result, "STCMY", mach_format::RSY_b, { reg_4_U, mask_4_U, db_20_4_S }, 48, 862);
    add_machine_instr(result, "STCMH", mach_format::RSY_b, { reg_4_U, mask_4_U, db_20_4_S }, 48, 862);
    add_machine_instr(result, "STCK", mach_format::S, { db_12_4_U }, 32, 863);
    add_machine_instr(result, "STCKF", mach_format::S, { db_12_4_U }, 32, 863);
    add_machine_instr(result, "STCKE", mach_format::S, { db_12_4_U }, 32, 864);
    add_machine_instr(result, "STFLE", mach_format::S, { db_20_4_S }, 48, 866);
    add_machine_instr(result, "STGSC", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 32, 867);
    add_machine_instr(result, "STH", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 867);
    add_machine_instr(result, "STHY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 868);
    add_machine_instr(result, "STHRL", mach_format::RIL_b, { reg_4_U, reg_imm_32_S }, 48, 868);
    add_machine_instr(result, "STHH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 868);
    add_machine_instr(result, "STFH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 868);
    add_machine_instr(result, "STM", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 32, 869);
    add_machine_instr(result, "STMY", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 869);
    add_machine_instr(result, "STMG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 869);
    add_machine_instr(result, "STMH", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 869);
    add_machine_instr(result, "STOC", mach_format::RSY_b, { reg_4_U, db_20_4_S, mask_4_U }, 48, 869);
    add_machine_instr(result, "STOCG", mach_format::RSY_b, { reg_4_U, db_20_4_S, mask_4_U }, 48, 869);
    add_machine_instr(result, "STOCFH", mach_format::RSY_b, { reg_4_U, db_20_4_S, mask_4_U }, 48, 870);
    add_machine_instr(result, "STPQ", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 870);
    add_machine_instr(result, "STRVH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 871);
    add_machine_instr(result, "STRV", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 871);
    add_machine_instr(result, "STRVG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 871);
    add_machine_instr(result, "SR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 871);
    add_machine_instr(result, "SGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 871);
    add_machine_instr(result, "SGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 871);
    add_machine_instr(result, "SRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 871);
    add_machine_instr(result, "SGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 872);
    add_machine_instr(result, "S", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 872);
    add_machine_instr(result, "SY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 872);
    add_machine_instr(result, "SG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 872);
    add_machine_instr(result, "SGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 872);
    add_machine_instr(result, "SH", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 872);
    add_machine_instr(result, "SHY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 872);
    add_machine_instr(result, "SGH", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 872);
    add_machine_instr(result, "SHHHR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 873);
    add_machine_instr(result, "SHHLR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 873);
    add_machine_instr(result, "SLR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 873);
    add_machine_instr(result, "SLGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 873);
    add_machine_instr(result, "SLGFR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 873);
    add_machine_instr(result, "SLRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 873);
    add_machine_instr(result, "SLGRK", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 873);
    add_machine_instr(result, "SL", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 874);
    add_machine_instr(result, "SLY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 874);
    add_machine_instr(result, "SLG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 874);
    add_machine_instr(result, "SLGF", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 874);
    add_machine_instr(result, "SLFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 874);
    add_machine_instr(result, "SLGFI", mach_format::RIL_a, { reg_4_U, imm_32_S }, 48, 874);
    add_machine_instr(result, "SLHHHR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 875);
    add_machine_instr(result, "SLHHLR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 875);
    add_machine_instr(result, "SLBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 875);
    add_machine_instr(result, "SLBGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 875);
    add_machine_instr(result, "SLB", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 875);
    add_machine_instr(result, "SLBG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 875);
    add_machine_instr(result, "SVC", mach_format::I, { imm_8_U }, 16, 876);
    add_machine_instr(result, "TS", mach_format::SI, { db_12_4_U }, 32, 876);
    add_machine_instr(result, "TAM", mach_format::E, {}, 16, 876);
    add_machine_instr(result, "TM", mach_format::SI, { db_12_4_U, imm_8_U }, 32, 877);
    add_machine_instr(result, "TMY", mach_format::SIY, { db_20_4_S, imm_8_U }, 48, 877);
    add_machine_instr(result, "TMHH", mach_format::RI_a, { reg_4_U, imm_16_U }, 32, 877);
    add_machine_instr(result, "TMHL", mach_format::RI_a, { reg_4_U, imm_16_U }, 32, 877);
    add_machine_instr(result, "TMH", mach_format::RI_a, { reg_4_U, imm_16_U }, 32, 877);
    add_machine_instr(result, "TMLH", mach_format::RI_a, { reg_4_U, imm_16_U }, 32, 877);
    add_machine_instr(result, "TML", mach_format::RI_a, { reg_4_U, imm_16_U }, 32, 877);
    add_machine_instr(result, "TMLL", mach_format::RI_a, { reg_4_U, imm_16_U }, 32, 877);
    add_machine_instr(result, "TABORT", mach_format::S, { db_12_4_U }, 32, 878);
    add_machine_instr(result, "TBEGIN", mach_format::SIL, { db_12_4_U, imm_16_S }, 48, 879);
    add_machine_instr(result, "TBEGINC", mach_format::SIL, { db_12_4_U, imm_16_S }, 48, 883);
    add_machine_instr(result, "TEND", mach_format::S, {}, 32, 885);
    add_machine_instr(result, "TR", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 48, 886);
    add_machine_instr(result, "TRT", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 48, 887);
    add_machine_instr(result, "TRTE", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 1, 32, 887);
    add_machine_instr(result, "TRTRE", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 1, 32, 888);
    add_machine_instr(result, "TRTR", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 48, 892);
    add_machine_instr(result, "TRE", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 893);
    add_machine_instr(result, "TROO", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 1, 32, 895);
    add_machine_instr(result, "TROT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 1, 32, 895);
    add_machine_instr(result, "TRTO", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 1, 32, 895);
    add_machine_instr(result, "TRTT", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 1, 32, 895);
    add_machine_instr(result, "UNPK", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 48, 900);
    add_machine_instr(result, "UNPKA", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 48, 901);
    add_machine_instr(result, "UNPKU", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 48, 902);
    add_machine_instr(result, "UPT", mach_format::E, {}, 16, 903);
    add_machine_instr(result, "AP", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 48, 920);
    add_machine_instr(result, "CP", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 48, 921);
    add_machine_instr(result, "DP", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 48, 921);
    add_machine_instr(result, "ED", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 48, 922);
    add_machine_instr(result, "EDMK", mach_format::SS_a, { db_12_8x4L_U, db_12_4_U }, 48, 925);
    add_machine_instr(result, "SRP", mach_format::SS_c, { db_12_4x4L_U, db_12_4_U, imm_4_U }, 48, 926);
    add_machine_instr(result, "MP", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 48, 926);
    add_machine_instr(result, "SP", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 48, 927);
    add_machine_instr(result, "TP", mach_format::RSL_a, { db_12_4x4L_U }, 48, 928);
    add_machine_instr(result, "ZAP", mach_format::SS_b, { db_12_4x4L_U, db_12_4x4L_U }, 48, 928);
    add_machine_instr(result, "THDR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 955);
    add_machine_instr(result, "THDER", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 955);
    add_machine_instr(result, "TBEDR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 32, 956);
    add_machine_instr(result, "TBDR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 32, 956);
    add_machine_instr(result, "CPSDR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U }, 32, 958);
    add_machine_instr(result, "EFPC", mach_format::RRE, { reg_4_U }, 32, 958);
    add_machine_instr(result, "LER", mach_format::RR, { reg_4_U, reg_4_U }, 16, 959);
    add_machine_instr(result, "LDR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 959);
    add_machine_instr(result, "LXR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 959);
    add_machine_instr(result, "LE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 959);
    add_machine_instr(result, "LD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 959);
    add_machine_instr(result, "LEY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 959);
    add_machine_instr(result, "LDY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 959);
    add_machine_instr(result, "LCDFR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 959);
    add_machine_instr(result, "LFPC", mach_format::S, { db_12_4_U }, 32, 959);
    add_machine_instr(result, "LFAS", mach_format::S, { db_12_4_U }, 32, 960);
    add_machine_instr(result, "LDGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 962);
    add_machine_instr(result, "LGDR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 962);
    add_machine_instr(result, "LNDFR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 962);
    add_machine_instr(result, "LPDFR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 962);
    add_machine_instr(result, "LZER", mach_format::RRE, { reg_4_U }, 32, 963);
    add_machine_instr(result, "LZXR", mach_format::RRE, { reg_4_U }, 32, 963);
    add_machine_instr(result, "LZDR", mach_format::RRE, { reg_4_U }, 32, 963);
    add_machine_instr(result, "PFPO", mach_format::E, {}, 16, 963);
    add_machine_instr(result, "SRNM", mach_format::S, { db_12_4_U }, 32, 975);
    add_machine_instr(result, "SRNMB", mach_format::S, { db_12_4_U }, 32, 975);
    add_machine_instr(result, "SRNMT", mach_format::S, { db_12_4_U }, 32, 975);
    add_machine_instr(result, "SFPC", mach_format::RRE, { reg_4_U }, 32, 975);
    add_machine_instr(result, "SFASR", mach_format::RRE, { reg_4_U }, 32, 976);
    add_machine_instr(result, "STE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 976);
    add_machine_instr(result, "STD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 976);
    add_machine_instr(result, "STDY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 977);
    add_machine_instr(result, "STEY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 977);
    add_machine_instr(result, "STFPC", mach_format::S, { db_12_4_U }, 32, 977);
    add_machine_instr(result, "BSA", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 989);
    add_machine_instr(result, "BAKR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 993);
    add_machine_instr(result, "BSG", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 995);
    add_machine_instr(result, "CRDTE", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1, 32, 999);
    add_machine_instr(result, "CSP", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1003);
    add_machine_instr(result, "CSPG", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1003);
    add_machine_instr(result, "ESEA", mach_format::RRE, { reg_4_U }, 32, 1006);
    add_machine_instr(result, "EPAR", mach_format::RRE, { reg_4_U }, 32, 1006);
    add_machine_instr(result, "EPAIR", mach_format::RRE, { reg_4_U }, 32, 1006);
    add_machine_instr(result, "ESAR", mach_format::RRE, { reg_4_U }, 32, 1006);
    add_machine_instr(result, "ESAIR", mach_format::RRE, { reg_4_U }, 32, 1007);
    add_machine_instr(result, "EREG", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1007);
    add_machine_instr(result, "EREGG", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1007);
    add_machine_instr(result, "ESTA", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1008);
    add_machine_instr(result, "IAC", mach_format::RRE, { reg_4_U }, 32, 1011);
    add_machine_instr(result, "IPK", mach_format::S, {}, 32, 1012);
    add_machine_instr(result, "IRBM", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1012);
    add_machine_instr(result, "ISKE", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1012);
    add_machine_instr(result, "IVSK", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1013);
    add_machine_instr(result, "IDTE", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1, 32, 1014);
    add_machine_instr(result, "IPTE", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 2, 32, 1019);
    add_machine_instr(result, "LASP", mach_format::SSE, { db_12_4_U, db_12_4_U }, 48, 1023);
    add_machine_instr(result, "LCTL", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 32, 1032);
    add_machine_instr(result, "LCTLG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 1032);
    add_machine_instr(result, "LPTEA", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1032);
    add_machine_instr(result, "LPSW", mach_format::SI, { db_12_4_U }, 32, 1036);
    add_machine_instr(result, "LPSWE", mach_format::S, { db_12_4_U }, 32, 1037);
    add_machine_instr(result, "LRA", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 1038);
    add_machine_instr(result, "LRAY", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 1038);
    add_machine_instr(result, "LRAG", mach_format::RXY_a, { reg_4_U, dxb_20_4x4_S }, 48, 1038);
    add_machine_instr(result, "LURA", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1042);
    add_machine_instr(result, "LURAG", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1042);
    add_machine_instr(result, "MSTA", mach_format::RRE, { reg_4_U }, 32, 1043);
    add_machine_instr(result, "MVPG", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1044);
    add_machine_instr(result, "MVCP", mach_format::SS_d, { drb_12_4x4_U, db_12_4_U, reg_4_U }, 48, 1046);
    add_machine_instr(result, "MVCS", mach_format::SS_d, { drb_12_4x4_U, db_12_4_U, reg_4_U }, 48, 1046);
    add_machine_instr(result, "MVCDK", mach_format::SSE, { db_12_4_U, db_12_4_U }, 48, 1048);
    add_machine_instr(result, "MVCK", mach_format::SS_d, { drb_12_4x4_U, db_12_4_U, reg_4_U }, 48, 1049);
    add_machine_instr(result, "MVCOS", mach_format::SSF, { db_12_4_U, db_12_4_U, reg_4_U }, 48, 1050);
    add_machine_instr(result, "MVCSK", mach_format::SSE, { db_12_4_U, db_12_4_U }, 48, 1053);
    add_machine_instr(result, "PGIN", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1054);
    add_machine_instr(result, "PGOUT", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1055);
    add_machine_instr(result, "PCKMO", mach_format::RRE, {}, 32, 1056);
    add_machine_instr(result, "PFMF", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1059);
    add_machine_instr(result, "PTFF", mach_format::E, {}, 16, 1063);
    add_machine_instr(result, "PTF", mach_format::RRE, { reg_4_U }, 32, 1071);
    add_machine_instr(result, "PC", mach_format::S, { db_12_4_U }, 32, 1072);
    add_machine_instr(result, "PR", mach_format::E, {}, 16, 1085);
    add_machine_instr(result, "PTI", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1089);
    add_machine_instr(result, "PT", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1089);
    add_machine_instr(result, "PALB", mach_format::RRE, {}, 32, 1098);
    add_machine_instr(result, "PTLB", mach_format::S, {}, 32, 1098);
    add_machine_instr(result, "RRBE", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1098);
    add_machine_instr(result, "RRBM", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1099);
    add_machine_instr(result, "RP", mach_format::S, { db_12_4_U }, 32, 1099);
    add_machine_instr(result, "SAC", mach_format::S, { db_12_4_U }, 32, 1102);
    add_machine_instr(result, "SACF", mach_format::S, { db_12_4_U }, 32, 1102);
    add_machine_instr(result, "SCK", mach_format::S, { db_12_4_U }, 32, 1103);
    add_machine_instr(result, "SCKC", mach_format::S, { db_12_4_U }, 32, 1104);
    add_machine_instr(result, "SCKPF", mach_format::E, {}, 16, 1105);
    add_machine_instr(result, "SPX", mach_format::S, { db_12_4_U }, 32, 1105);
    add_machine_instr(result, "SPT", mach_format::S, { db_12_4_U }, 32, 1105);
    add_machine_instr(result, "SPKA", mach_format::S, { db_12_4_U }, 32, 1106);
    add_machine_instr(result, "SSAR", mach_format::RRE, { reg_4_U }, 32, 1107);
    add_machine_instr(result, "SSAIR", mach_format::RRE, { reg_4_U }, 32, 1107);
    add_machine_instr(result, "SSKE", mach_format::RRF_c, { reg_4_U, reg_4_U, mask_4_U }, 1, 32, 1112);
    add_machine_instr(result, "SSM", mach_format::SI, { db_12_4_U }, 32, 1115);
    add_machine_instr(result, "SIGP", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 32, 1115);
    add_machine_instr(result, "STCKC", mach_format::S, { db_12_4_U }, 32, 1117);
    add_machine_instr(result, "STCTL", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 32, 1117);
    add_machine_instr(result, "STCTG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 1117);
    add_machine_instr(result, "STAP", mach_format::S, { db_12_4_U }, 32, 1118);
    add_machine_instr(result, "STIDP", mach_format::S, { db_12_4_U }, 32, 1118);
    add_machine_instr(result, "STPT", mach_format::S, { db_12_4_U }, 32, 1120);
    add_machine_instr(result, "STFL", mach_format::S, { db_12_4_U }, 32, 1120);
    add_machine_instr(result, "STPX", mach_format::S, { db_12_4_U }, 32, 1121);
    add_machine_instr(result, "STRAG", mach_format::SSE, { db_12_4_U, db_12_4_U }, 32, 1121);
    add_machine_instr(result, "STSI", mach_format::S, { db_12_4_U }, 32, 1122);
    add_machine_instr(result, "STOSM", mach_format::SI, { db_12_4_U, imm_8_S }, 32, 1146);
    add_machine_instr(result, "STNSM", mach_format::SI, { db_12_4_U, imm_8_S }, 32, 1146);
    add_machine_instr(result, "STURA", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1147);
    add_machine_instr(result, "STURG", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1147);
    add_machine_instr(result, "TAR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1147);
    add_machine_instr(result, "TB", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1149);
    add_machine_instr(result, "TPEI", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1151);
    add_machine_instr(result, "TPROT", mach_format::SSE, { db_12_4_U, db_12_4_U }, 48, 1152);
    add_machine_instr(result, "TRACE", mach_format::RS_a, { reg_4_U, reg_4_U, db_12_4_U }, 32, 1155);
    add_machine_instr(result, "TRACG", mach_format::RSY_a, { reg_4_U, reg_4_U, db_20_4_S }, 48, 1155);
    add_machine_instr(result, "TRAP2", mach_format::E, {}, 16, 1156);
    add_machine_instr(result, "TRAP4", mach_format::S, { db_12_4_U }, 32, 1156);
    add_machine_instr(result, "XSCH", mach_format::S, {}, 32, 1215);
    add_machine_instr(result, "CSCH", mach_format::S, {}, 32, 1217);
    add_machine_instr(result, "HSCH", mach_format::S, {}, 32, 1218);
    add_machine_instr(result, "MSCH", mach_format::S, { db_12_4_U }, 32, 1219);
    add_machine_instr(result, "RCHP", mach_format::S, {}, 32, 1221);
    add_machine_instr(result, "RSCH", mach_format::S, {}, 32, 1222);
    add_machine_instr(result, "SAL", mach_format::S, {}, 32, 1224);
    add_machine_instr(result, "SCHM", mach_format::S, {}, 32, 1225);
    add_machine_instr(result, "SSCH", mach_format::S, { db_12_4_U }, 32, 1227);
    add_machine_instr(result, "STCPS", mach_format::S, { db_12_4_U }, 32, 1228);
    add_machine_instr(result, "STCRW", mach_format::S, { db_12_4_U }, 32, 1229);
    add_machine_instr(result, "STSCH", mach_format::S, { db_12_4_U }, 32, 1230);
    add_machine_instr(result, "TPI", mach_format::S, { db_12_4_U }, 32, 1231);
    add_machine_instr(result, "TSCH", mach_format::S, { db_12_4_U }, 32, 1232);

    add_machine_instr(result, "AER", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1412);
    add_machine_instr(result, "ADR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1412);
    add_machine_instr(result, "AXR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1412);
    add_machine_instr(result, "AE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 1412);
    add_machine_instr(result, "AD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 1412);
    add_machine_instr(result, "AWR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1413);
    add_machine_instr(result, "AUR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1413);
    add_machine_instr(result, "AU", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 1413);
    add_machine_instr(result, "AW", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 1413);
    add_machine_instr(result, "CER", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1414);
    add_machine_instr(result, "CDR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1414);
    add_machine_instr(result, "CXR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1414);
    add_machine_instr(result, "CE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 1414);
    add_machine_instr(result, "CD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 1414);
    add_machine_instr(result, "CEFR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1415);
    add_machine_instr(result, "CDFR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1415);
    add_machine_instr(result, "CXFR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1415);
    add_machine_instr(result, "CEGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1415);
    add_machine_instr(result, "CDGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1415);
    add_machine_instr(result, "CXGR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1415);
    add_machine_instr(result, "CFER", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 32, 1415);
    add_machine_instr(result, "CFDR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 32, 1415);
    add_machine_instr(result, "CFXR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 32, 1415);
    add_machine_instr(result, "CGER", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 32, 1415);
    add_machine_instr(result, "CGDR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 32, 1415);
    add_machine_instr(result, "CGXR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 32, 1415);
    add_machine_instr(result, "DDR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1416);
    add_machine_instr(result, "DER", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1416);
    add_machine_instr(result, "DXR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1416);
    add_machine_instr(result, "DD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 1416);
    add_machine_instr(result, "DE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 1416);
    add_machine_instr(result, "HDR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1417);
    add_machine_instr(result, "HER", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1417);
    add_machine_instr(result, "LTER", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1417);
    add_machine_instr(result, "LTDR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1417);
    add_machine_instr(result, "LTXR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1418);
    add_machine_instr(result, "LCER", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1418);
    add_machine_instr(result, "LCDR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1418);
    add_machine_instr(result, "LCXR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1418);
    add_machine_instr(result, "FIER", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1419);
    add_machine_instr(result, "FIDR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1419);
    add_machine_instr(result, "FIXR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1419);
    add_machine_instr(result, "LDER", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1419);
    add_machine_instr(result, "LXDR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1419);
    add_machine_instr(result, "LXER", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1419);
    add_machine_instr(result, "LDE", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1419);
    add_machine_instr(result, "LXD", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1419);
    add_machine_instr(result, "LXE", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1419);
    add_machine_instr(result, "LNDR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1420);
    add_machine_instr(result, "LNER", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1420);
    add_machine_instr(result, "LPDR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1420);
    add_machine_instr(result, "LPER", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1420);
    add_machine_instr(result, "LNXR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1420);
    add_machine_instr(result, "LPXR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1420);
    add_machine_instr(result, "LEDR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1421);
    add_machine_instr(result, "LDXR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1421);
    add_machine_instr(result, "LRER", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1421);
    add_machine_instr(result, "LRDR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1421);
    add_machine_instr(result, "LEXR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1421);
    add_machine_instr(result, "MEER", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1421);
    add_machine_instr(result, "MDR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1421);
    add_machine_instr(result, "MXR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1421);
    add_machine_instr(result, "MDER", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1421);
    add_machine_instr(result, "MER", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1421);
    add_machine_instr(result, "MXDR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1421);
    add_machine_instr(result, "MEE", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1422);
    add_machine_instr(result, "MD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 1422);
    add_machine_instr(result, "MDE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 1422);
    add_machine_instr(result, "MXD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 1422);
    add_machine_instr(result, "ME", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 1422);
    add_machine_instr(result, "MAER", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 32, 1423);
    add_machine_instr(result, "MADR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 32, 1423);
    add_machine_instr(result, "MAD", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1423);
    add_machine_instr(result, "MAE", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1423);
    add_machine_instr(result, "MSER", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 32, 1423);
    add_machine_instr(result, "MSDR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 32, 1423);
    add_machine_instr(result, "MSE", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1423);
    add_machine_instr(result, "MSD", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1423);
    add_machine_instr(result, "MAYR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 32, 1424);
    add_machine_instr(result, "MAYHR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 32, 1424);
    add_machine_instr(result, "MAYLR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 32, 1424);
    add_machine_instr(result, "MAY", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1424);
    add_machine_instr(result, "MAYH", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1424);
    add_machine_instr(result, "MAYL", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1424);
    add_machine_instr(result, "MYR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 32, 1426);
    add_machine_instr(result, "MYHR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 32, 1426);
    add_machine_instr(result, "MYLR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 32, 1426);
    add_machine_instr(result, "MY", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1426);
    add_machine_instr(result, "MYH", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1426);
    add_machine_instr(result, "MYL", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1426);
    add_machine_instr(result, "SQER", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1427);
    add_machine_instr(result, "SQDR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1427);
    add_machine_instr(result, "SQXR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1427);
    add_machine_instr(result, "SQE", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1427);
    add_machine_instr(result, "SQD", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1427);
    add_machine_instr(result, "SER", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1428);
    add_machine_instr(result, "SDR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1428);
    add_machine_instr(result, "SXR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1428);
    add_machine_instr(result, "SE", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 1428);
    add_machine_instr(result, "SD", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 1428);
    add_machine_instr(result, "SUR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1429);
    add_machine_instr(result, "SWR", mach_format::RR, { reg_4_U, reg_4_U }, 16, 1429);
    add_machine_instr(result, "SU", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 1429);
    add_machine_instr(result, "SW", mach_format::RX_a, { reg_4_U, dxb_12_4x4_U }, 32, 1429);
    add_machine_instr(result, "AEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1445);
    add_machine_instr(result, "ADBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1445);
    add_machine_instr(result, "AXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1445);
    add_machine_instr(result, "AEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1445);
    add_machine_instr(result, "ADB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1445);
    add_machine_instr(result, "CEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1447);
    add_machine_instr(result, "CDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1447);
    add_machine_instr(result, "CXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1447);
    add_machine_instr(result, "CDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1447);
    add_machine_instr(result, "CEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1447);
    add_machine_instr(result, "KEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1448);
    add_machine_instr(result, "KDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1448);
    add_machine_instr(result, "KXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1448);
    add_machine_instr(result, "KDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1448);
    add_machine_instr(result, "KEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1448);
    add_machine_instr(result, "CEFBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1449);
    add_machine_instr(result, "CDFBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1449);
    add_machine_instr(result, "CXFBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1449);
    add_machine_instr(result, "CEGBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1449);
    add_machine_instr(result, "CDGBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1449);
    add_machine_instr(result, "CXGBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1449);
    add_machine_instr(result, "CEFBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1449);
    add_machine_instr(result, "CDFBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1449);
    add_machine_instr(result, "CXFBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1449);
    add_machine_instr(result, "CEGBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1449);
    add_machine_instr(result, "CDGBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1449);
    add_machine_instr(result, "CXGBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1449);
    add_machine_instr(result, "CELFBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1451);
    add_machine_instr(result, "CDLFBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1451);
    add_machine_instr(result, "CXLFBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1451);
    add_machine_instr(result, "CELGBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1451);
    add_machine_instr(result, "CDLGBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1451);
    add_machine_instr(result, "CXLGBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1451);
    add_machine_instr(result, "CFEBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 32, 1452);
    add_machine_instr(result, "CFDBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 32, 1452);
    add_machine_instr(result, "CFXBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 32, 1452);
    add_machine_instr(result, "CGEBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 32, 1452);
    add_machine_instr(result, "CGDBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 32, 1452);
    add_machine_instr(result, "CGXBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 32, 1452);
    add_machine_instr(result, "CFEBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1452);
    add_machine_instr(result, "CFDBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1452);
    add_machine_instr(result, "CFXBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1452);
    add_machine_instr(result, "CGEBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1452);
    add_machine_instr(result, "CGDBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1452);
    add_machine_instr(result, "CGXBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1452);
    add_machine_instr(result, "CLFEBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1455);
    add_machine_instr(result, "CLFDBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1455);
    add_machine_instr(result, "CLFXBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1455);
    add_machine_instr(result, "CLGEBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1455);
    add_machine_instr(result, "CLGDBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1455);
    add_machine_instr(result, "CLGXBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1455);
    add_machine_instr(result, "DEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1457);
    add_machine_instr(result, "DDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1457);
    add_machine_instr(result, "DXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1457);
    add_machine_instr(result, "DEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1457);
    add_machine_instr(result, "DDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1457);
    add_machine_instr(result, "DIEBR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1458);
    add_machine_instr(result, "DIDBR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1458);
    add_machine_instr(result, "LTEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1461);
    add_machine_instr(result, "LTDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1461);
    add_machine_instr(result, "LTXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1461);
    add_machine_instr(result, "LCEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1461);
    add_machine_instr(result, "LCDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1461);
    add_machine_instr(result, "LCXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1461);
    add_machine_instr(result, "FIEBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 32, 1462);
    add_machine_instr(result, "FIDBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 32, 1462);
    add_machine_instr(result, "FIXBR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 32, 1462);
    add_machine_instr(result, "FIEBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1462);
    add_machine_instr(result, "FIDBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1462);
    add_machine_instr(result, "FIXBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1462);
    add_machine_instr(result, "LDEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1463);
    add_machine_instr(result, "LXDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1463);
    add_machine_instr(result, "LXEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1463);
    add_machine_instr(result, "LDEB", mach_format::RRE, { reg_4_U, reg_4_U }, 48, 1464);
    add_machine_instr(result, "LXDB", mach_format::RRE, { reg_4_U, reg_4_U }, 48, 1464);
    add_machine_instr(result, "LXEB", mach_format::RRE, { reg_4_U, reg_4_U }, 48, 1464);
    add_machine_instr(result, "LNEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1464);
    add_machine_instr(result, "LNDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1464);
    add_machine_instr(result, "LNXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1464);
    add_machine_instr(result, "LPEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1465);
    add_machine_instr(result, "LPDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1465);
    add_machine_instr(result, "LPXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1465);
    add_machine_instr(result, "LEDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1465);
    add_machine_instr(result, "LDXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1465);
    add_machine_instr(result, "LEXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1465);
    add_machine_instr(result, "LEDBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1465);
    add_machine_instr(result, "LDXBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1465);
    add_machine_instr(result, "LEXBRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1465);
    add_machine_instr(result, "MEEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1467);
    add_machine_instr(result, "MDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1467);
    add_machine_instr(result, "MXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1467);
    add_machine_instr(result, "MDEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1467);
    add_machine_instr(result, "MXDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1467);
    add_machine_instr(result, "MEEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1467);
    add_machine_instr(result, "MDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1467);
    add_machine_instr(result, "MDEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1467);
    add_machine_instr(result, "MXDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1467);
    add_machine_instr(result, "MADBR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 32, 1468);
    add_machine_instr(result, "MAEBR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 32, 1468);
    add_machine_instr(result, "MAEB", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1468);
    add_machine_instr(result, "MADB", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1468);
    add_machine_instr(result, "MSEBR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 32, 1468);
    add_machine_instr(result, "MSDBR", mach_format::RRD, { reg_4_U, reg_4_U, reg_4_U }, 32, 1468);
    add_machine_instr(result, "MSEB", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1468);
    add_machine_instr(result, "MSDB", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1468);
    add_machine_instr(result, "SQEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1470);
    add_machine_instr(result, "SQDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1470);
    add_machine_instr(result, "SQXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1470);
    add_machine_instr(result, "SQEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1470);
    add_machine_instr(result, "SQDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1470);
    add_machine_instr(result, "SEBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1470);
    add_machine_instr(result, "SDBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1470);
    add_machine_instr(result, "SXBR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1470);
    add_machine_instr(result, "SEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1470);
    add_machine_instr(result, "SDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1470);
    add_machine_instr(result, "TCEB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1471);
    add_machine_instr(result, "TCDB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1471);
    add_machine_instr(result, "TCXB", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1471);
    add_machine_instr(result, "ADTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 1491);
    add_machine_instr(result, "AXTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 1491);
    add_machine_instr(result, "ADTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1491);
    add_machine_instr(result, "AXTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1491);
    add_machine_instr(result, "CDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1494);
    add_machine_instr(result, "CXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1494);
    add_machine_instr(result, "KDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1495);
    add_machine_instr(result, "KXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1495);
    add_machine_instr(result, "CEDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1495);
    add_machine_instr(result, "CEXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1495);
    add_machine_instr(result, "CDGTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1496);
    add_machine_instr(result, "CXGTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1496);
    add_machine_instr(result, "CDGTRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1496);
    add_machine_instr(result, "CXGTRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1496);
    add_machine_instr(result, "CDFTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1496);
    add_machine_instr(result, "CXFTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1496);
    add_machine_instr(result, "CDLGTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1497);
    add_machine_instr(result, "CXLGTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1497);
    add_machine_instr(result, "CDLFTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1497);
    add_machine_instr(result, "CXLFTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1497);
    add_machine_instr(result, "CDPT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 48, 1498);
    add_machine_instr(result, "CXPT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 48, 1498);
    add_machine_instr(result, "CDSTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1500);
    add_machine_instr(result, "CXSTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1500);
    add_machine_instr(result, "CDUTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1500);
    add_machine_instr(result, "CXUTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1500);
    add_machine_instr(result, "CDZT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 48, 1501);
    add_machine_instr(result, "CXZT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 48, 1501);
    add_machine_instr(result, "CGDTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 32, 1501);
    add_machine_instr(result, "CGXTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U }, 32, 1501);
    add_machine_instr(result, "CGDTRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1502);
    add_machine_instr(result, "CGXTRA", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1502);
    add_machine_instr(result, "CFDTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1502);
    add_machine_instr(result, "CFXTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1502);
    add_machine_instr(result, "CLGDTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1504);
    add_machine_instr(result, "CLGXTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1504);
    add_machine_instr(result, "CLFDTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1504);
    add_machine_instr(result, "CLFXTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1504);
    add_machine_instr(result, "CPDT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 48, 1505);
    add_machine_instr(result, "CPXT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 48, 1505);
    add_machine_instr(result, "CSDTR", mach_format::RRF_d, { reg_4_U, reg_4_U, mask_4_U }, 32, 1507);
    add_machine_instr(result, "CSXTR", mach_format::RRF_d, { reg_4_U, reg_4_U, mask_4_U }, 32, 1507);
    add_machine_instr(result, "CUDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1507);
    add_machine_instr(result, "CUXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1507);
    add_machine_instr(result, "CZDT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 48, 1508);
    add_machine_instr(result, "CZXT", mach_format::RSL_b, { reg_4_U, db_12_8x4L_U, mask_4_U }, 48, 1508);
    add_machine_instr(result, "DDTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 1509);
    add_machine_instr(result, "DXTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 1509);
    add_machine_instr(result, "DDTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1509);
    add_machine_instr(result, "DXTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1509);
    add_machine_instr(result, "EEXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1511);
    add_machine_instr(result, "EEDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1511);
    add_machine_instr(result, "ESDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1511);
    add_machine_instr(result, "ESXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1511);
    add_machine_instr(result, "IEDTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U }, 32, 1512);
    add_machine_instr(result, "IEXTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U }, 32, 1512);
    add_machine_instr(result, "LTDTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1513);
    add_machine_instr(result, "LTXTR", mach_format::RRE, { reg_4_U, reg_4_U }, 32, 1513);
    add_machine_instr(result, "FIDTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1514);
    add_machine_instr(result, "FIXTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1514);
    add_machine_instr(result, "LDETR", mach_format::RRF_d, { reg_4_U, reg_4_U, mask_4_U }, 32, 1517);
    add_machine_instr(result, "LXDTR", mach_format::RRF_d, { reg_4_U, reg_4_U, mask_4_U }, 32, 1517);
    add_machine_instr(result, "LEDTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1518);
    add_machine_instr(result, "LDXTR", mach_format::RRF_e, { reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1518);
    add_machine_instr(result, "MDTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 1519);
    add_machine_instr(result, "MXTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 1519);
    add_machine_instr(result, "MDTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1520);
    add_machine_instr(result, "MXTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1520);
    add_machine_instr(result, "QADTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1521);
    add_machine_instr(result, "QAXTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1521);
    add_machine_instr(result, "RRDTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1524);
    add_machine_instr(result, "RRXTR", mach_format::RRF_b, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1524);
    add_machine_instr(result, "SLDT", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1526);
    add_machine_instr(result, "SLXT", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1526);
    add_machine_instr(result, "SRDT", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1526);
    add_machine_instr(result, "SRXT", mach_format::RXF, { reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1526);
    add_machine_instr(result, "SDTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 1527);
    add_machine_instr(result, "SXTR", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U }, 32, 1527);
    add_machine_instr(result, "SDTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1527);
    add_machine_instr(result, "SXTRA", mach_format::RRF_a, { reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1527);
    add_machine_instr(result, "TDCET", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1528);
    add_machine_instr(result, "TDCDT", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1528);
    add_machine_instr(result, "TDCXT", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1528);
    add_machine_instr(result, "TDGET", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1529);
    add_machine_instr(result, "TDGDT", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1529);
    add_machine_instr(result, "TDGXT", mach_format::RXE, { reg_4_U, dxb_12_4x4_U }, 48, 1529);
    add_machine_instr(result, "VBPERM", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1536);
    add_machine_instr(result, "VGEF", mach_format::VRV, { vec_reg_4_U, dvb_12_4x4_U, mask_4_U }, 48, 1536);
    add_machine_instr(result, "VGEG", mach_format::VRV, { vec_reg_4_U, dvb_12_4x4_U, mask_4_U }, 48, 1536);
    add_machine_instr(result, "VGBM", mach_format::VRI_a, { vec_reg_4_U, imm_16_U }, 48, 1537);
    add_machine_instr(result, "VGM", mach_format::VRI_b, { vec_reg_4_U, imm_8_U, imm_8_U, mask_4_U }, 48, 1537);
    add_machine_instr(result, "VL", mach_format::VRX, { vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 1, 48, 1538);
    add_machine_instr(result, "VLREP", mach_format::VRX, { vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1538);
    add_machine_instr(result, "VLR", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U }, 48, 1538);
    add_machine_instr(result, "VLEB", mach_format::VRX, { vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1538);
    add_machine_instr(result, "VLEH", mach_format::VRX, { vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1539);
    add_machine_instr(result, "VLEIH", mach_format::VRI_a, { vec_reg_4_U, imm_16_S, mask_4_U }, 48, 1539);
    add_machine_instr(result, "VLEF", mach_format::VRX, { vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1539);
    add_machine_instr(result, "VLEIF", mach_format::VRI_a, { vec_reg_4_U, imm_16_S, mask_4_U }, 48, 1539);
    add_machine_instr(result, "VLEG", mach_format::VRX, { vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1539);
    add_machine_instr(result, "VLEIG", mach_format::VRI_a, { vec_reg_4_U, imm_16_S, mask_4_U }, 48, 1539);
    add_machine_instr(result, "VLEIB", mach_format::VRI_a, { vec_reg_4_U, imm_16_S, mask_4_U }, 48, 1539);
    add_machine_instr(result, "VLGV", mach_format::VRS_c, { reg_4_U, vec_reg_4_U, db_12_4_U, mask_4_U }, 48, 1539);
    add_machine_instr(result, "VLLEZ", mach_format::VRX, { vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1540);
    add_machine_instr(
        result, "VLM", mach_format::VRS_a, { vec_reg_4_U, vec_reg_4_U, db_12_4_U, mask_4_U }, 1, 48, 1541);
    add_machine_instr(result, "VLRLR", mach_format::VRS_d, { vec_reg_4_U, reg_4_U, db_12_4_U }, 48, 1541);
    add_machine_instr(result, "VLRL", mach_format::VSI, { vec_reg_4_U, db_12_4_U, imm_8_U }, 48, 1541);
    add_machine_instr(result, "VLBB", mach_format::VRX, { vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1542);
    add_machine_instr(result, "VLVG", mach_format::VRS_b, { vec_reg_4_U, reg_4_U, db_12_4_U, mask_4_U }, 48, 1543);
    add_machine_instr(result, "VLVGP", mach_format::VRR_f, { vec_reg_4_U, reg_4_U, reg_4_U }, 48, 1543);
    add_machine_instr(result, "VLL", mach_format::VRS_b, { vec_reg_4_U, reg_4_U, db_12_4_U }, 48, 1543);
    add_machine_instr(
        result, "VMRH", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1544);
    add_machine_instr(
        result, "VMRL", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1544);
    add_machine_instr(result, "VPK", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1545);
    add_machine_instr(
        result, "VPKS", mach_format::VRR_b, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1545);
    add_machine_instr(
        result, "VPKLS", mach_format::VRR_b, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1546);
    add_machine_instr(
        result, "VPERM", mach_format::VRR_e, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1547);
    add_machine_instr(
        result, "VPDI", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1547);
    add_machine_instr(result, "VREP", mach_format::VRI_c, { vec_reg_4_U, vec_reg_4_U, imm_16_U, mask_4_U }, 48, 1547);
    add_machine_instr(result, "VREPI", mach_format::VRI_a, { vec_reg_4_U, imm_16_S, mask_4_U }, 48, 1548);
    add_machine_instr(result, "VSCEF", mach_format::VRV, { vec_reg_4_U, dvb_12_4x4_U, mask_4_U }, 48, 1548);
    add_machine_instr(result, "VSCEG", mach_format::VRV, { vec_reg_4_U, dvb_12_4x4_U, mask_4_U }, 48, 1548);
    add_machine_instr(
        result, "VSEL", mach_format::VRR_e, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1549);
    add_machine_instr(result, "VSEG", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1549);
    add_machine_instr(result, "VST", mach_format::VRX, { vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 1, 48, 1550);
    add_machine_instr(result, "VSTEB", mach_format::VRX, { vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1550);
    add_machine_instr(result, "VSTEH", mach_format::VRX, { vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1550);
    add_machine_instr(result, "VSTEF", mach_format::VRX, { vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1550);
    add_machine_instr(result, "VSTEG", mach_format::VRX, { vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1550);
    add_machine_instr(
        result, "VSTM", mach_format::VRS_a, { vec_reg_4_U, vec_reg_4_U, db_12_4_U, mask_4_U }, 1, 48, 1551);
    add_machine_instr(result, "VSTRLR", mach_format::VRS_d, { vec_reg_4_U, reg_4_U, db_12_4_U }, 48, 1551);
    add_machine_instr(result, "VSTRL", mach_format::VSI, { vec_reg_4_U, db_12_4_U, imm_8_U }, 48, 1551);
    add_machine_instr(result, "VSTL", mach_format::VRS_b, { vec_reg_4_U, reg_4_U, db_12_4_U }, 48, 1552);
    add_machine_instr(result, "VUPH", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1552);
    add_machine_instr(result, "VUPL", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1553);
    add_machine_instr(result, "VUPLH", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1553);
    add_machine_instr(result, "VUPLL", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1554);
    add_machine_instr(result, "VA", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1557);
    add_machine_instr(
        result, "VACC", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1558);
    add_machine_instr(
        result, "VAC", mach_format::VRR_d, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1558);
    add_machine_instr(result,
        "VACCC",
        mach_format::VRR_d,
        { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U },
        48,
        1559);
    add_machine_instr(result, "VN", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1559);
    add_machine_instr(result, "VNC", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1559);
    add_machine_instr(
        result, "VAVG", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1560);
    add_machine_instr(
        result, "VAVGL", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1560);
    add_machine_instr(result, "VCKSM", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1560);
    add_machine_instr(result, "VEC", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1561);
    add_machine_instr(result, "VECL", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1561);
    add_machine_instr(
        result, "VCEQ", mach_format::VRR_b, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1561);
    add_machine_instr(
        result, "VCH", mach_format::VRR_b, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1562);
    add_machine_instr(
        result, "VCHL", mach_format::VRR_b, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1563);
    add_machine_instr(result, "VCLZ", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1564);
    add_machine_instr(result, "VCTZ", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1564);
    add_machine_instr(
        result, "VGFM", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1565);
    add_machine_instr(result, "VX", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1565);
    add_machine_instr(result, "VLC", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1566);
    add_machine_instr(result,
        "VGFMA",
        mach_format::VRR_d,
        { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U },
        48,
        1566);
    add_machine_instr(result, "VLP", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1566);
    add_machine_instr(result, "VMX", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1567);
    add_machine_instr(
        result, "VMXL", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1567);
    add_machine_instr(result, "VMN", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1567);
    add_machine_instr(
        result, "VMNL", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1568);
    add_machine_instr(
        result, "VMAL", mach_format::VRR_d, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1568);
    add_machine_instr(
        result, "VMAH", mach_format::VRR_d, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1569);
    add_machine_instr(result,
        "VMALH",
        mach_format::VRR_d,
        { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U },
        48,
        1569);
    add_machine_instr(
        result, "VMAE", mach_format::VRR_d, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1569);
    add_machine_instr(result,
        "VMALE",
        mach_format::VRR_d,
        { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U },
        48,
        1569);
    add_machine_instr(
        result, "VMAO", mach_format::VRR_d, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1570);
    add_machine_instr(result,
        "VMALO",
        mach_format::VRR_d,
        { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U },
        48,
        1570);
    add_machine_instr(result, "VMH", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1570);
    add_machine_instr(
        result, "VMLH", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1571);
    add_machine_instr(result, "VML", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1571);
    add_machine_instr(result, "VME", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1572);
    add_machine_instr(
        result, "VMLE", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1572);
    add_machine_instr(result, "VMO", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1572);
    add_machine_instr(
        result, "VMLO", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1572);
    add_machine_instr(result,
        "VMSL",
        mach_format::VRR_d,
        { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U },
        48,
        1573);
    add_machine_instr(result, "VNN", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1574);
    add_machine_instr(result, "VNO", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1574);
    result.insert(std::pair<const std::string, machine_instruction_ptr>("VNOT",
        std::make_unique<vnot_instruction>("VNOT",
            mach_format::VRR_c,
            std::vector<machine_operand_format> { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U },
            48,
            1574)));
    add_machine_instr(result, "VNX", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1574);
    add_machine_instr(result, "VO", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1574);
    add_machine_instr(result, "VOC", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1575);
    add_machine_instr(result, "VPOPCT", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1575);
    add_machine_instr(
        result, "VERLLV", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1575);
    add_machine_instr(result, "VERLL", mach_format::VRS_a, { vec_reg_4_U, vec_reg_4_U, db_12_4_U, mask_4_U }, 48, 1575);
    add_machine_instr(
        result, "VERIM", mach_format::VRI_d, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, imm_8_U, mask_4_U }, 48, 1576);
    add_machine_instr(
        result, "VESLV", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1577);
    add_machine_instr(result, "VESL", mach_format::VRS_a, { vec_reg_4_U, vec_reg_4_U, db_12_4_U, mask_4_U }, 48, 1577);
    add_machine_instr(
        result, "VESRAV", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1577);
    add_machine_instr(result, "VESRA", mach_format::VRS_a, { vec_reg_4_U, vec_reg_4_U, db_12_4_U, mask_4_U }, 48, 1577);
    add_machine_instr(
        result, "VESRLV", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1578);
    add_machine_instr(result, "VESRL", mach_format::VRS_a, { vec_reg_4_U, vec_reg_4_U, db_12_4_U, mask_4_U }, 48, 1578);
    add_machine_instr(
        result, "VSLDB", mach_format::VRI_d, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, imm_8_U }, 48, 1579);
    add_machine_instr(result, "VSL", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1579);
    add_machine_instr(result, "VSLB", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1579);
    add_machine_instr(result, "VSRA", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1579);
    add_machine_instr(result, "VSRAB", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1580);
    add_machine_instr(result, "VSRL", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1580);
    add_machine_instr(result, "VSRLB", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1580);
    add_machine_instr(result, "VS", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1580);
    add_machine_instr(
        result, "VSCBI", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1581);
    add_machine_instr(
        result, "VSBI", mach_format::VRR_d, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1581);
    add_machine_instr(result,
        "VSBCBI",
        mach_format::VRR_d,
        { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U },
        48,
        1582);
    add_machine_instr(
        result, "VSUMG", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1582);
    add_machine_instr(
        result, "VSUMQ", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1583);
    add_machine_instr(
        result, "VSUM", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1583);
    add_machine_instr(result, "VTM", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U }, 48, 1584);
    add_machine_instr(
        result, "VFAE", mach_format::VRR_b, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 1, 48, 1585);
    add_machine_instr(
        result, "VFEE", mach_format::VRR_b, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 1, 48, 1587);
    add_machine_instr(result,
        "VFENE",
        mach_format::VRR_b,
        { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U },
        1,
        48,
        1588);
    add_machine_instr(
        result, "VISTR", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 1, 48, 1589);
    add_machine_instr(result,
        "VSTRC",
        mach_format::VRR_d,
        { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U },
        1,
        48,
        1590);
    add_machine_instr(
        result, "VFA", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1595);
    add_machine_instr(result, "WFC", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1599);
    add_machine_instr(result, "WFK", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1600);
    add_machine_instr(result,
        "VFCE",
        mach_format::VRR_c,
        { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U },
        48,
        1601);
    add_machine_instr(result,
        "VFCH",
        mach_format::VRR_c,
        { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U },
        48,
        1603);
    add_machine_instr(result,
        "VFCHE",
        mach_format::VRR_c,
        { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U },
        48,
        1605);
    add_machine_instr(
        result, "VCDG", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U }, 48, 1607);
    add_machine_instr(
        result, "VCDLG", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U }, 48, 1608);
    add_machine_instr(
        result, "VCGD", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U }, 48, 1609);
    add_machine_instr(
        result, "VCLGD", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U }, 48, 1611);
    add_machine_instr(
        result, "VFD", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1613);
    add_machine_instr(
        result, "VFI", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U }, 48, 1615);
    add_machine_instr(result, "VFLL", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1617);
    add_machine_instr(
        result, "VFLR", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U }, 48, 1618);
    add_machine_instr(result,
        "VFMAX",
        mach_format::VRR_c,
        { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U },
        48,
        1619);
    add_machine_instr(result,
        "VFMIN",
        mach_format::VRR_c,
        { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U },
        48,
        1625);
    add_machine_instr(
        result, "VFM", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1631);
    add_machine_instr(result,
        "VFMA",
        mach_format::VRR_e,
        { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U },
        48,
        1633);
    add_machine_instr(result,
        "VFMS",
        mach_format::VRR_e,
        { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U },
        48,
        1633);
    add_machine_instr(result,
        "VFNMA",
        mach_format::VRR_e,
        { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U },
        48,
        1633);
    add_machine_instr(result,
        "VFNMS",
        mach_format::VRR_e,
        { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U },
        48,
        1633);
    add_machine_instr(
        result, "VFPSO", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U }, 48, 1635);
    add_machine_instr(result, "VFSQ", mach_format::VRR_a, { vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1636);
    add_machine_instr(
        result, "VFS", mach_format::VRR_c, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1637);
    add_machine_instr(
        result, "VFTCI", mach_format::VRI_e, { vec_reg_4_U, vec_reg_4_U, imm_12_S, mask_4_U, mask_4_U }, 48, 1638);
    add_machine_instr(
        result, "VAP", mach_format::VRI_f, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, imm_8_U, mask_4_U }, 48, 1643);
    add_machine_instr(result, "VCP", mach_format::VRR_h, { vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1644);
    add_machine_instr(result, "VCVB", mach_format::VRR_i, { reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1645);
    add_machine_instr(result, "VCVBG", mach_format::VRR_i, { reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1645);
    add_machine_instr(result, "VCVD", mach_format::VRI_i, { vec_reg_4_U, reg_4_U, imm_8_S, mask_4_U }, 48, 1646);
    add_machine_instr(result, "VCVDG", mach_format::VRI_i, { vec_reg_4_U, reg_4_U, imm_8_S, mask_4_U }, 48, 1646);
    add_machine_instr(
        result, "VDP", mach_format::VRI_f, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, imm_8_U, mask_4_U }, 48, 1648);
    add_machine_instr(
        result, "VMP", mach_format::VRI_f, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, imm_8_U, mask_4_U }, 48, 1650);
    add_machine_instr(
        result, "VMSP", mach_format::VRI_f, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, imm_8_U, mask_4_U }, 48, 1651);
    add_machine_instr(
        result, "VRP", mach_format::VRI_f, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, imm_8_U, mask_4_U }, 48, 1654);
    add_machine_instr(
        result, "VSDP", mach_format::VRI_f, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, imm_8_U, mask_4_U }, 48, 1656);
    add_machine_instr(
        result, "VSP", mach_format::VRI_f, { vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, imm_8_U, mask_4_U }, 48, 1658);
    add_machine_instr(result, "VLIP", mach_format::VRI_h, { vec_reg_4_U, imm_16_S, imm_4_U }, 48, 1649);
    add_machine_instr(result, "VPKZ", mach_format::VSI, { vec_reg_4_U, db_12_4_U, imm_8_U }, 48, 1652);
    add_machine_instr(
        result, "VPSOP", mach_format::VRI_g, { vec_reg_4_U, vec_reg_4_U, imm_8_U, imm_8_U, mask_4_U }, 48, 1653);
    add_machine_instr(
        result, "VSRP", mach_format::VRI_g, { vec_reg_4_U, vec_reg_4_U, imm_8_U, imm_8_S, mask_4_U }, 48, 1657);
    add_machine_instr(result, "VTP", mach_format::VRR_g, { vec_reg_4_U }, 48, 1660);
    add_machine_instr(result, "VUPKZ", mach_format::VSI, { vec_reg_4_U, db_12_4_U, imm_8_U }, 48, 1660);
    return result;
}

void add_mnemonic_code(std::map<const std::string, mnemonic_code>& result, std::string instr, mnemonic_code code)
{
    result.insert(std::make_pair<const std::string, mnemonic_code>(std::move(instr), std::move(code)));
}

std::map<const std::string, mnemonic_code> hlasm_plugin::parser_library::context::instruction::get_mnemonic_codes()
{
    std::map<const std::string, mnemonic_code> result;
    add_mnemonic_code(result, "B", { "BC", { { 0, 15 } } });
    add_mnemonic_code(result, "BR", { "BCR", { { 0, 15 } } });
    add_mnemonic_code(result, "J", { "BRC", { { 0, 15 } } });
    add_mnemonic_code(result, "NOP", { "BC", { { 0, 0 } } });
    add_mnemonic_code(result, "NOPR", { "BCR", { { 0, 0 } } });
    add_mnemonic_code(result, "JNOP", { "BRC", { { 0, 0 } } });
    add_mnemonic_code(result, "BH", { "BC", { { 0, 2 } } });
    add_mnemonic_code(result, "BHR", { "BCR", { { 0, 2 } } });
    add_mnemonic_code(result, "JH", { "BRC", { { 0, 2 } } });
    add_mnemonic_code(result, "BL", { "BC", { { 0, 4 } } });
    add_mnemonic_code(result, "BLR", { "BCR", { { 0, 4 } } });
    add_mnemonic_code(result, "JL", { "BRC", { { 0, 4 } } });
    add_mnemonic_code(result, "BE", { "BC", { { 0, 8 } } });
    add_mnemonic_code(result, "BER", { "BCR", { { 0, 8 } } });
    add_mnemonic_code(result, "JE", { "BRC", { { 0, 8 } } });
    add_mnemonic_code(result, "BNH", { "BC", { { 0, 13 } } });
    add_mnemonic_code(result, "BNHR", { "BCR", { { 0, 13 } } });
    add_mnemonic_code(result, "JNH", { "BRC", { { 0, 13 } } });
    add_mnemonic_code(result, "BNL", { "BC", { { 0, 11 } } });
    add_mnemonic_code(result, "BNLR", { "BCR", { { 0, 11 } } });
    add_mnemonic_code(result, "JNL", { "BRC", { { 0, 11 } } });
    add_mnemonic_code(result, "BNE", { "BC", { { 0, 7 } } });
    add_mnemonic_code(result, "BNER", { "BCR", { { 0, 7 } } });
    add_mnemonic_code(result, "JNE", { "BRC", { { 0, 7 } } });
    add_mnemonic_code(result, "BP", { "BC", { { 0, 2 } } });
    add_mnemonic_code(result, "BPR", { "BCR", { { 0, 2 } } });
    add_mnemonic_code(result, "JP", { "BRC", { { 0, 2 } } });
    add_mnemonic_code(result, "JM", { "BRC", { { 0, 4 } } });
    add_mnemonic_code(result, "JZ", { "BRC", { { 0, 8 } } });
    add_mnemonic_code(result, "JO", { "BRC", { { 0, 1 } } });
    add_mnemonic_code(result, "BNP", { "BC", { { 0, 13 } } });
    add_mnemonic_code(result, "BNPR", { "BCR", { { 0, 13 } } });
    add_mnemonic_code(result, "JNP", { "BRC", { { 0, 13 } } });
    add_mnemonic_code(result, "BNM", { "BC", { { 0, 11 } } });
    add_mnemonic_code(result, "JNM", { "BRC", { { 0, 11 } } });
    add_mnemonic_code(result, "BNZ", { "BC", { { 0, 7 } } });
    add_mnemonic_code(result, "JNZ", { "BRC", { { 0, 7 } } });
    add_mnemonic_code(result, "BNO", { "BC", { { 0, 14 } } });
    add_mnemonic_code(result, "JNO", { "BRC", { { 0, 14 } } });
    add_mnemonic_code(result, "BO", { "BC", { { 0, 1 } } });
    add_mnemonic_code(result, "BOR", { "BCR", { { 0, 1 } } });
    add_mnemonic_code(result, "BM", { "BC", { { 0, 4 } } });
    add_mnemonic_code(result, "BMR", { "BCR", { { 0, 4 } } });
    add_mnemonic_code(result, "BZ", { "BC", { { 0, 8 } } });
    add_mnemonic_code(result, "BZR", { "BCR", { { 0, 8 } } });
    add_mnemonic_code(result, "BNOR", { "BCR", { { 0, 14 } } });
    add_mnemonic_code(result, "BNMR", { "BCR", { { 0, 11 } } });
    add_mnemonic_code(result, "BNZR", { "BCR", { { 0, 7 } } });
    add_mnemonic_code(result, "BRUL", { "BRCL", { { 0, 15 } } });
    add_mnemonic_code(result, "BRHL", { "BRCL", { { 0, 2 } } });
    add_mnemonic_code(result, "BRLL", { "BRCL", { { 0, 4 } } });
    add_mnemonic_code(result, "BREL", { "BRCL", { { 0, 8 } } });
    add_mnemonic_code(result, "BRNHL", { "BRCL", { { 0, 13 } } });
    add_mnemonic_code(result, "BRNLL", { "BRCL", { { 0, 11 } } });
    add_mnemonic_code(result, "BRNEL", { "BRCL", { { 0, 7 } } });
    add_mnemonic_code(result, "BRPL", { "BRCL", { { 0, 2 } } });
    add_mnemonic_code(result, "BRML", { "BRCL", { { 0, 4 } } });
    add_mnemonic_code(result, "BRZL", { "BRCL", { { 0, 8 } } });
    add_mnemonic_code(result, "BROL", { "BRCL", { { 0, 1 } } });
    add_mnemonic_code(result, "BRNPL", { "BRCL", { { 0, 13 } } });
    add_mnemonic_code(result, "BRNML", { "BRCL", { { 0, 11 } } });
    add_mnemonic_code(result, "BRNZL", { "BRCL", { { 0, 7 } } });
    add_mnemonic_code(result, "BRNOL", { "BRCL", { { 0, 14 } } });
    add_mnemonic_code(result, "BRO", { "BRC", { { 0, 1 } } });
    add_mnemonic_code(result, "BRP", { "BRC", { { 0, 2 } } });
    add_mnemonic_code(result, "BRH", { "BRC", { { 0, 2 } } });
    add_mnemonic_code(result, "BRL", { "BRC", { { 0, 4 } } });
    add_mnemonic_code(result, "BRM", { "BRC", { { 0, 4 } } });
    add_mnemonic_code(result, "BRNE", { "BRC", { { 0, 7 } } });
    add_mnemonic_code(result, "BRNZ", { "BRC", { { 0, 7 } } });
    add_mnemonic_code(result, "BRE", { "BRC", { { 0, 8 } } });
    add_mnemonic_code(result, "BRZ", { "BRC", { { 0, 8 } } });
    add_mnemonic_code(result, "BRNL", { "BRC", { { 0, 11 } } });
    add_mnemonic_code(result, "BRNM", { "BRC", { { 0, 11 } } });
    add_mnemonic_code(result, "BRNH", { "BRC", { { 0, 13 } } });
    add_mnemonic_code(result, "BRNP", { "BRC", { { 0, 13 } } });
    add_mnemonic_code(result, "BRNO", { "BRC", { { 0, 14 } } });
    add_mnemonic_code(result, "BRU", { "BRC", { { 0, 15 } } });
    add_mnemonic_code(result, "JLU", { "BRCL", { { 0, 15 } } });
    add_mnemonic_code(result, "JLNOP", { "BRCL", { { 0, 0 } } });
    add_mnemonic_code(result, "JLH", { "BRCL", { { 0, 2 } } });
    add_mnemonic_code(result, "JLL", { "BRCL", { { 0, 4 } } });
    add_mnemonic_code(result, "JLE", { "BRCL", { { 0, 8 } } });
    add_mnemonic_code(result, "JLNH", { "BRCL", { { 0, 13 } } });
    add_mnemonic_code(result, "JLNL", { "BRCL", { { 0, 11 } } });
    add_mnemonic_code(result, "JLNE", { "BRCL", { { 0, 7 } } });
    add_mnemonic_code(result, "JLP", { "BRCL", { { 0, 2 } } });
    add_mnemonic_code(result, "JLM", { "BRCL", { { 0, 4 } } });
    add_mnemonic_code(result, "JLZ", { "BRCL", { { 0, 8 } } });
    add_mnemonic_code(result, "JLO", { "BRCL", { { 0, 1 } } });
    add_mnemonic_code(result, "JLNP", { "BRCL", { { 0, 13 } } });
    add_mnemonic_code(result, "JLNM", { "BRCL", { { 0, 11 } } });
    add_mnemonic_code(result, "JLNZ", { "BRCL", { { 0, 7 } } });
    add_mnemonic_code(result, "JLNO", { "BRCL", { { 0, 14 } } });
    add_mnemonic_code(result, "JAS", { "BRAS", {} });
    add_mnemonic_code(result, "JASL", { "BRASL", {} });
    add_mnemonic_code(result, "JCT", { "BRCT", {} });
    add_mnemonic_code(result, "JCTG", { "BRCTG", {} });
    add_mnemonic_code(result, "JXH", { "BRXH", {} });
    add_mnemonic_code(result, "JXHG", { "BRXHG", {} });
    add_mnemonic_code(result, "JXLE", { "BRXLE", {} });
    add_mnemonic_code(result, "JXLEG", { "BRXLG", {} });
    add_mnemonic_code(result, "BIO", { "BIC", { { 0, 1 } } });
    add_mnemonic_code(result, "BIP", { "BIC", { { 0, 2 } } });
    add_mnemonic_code(result, "BIH", { "BIC", { { 0, 2 } } });
    add_mnemonic_code(result, "BIM", { "BIC", { { 0, 4 } } });
    add_mnemonic_code(result, "BIL", { "BIC", { { 0, 4 } } });
    add_mnemonic_code(result, "BINZ", { "BIC", { { 0, 7 } } });
    add_mnemonic_code(result, "BINE", { "BIC", { { 0, 7 } } });
    add_mnemonic_code(result, "BIZ", { "BIC", { { 0, 8 } } });
    add_mnemonic_code(result, "BIE", { "BIC", { { 0, 8 } } });
    add_mnemonic_code(result, "BINM", { "BIC", { { 0, 11 } } });
    add_mnemonic_code(result, "BINL", { "BIC", { { 0, 11 } } });
    add_mnemonic_code(result, "BINP", { "BIC", { { 0, 13 } } });
    add_mnemonic_code(result, "BINH", { "BIC", { { 0, 13 } } });
    add_mnemonic_code(result, "BINO", { "BIC", { { 0, 14 } } });
    add_mnemonic_code(result, "BI", { "BIC", { { 0, 15 } } });
    add_mnemonic_code(result, "VZERO", { "VGBM", { { 0, 1 } } });
    add_mnemonic_code(result, "VONE", { "VGBM", { { 1, 65535 } } });
    add_mnemonic_code(result, "VGMB", { "VGM", { { 3, 0 } } });
    add_mnemonic_code(result, "VGMH", { "VGM", { { 3, 1 } } });
    add_mnemonic_code(result, "VGMF", { "VGM", { { 3, 2 } } });
    add_mnemonic_code(result, "VGMG", { "VGM", { { 3, 3 } } });
    add_mnemonic_code(result, "VLREPB", { "VLREP", { { 2, 0 } } });
    add_mnemonic_code(result, "VLREPH", { "VLREP", { { 2, 1 } } });
    add_mnemonic_code(result, "VLREPF", { "VLREP", { { 2, 2 } } });
    add_mnemonic_code(result, "VLREPG", { "VLREP", { { 2, 3 } } });
    add_mnemonic_code(result, "VLGVB", { "VLGV", { { 3, 0 } } });
    add_mnemonic_code(result, "VLGVH", { "VLGV", { { 3, 1 } } });
    add_mnemonic_code(result, "VLGVF", { "VLGV", { { 3, 2 } } });
    add_mnemonic_code(result, "VLGVG", { "VLGV", { { 3, 3 } } });
    add_mnemonic_code(result, "VLLEZB", { "VLLEZ", { { 2, 0 } } });
    add_mnemonic_code(result, "VLLEZH", { "VLLEZ", { { 2, 1 } } });
    add_mnemonic_code(result, "VLLEZF", { "VLLEZ", { { 2, 2 } } });
    add_mnemonic_code(result, "VLLEZG", { "VLLEZ", { { 2, 3 } } });
    add_mnemonic_code(result, "VLLEZLF", { "VLLEZ", { { 2, 6 } } });
    add_mnemonic_code(result, "VLVGB", { "VLVG", { { 3, 0 } } });
    add_mnemonic_code(result, "VLVGH", { "VLVG", { { 3, 1 } } });
    add_mnemonic_code(result, "VLVGF", { "VLVG", { { 3, 2 } } });
    add_mnemonic_code(result, "VLVGG", { "VLVG", { { 3, 3 } } });
    add_mnemonic_code(result, "VMRHB", { "VMRH", { { 3, 0 } } });
    add_mnemonic_code(result, "VMRHH", { "VMRH", { { 3, 1 } } });
    add_mnemonic_code(result, "VMRHF", { "VMRH", { { 3, 2 } } });
    add_mnemonic_code(result, "VMRHG", { "VMRH", { { 3, 3 } } });
    add_mnemonic_code(result, "VMRLB", { "VMRL", { { 3, 0 } } });
    add_mnemonic_code(result, "VMRLH", { "VMRL", { { 3, 1 } } });
    add_mnemonic_code(result, "VMRLF", { "VMRL", { { 3, 2 } } });
    add_mnemonic_code(result, "VMRLG", { "VMRL", { { 3, 3 } } });
    add_mnemonic_code(result, "VPKSH", { "VPKS", { { 3, 1 }, { 4, 0 } } });
    add_mnemonic_code(result, "VPKSF", { "VPKS", { { 3, 2 }, { 4, 0 } } });
    add_mnemonic_code(result, "VPKSG", { "VPKS", { { 3, 3 }, { 4, 0 } } });
    add_mnemonic_code(result, "VPKSHS", { "VPKS", { { 3, 1 }, { 4, 1 } } });
    add_mnemonic_code(result, "VPKSFS", { "VPKS", { { 3, 2 }, { 4, 1 } } });
    add_mnemonic_code(result, "VPKSGS", { "VPKS", { { 3, 3 }, { 4, 1 } } });
    add_mnemonic_code(result, "VPKLSH", { "VPKLS", { { 3, 1 }, { 4, 0 } } });
    add_mnemonic_code(result, "VPKLSF", { "VPKLS", { { 3, 2 }, { 4, 0 } } });
    add_mnemonic_code(result, "VPKLSG", { "VPKLS", { { 3, 3 }, { 4, 0 } } });
    add_mnemonic_code(result, "VPKLSHS", { "VPKLS", { { 3, 1 }, { 4, 1 } } });
    add_mnemonic_code(result, "VPKLSFS", { "VPKLS", { { 3, 2 }, { 4, 1 } } });
    add_mnemonic_code(result, "VPKLSGS", { "VPKLS", { { 3, 3 }, { 4, 1 } } });
    add_mnemonic_code(result, "VREPB", { "VREP", { { 3, 0 } } });
    add_mnemonic_code(result, "VREPH", { "VREP", { { 3, 1 } } });
    add_mnemonic_code(result, "VREPF", { "VREP", { { 3, 2 } } });
    add_mnemonic_code(result, "VREPG", { "VREP", { { 3, 3 } } });
    add_mnemonic_code(result, "VREPIB", { "VREPI", { { 2, 0 } } });
    add_mnemonic_code(result, "VREPIH", { "VREPI", { { 2, 1 } } });
    add_mnemonic_code(result, "VREPIF", { "VREPI", { { 2, 2 } } });
    add_mnemonic_code(result, "VREPIG", { "VREPI", { { 2, 3 } } });
    add_mnemonic_code(result, "VSEGB", { "VSEG", { { 2, 0 } } });
    add_mnemonic_code(result, "VSEGH", { "VSEG", { { 2, 1 } } });
    add_mnemonic_code(result, "VSEGF", { "VSEG", { { 2, 2 } } });
    add_mnemonic_code(result, "VUPHB", { "VUPH", { { 2, 0 } } });
    add_mnemonic_code(result, "VUPHH", { "VUPH", { { 2, 1 } } });
    add_mnemonic_code(result, "VUPHF", { "VUPH", { { 2, 2 } } });
    add_mnemonic_code(result, "VUPLHB", { "VUPLH", { { 2, 0 } } });
    add_mnemonic_code(result, "VUPLHG", { "VUPLH", { { 2, 1 } } });
    add_mnemonic_code(result, "VUPLHF", { "VUPLH", { { 2, 2 } } });
    add_mnemonic_code(result, "VUPLB", { "VUPL", { { 2, 0 } } });
    add_mnemonic_code(result, "VUPLHW", { "VUPL", { { 2, 1 } } });
    add_mnemonic_code(result, "VUPLF", { "VUPL", { { 2, 2 } } });
    add_mnemonic_code(result, "VUPLLB", { "VUPLL", { { 2, 0 } } });
    add_mnemonic_code(result, "VUPLLH", { "VUPLL", { { 2, 1 } } });
    add_mnemonic_code(result, "VUPLLF", { "VUPLL", { { 2, 2 } } });
    add_mnemonic_code(result, "VAB", { "VA", { { 3, 0 } } });
    add_mnemonic_code(result, "VAH", { "VA", { { 3, 1 } } });
    add_mnemonic_code(result, "VAF", { "VA", { { 3, 2 } } });
    add_mnemonic_code(result, "VAG", { "VA", { { 3, 3 } } });
    add_mnemonic_code(result, "VAQ", { "VA", { { 3, 4 } } });
    add_mnemonic_code(result, "VACCB", { "VACC", { { 3, 0 } } });
    add_mnemonic_code(result, "VACCH", { "VACC", { { 3, 1 } } });
    add_mnemonic_code(result, "VACCF", { "VACC", { { 3, 2 } } });
    add_mnemonic_code(result, "VACCG", { "VACC", { { 3, 3 } } });
    add_mnemonic_code(result, "VACCQ", { "VACC", { { 3, 4 } } });
    add_mnemonic_code(result, "VACQ", { "VAC", { { 3, 4 } } });
    add_mnemonic_code(result, "VACCCQ", { "VACCC", { { 3, 4 } } });
    add_mnemonic_code(result, "VAVGB", { "VAVG", { { 3, 0 } } });
    add_mnemonic_code(result, "VAVGH", { "VAVG", { { 3, 1 } } });
    add_mnemonic_code(result, "VAVGF", { "VAVG", { { 3, 2 } } });
    add_mnemonic_code(result, "VAVGG", { "VAVG", { { 3, 3 } } });
    add_mnemonic_code(result, "VAVGLB", { "VAVGL", { { 3, 0 } } });
    add_mnemonic_code(result, "VAVGLH", { "VAVGL", { { 3, 1 } } });
    add_mnemonic_code(result, "VAVGLF", { "VAVGL", { { 3, 2 } } });
    add_mnemonic_code(result, "VAVGLG", { "VAVGL", { { 3, 3 } } });
    add_mnemonic_code(result, "VECB", { "VEC", { { 2, 0 } } });
    add_mnemonic_code(result, "VECH", { "VEC", { { 2, 1 } } });
    add_mnemonic_code(result, "VECF", { "VEC", { { 2, 2 } } });
    add_mnemonic_code(result, "VECG", { "VEC", { { 2, 3 } } });
    add_mnemonic_code(result, "VECLB", { "VECL", { { 2, 0 } } });
    add_mnemonic_code(result, "VECLH", { "VECL", { { 2, 1 } } });
    add_mnemonic_code(result, "VECLF", { "VECL", { { 2, 2 } } });
    add_mnemonic_code(result, "VECLG", { "VECL", { { 2, 3 } } });
    add_mnemonic_code(result, "VCEQB", { "VCEQ", { { 3, 0 }, { 4, 0 } } });
    add_mnemonic_code(result, "VCEQH", { "VCEQ", { { 3, 1 }, { 4, 0 } } });
    add_mnemonic_code(result, "VCEQF", { "VCEQ", { { 3, 2 }, { 4, 0 } } });
    add_mnemonic_code(result, "VCEQG", { "VCEQ", { { 3, 3 }, { 4, 0 } } });
    add_mnemonic_code(result, "VCEQBS", { "VCEQ", { { 3, 0 }, { 4, 1 } } });
    add_mnemonic_code(result, "VCEQHS", { "VCEQ", { { 3, 1 }, { 4, 1 } } });
    add_mnemonic_code(result, "VCEQFS", { "VCEQ", { { 3, 2 }, { 4, 1 } } });
    add_mnemonic_code(result, "VCEQGS", { "VCEQ", { { 3, 3 }, { 4, 1 } } });
    add_mnemonic_code(result, "VCHB", { "VCH", { { 3, 0 }, { 4, 0 } } });
    add_mnemonic_code(result, "VCHH", { "VCH", { { 3, 1 }, { 4, 0 } } });
    add_mnemonic_code(result, "VCHF", { "VCH", { { 3, 2 }, { 4, 0 } } });
    add_mnemonic_code(result, "VCHG", { "VCH", { { 3, 3 }, { 4, 0 } } });
    add_mnemonic_code(result, "VCHBS", { "VCH", { { 3, 0 }, { 4, 1 } } });
    add_mnemonic_code(result, "VCHHS", { "VCH", { { 3, 1 }, { 4, 1 } } });
    add_mnemonic_code(result, "VCHFS", { "VCH", { { 3, 2 }, { 4, 1 } } });
    add_mnemonic_code(result, "VCHGS", { "VCH", { { 3, 3 }, { 4, 1 } } });
    add_mnemonic_code(result, "VCHLB", { "VCHL", { { 3, 0 }, { 4, 0 } } });
    add_mnemonic_code(result, "VCHLH", { "VCHL", { { 3, 1 }, { 4, 0 } } });
    add_mnemonic_code(result, "VCHLF", { "VCHL", { { 3, 2 }, { 4, 0 } } });
    add_mnemonic_code(result, "VCHLG", { "VCHL", { { 3, 3 }, { 4, 0 } } });
    add_mnemonic_code(result, "VCHLBS", { "VCHL", { { 3, 0 }, { 4, 1 } } });
    add_mnemonic_code(result, "VCHLHS", { "VCHL", { { 3, 1 }, { 4, 1 } } });
    add_mnemonic_code(result, "VCHLFS", { "VCHL", { { 3, 2 }, { 4, 1 } } });
    add_mnemonic_code(result, "VCHLGS", { "VCHL", { { 3, 3 }, { 4, 1 } } });
    add_mnemonic_code(result, "VCLZB", { "VCLZ", { { 2, 0 } } });
    add_mnemonic_code(result, "VCLZH", { "VCLZ", { { 2, 1 } } });
    add_mnemonic_code(result, "VCLZF", { "VCLZ", { { 2, 2 } } });
    add_mnemonic_code(result, "VCLZG", { "VCLZ", { { 2, 3 } } });
    add_mnemonic_code(result, "VGFMB", { "VGFM", { { 3, 0 } } });
    add_mnemonic_code(result, "VGFMH", { "VGFM", { { 3, 1 } } });
    add_mnemonic_code(result, "VGFMF", { "VGFM", { { 3, 2 } } });
    add_mnemonic_code(result, "VGFMG", { "VGFM", { { 3, 3 } } });
    add_mnemonic_code(result, "VGFMAB", { "VGFMA", { { 4, 0 } } });
    add_mnemonic_code(result, "VGFMAH", { "VGFMA", { { 4, 1 } } });
    add_mnemonic_code(result, "VGFMAF", { "VGFMA", { { 4, 2 } } });
    add_mnemonic_code(result, "VGFMAG", { "VGFMA", { { 4, 3 } } });
    add_mnemonic_code(result, "VLCB", { "VLC", { { 2, 0 } } });
    add_mnemonic_code(result, "VLCH", { "VLC", { { 2, 1 } } });
    add_mnemonic_code(result, "VLCF", { "VLC", { { 2, 2 } } });
    add_mnemonic_code(result, "VLCG", { "VLC", { { 2, 3 } } });
    add_mnemonic_code(result, "VLPB", { "VLP", { { 2, 0 } } });
    add_mnemonic_code(result, "VLPH", { "VLP", { { 2, 1 } } });
    add_mnemonic_code(result, "VLPF", { "VLP", { { 2, 2 } } });
    add_mnemonic_code(result, "VLPG", { "VLP", { { 2, 3 } } });
    add_mnemonic_code(result, "VMXB", { "VMX", { { 3, 0 } } });
    add_mnemonic_code(result, "VMXH", { "VMX", { { 3, 1 } } });
    add_mnemonic_code(result, "VMXF", { "VMX", { { 3, 2 } } });
    add_mnemonic_code(result, "VMXG", { "VMX", { { 3, 3 } } });
    add_mnemonic_code(result, "VMXLB", { "VMXL", { { 3, 0 } } });
    add_mnemonic_code(result, "VMXLH", { "VMXL", { { 3, 1 } } });
    add_mnemonic_code(result, "VMXLF", { "VMXL", { { 3, 2 } } });
    add_mnemonic_code(result, "VMXLG", { "VMXL", { { 3, 3 } } });
    add_mnemonic_code(result, "VMNB", { "VMN", { { 3, 0 } } });
    add_mnemonic_code(result, "VMNH", { "VMN", { { 3, 1 } } });
    add_mnemonic_code(result, "VMNF", { "VMN", { { 3, 2 } } });
    add_mnemonic_code(result, "VMNG", { "VMN", { { 3, 3 } } });
    add_mnemonic_code(result, "VMNLB", { "VMNL", { { 3, 0 } } });
    add_mnemonic_code(result, "VMNLH", { "VMNL", { { 3, 1 } } });
    add_mnemonic_code(result, "VMNLF", { "VMNL", { { 3, 2 } } });
    add_mnemonic_code(result, "VMNLG", { "VMNL", { { 3, 3 } } });
    add_mnemonic_code(result, "VMALB", { "VMAL", { { 4, 0 } } });
    add_mnemonic_code(result, "VMALHW", { "VMAL", { { 4, 1 } } });
    add_mnemonic_code(result, "VMALF", { "VMAL", { { 4, 2 } } });
    add_mnemonic_code(result, "VMAHB", { "VMAH", { { 4, 0 } } });
    add_mnemonic_code(result, "VMAHH", { "VMAH", { { 4, 1 } } });
    add_mnemonic_code(result, "VMAHF", { "VMAH", { { 4, 2 } } });
    add_mnemonic_code(result, "VMALHB", { "VMALH", { { 4, 0 } } });
    add_mnemonic_code(result, "VMALHH", { "VMALH", { { 4, 1 } } });
    add_mnemonic_code(result, "VMALHF", { "VMALH", { { 4, 2 } } });
    add_mnemonic_code(result, "VMAEB", { "VMAE", { { 4, 0 } } });
    add_mnemonic_code(result, "VMAEH", { "VMAE", { { 4, 1 } } });
    add_mnemonic_code(result, "VMAEF", { "VMAE", { { 4, 2 } } });
    add_mnemonic_code(result, "VMALEB", { "VMALE", { { 4, 0 } } });
    add_mnemonic_code(result, "VMALEH", { "VMALE", { { 4, 1 } } });
    add_mnemonic_code(result, "VMALEF", { "VMALE", { { 4, 2 } } });
    add_mnemonic_code(result, "VMAOB", { "VMAO", { { 4, 0 } } });
    add_mnemonic_code(result, "VMAOH", { "VMAO", { { 4, 1 } } });
    add_mnemonic_code(result, "VMAOF", { "VMAO", { { 4, 2 } } });
    add_mnemonic_code(result, "VMALOB", { "VMALO", { { 4, 0 } } });
    add_mnemonic_code(result, "VMALOH", { "VMALO", { { 4, 1 } } });
    add_mnemonic_code(result, "VMALOF", { "VMALO", { { 4, 2 } } });
    add_mnemonic_code(result, "VMHB", { "VMH", { { 3, 0 } } });
    add_mnemonic_code(result, "VMHH", { "VMH", { { 3, 1 } } });
    add_mnemonic_code(result, "VMHF", { "VMH", { { 3, 2 } } });
    add_mnemonic_code(result, "VMLHB", { "VMLH", { { 3, 0 } } });
    add_mnemonic_code(result, "VMLHH", { "VMLH", { { 3, 1 } } });
    add_mnemonic_code(result, "VMLHF", { "VMLH", { { 3, 2 } } });
    add_mnemonic_code(result, "VMLB", { "VML", { { 3, 0 } } });
    add_mnemonic_code(result, "VMLHW", { "VML", { { 3, 1 } } });
    add_mnemonic_code(result, "VMLF", { "VML", { { 3, 2 } } });
    add_mnemonic_code(result, "VMEB", { "VME", { { 3, 0 } } });
    add_mnemonic_code(result, "VMEH", { "VME", { { 3, 1 } } });
    add_mnemonic_code(result, "VMEF", { "VME", { { 3, 2 } } });
    add_mnemonic_code(result, "VMLEB", { "VMLE", { { 3, 0 } } });
    add_mnemonic_code(result, "VMLEH", { "VMLE", { { 3, 1 } } });
    add_mnemonic_code(result, "VMLEF", { "VMLE", { { 3, 2 } } });
    add_mnemonic_code(result, "VMSLG", { "VMSL", { { 4, 3 } } });
    add_mnemonic_code(result, "VMOB", { "VMO", { { 3, 0 } } });
    add_mnemonic_code(result, "VMOH", { "VMO", { { 3, 1 } } });
    add_mnemonic_code(result, "VMOF", { "VMO", { { 3, 2 } } });
    add_mnemonic_code(result, "VMLOB", { "VMLO", { { 3, 0 } } });
    add_mnemonic_code(result, "VMLOH", { "VMLO", { { 3, 1 } } });
    add_mnemonic_code(result, "VMLOF", { "VMLO", { { 3, 2 } } });
    add_mnemonic_code(result, "VPOPCTB", { "VPOPCT", { { 2, 0 } } });
    add_mnemonic_code(result, "VPOPCTH", { "VPOPCT", { { 2, 1 } } });
    add_mnemonic_code(result, "VPOPCTF", { "VPOPCT", { { 2, 2 } } });
    add_mnemonic_code(result, "VPOPCTG", { "VPOPCT", { { 2, 3 } } });
    add_mnemonic_code(result, "VERLLVB", { "VERLLV", { { 3, 0 } } });
    add_mnemonic_code(result, "VERLLVH", { "VERLLV", { { 3, 1 } } });
    add_mnemonic_code(result, "VERLLVF", { "VERLLV", { { 3, 2 } } });
    add_mnemonic_code(result, "VERLLVG", { "VERLLV", { { 3, 3 } } });
    add_mnemonic_code(result, "VERLLB", { "VERLL", { { 3, 0 } } });
    add_mnemonic_code(result, "VERLLH", { "VERLL", { { 3, 1 } } });
    add_mnemonic_code(result, "VERLLF", { "VERLL", { { 3, 2 } } });
    add_mnemonic_code(result, "VERLLG", { "VERLL", { { 3, 3 } } });
    add_mnemonic_code(result, "VERIMB", { "VERIM", { { 4, 0 } } });
    add_mnemonic_code(result, "VERIMH", { "VERIM", { { 4, 1 } } });
    add_mnemonic_code(result, "VERIMF", { "VERIM", { { 4, 2 } } });
    add_mnemonic_code(result, "VERIMG", { "VERIM", { { 4, 3 } } });
    add_mnemonic_code(result, "VESLVB", { "VESLV", { { 3, 0 } } });
    add_mnemonic_code(result, "VESLVH", { "VESLV", { { 3, 1 } } });
    add_mnemonic_code(result, "VESLVF", { "VESLV", { { 3, 2 } } });
    add_mnemonic_code(result, "VESLVG", { "VESLV", { { 3, 3 } } });
    add_mnemonic_code(result, "VESLB", { "VESL", { { 3, 0 } } });
    add_mnemonic_code(result, "VESLH", { "VESL", { { 3, 1 } } });
    add_mnemonic_code(result, "VESLF", { "VESL", { { 3, 2 } } });
    add_mnemonic_code(result, "VESLG", { "VESL", { { 3, 3 } } });
    add_mnemonic_code(result, "VESRAVB", { "VESRAV", { { 3, 0 } } });
    add_mnemonic_code(result, "VESRAVH", { "VESRAV", { { 3, 1 } } });
    add_mnemonic_code(result, "VESRAVF", { "VESRAV", { { 3, 2 } } });
    add_mnemonic_code(result, "VESRAVG", { "VESRAV", { { 3, 3 } } });
    add_mnemonic_code(result, "VESRAB", { "VESRA", { { 3, 0 } } });
    add_mnemonic_code(result, "VESRAH", { "VESRA", { { 3, 1 } } });
    add_mnemonic_code(result, "VESRAF", { "VESRA", { { 3, 2 } } });
    add_mnemonic_code(result, "VESRAG", { "VESRA", { { 3, 3 } } });
    add_mnemonic_code(result, "VESRLVB", { "VESRLV", { { 3, 0 } } });
    add_mnemonic_code(result, "VESRLVH", { "VESRLV", { { 3, 1 } } });
    add_mnemonic_code(result, "VESRLVF", { "VESRLV", { { 3, 2 } } });
    add_mnemonic_code(result, "VESRLVG", { "VESRLV", { { 3, 3 } } });
    add_mnemonic_code(result, "VESRLB", { "VESRL", { { 3, 0 } } });
    add_mnemonic_code(result, "VESRLH", { "VESRL", { { 3, 1 } } });
    add_mnemonic_code(result, "VESRLF", { "VESRL", { { 3, 2 } } });
    add_mnemonic_code(result, "VESRLG", { "VESRL", { { 3, 3 } } });
    add_mnemonic_code(result, "VSB", { "VS", { { 3, 0 } } });
    add_mnemonic_code(result, "VSH", { "VS", { { 3, 1 } } });
    add_mnemonic_code(result, "VSF", { "VS", { { 3, 2 } } });
    add_mnemonic_code(result, "VSG", { "VS", { { 3, 3 } } });
    add_mnemonic_code(result, "VSQ", { "VS", { { 3, 4 } } });
    add_mnemonic_code(result, "VSCBIB", { "VSCBI", { { 3, 0 } } });
    add_mnemonic_code(result, "VSCBIH", { "VSCBI", { { 3, 1 } } });
    add_mnemonic_code(result, "VSCBIF", { "VSCBI", { { 3, 2 } } });
    add_mnemonic_code(result, "VSCBIG", { "VSCBI", { { 3, 3 } } });
    add_mnemonic_code(result, "VSCBIQ", { "VSCBI", { { 3, 4 } } });
    add_mnemonic_code(result, "VSBIQ", { "VSBI", { { 4, 4 } } });
    add_mnemonic_code(result, "VSBCBIQ", { "VSBCBI", { { 4, 4 } } });
    add_mnemonic_code(result, "VSUMQF", { "VSUMQ", { { 3, 2 } } });
    add_mnemonic_code(result, "VSUMQG", { "VSUMQ", { { 3, 3 } } });
    add_mnemonic_code(result, "VSUMGH", { "VSUMG", { { 3, 1 } } });
    add_mnemonic_code(result, "VSUMGF", { "VSUMG", { { 3, 2 } } });
    add_mnemonic_code(result, "VSUMB", { "VSUM", { { 3, 0 } } });
    add_mnemonic_code(result, "VSUMH", { "VSUM", { { 3, 1 } } });
    add_mnemonic_code(result, "VFAEB", { "VFAE", { { 3, 0 } } });
    add_mnemonic_code(result, "VFAEH", { "VFAE", { { 3, 1 } } });
    add_mnemonic_code(result, "VFAEF", { "VFAE", { { 3, 2 } } });
    add_mnemonic_code(result, "VFEEB", { "VFEE", { { 3, 0 } } });
    add_mnemonic_code(result, "VFEEH", { "VFEE", { { 3, 1 } } });
    add_mnemonic_code(result, "VFEEF", { "VFEE", { { 3, 2 } } });
    add_mnemonic_code(result, "VFEEBS", { "VFEE", { { 3, 0 }, { 4, 1 } } });
    add_mnemonic_code(result, "VFEEGS", { "VFEE", { { 3, 1 }, { 4, 1 } } });
    add_mnemonic_code(result, "VFEEFS", { "VFEE", { { 3, 2 }, { 4, 1 } } });
    add_mnemonic_code(result, "VFEEZB", { "VFEE", { { 3, 0 }, { 4, 2 } } });
    add_mnemonic_code(result, "VFEEZH", { "VFEE", { { 3, 1 }, { 4, 2 } } });
    add_mnemonic_code(result, "VFEEZF", { "VFEE", { { 3, 2 }, { 4, 2 } } });
    add_mnemonic_code(result, "VFEEZBS", { "VFEE", { { 3, 0 }, { 4, 3 } } });
    add_mnemonic_code(result, "VFEEZHS", { "VFEE", { { 3, 1 }, { 4, 3 } } });
    add_mnemonic_code(result, "VFEEZFS", { "VFEE", { { 3, 2 }, { 4, 3 } } });
    add_mnemonic_code(result, "VFENEB", { "VFENE", { { 3, 0 } } });
    add_mnemonic_code(result, "VFENEH", { "VFENE", { { 3, 1 } } });
    add_mnemonic_code(result, "VFENEF", { "VFENE", { { 3, 2 } } });
    add_mnemonic_code(result, "VFENEBS", { "VFENE", { { 3, 0 }, { 4, 1 } } });
    add_mnemonic_code(result, "VFENEHS", { "VFENE", { { 3, 1 }, { 4, 1 } } });
    add_mnemonic_code(result, "VFENEFS", { "VFENE", { { 3, 2 }, { 4, 1 } } });
    add_mnemonic_code(result, "VFENEZB", { "VFENE", { { 3, 0 }, { 4, 2 } } });
    add_mnemonic_code(result, "VFENEZH", { "VFENE", { { 3, 1 }, { 4, 2 } } });
    add_mnemonic_code(result, "VFENEZF", { "VFENE", { { 3, 2 }, { 4, 2 } } });
    add_mnemonic_code(result, "VFENEZBS", { "VFENE", { { 3, 0 }, { 4, 3 } } });
    add_mnemonic_code(result, "VFENEZHS", { "VFENE", { { 3, 1 }, { 4, 3 } } });
    add_mnemonic_code(result, "VFENEZFS", { "VFENE", { { 3, 2 }, { 4, 3 } } });
    add_mnemonic_code(result, "VISTRB", { "VISTR", { { 3, 0 } } });
    add_mnemonic_code(result, "VISTRH", { "VISTR", { { 3, 1 } } });
    add_mnemonic_code(result, "VISTRF", { "VISTR", { { 3, 2 } } });
    add_mnemonic_code(result, "VISTRBS", { "VISTR", { { 3, 0 }, { 4, 1 } } });
    add_mnemonic_code(result, "VISTRHS", { "VISTR", { { 3, 1 }, { 4, 1 } } });
    add_mnemonic_code(result, "VISTRFS", { "VISTR", { { 3, 2 }, { 4, 1 } } });
    add_mnemonic_code(result, "VSTRCB", { "VSTRC", { { 4, 0 } } });
    add_mnemonic_code(result, "VSTRCH", { "VSTRC", { { 4, 1 } } });
    add_mnemonic_code(result, "VSTRCF", { "VSTRC", { { 4, 2 } } });
    add_mnemonic_code(result, "VFASB", { "VFA", { { 3, 2 }, { 4, 0 } } });
    add_mnemonic_code(result, "VFADB", { "VFA", { { 3, 3 }, { 4, 0 } } });
    add_mnemonic_code(result, "WFASB", { "VFA", { { 3, 2 }, { 4, 8 } } });
    add_mnemonic_code(result, "WFADB", { "VFA", { { 3, 3 }, { 4, 8 } } });
    add_mnemonic_code(result, "WFAXB", { "VFA", { { 3, 4 }, { 4, 8 } } });
    add_mnemonic_code(result, "WFCSB", { "WFC", { { 3, 2 }, { 4, 0 } } });
    add_mnemonic_code(result, "WFCDB", { "WFC", { { 3, 3 }, { 4, 0 } } });
    add_mnemonic_code(result, "WFCXB", { "WFC", { { 3, 4 }, { 4, 0 } } });
    add_mnemonic_code(result, "WFKSB", { "WFK", { { 3, 2 }, { 4, 0 } } });
    add_mnemonic_code(result, "WFKDB", { "WFK", { { 3, 3 }, { 4, 0 } } });
    add_mnemonic_code(result, "WFKXB", { "WFK", { { 3, 4 }, { 4, 0 } } });
    add_mnemonic_code(result, "VFCESB", { "VFCE", { { 3, 2 }, { 4, 0 }, { 5, 0 } } });
    add_mnemonic_code(result, "VFCESBS", { "VFCE", { { 3, 2 }, { 4, 0 }, { 5, 1 } } });
    add_mnemonic_code(result, "VFCEDB", { "VFCE", { { 3, 3 }, { 4, 0 }, { 5, 0 } } });
    add_mnemonic_code(result, "VFCEDBS", { "VFCE", { { 3, 3 }, { 4, 0 }, { 5, 1 } } });
    add_mnemonic_code(result, "WFCESB", { "VFCE", { { 3, 2 }, { 4, 8 }, { 5, 0 } } });
    add_mnemonic_code(result, "WFCESBS", { "VFCE", { { 3, 2 }, { 4, 8 }, { 5, 1 } } });
    add_mnemonic_code(result, "WFCEDB", { "VFCE", { { 3, 3 }, { 4, 8 }, { 5, 0 } } });
    add_mnemonic_code(result, "WFCEDBS", { "VFCE", { { 3, 3 }, { 4, 8 }, { 5, 1 } } });
    add_mnemonic_code(result, "WFCEXB", { "VFCE", { { 3, 4 }, { 4, 8 }, { 5, 0 } } });
    add_mnemonic_code(result, "WFCEXBS", { "VFCE", { { 3, 4 }, { 4, 8 }, { 5, 1 } } });
    add_mnemonic_code(result, "VFKESB", { "VFCE", { { 3, 2 }, { 4, 4 }, { 5, 0 } } });
    add_mnemonic_code(result, "VFKESBS", { "VFCE", { { 3, 2 }, { 4, 4 }, { 5, 1 } } });
    add_mnemonic_code(result, "VFKEDB", { "VFCE", { { 3, 3 }, { 4, 4 }, { 5, 0 } } });
    add_mnemonic_code(result, "VFKEDBS", { "VFCE", { { 3, 3 }, { 4, 4 }, { 5, 1 } } });
    add_mnemonic_code(result, "WFKESB", { "VFCE", { { 3, 2 }, { 4, 12 }, { 5, 0 } } });
    add_mnemonic_code(result, "WFKESBS", { "VFCE", { { 3, 2 }, { 4, 12 }, { 5, 1 } } });
    add_mnemonic_code(result, "WFKEDB", { "VFCE", { { 3, 3 }, { 4, 12 }, { 5, 0 } } });
    add_mnemonic_code(result, "WFKEDBS", { "VFCE", { { 3, 3 }, { 4, 12 }, { 5, 1 } } });
    add_mnemonic_code(result, "WFKEXB", { "VFCE", { { 3, 4 }, { 4, 12 }, { 5, 0 } } });
    add_mnemonic_code(result, "WFKEXBS", { "VFCE", { { 3, 4 }, { 4, 12 }, { 5, 1 } } });
    add_mnemonic_code(result, "VFCHSB", { "VFCH", { { 3, 2 }, { 4, 0 }, { 5, 0 } } });
    add_mnemonic_code(result, "VFCHSBS", { "VFCH", { { 3, 2 }, { 4, 0 }, { 5, 1 } } });
    add_mnemonic_code(result, "VFCHDB", { "VFCH", { { 3, 3 }, { 4, 0 }, { 5, 0 } } });
    add_mnemonic_code(result, "VFCHDBS", { "VFCH", { { 3, 3 }, { 4, 0 }, { 5, 1 } } });
    add_mnemonic_code(result, "WFCHSB", { "VFCH", { { 3, 2 }, { 4, 8 }, { 5, 0 } } });
    add_mnemonic_code(result, "WFCHSBS", { "VFCH", { { 3, 2 }, { 4, 8 }, { 5, 1 } } });
    add_mnemonic_code(result, "WFCHDB", { "VFCH", { { 3, 3 }, { 4, 8 }, { 5, 0 } } });
    add_mnemonic_code(result, "WFCHDBS", { "VFCH", { { 3, 3 }, { 4, 8 }, { 5, 1 } } });
    add_mnemonic_code(result, "WFCHXB", { "VFCH", { { 3, 4 }, { 4, 8 }, { 5, 0 } } });
    add_mnemonic_code(result, "WFCHXBS", { "VFCH", { { 3, 4 }, { 4, 8 }, { 5, 1 } } });
    add_mnemonic_code(result, "VFKHSB", { "VFCH", { { 3, 2 }, { 4, 4 }, { 5, 0 } } });
    add_mnemonic_code(result, "VFKHSBS", { "VFCH", { { 3, 2 }, { 4, 4 }, { 5, 1 } } });
    add_mnemonic_code(result, "VFKHDB", { "VFCH", { { 3, 3 }, { 4, 4 }, { 5, 0 } } });
    add_mnemonic_code(result, "VFKHDBS", { "VFCH", { { 3, 3 }, { 4, 4 }, { 5, 1 } } });
    add_mnemonic_code(result, "WFKHSB", { "VFCH", { { 3, 2 }, { 4, 12 }, { 5, 0 } } });
    add_mnemonic_code(result, "WFKHSBS", { "VFCH", { { 3, 2 }, { 4, 12 }, { 5, 1 } } });
    add_mnemonic_code(result, "WFKHDB", { "VFCH", { { 3, 3 }, { 4, 12 }, { 5, 0 } } });
    add_mnemonic_code(result, "WFKHDBS", { "VFCH", { { 3, 3 }, { 4, 12 }, { 5, 1 } } });
    add_mnemonic_code(result, "WFKHXB", { "VFCH", { { 3, 4 }, { 4, 12 }, { 5, 0 } } });
    add_mnemonic_code(result, "WFKHXBS", { "VFCH", { { 3, 4 }, { 4, 12 }, { 5, 1 } } });
    add_mnemonic_code(result, "VFCHESB", { "VFCHE", { { 3, 2 }, { 4, 0 }, { 5, 0 } } });
    add_mnemonic_code(result, "VFCHESBS", { "VFCHE", { { 3, 2 }, { 4, 0 }, { 5, 1 } } });
    add_mnemonic_code(result, "VFCHEDB", { "VFCHE", { { 3, 3 }, { 4, 0 }, { 5, 0 } } });
    add_mnemonic_code(result, "VFCHEDBS", { "VFCHE", { { 3, 3 }, { 4, 0 }, { 5, 1 } } });
    add_mnemonic_code(result, "WFCHESB", { "VFCHE", { { 3, 2 }, { 4, 8 }, { 5, 0 } } });
    add_mnemonic_code(result, "WFCHESBS", { "VFCHE", { { 3, 2 }, { 4, 8 }, { 5, 1 } } });
    add_mnemonic_code(result, "WFCHEDB", { "VFCHE", { { 3, 3 }, { 4, 8 }, { 5, 0 } } });
    add_mnemonic_code(result, "WFCHEDBS", { "VFCHE", { { 3, 3 }, { 4, 8 }, { 5, 1 } } });
    add_mnemonic_code(result, "WFCHEXB", { "VFCHE", { { 3, 4 }, { 4, 8 }, { 5, 0 } } });
    add_mnemonic_code(result, "WFCHEXBS", { "VFCHE", { { 3, 4 }, { 4, 8 }, { 5, 1 } } });
    add_mnemonic_code(result, "VFKHESB", { "VFCHE", { { 3, 2 }, { 4, 4 }, { 5, 0 } } });
    add_mnemonic_code(result, "VFKHESBS", { "VFCHE", { { 3, 2 }, { 4, 4 }, { 5, 1 } } });
    add_mnemonic_code(result, "VFKHEDB", { "VFCHE", { { 3, 3 }, { 4, 4 }, { 5, 0 } } });
    add_mnemonic_code(result, "VFKHEDBS", { "VFCHE", { { 3, 3 }, { 4, 4 }, { 5, 1 } } });
    add_mnemonic_code(result, "WFKHESB", { "VFCHE", { { 3, 2 }, { 4, 12 }, { 5, 0 } } });
    add_mnemonic_code(result, "WFKHESBS", { "VFCHE", { { 3, 2 }, { 4, 12 }, { 5, 1 } } });
    add_mnemonic_code(result, "WFKHEDB", { "VFCHE", { { 3, 3 }, { 4, 12 }, { 5, 0 } } });
    add_mnemonic_code(result, "WFKHEDBS", { "VFCHE", { { 3, 3 }, { 4, 12 }, { 5, 1 } } });
    add_mnemonic_code(result, "WFKHEXB", { "VFCHE", { { 3, 4 }, { 4, 12 }, { 5, 0 } } });
    add_mnemonic_code(result, "VCDGB", { "VCDG", { { 2, 3 } } });
    add_mnemonic_code(result, "VCDLGB", { "VCDLG", { { 2, 3 } } });
    add_mnemonic_code(result, "VCGDB", { "VCGD", { { 2, 3 } } });
    add_mnemonic_code(result, "VCLGDB", { "VCLGD", { { 2, 3 } } });
    add_mnemonic_code(result, "VFDSB", { "VFD", { { 3, 2 }, { 4, 0 } } });
    add_mnemonic_code(result, "WFDSB", { "VFD", { { 3, 2 }, { 4, 8 } } });
    add_mnemonic_code(result, "VFDDB", { "VFD", { { 3, 3 }, { 4, 0 } } });
    add_mnemonic_code(result, "WFDDB", { "VFD", { { 3, 3 }, { 4, 8 } } });
    add_mnemonic_code(result, "WFDXB", { "VFD", { { 3, 4 }, { 4, 8 } } });
    add_mnemonic_code(result, "VFISB", { "VFI", { { 2, 2 } } });
    add_mnemonic_code(result, "VFIDB", { "VFI", { { 2, 3 } } });
    add_mnemonic_code(result, "VLDE", { "VFLL", {} });
    add_mnemonic_code(result, "VLDEB", { "VFLL", { { 2, 2 }, { 3, 0 } } });
    add_mnemonic_code(result, "WLDEB", { "VFLL", { { 2, 2 }, { 3, 8 } } });
    add_mnemonic_code(result, "VFLLS", { "VFLL", { { 2, 2 }, { 3, 0 } } });
    add_mnemonic_code(result, "WFLLS", { "VFLL", { { 2, 2 }, { 3, 8 } } });
    add_mnemonic_code(result, "WFLLD", { "VFLL", { { 2, 3 }, { 3, 8 } } });
    add_mnemonic_code(result, "VLED", { "VFLR", {} });
    add_mnemonic_code(result, "VLEDB", { "VFLR", { { 2, 3 } } });
    add_mnemonic_code(result, "VFLRD", { "VFLR", { { 2, 3 } } });
    add_mnemonic_code(result, "VFMAXSB", { "VFMAX", { { 3, 2 }, { 4, 0 } } });
    add_mnemonic_code(result, "VFMAXDB", { "VFMAX", { { 3, 3 }, { 4, 0 } } });
    add_mnemonic_code(result, "WFMAXSB", { "VFMAX", { { 3, 2 }, { 4, 8 } } });
    add_mnemonic_code(result, "WFMAXDB", { "VFMAX", { { 3, 3 }, { 4, 8 } } });
    add_mnemonic_code(result, "WFMAXXB", { "VFMAX", { { 3, 4 }, { 4, 8 } } });
    add_mnemonic_code(result, "VFMINSB", { "VFMIN", { { 3, 2 }, { 4, 0 } } });
    add_mnemonic_code(result, "VFMINDB", { "VFMIN", { { 3, 3 }, { 4, 0 } } });
    add_mnemonic_code(result, "WFMINSB", { "VFMIN", { { 3, 2 }, { 4, 8 } } });
    add_mnemonic_code(result, "WFMINDB", { "VFMIN", { { 3, 3 }, { 4, 8 } } });
    add_mnemonic_code(result, "WFMINXB", { "VFMIN", { { 3, 4 }, { 4, 8 } } });
    add_mnemonic_code(result, "VFMSB", { "VFM", { { 3, 2 }, { 4, 0 } } });
    add_mnemonic_code(result, "VFMDB", { "VFM", { { 3, 3 }, { 4, 0 } } });
    add_mnemonic_code(result, "WFMSB", { "VFM", { { 3, 2 }, { 4, 8 } } });
    add_mnemonic_code(result, "WFMDB", { "VFM", { { 3, 3 }, { 4, 8 } } });
    add_mnemonic_code(result, "WFMXB", { "VFM", { { 3, 4 }, { 4, 8 } } });
    add_mnemonic_code(result, "VFMASB", { "VFMA", { { 4, 0 }, { 5, 2 } } });
    add_mnemonic_code(result, "VFMADB", { "VFMA", { { 4, 0 }, { 5, 3 } } });
    add_mnemonic_code(result, "WFMASB", { "VFMA", { { 4, 8 }, { 5, 2 } } });
    add_mnemonic_code(result, "WFMADB", { "VFMA", { { 4, 8 }, { 5, 3 } } });
    add_mnemonic_code(result, "WFMAXB", { "VFMA", { { 4, 8 }, { 5, 4 } } });
    add_mnemonic_code(result, "VFMSSB", { "VFMS", { { 4, 0 }, { 5, 2 } } });
    add_mnemonic_code(result, "VFMSDB", { "VFMS", { { 4, 0 }, { 5, 3 } } });
    add_mnemonic_code(result, "WFMSSB", { "VFMS", { { 4, 8 }, { 5, 2 } } });
    add_mnemonic_code(result, "WFMSDB", { "VFMS", { { 4, 8 }, { 5, 3 } } });
    add_mnemonic_code(result, "WFMSXB", { "VFMS", { { 4, 8 }, { 5, 4 } } });
    add_mnemonic_code(result, "VFNMASB", { "VFNMA", { { 4, 0 }, { 5, 2 } } });
    add_mnemonic_code(result, "VFNMADB", { "VFNMA", { { 4, 0 }, { 5, 3 } } });
    add_mnemonic_code(result, "WFNMASB", { "VFNMA", { { 4, 8 }, { 5, 2 } } });
    add_mnemonic_code(result, "WFNMADB", { "VFNMA", { { 4, 8 }, { 5, 3 } } });
    add_mnemonic_code(result, "WFNMAXB", { "VFNMA", { { 4, 8 }, { 5, 4 } } });
    add_mnemonic_code(result, "VFNMSSB", { "VFNMS", { { 4, 0 }, { 5, 2 } } });
    add_mnemonic_code(result, "VFNMSDB", { "VFNMS", { { 4, 0 }, { 5, 3 } } });
    add_mnemonic_code(result, "WFNMSSB", { "VFNMS", { { 4, 8 }, { 5, 2 } } });
    add_mnemonic_code(result, "WFNMSDB", { "VFNMS", { { 4, 8 }, { 5, 3 } } });
    add_mnemonic_code(result, "WFNMSXB", { "VFNMS", { { 4, 8 }, { 5, 4 } } });
    add_mnemonic_code(result, "VFPSOSB", { "VFPSO", { { 2, 2 }, { 3, 0 } } });
    add_mnemonic_code(result, "WFPSOSB", { "VFPSO", { { 2, 2 }, { 3, 8 } } });
    add_mnemonic_code(result, "VFLCSB", { "VFPSO", { { 2, 2 }, { 3, 0 }, { 4, 0 } } });
    add_mnemonic_code(result, "WFLCSB", { "VFPSO", { { 2, 2 }, { 3, 8 }, { 4, 0 } } });
    add_mnemonic_code(result, "VFLNSB", { "VFPSO", { { 2, 2 }, { 3, 0 }, { 4, 1 } } });
    add_mnemonic_code(result, "WFLNSB", { "VFPSO", { { 2, 2 }, { 3, 8 }, { 4, 1 } } });
    add_mnemonic_code(result, "VFLPSB", { "VFPSO", { { 2, 2 }, { 3, 0 }, { 4, 2 } } });
    add_mnemonic_code(result, "WFLPSB", { "VFPSO", { { 2, 2 }, { 3, 8 }, { 4, 2 } } });
    add_mnemonic_code(result, "VFPSODB", { "VFPSO", { { 2, 3 }, { 3, 0 } } });
    add_mnemonic_code(result, "WFPSODB", { "VFPSO", { { 2, 3 }, { 3, 8 } } });
    add_mnemonic_code(result, "VFLCDB", { "VFPSO", { { 2, 3 }, { 3, 0 }, { 4, 0 } } });
    add_mnemonic_code(result, "WFLCDB", { "VFPSO", { { 2, 3 }, { 3, 8 }, { 4, 0 } } });
    add_mnemonic_code(result, "VFLNDB", { "VFPSO", { { 2, 3 }, { 3, 0 }, { 4, 1 } } });
    add_mnemonic_code(result, "WFLNDB", { "VFPSO", { { 2, 3 }, { 3, 8 }, { 4, 1 } } });
    add_mnemonic_code(result, "VFLPDB", { "VFPSO", { { 2, 3 }, { 3, 0 }, { 4, 2 } } });
    add_mnemonic_code(result, "WFLPDB", { "VFPSO", { { 2, 3 }, { 3, 8 }, { 4, 2 } } });
    add_mnemonic_code(result, "WFPSOXB", { "VFPSO", { { 2, 4 }, { 3, 8 } } });
    add_mnemonic_code(result, "WFLCXB", { "VFPSO", { { 2, 4 }, { 3, 8 }, { 4, 0 } } });
    add_mnemonic_code(result, "WFLNXB", { "VFPSO", { { 2, 4 }, { 3, 8 }, { 4, 1 } } });
    add_mnemonic_code(result, "WFLPXB", { "VFPSO", { { 2, 4 }, { 3, 8 }, { 4, 2 } } });
    add_mnemonic_code(result, "VFSQSB", { "VFSQ", { { 2, 2 }, { 3, 0 } } });
    add_mnemonic_code(result, "VFSQDB", { "VFSQ", { { 2, 3 }, { 3, 0 } } });
    add_mnemonic_code(result, "WFSQSB", { "VFSQ", { { 2, 2 }, { 3, 8 } } });
    add_mnemonic_code(result, "WFSQDB", { "VFSQ", { { 2, 3 }, { 3, 8 } } });
    add_mnemonic_code(result, "WFSQXB", { "VFSQ", { { 2, 4 }, { 3, 8 } } });
    add_mnemonic_code(result, "VFSSB", { "VFS", { { 2, 2 }, { 3, 0 } } });
    add_mnemonic_code(result, "VFSDB", { "VFS", { { 2, 3 }, { 3, 0 } } });
    add_mnemonic_code(result, "WFSSB", { "VFS", { { 2, 2 }, { 3, 8 } } });
    add_mnemonic_code(result, "WFSDB", { "VFS", { { 2, 3 }, { 3, 8 } } });
    add_mnemonic_code(result, "WFSXB", { "VFS", { { 2, 4 }, { 3, 8 } } });
    add_mnemonic_code(result, "VFTCISB", { "VFTCI", { { 3, 2 }, { 4, 0 } } });
    add_mnemonic_code(result, "VFTCIDB", { "VFTCI", { { 3, 3 }, { 4, 0 } } });
    add_mnemonic_code(result, "WFTCISB", { "VFTCI", { { 3, 2 }, { 4, 8 } } });
    add_mnemonic_code(result, "WFTCIDB", { "VFTCI", { { 3, 3 }, { 4, 8 } } });
    add_mnemonic_code(result, "WFTCIXB", { "VFTCI", { { 3, 4 }, { 4, 8 } } });
    // instruction under this position contain an OR operation not marked in this list

    // in case the operand is ommited, the OR number should be assigned to the value of the ommited operand
    add_mnemonic_code(result, "VFAEBS", { "VFAE", { { 3, 0 } } }); // operand with index 4 ORed with 1
    add_mnemonic_code(result, "VFAEHS", { "VFAE", { { 3, 1 } } }); // operand with index 4 ORed with 1
    add_mnemonic_code(result, "VFAEFS", { "VFAE", { { 3, 2 } } }); // operand with index 4 ORed with 1
    add_mnemonic_code(result, "VFAEZB", { "VFAE", { { 3, 0 } } }); // operand with index 4 ORed with 2
    add_mnemonic_code(result, "VFAEZH", { "VFAE", { { 3, 1 } } }); // operand with index 4 ORed with 2
    add_mnemonic_code(result, "VFAEZF", { "VFAE", { { 3, 2 } } }); // operand with index 4 ORed with 2
    add_mnemonic_code(result, "VFAEZBS", { "VFAE", { { 3, 0 } } }); // operand with index 4 ORed with 3
    add_mnemonic_code(result, "VFAEZHS", { "VFAE", { { 3, 1 } } }); // operand with index 4 ORed with 3
    add_mnemonic_code(result, "VFAEZFS", { "VFAE", { { 3, 2 } } }); // operand with index 4 ORed with 3
    add_mnemonic_code(result, "VSTRCBS", { "VSTRC", { { 4, 0 } } }); // operand with index 5 ORed with 1
    add_mnemonic_code(result, "VSTRCHS", { "VSTRC", { { 4, 1 } } }); // operand with index 5 ORed with 1
    add_mnemonic_code(result, "VSTRCFS", { "VSTRC", { { 4, 2 } } }); // operand with index 5 ORed with 1
    add_mnemonic_code(result, "VSTRCZB", { "VSTRC", { { 4, 0 } } }); // operand with index 5 ORed with 2
    add_mnemonic_code(result, "VSTRCZH", { "VSTRC", { { 4, 1 } } }); // operand with index 5 ORed with 2
    add_mnemonic_code(result, "VSTRCZF", { "VSTRC", { { 4, 2 } } }); // operand with index 5 ORed with 2
    add_mnemonic_code(result, "VSTRCZBS", { "VSTRC", { { 4, 0 } } }); // operand with index 5 ORed with 3
    add_mnemonic_code(result, "VSTRCZHS", { "VSTRC", { { 4, 1 } } }); // operand with index 5 ORed with 3
    add_mnemonic_code(result, "VSTRCZFS", { "VSTRC", { { 4, 2 } } }); // operand with index 5 ORed with 3
                                                                      // always OR
    add_mnemonic_code(result, "WFISB", { "VFI", { { 2, 2 } } }); // operand with index 3 ORed with 8
    add_mnemonic_code(result, "WFIDB", { "VFI", { { 2, 3 } } }); // operand with index 3 ORed with 8
    add_mnemonic_code(result, "WFIXB", { "VFI", { { 2, 4 } } }); // operand with index 3 ORed with 8
    add_mnemonic_code(result, "WCDGB", { "VCDG", { { 2, 3 } } }); // operand with index 3 ORed with 8
    add_mnemonic_code(result, "WCDLGB", { "VCDLG", { { 2, 3 } } }); // operand with index 3 ORed with 8
    add_mnemonic_code(result, "WCGDB", { "VCGD", { { 2, 3 } } }); // operand with index 3 ORed with 8
    add_mnemonic_code(result, "WCLGDB", { "VCLGD", { { 2, 3 } } }); // operand with index 3 ORed with 8
    add_mnemonic_code(result, "WLEDB", { "VFLR", { { 2, 3 } } }); // operand with index 3 ORed with 8
    add_mnemonic_code(result, "WFLRD", { "VFLR", { { 2, 3 } } }); // operand with index 3 ORed with 8
    add_mnemonic_code(result, "WFLRX", { "VFLR", { { 2, 4 } } }); // operand with index 3 ORed with 8
    // mnemonics not in principles
    add_mnemonic_code(result, "CIJE", { "CIJ", { { 2, 8 } } });
    add_mnemonic_code(result, "CIJH", { "CIJ", { { 2, 2 } } });
    add_mnemonic_code(result, "CIJL", { "CIJ", { { 2, 4 } } });
    add_mnemonic_code(result, "CIJNE", { "CIJ", { { 2, 6 } } });
    add_mnemonic_code(result, "CIJNH", { "CIJ", { { 2, 12 } } });
    add_mnemonic_code(result, "CIJNL", { "CIJ", { { 2, 10 } } });
    add_mnemonic_code(result, "CGIBE", { "CGIB", { { 2, 8 } } });
    add_mnemonic_code(result, "CGIBH", { "CGIB", { { 2, 2 } } });
    add_mnemonic_code(result, "CGIBL", { "CGIB", { { 2, 4 } } });
    add_mnemonic_code(result, "CGIBNE", { "CGIB", { { 2, 6 } } });
    add_mnemonic_code(result, "CGIBNH", { "CGIB", { { 2, 12 } } });
    add_mnemonic_code(result, "CGIBNL", { "CGIB", { { 2, 10 } } });
    add_mnemonic_code(result, "CGIJE", { "CGIJ", { { 2, 8 } } });
    add_mnemonic_code(result, "CGIJH", { "CGIJ", { { 2, 2 } } });
    add_mnemonic_code(result, "CGIJL", { "CGIJ", { { 2, 4 } } });
    add_mnemonic_code(result, "CGIJNE", { "CGIJ", { { 2, 6 } } });
    add_mnemonic_code(result, "CGIJNH", { "CGIJ", { { 2, 12 } } });
    add_mnemonic_code(result, "CGIJNL", { "CGIJ", { { 2, 10 } } });
    add_mnemonic_code(result, "CGITE", { "CGIT", { { 2, 8 } } });
    add_mnemonic_code(result, "CGITH", { "CGIT", { { 2, 2 } } });
    add_mnemonic_code(result, "CGITL", { "CGIT", { { 2, 4 } } });
    add_mnemonic_code(result, "CGITNE", { "CGIT", { { 2, 6 } } });
    add_mnemonic_code(result, "CGITNH", { "CGIT", { { 2, 12 } } });
    add_mnemonic_code(result, "CGITNL", { "CGIT", { { 2, 10 } } });
    add_mnemonic_code(result, "CGRBE", { "CGRB", { { 2, 8 } } });
    add_mnemonic_code(result, "CGRBH", { "CGRB", { { 2, 2 } } });
    add_mnemonic_code(result, "CGRBL", { "CGRB", { { 2, 4 } } });
    add_mnemonic_code(result, "CGRBNE", { "CGRB", { { 2, 6 } } });
    add_mnemonic_code(result, "CGRBNH", { "CGRB", { { 2, 12 } } });
    add_mnemonic_code(result, "CGRBNL", { "CGRB", { { 2, 10 } } });
    add_mnemonic_code(result, "CGRJE", { "CGRJ", { { 2, 8 } } });
    add_mnemonic_code(result, "CGRJH", { "CGRJ", { { 2, 2 } } });
    add_mnemonic_code(result, "CGRJL", { "CGRJ", { { 2, 4 } } });
    add_mnemonic_code(result, "CGRJNE", { "CGRJ", { { 2, 6 } } });
    add_mnemonic_code(result, "CGRJNH", { "CGRJ", { { 2, 12 } } });
    add_mnemonic_code(result, "CGRJNL", { "CGRJ", { { 2, 10 } } });
    add_mnemonic_code(result, "CGRTE", { "CGRT", { { 2, 8 } } });
    add_mnemonic_code(result, "CGRTH", { "CGRT", { { 2, 2 } } });
    add_mnemonic_code(result, "CGRTL", { "CGRT", { { 2, 4 } } });
    add_mnemonic_code(result, "CGRTNE", { "CGRT", { { 2, 6 } } });
    add_mnemonic_code(result, "CGRTNH", { "CGRT", { { 2, 12 } } });
    add_mnemonic_code(result, "CGRTNL", { "CGRT", { { 2, 10 } } });
    add_mnemonic_code(result, "CIBE", { "CIB", { { 2, 8 } } });
    add_mnemonic_code(result, "CIBH", { "CIB", { { 2, 2 } } });
    add_mnemonic_code(result, "CIBL", { "CIB", { { 2, 4 } } });
    add_mnemonic_code(result, "CIBNE", { "CIB", { { 2, 6 } } });
    add_mnemonic_code(result, "CIBNH", { "CIB", { { 2, 12 } } });
    add_mnemonic_code(result, "CIBNL", { "CIB", { { 2, 10 } } });
    add_mnemonic_code(result, "CITE", { "CIT", { { 2, 8 } } });
    add_mnemonic_code(result, "CITH", { "CIT", { { 2, 2 } } });
    add_mnemonic_code(result, "CITL", { "CIT", { { 2, 4 } } });
    add_mnemonic_code(result, "CITNE", { "CIT", { { 2, 6 } } });
    add_mnemonic_code(result, "CITNH", { "CIT", { { 2, 12 } } });
    add_mnemonic_code(result, "CITNL", { "CIT", { { 2, 10 } } });
    add_mnemonic_code(result, "CLFITE", { "CLFIT", { { 2, 8 } } });
    add_mnemonic_code(result, "CLFITH", { "CLFIT", { { 2, 2 } } });
    add_mnemonic_code(result, "CLFITL", { "CLFIT", { { 2, 4 } } });
    add_mnemonic_code(result, "CLFITNE", { "CLFIT", { { 2, 6 } } });
    add_mnemonic_code(result, "CLFITNH", { "CLFIT", { { 2, 12 } } });
    add_mnemonic_code(result, "CLFITNL", { "CLFIT", { { 2, 10 } } });
    add_mnemonic_code(result, "CLGIBE", { "CLGIB", { { 2, 8 } } });
    add_mnemonic_code(result, "CLGIBH", { "CLGIB", { { 2, 2 } } });
    add_mnemonic_code(result, "CLGIBL", { "CLGIB", { { 2, 4 } } });
    add_mnemonic_code(result, "CLGIBNE", { "CLGIB", { { 2, 6 } } });
    add_mnemonic_code(result, "CLGIBNH", { "CLGIB", { { 2, 12 } } });
    add_mnemonic_code(result, "CLGIBNL", { "CLGIB", { { 2, 10 } } });
    add_mnemonic_code(result, "CLGIJE", { "CLGIJ", { { 2, 8 } } });
    add_mnemonic_code(result, "CLGIJH", { "CLGIJ", { { 2, 2 } } });
    add_mnemonic_code(result, "CLGIJL", { "CLGIJ", { { 2, 4 } } });
    add_mnemonic_code(result, "CLGIJNE", { "CLGIJ", { { 2, 6 } } });
    add_mnemonic_code(result, "CLGIJNH", { "CLGIJ", { { 2, 12 } } });
    add_mnemonic_code(result, "CLGIJNL", { "CLGIJ", { { 2, 10 } } });
    add_mnemonic_code(result, "CLGITE", { "CLGIT", { { 2, 8 } } });
    add_mnemonic_code(result, "CLGITH", { "CLGIT", { { 2, 2 } } });
    add_mnemonic_code(result, "CLGITL", { "CLGIT", { { 2, 4 } } });
    add_mnemonic_code(result, "CLGITNE", { "CLGIT", { { 2, 6 } } });
    add_mnemonic_code(result, "CLGITNH", { "CLGIT", { { 2, 12 } } });
    add_mnemonic_code(result, "CLGITNL", { "CLGIT", { { 2, 10 } } });
    add_mnemonic_code(result, "CLGRBE", { "CLGRB", { { 2, 8 } } });
    add_mnemonic_code(result, "CLGRBH", { "CLGRB", { { 2, 2 } } });
    add_mnemonic_code(result, "CLGRBL", { "CLGRB", { { 2, 4 } } });
    add_mnemonic_code(result, "CLGRBNE", { "CLGRB", { { 2, 6 } } });
    add_mnemonic_code(result, "CLGRBNH", { "CLGRB", { { 2, 12 } } });
    add_mnemonic_code(result, "CLGRBNL", { "CLGRB", { { 2, 10 } } });
    add_mnemonic_code(result, "CLGRJE", { "CLGRJ", { { 2, 8 } } });
    add_mnemonic_code(result, "CLGRJH", { "CLGRJ", { { 2, 2 } } });
    add_mnemonic_code(result, "CLGRJL", { "CLGRJ", { { 2, 4 } } });
    add_mnemonic_code(result, "CLGRJNE", { "CLGRJ", { { 2, 6 } } });
    add_mnemonic_code(result, "CLGRJNH", { "CLGRJ", { { 2, 12 } } });
    add_mnemonic_code(result, "CLGRJNL", { "CLGRJ", { { 2, 10 } } });
    add_mnemonic_code(result, "CLGRTE", { "CLGRT", { { 2, 8 } } });
    add_mnemonic_code(result, "CLGRTH", { "CLGRT", { { 2, 2 } } });
    add_mnemonic_code(result, "CLGRTL", { "CLGRT", { { 2, 4 } } });
    add_mnemonic_code(result, "CLGRTNE", { "CLGRT", { { 2, 6 } } });
    add_mnemonic_code(result, "CLGRTNH", { "CLGRT", { { 2, 12 } } });
    add_mnemonic_code(result, "CLGRTNL", { "CLGRT", { { 2, 10 } } });
    add_mnemonic_code(result, "CLGTE", { "CLGT", { { 1, 8 } } });
    add_mnemonic_code(result, "CLGTH", { "CLGT", { { 1, 2 } } });
    add_mnemonic_code(result, "CLGTL", { "CLGT", { { 1, 4 } } });
    add_mnemonic_code(result, "CLGTNE", { "CLGT", { { 1, 6 } } });
    add_mnemonic_code(result, "CLGTNH", { "CLGT", { { 1, 12 } } });
    add_mnemonic_code(result, "CLGTNL", { "CLGT", { { 1, 10 } } });
    add_mnemonic_code(result, "CLIBE", { "CLIB", { { 2, 8 } } });
    add_mnemonic_code(result, "CLIBH", { "CLIB", { { 2, 2 } } });
    add_mnemonic_code(result, "CLIBL", { "CLIB", { { 2, 4 } } });
    add_mnemonic_code(result, "CLIBNE", { "CLIB", { { 2, 6 } } });
    add_mnemonic_code(result, "CLIBNH", { "CLIB", { { 2, 12 } } });
    add_mnemonic_code(result, "CLIBNL", { "CLIB", { { 2, 10 } } });
    add_mnemonic_code(result, "CLIJE", { "CLIJ", { { 2, 8 } } });
    add_mnemonic_code(result, "CLIJH", { "CLIJ", { { 2, 2 } } });
    add_mnemonic_code(result, "CLIJL", { "CLIJ", { { 2, 4 } } });
    add_mnemonic_code(result, "CLIJNE", { "CLIJ", { { 2, 6 } } });
    add_mnemonic_code(result, "CLIJNH", { "CLIJ", { { 2, 12 } } });
    add_mnemonic_code(result, "CLIJNL", { "CLIJ", { { 2, 10 } } });
    add_mnemonic_code(result, "CLRBE", { "CLRB", { { 2, 8 } } });
    add_mnemonic_code(result, "CLRBH", { "CLRB", { { 2, 2 } } });
    add_mnemonic_code(result, "CLRBL", { "CLRB", { { 2, 4 } } });
    add_mnemonic_code(result, "CLRBNE", { "CLRB", { { 2, 6 } } });
    add_mnemonic_code(result, "CLRBNH", { "CLRB", { { 2, 12 } } });
    add_mnemonic_code(result, "CLRBNL", { "CLRB", { { 2, 10 } } });
    add_mnemonic_code(result, "CLRJE", { "CLRJ", { { 2, 8 } } });
    add_mnemonic_code(result, "CLRJH", { "CLRJ", { { 2, 2 } } });
    add_mnemonic_code(result, "CLRJL", { "CLRJ", { { 2, 4 } } });
    add_mnemonic_code(result, "CLRJNE", { "CLRJ", { { 2, 6 } } });
    add_mnemonic_code(result, "CLRJNH", { "CLRJ", { { 2, 12 } } });
    add_mnemonic_code(result, "CLRJNL", { "CLRJ", { { 2, 10 } } });
    add_mnemonic_code(result, "CLRTE", { "CLRT", { { 2, 8 } } });
    add_mnemonic_code(result, "CLRTH", { "CLRT", { { 2, 2 } } });
    add_mnemonic_code(result, "CLRTL", { "CLRT", { { 2, 4 } } });
    add_mnemonic_code(result, "CLRTNE", { "CLRT", { { 2, 6 } } });
    add_mnemonic_code(result, "CLRTNH", { "CLRT", { { 2, 12 } } });
    add_mnemonic_code(result, "CLRTNL", { "CLRT", { { 2, 10 } } });
    add_mnemonic_code(result, "CLTE", { "CLT", { { 1, 8 } } });
    add_mnemonic_code(result, "CLTH", { "CLT", { { 1, 2 } } });
    add_mnemonic_code(result, "CLTL", { "CLT", { { 1, 4 } } });
    add_mnemonic_code(result, "CLTNE", { "CLT", { { 1, 6 } } });
    add_mnemonic_code(result, "CLTNH", { "CLT", { { 1, 12 } } });
    add_mnemonic_code(result, "CLTNL", { "CLT", { { 1, 10 } } });
    add_mnemonic_code(result, "CRBE", { "CRB", { { 2, 8 } } });
    add_mnemonic_code(result, "CRBH", { "CRB", { { 2, 2 } } });
    add_mnemonic_code(result, "CRBL", { "CRB", { { 2, 4 } } });
    add_mnemonic_code(result, "CRBNE", { "CRB", { { 2, 6 } } });
    add_mnemonic_code(result, "CRBNH", { "CRB", { { 2, 12 } } });
    add_mnemonic_code(result, "CRBNL", { "CRB", { { 2, 10 } } });
    add_mnemonic_code(result, "CRJE", { "CRJ", { { 2, 8 } } });
    add_mnemonic_code(result, "CRJH", { "CRJ", { { 2, 2 } } });
    add_mnemonic_code(result, "CRJL", { "CRJ", { { 2, 4 } } });
    add_mnemonic_code(result, "CRJNE", { "CRJ", { { 2, 6 } } });
    add_mnemonic_code(result, "CRJNH", { "CRJ", { { 2, 12 } } });
    add_mnemonic_code(result, "CRJNL", { "CRJ", { { 2, 10 } } });
    add_mnemonic_code(result, "CRTE", { "CRT", { { 2, 8 } } });
    add_mnemonic_code(result, "CRTH", { "CRT", { { 2, 2 } } });
    add_mnemonic_code(result, "CRTL", { "CRT", { { 2, 4 } } });
    add_mnemonic_code(result, "CRTNE", { "CRT", { { 2, 6 } } });
    add_mnemonic_code(result, "CRTNH", { "CRT", { { 2, 12 } } });
    add_mnemonic_code(result, "CRTNL", { "CRT", { { 2, 10 } } });
    add_mnemonic_code(result, "LOCGE", { "LOCG", { { 2, 8 } } });
    add_mnemonic_code(result, "LOCGH", { "LOCG", { { 2, 2 } } });
    add_mnemonic_code(result, "LOCGL", { "LOCG", { { 2, 4 } } });
    add_mnemonic_code(result, "LOCGNE", { "LOCG", { { 2, 6 } } });
    add_mnemonic_code(result, "LOCGNH", { "LOCG", { { 2, 12 } } });
    add_mnemonic_code(result, "LOCGNL", { "LOCG", { { 2, 10 } } });
    add_mnemonic_code(result, "LOCRE", { "LOCR", { { 2, 8 } } });
    add_mnemonic_code(result, "LOCRH", { "LOCR", { { 2, 2 } } });
    add_mnemonic_code(result, "LOCRL", { "LOCR", { { 2, 4 } } });
    add_mnemonic_code(result, "LOCRNE", { "LOCR", { { 2, 6 } } });
    add_mnemonic_code(result, "LOCRNH", { "LOCR", { { 2, 12 } } });
    add_mnemonic_code(result, "LOCRNL", { "LOCR", { { 2, 10 } } });
    add_mnemonic_code(result, "LOCGRE", { "LOCGR", { { 2, 8 } } });
    add_mnemonic_code(result, "LOCGRH", { "LOCGR", { { 2, 2 } } });
    add_mnemonic_code(result, "LOCGRL", { "LOCGR", { { 2, 4 } } });
    add_mnemonic_code(result, "LOCGRNE", { "LOCGR", { { 2, 6 } } });
    add_mnemonic_code(result, "LOCGRNH", { "LOCGR", { { 2, 12 } } });
    add_mnemonic_code(result, "LOCGRNL", { "LOCGR", { { 2, 10 } } });
    add_mnemonic_code(result, "LOCE", { "LOC", { { 2, 8 } } });
    add_mnemonic_code(result, "LOCH", { "LOC", { { 2, 2 } } });
    add_mnemonic_code(result, "LOCL", { "LOC", { { 2, 4 } } });
    add_mnemonic_code(result, "LOCNE", { "LOC", { { 2, 6 } } });
    add_mnemonic_code(result, "LOCNH", { "LOC", { { 2, 12 } } });
    add_mnemonic_code(result, "LOCNL", { "LOC", { { 2, 10 } } });
    add_mnemonic_code(result, "STOCGE", { "STOCG", { { 2, 8 } } });
    add_mnemonic_code(result, "STOCGH", { "STOCG", { { 2, 2 } } });
    add_mnemonic_code(result, "STOCGL", { "STOCG", { { 2, 4 } } });
    add_mnemonic_code(result, "STOCGNE", { "STOCG", { { 2, 6 } } });
    add_mnemonic_code(result, "STOCGNH", { "STOCG", { { 2, 12 } } });
    add_mnemonic_code(result, "STOCGNL", { "STOCG", { { 2, 10 } } });
    add_mnemonic_code(result, "STOCE", { "STOC", { { 2, 8 } } });
    add_mnemonic_code(result, "STOCH", { "STOC", { { 2, 2 } } });
    add_mnemonic_code(result, "STOCL", { "STOC", { { 2, 4 } } });
    add_mnemonic_code(result, "STOCNE", { "STOC", { { 2, 6 } } });
    add_mnemonic_code(result, "STOCNH", { "STOC", { { 2, 12 } } });
    add_mnemonic_code(result, "STOCNL", { "STOC", { { 2, 10 } } });
    return result;
}

std::map<const std::string, machine_instruction_ptr> instruction::machine_instructions =
    instruction::get_machine_instructions();

const std::map<const std::string, mnemonic_code> instruction::mnemonic_codes = instruction::get_mnemonic_codes();