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

#include "diagnostic_op.h"

#include <type_traits>

#include "utils/concat.h"
#include "utils/resource_location.h"
#include "utils/unicode_text.h"

namespace hlasm_plugin::parser_library {

using hlasm_plugin::utils::concat;

// diagnostic_op errors

// assembler instruction errors

diagnostic_op diagnostic_op::error_I999(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "I999",
        concat("Fatal error at ", instr_name, " instruction: implementation error."),
        range);
}

diagnostic_op diagnostic_op::error_A001_complex_op_expected(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A001",
        concat("Error at ", instr_name, ": operand in a form identifier(parameters) expected"),
        range);
}

diagnostic_op diagnostic_op::error_A004_data_def_expected(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "A004", "Data definition operand expected", range);
}

diagnostic_op diagnostic_op::error_A010_minimum(std::string_view instr_name, size_t min_params, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A010",
        concat("Error at ", instr_name, " instruction: number of operands has to be at least ", min_params),
        range);
}

diagnostic_op diagnostic_op::error_A011_exact(std::string_view instr_name, size_t number_of_params, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A011",
        concat("Error at ", instr_name, " instruction: number of operands has to be ", number_of_params),
        range);
}

diagnostic_op diagnostic_op::error_A012_from_to(
    std::string_view instr_name, size_t number_from, size_t number_to, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A012",
        concat("Error at ",
            instr_name,
            " instruction: number of operands has to be from ",
            number_from,
            " to ",
            number_to),
        range);
}

diagnostic_op diagnostic_op::error_A013_either(
    std::string_view instr_name, int option_one, int option_two, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A013",
        concat("Error at ",
            instr_name,
            " instruction: number of operands has to be either ",
            option_one,
            " or ",
            option_two),
        range);
}

diagnostic_op diagnostic_op::error_A014_lower_than(std::string_view instr_name, size_t number, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A014",
        concat("Error at ", instr_name, " instruction: number of operands has to be lower than ", number),
        range);
}

diagnostic_op diagnostic_op::error_A015_minimum(
    std::string_view instr_name, std::string_view op_name, size_t min_params, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A015",
        concat("Error at ",
            instr_name,
            " instruction at ",
            op_name,
            " operand: number of parameters has to be at least ",
            min_params),
        range);
}

diagnostic_op diagnostic_op::error_A016_exact(
    std::string_view instr_name, std::string_view op_name, size_t number_of_params, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A016",
        concat("Error at ",
            instr_name,
            " instruction at ",
            op_name,
            " operand: number of parameters has to be ",
            number_of_params),
        range);
}

diagnostic_op diagnostic_op::error_A017_from_to(
    std::string_view instr_name, std::string_view op_name, size_t number_from, size_t number_to, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A017",
        concat("Error at ",
            instr_name,
            " instruction at ",
            op_name,
            " operand: number of parameters has to be from ",
            number_from,
            " to ",
            number_to),
        range);
}

diagnostic_op diagnostic_op::error_A018_either(
    std::string_view instr_name, std::string_view op_name, int option_one, int option_two, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A018",
        concat("Error at ",
            instr_name,
            " instruction at ",
            op_name,
            " operand: number of parameters has to be either ",
            option_one,
            " or ",
            option_two),
        range);
}

diagnostic_op diagnostic_op::error_A019_lower_than(
    std::string_view instr_name, std::string_view op_name, size_t number, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A019",
        concat("Error at ",
            instr_name,
            " instruction at ",
            op_name,
            " operand: number of parameters has to be lower than ",
            number),
        range);
}

diagnostic_op diagnostic_op::error_A020_absolute_val_or_empty_expected(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A020",
        concat(
            "Error at ", instr_name, " instruction: operand must either specify an absolute value or must be omitted"),
        range);
}

diagnostic_op diagnostic_op::error_A021_cannot_be_empty(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A021",
        concat("Error at ", instr_name, " instruction: operand cannot be empty"),
        range);
}

diagnostic_op diagnostic_op::error_A100_XATTR_identifier(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A100",
        "Error at XATTR instruction: operand identifier value must be one of the following: ATTRIBUTES|ATTR, "
        "LINKAGE|LINK, SCOPE, PSECT, REFERENCE|REF",
        range);
}

diagnostic_op diagnostic_op::error_A101_USING_base_val(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A101",
        "Error at USING instruction: first operand base value must lie between 0 and 2^31-1",
        range);
}

diagnostic_op diagnostic_op::error_A102_USING_end_val(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A102",
        "Error at USING instruction: first operand end value must lie between 0 and 2^31-1 or must be omitted",
        range);
}

diagnostic_op diagnostic_op::error_A103_USING_end_exceed(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A103",
        "Error at USING instruction: first operand end value must exceed base value or must be omitted",
        range);
}

diagnostic_op diagnostic_op::error_A104_USING_first_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A104",
        "Error at USING instruction: first operand format must either be a base expression, or must be in a form "
        "(base,end)",
        range);
}

diagnostic_op diagnostic_op::error_A105_USING_base_register_val(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A105",
        "Error at USING instruction: base register value must lie between 0 and 15",
        range);
}

diagnostic_op diagnostic_op::error_A106_TITLE_string_chars(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A106",
        "Error at TITLE instruction: operand value must be a title string of 1 to 100 characters",
        range);
}

diagnostic_op diagnostic_op::error_A107_RMODE_op_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A107",
        "Error at RMODE instruction: operand value must be one of the following: 24, 31, 64, ANY",
        range);
}

diagnostic_op diagnostic_op::error_A108_PUNCH_too_long(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "A108", "Punch record exceeds 80 characters", range);
}

diagnostic_op diagnostic_op::error_A109_PRINT_op_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A109",
        "Error at PRINT instruction: operand value must be one of the following: GEN|NOGEN, DATA|NODATA, "
        "MCALL|NOMCALL, MSOURCE|NOMSOURCE, UHEAD|NOUHEAD, ON, OFF, NOPRINT",
        range);
}

diagnostic_op diagnostic_op::error_A110_STACK_last_op_format_val(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A110",
        concat("Error at ",
            instr_name,
            " instruction: last operand value must be one of the following: PRINT, USING, ACONTROL, NOPRINT"),
        range);
}

diagnostic_op diagnostic_op::error_A111_STACK_other_op_format_val(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A111",
        concat("Error at ",
            instr_name,
            " instruction: operand value must be one of the following: PRINT, USING, ACONTROL (NOPRINT can be "
            "specified for last operand of ",
            instr_name,
            " only)"),
        range);
}

diagnostic_op diagnostic_op::error_A112_STACK_option_specified(
    std::string_view instr_name, std::string_view op_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A112",
        concat("Error at ", instr_name, " instruction: ", op_name, " can be specified only once"),
        range);
}

diagnostic_op diagnostic_op::error_A113_STACK_NOPRINT_end(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A113",
        concat("Error at ", instr_name, " instruction: NOPRINT option can be specified for last operand only"),
        range);
}

diagnostic_op diagnostic_op::error_A114_STACK_NOPRINT_solo(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A114",
        concat("Error at ",
            instr_name,
            " instruction: NOPRINT option cannot be the only option specified. Other possible operand values: PRINT, "
            "USING, ACONTROL."),
        range);
}

diagnostic_op diagnostic_op::error_A115_ORG_op_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A115",
        "Error at ORG instruction: operand value must be either an absolute expression or the operand must be omitted",
        range);
}

diagnostic_op diagnostic_op::error_A116_ORG_boundary_operand(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A116",
        "Error at ORG instruction: boundary operand value must be either a number that is a power of 2 with a range "
        "from 2 to 4096, or must be omitted",
        range);
}

diagnostic_op diagnostic_op::error_A117_MNOTE_message_size(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A117",
        "Error at MNOTE instruction: operand must specify a message string of maximum 1020 characters",
        range);
}

diagnostic_op diagnostic_op::error_A118_MNOTE_operands_size(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A118",
        "Error at MNOTE instruction: the total size of all operands cannot exceed 1024 bytes",
        range);
}

diagnostic_op diagnostic_op::error_A119_MNOTE_first_op_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A119",
        "Error at MNOTE instruction: first operand must be either a severity value in the range 0 through 255, an "
        "asterisk or the operand must be omitted",
        range);
}

diagnostic_op diagnostic_op::error_A120_ISEQ_op_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A120",
        "Error at ISEQ instruction: operands must be both either omitted, or they both must be decimal self-defining "
        "terms with value in the range 1 through 80",
        range);
}

diagnostic_op diagnostic_op::error_A121_ISEQ_right_GT_left(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A121",
        "Error at ISEQ instruction: right operand value must be greater than left operand value, or both operands must "
        "be omitted",
        range);
}

diagnostic_op diagnostic_op::error_A122_ICTL_op_format_first(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A122",
        "Error at ICTL instruction: operand value must be a decimal self-defining term",
        range);
}

diagnostic_op diagnostic_op::error_A123_ICTL_begin_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A123",
        "Error at ICTL instruction: begin operand must be a self-defining term with value in the range 1 through 40",
        range);
}

diagnostic_op diagnostic_op::error_A124_ICTL_end_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A124",
        "Error at ICTL instruction: end operand must be a self-defining term with value in the range 41 through 80",
        range);
}

diagnostic_op diagnostic_op::error_A125_ICTL_begin_end_diff(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A125",
        "Error at ICTL instruction: end operand value cannot not be less than begin+5",
        range);
}

diagnostic_op diagnostic_op::error_A126_ICTL_continuation_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A126",
        "Error at ICTL instruction: continuation operand must be a self-defining term with value in the range 2 "
        "through 40",
        range);
}

diagnostic_op diagnostic_op::error_A127_ICTL_begin_continuation_diff(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A127",
        "Error at ICTL instruction: continuation operand value must be greater than begin value",
        range);
}

diagnostic_op diagnostic_op::error_A129_EXTRN_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A129",
        "Error at EXTRN instruction: operand must either specify an external symbol, or it must be in a format "
        "PART(external symbols), where external symbols must also be a reference to a part as defined on the CATTR "
        "instruction.",
        range);
}

diagnostic_op diagnostic_op::error_A130_EXITCTL_exit_type_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A130",
        "Error at EXITCTL instruction: first operand must specify an exit type, which must have one of the following "
        "values: SOURCE, LIBRARY, LISTING, PUNCH, ADATA, TERM, OBJECT",
        range);
}

diagnostic_op diagnostic_op::error_A131_EXITCTL_control_value_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A131",
        "Error at EXITCTL instruction: operand must either be omitted or must specify a control value in the range "
        "-2^31 through 2^31-1.",
        range);
}

diagnostic_op diagnostic_op::error_A132_EQU_value_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A132",
        "Error at EQU instruction: first operand must represent either an absolute value, a relocatable value, or a "
        "complexly relocatable value",
        range);
}

diagnostic_op diagnostic_op::error_A133_EQU_len_att_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A133",
        "Error at EQU instruction: operand representing length attribute value must either be an absolute value in the "
        "range 0 through 65535 or must be omitted",
        range);
}

diagnostic_op diagnostic_op::error_A134_EQU_type_att_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A134",
        "Error at EQU instruction: operand representing type attribute value must either be an absolute value in the "
        "range 0 through 255 or must be omitted",
        range);
}

diagnostic_op diagnostic_op::error_A135_EQU_asm_type_val_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A135",
        "Error at EQU instruction: operand representing assembler type value must either be omitted or must have one "
        "of the following values: AR, CR, CR32, CR64, FPR, GR, VR, GR32, GR64",
        range);
}

diagnostic_op diagnostic_op::error_A136_ENTRY_op_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A136",
        "Error at ENTRY instruction: operand must specify an entry point, which must be a valid relocatable symbol",
        range);
}

diagnostic_op diagnostic_op::warning_A137_END_lang_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::warning,
        "A137",
        "Language operand parameter must be in the following format: (char10, char4, char5)",
        range);
}

diagnostic_op diagnostic_op::warning_A138_END_lang_first(const range& range)
{
    return diagnostic_op(diagnostic_severity::warning,
        "A138",
        "First parameter of language operand (language translator identifier) must be a 1 to 10 character code",
        range);
}

diagnostic_op diagnostic_op::warning_A139_END_lang_second(const range& range)
{
    return diagnostic_op(diagnostic_severity::warning,
        "A139",
        "Second parameter of language operand (version and release code) must be exactly 4 character long",
        range);
}

diagnostic_op diagnostic_op::warning_A140_END_lang_third(const range& range)
{
    return diagnostic_op(diagnostic_severity::warning,
        "A140",
        "Third parameter of language operand must be exactly 5 character long and should specify the compile date in "
        "the \"YYDDD\" format",
        range);
}

diagnostic_op diagnostic_op::error_A141_DROP_op_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A141",
        "Error at DROP instruction: operand must either specify a base register (value 0 through 15), label (either an "
        "ordinary or a variable symbol) or must be omitted",
        range);
}

diagnostic_op diagnostic_op::error_A142_COPY_op_format(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "A142", "Error at COPY instruction: operand must specify a member name", range);
}

diagnostic_op diagnostic_op::error_A143_must_be_absolute_expr(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A143",
        concat("Error at ", instr_name, " instruction: operand value must be an absolute expression"),
        range);
}

diagnostic_op diagnostic_op::error_A144_CNOP_byte_size(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A144",
        "Error at CNOP instruction: byte operand must specify an even positive value",
        range);
}

diagnostic_op diagnostic_op::error_A145_CNOP_boundary_size(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A145",
        "Error at CNOP instruction: boundary operand must specify a value that is a power of 2 with a range from 2 to "
        "4096",
        range);
}

diagnostic_op diagnostic_op::error_A146_CNOP_byte_GT_boundary(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A146",
        "Error at CNOP instruction: the value of byte operand value must be in range 0 through boundary-2",
        range);
}

diagnostic_op diagnostic_op::error_A147_CCW_op_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A147",
        concat("Error at ", instr_name, " instruction: all operands must be specified"),
        range);
}

diagnostic_op diagnostic_op::error_A148_EXPR_op_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A148",
        concat("Error at ",
            instr_name,
            " instruction: operand must either specify a non-negative absolute value, or it must be omitted"),
        range);
}

diagnostic_op diagnostic_op::error_A149_CATTR_identifier_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A149",
        "Error at CATTR instruction: binder attribute operand must be in one of the following formats: DEFLOAD, "
        "EXECUTABLE, NOTEXECUTABLE, MOVABLE, NOLOAD, NOTREUS, READONLY, REFR, REMOVABLE, RENT, REUS, "
        "RMODE(24|31|64|ANY), ALIGN(n), FILL(nnn), PART(part-name), PRIORITY(nnnnn)",
        range);
}

diagnostic_op diagnostic_op::error_A150_AMODE_op_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A150",
        "Error at AMODE instruction: operand must have one of the following values: 24, 31, 64, ANY, ANY31, ANY64",
        range);
}

diagnostic_op diagnostic_op::error_A151_ALIAS_op_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A151",
        "Error at ALIAS instruction: operand must specify an alias string in one of the following formats: "
        "C'aaaaaaaa', X'xxxxxxxx'",
        range);
}

diagnostic_op diagnostic_op::error_A152_ALIAS_C_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A152",
        "Error at ALIAS instruction: C'aaaaaaaa' operands format supports only characters with hexadecimal value in "
        "the range X'42' - X'FE'",
        range);
}

diagnostic_op diagnostic_op::error_A153_ALIAS_X_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A153",
        "Error at ALIAS instruction: the string 'xxxxxxxx' of operand format X'xxxxxxxx' must contain only hexadecimal "
        "digits",
        range);
}

diagnostic_op diagnostic_op::error_A154_ALIAS_X_format_no_of_chars(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A154",
        "Error at ALIAS instruction: the string 'xxxxxxxx' of X'xxxxxxxx' operand format must have an even length",
        range);
}

diagnostic_op diagnostic_op::error_A155_ALIAS_X_format_range(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A155",
        "Error at ALIAS instruction: each character pair of the string 'xxxxxxxx' of operand format X'xxxxxxxx' must "
        "be in the range X'42' - X'FE'",
        range);
}

diagnostic_op diagnostic_op::error_A156_AINSERT_second_op_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A156",
        "Error at AINSERT instruction: second operand must have one of the following values: BACK, FRONT",
        range);
}

diagnostic_op diagnostic_op::error_A157_AINSERT_first_op_size(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A157",
        "Error at AINSERT instruction: record operand must be a string enclosed in apostrophes of maximum 80 "
        "characters",
        range);
}

diagnostic_op diagnostic_op::error_A158_ADATA_val_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A158",
        "Error at ADATA instruction: operand value must either be a decimal self defining term, or the operand must be "
        "omitted",
        range);
}

diagnostic_op diagnostic_op::error_A159_ADATA_val_size(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A159",
        "Error at ADATA instruction: operand value must be in range -2^32 through 2^32-1",
        range);
}

diagnostic_op diagnostic_op::error_A160_ADATA_char_string_size(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A160",
        "Error at ADATA instruction: the maximum size of character string operand is 255 bytes",
        range);
}

diagnostic_op diagnostic_op::error_A161_ACONTROL_op_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A161",
        "Error at ACONTROL instruction: operand must be in one of the following formats: AFPR|NOAFPR, RA2|NORA2, "
        "LIBMAC|LMAC, NOLIBMAC|NOLMAC, (COMPAT|CPAT)(keywords), NOCOMPAT|NOCPAT, FLAG(flags), OPTABLE(opcode, "
        "listing?), (TYPECHECK|TC)(parameters), NOTYPECHECK|NOTC",
        range);
}

diagnostic_op diagnostic_op::error_A162_PROCESS_uknown_option(std::string_view option, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A162",
        concat("Error at *PROCESS instruction: unknown assembler option ", option),
        range);
}

diagnostic_op diagnostic_op::error_A163_ALIAS_mandatory_label(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "A163", "Label not provided on ALIAS instruction", range);
}

diagnostic_op diagnostic_op::error_A164_USING_mapping_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A164",
        "Error at USING instruction: Register number or expression expected.",
        range);
}

diagnostic_op diagnostic_op::error_A165_POP_USING(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "A165", "Illegal POP USING - stack is empty.", range);
}

diagnostic_op diagnostic_op::error_A166_GOFF_required(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "A166", "GOFF/XOBJECT option is required", range);
}

diagnostic_op diagnostic_op::error_A167_CATTR_label(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "A167", "Valid class name required", range);
}

diagnostic_op diagnostic_op::error_A168_XATTR_label(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "A168", "External symbol name required", range);
}

diagnostic_op diagnostic_op::error_A169_no_section(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "A169", "Executable section must be started", range);
}

diagnostic_op diagnostic_op::error_A170_section_type_mismatch(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "A170", "Section type mismatch", range);
}

diagnostic_op diagnostic_op::warn_A171_operands_ignored(const range& range)
{
    return diagnostic_op(diagnostic_severity::warning, "A171", "Operands ignored", range);
}

diagnostic_op diagnostic_op::warn_A172_psect_redefinition(const range& range)
{
    return diagnostic_op(diagnostic_severity::warning, "A172", "PSECT cannot be set multiple times", range);
}

diagnostic_op diagnostic_op::error_A173_invalid_psect(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "A173", "Invalid PSECT symbol", range);
}

diagnostic_op diagnostic_op::error_A174_EQU_prog_type_val_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A174",
        "Error at EQU instruction: operand representing program type attribute value must either be an absolute value",
        range);
}

diagnostic_op diagnostic_op::error_A175_data_prog_type_deps(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A175",
        "Program type attribute must be a 32-bit value that can be evaluated at the time of definition",
        range);
}

diagnostic_op diagnostic_op::error_A200_SCOPE_param(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A200",
        concat("Error at ",
            instr_name,
            " instruction: SCOPE operand parameter name must be one of the following: SECTION|S, MODULE|M, LIBRARY|L, "
            "IMPORT|X, EXPORT|X"),
        range);
}

diagnostic_op diagnostic_op::error_A201_LINKAGE_param(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A201",
        concat("Error at ",
            instr_name,
            " instruction: LINKAGE operand parameter value must be one of the following: OS, XPLINK"),
        range);
}

diagnostic_op diagnostic_op::error_A202_REF_direct(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A202",
        concat("Error at ",
            instr_name,
            " instruction at REFERENCE operand: both DIRECT and INDIRECT parameter cannot be specified"),
        range);
}

diagnostic_op diagnostic_op::error_A203_REF_data(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A203",
        concat("Error at ",
            instr_name,
            " instruction at REFERENCE operand: both DATA and CODE parameter cannot be specified"),
        range);
}

diagnostic_op diagnostic_op::error_A204_RMODE_param_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A204",
        concat("Error at ",
            instr_name,
            " instruction: RMODE operand parameter value must be one of the following: 24, 31, 64, ANY"),
        range);
}

diagnostic_op diagnostic_op::error_A205_ALIGN_param_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A205",
        concat("Error at ",
            instr_name,
            " instruction: ALIGN operand parameter value must be one of the following: 0, 1, 2, 3, 4, 12"),
        range);
}

diagnostic_op diagnostic_op::error_A206_FILL_param_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A206",
        concat("Error at ",
            instr_name,
            " instruction: FILL operand parameter must be an unsigned decimal number in range 0 through 255"),
        range);
}

diagnostic_op diagnostic_op::error_A207_PART_param_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A207",
        concat("Error at ",
            instr_name,
            " instruction: PART operand parameter value must be a part-name of maximum 63 characters"),
        range);
}

diagnostic_op diagnostic_op::error_A208_PRIORITY_param_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A208",
        concat("Error at ",
            instr_name,
            " instruction: PRIORITY operand parameter value must be an unsigned decimal number in range 0 through "
            "2^31-1"),
        range);
}

diagnostic_op diagnostic_op::error_A209_COMPAT_param_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A209",
        concat("Error at ",
            instr_name,
            " instruction: COMPAT option parameter must have one of the following values: CASE, NOCASE, LITTYPE|LIT, "
            "NOLITTYPE|NOLIT, MACROCASE|MC, NOMACROCASE|NOMC, SYSLIST|SYSL, NOSYSLIST|NOSYSL, TRANSDT|TRS, "
            "NOTRANSDT|NOTRS"),
        range);
}

diagnostic_op diagnostic_op::error_A210_FLAG_integer_size(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A210",
        concat("Error at ",
            instr_name,
            " instruction: the value of FLAG option parameter specifying error diagnostic message must be in range 0 "
            "through 255"),
        range);
}

diagnostic_op diagnostic_op::error_A211_FLAG_op_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A211",
        concat("Error at ",
            instr_name,
            " instruction: FLAG option parameter must have one of the following values: ALIGN|AL, NOALIGN|NOAL, CONT, "
            "NOCONT, PAGE0, NOPAGE0, SUB, NOSUB, USING0|US0, NOUSING0|NOUS0, IMPLEN, NOIMPLEN, EXLITW, NOEXLITW, "
            "SUBSTR|NOSUBSTR"),
        range);
}

diagnostic_op diagnostic_op::error_A212_OPTABLE_first_op(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A212",
        concat("Error at ",
            instr_name,
            " instruction: first parameter of the OPTABLE option must be one of the following values: DOS, ESA, XA, "
            "370, YOP, ZOP, ZS3, ZS4, ZS5, ZS6, ZS7, ZS8"),
        range);
}

diagnostic_op diagnostic_op::error_A213_OPTABLE_second_op(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A213",
        concat("Error at ",
            instr_name,
            " instruction: second parameter of the OPTABLE option must either be omitted or must have one of the "
            "following values: LIST, NOLIST"),
        range);
}

diagnostic_op diagnostic_op::error_A214_TYPECHECK_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A214",
        concat("Error at ",
            instr_name,
            " instruction: the parameter of the TYPECHECK option must be one of the following values: MAGNITUDE|MAG, "
            "REGISTER|REG, NOMAGNITUDE|NOMAG, NOREGISTER|NOREG"),
        range);
}

diagnostic_op diagnostic_op::error_A215_CODEPAGE_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A215",
        concat("Error at ",
            instr_name,
            " instruction: the parameter of the CODEPAGE option must be either in a nnnnn format specifying a decimal "
            "value, or a X'xxxx' format specifying a hexadecimal value"),
        range);
}

diagnostic_op diagnostic_op::error_A216_CODEPAGE_value(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A216",
        concat("Error at ",
            instr_name,
            " instruction: the value of the parameter of the CODEPAGE option must evaluate to an absolute value in the "
            "range 1140 through 1148"),
        range);
}

diagnostic_op diagnostic_op::error_A217_INFO_value(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A217",
        concat("Error at ",
            instr_name,
            " instruction: INFO parameter must either be in a yyyymmdd format specifying date, or it must be omitted"),
        range);
}

diagnostic_op diagnostic_op::error_A218_MXREF_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A218",
        concat("Error at ",
            instr_name,
            " instruction: parameter of the MXREF option must either be omitted or must be one of the following "
            "values: FULL, XREF, SOURCE"),
        range);
}

diagnostic_op diagnostic_op::error_A219_SECTALGN_par_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A219",
        concat("Error at ",
            instr_name,
            " instruction at SECTALGN option: alignment parameter must specify a positive absolute value"),
        range);
}

diagnostic_op diagnostic_op::error_A220_SECTALGN_par_value(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A220",
        concat("Error at ",
            instr_name,
            " instruction at SECTALGN option: alignment parameter value must be a power of 2 in the range 8 through "
            "4096"),
        range);
}

diagnostic_op diagnostic_op::error_A221_MACH_second_par_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A221",
        concat("Error at ",
            instr_name,
            " instruction: second parameter of the MACHINE option must either be omitted or must have one of the "
            "following values: LIST, NOLIST"),
        range);
}

diagnostic_op diagnostic_op::error_A222_MACH_first_par_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A222",
        concat("Error at ",
            instr_name,
            " instruction: first parameter of the MACHINE option must have one of the following values: S370, S370XA, "
            "S370ESA, S390, S390E, ZSERIES, ZS, (ZS|ZSERIES)-(2|3|4|5|6|7|8)"),
        range);
}

diagnostic_op diagnostic_op::error_A223_PCONTROL_par_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A223",
        concat("Error at ",
            instr_name,
            " instruction: PCONTROL option parameter must have one of the following values: ON, OFF, MCALL|MC, "
            "NOMCALL|NOMC, MSOURCE|MS, NOMSOURCE|NOMS, UHEAD|UHD, NOUHEAD|NOUHD, GEN, NOGEN, DATA, NODATA"),
        range);
}

diagnostic_op diagnostic_op::error_A224_XREF_par_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A224",
        concat("Error at ",
            instr_name,
            " instruction: XREF option must either have exactly one parameter with FULL value, or multiple parameters "
            "with either SHORT or UNREFS value"),
        range);
}

diagnostic_op diagnostic_op::error_A225_SUPRWARN_par_format(
    std::string_view instr_name, std::string_view op_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A225",
        concat("Error at ",
            instr_name,
            " instruction at ",
            op_name,
            " option: operand must specify a 1-4 digit message number"),
        range);
}

diagnostic_op diagnostic_op::error_A226_SUPRWARN_par_size(
    std::string_view instr_name, std::string_view op_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A226",
        concat(
            "Error at ", instr_name, " instruction at ", op_name, " option: message parameter must contain 1-4 digits"),
        range);
}

diagnostic_op diagnostic_op::error_A227_USING_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A227",
        concat("Error at ",
            instr_name,
            " instruction: USING option parameter must have one of the following values: MAP, NOMAP, WARN(n), NOWARN, "
            "LIMIT(xxxx), NOLIMIT"),
        range);
}

diagnostic_op diagnostic_op::error_A228_USING_complex_param_no(
    std::string_view instr_name, std::string_view param_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A228",
        concat("Error at ",
            instr_name,
            " instruction at USING option: ",
            param_name,
            " parameter must be specified in the following format ",
            param_name,
            "(value)"),
        range);
}

diagnostic_op diagnostic_op::error_A229_USING_WARN_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A229",
        concat("Error at ",
            instr_name,
            " instruction at USING option: the condition number associated with the WARN(n) suboption must be in range "
            "0 through 15"),
        range);
}

diagnostic_op diagnostic_op::error_A230_USING_LIMIT_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A230",
        concat("Error at ",
            instr_name,
            " instruction at USING option: value associated with the LIMIT(xxxx) suboption must be either a decimal "
            "value, or must be in a X'xxx' format specifying a hexadecimal value"),
        range);
}

diagnostic_op diagnostic_op::error_A231_USING_LIMIT_decimal(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A231",
        concat("Error at ",
            instr_name,
            " instruction at USING option: the maximum value of the decimal value xxxx associated with the LIMIT(xxxx) "
            "suboption is 4095"),
        range);
}

diagnostic_op diagnostic_op::error_A232_USING_LIMIT_hexa(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A232",
        concat("Error at ",
            instr_name,
            " instruction at USING option: the maximum value of the hexadecimal value xxxx associated with the "
            "LIMIT(xxxx) suboption is FFF"),
        range);
}

diagnostic_op diagnostic_op::error_A233_FAIL_param_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A233",
        concat("Error at ",
            instr_name,
            " instruction: FAIL option parameter must have one of the following values: MSG(msgsev), NOMSG, "
            "MNOTE(mnotesev), NOMNOTE, MAXERRS(maxerrs), NOMAXERRS(maxerrs)"),
        range);
}

diagnostic_op diagnostic_op::error_A234_FAIL_complex_param_no(
    std::string_view instr_name, std::string_view param_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A234",
        concat("Error at ",
            instr_name,
            " instruction at FAIL option: ",
            param_name,
            " parameter must be specified in the following format ",
            param_name,
            "(value)"),
        range);
}

diagnostic_op diagnostic_op::error_A235_FAIL_param_number_format(
    std::string_view instr_name, std::string_view op_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A235",
        concat("Error at ",
            instr_name,
            " instruction at FAIL option: the parameter of the ",
            op_name,
            " suboption must be an absolute value"),
        range);
}

diagnostic_op diagnostic_op::error_A236_FAIL_MAXXERS_value(
    std::string_view instr_name, std::string_view op_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A236",
        concat("Error at ",
            instr_name,
            " instruction at FAIL option: the value of the maxerrs parameter specified in ",
            op_name,
            "(maxerrs) suboption must be in range 32 through 65535"),
        range);
}

diagnostic_op diagnostic_op::error_A237_FAIL_severity_message(
    std::string_view instr_name, std::string_view op_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A237",
        concat("Error at ",
            instr_name,
            " instruction at FAIL option: the value of the parameter of the ",
            op_name,
            " suboption must be in range 0 through 7"),
        range);
}

diagnostic_op diagnostic_op::error_A238_REF_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A288",
        concat("Error at ",
            instr_name,
            " instruction at REFERENCE option: parameter must be in one of the following formats: DIRECT|INDIRECT, "
            "CODE|DATA"),
        range);
}

diagnostic_op diagnostic_op::error_A239_ADATA_char_string_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A239",
        "Error at ADATA instruction: the last operand must either be a character string enclosed in single quotes, or "
        "it must be omitted",
        range);
}

diagnostic_op diagnostic_op::error_A240_expression_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A240",
        concat(
            "Error at ", instr_name, " instruction: operand must either specify an absolute value or must be omitted"),
        range);
}

diagnostic_op diagnostic_op::error_A242_ICTL_op_format_second_third(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A242",
        "Error at ICTL instruction: operand value must either be a decimal self-defining term or the operand must be "
        "omitted",
        range);
}

diagnostic_op diagnostic_op::warning_A243_END_expr_format(const range& range)
{
    return diagnostic_op(diagnostic_severity::warning,
        "A243",
        "First operand must either be an expression or the operand must be omitted",
        range);
}

diagnostic_op diagnostic_op::error_A244_PUNCH_char_string(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A244",
        "Error at PUNCH instruction: operand must specify a character string",
        range);
}

diagnostic_op diagnostic_op::error_A245_ORG_expression(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A245",
        "Error at ORG instruction: operand must specify relocatable expression",
        range);
}

diagnostic_op diagnostic_op::error_A246_OPSYN(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A246",
        "Error at OPSYN instruction: operand must either specify an operation code or must be omitted",
        range);
}

diagnostic_op diagnostic_op::error_A247_must_be_rel_abs_expr(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A247",
        concat("Error at ", instr_name, " instruction: operand must be a relocatable or an absolute expression"),
        range);
}

diagnostic_op diagnostic_op::error_A248_PSECT_param_format(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A248",
        concat("Error at ",
            instr_name,
            " instruction: PSECT operand parameter value must be a symbol name of maximum 63 characters"),
        range);
}

diagnostic_op diagnostic_op::warning_A248_END_lang_char_sequence(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::warning, "A248", "Language operand parameter must be a character sequence", range);
}

diagnostic_op diagnostic_op::warning_A249_sequence_symbol_expected(const range& range)
{
    return diagnostic_op(diagnostic_severity::warning, "A249", "Sequence symbol expected", range);
}

diagnostic_op diagnostic_op::error_A250_absolute_with_known_symbols(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A250",
        "Operand must be an absolute expression of previously defined symbols",
        range);
}

diagnostic_op diagnostic_op::warn_A251_unexpected_label(const range& range)
{
    return diagnostic_op(diagnostic_severity::warning, "A251", "Unexpected label", range);
}

diagnostic_op diagnostic_op::warning_A300_op_apostrophes_missing(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::warning,
        "A300",
        concat("Warning at ", instr_name, " instruction: operand not properly enclosed in quotes"),
        range);
}

diagnostic_op diagnostic_op::error_A301_op_apostrophes_missing(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "A301",
        concat("Error at ", instr_name, " instruction: operand not properly enclosed in quotes"),
        range);
}

diagnostic_op diagnostic_op::warning_A302_punch_empty(const range& range)
{
    return diagnostic_op(diagnostic_severity::warning, "A302", "Operand is empty, record not punched", range);
}

diagnostic_op diagnostic_op::error_NOERR(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "NOERR", "No error found", range);
}

bool diagnostic_op::is_error(const diagnostic_op& diag) { return diag.code != "NOERR"; }


// machine instruction errors

diagnostic_op diagnostic_op::error_M100(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M100",
        concat("Error at ", instr_name, " instruction: operand must be in an address D(X,B) format"),
        range);
}

diagnostic_op diagnostic_op::error_M101(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M101",
        concat("Error at ", instr_name, " instruction: operand must be in an address D(L,B) format"),
        range);
}

diagnostic_op diagnostic_op::error_M102(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M102",
        concat("Error at ", instr_name, " instruction: operand must be in an address D(R,B) format"),
        range);
}

diagnostic_op diagnostic_op::error_M103(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M103",
        concat("Error at ", instr_name, " instruction: operand must be in an address D(V,B) format"),
        range);
}

diagnostic_op diagnostic_op::error_M104(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M104",
        concat("Error at ", instr_name, " instruction: operand must be in an address D(B) format"),
        range);
}

diagnostic_op diagnostic_op::error_M110(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M110",
        concat("Error at ", instr_name, " instruction: operand must be an absolute register value"),
        range);
}

diagnostic_op diagnostic_op::error_M111(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M111",
        concat("Error at ", instr_name, " instruction: operand must be an absolute mask value"),
        range);
}

diagnostic_op diagnostic_op::error_M112(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M112",
        concat("Error at ", instr_name, " instruction: operand must be an absolute immediate value"),
        range);
}

diagnostic_op diagnostic_op::error_M113(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M113",
        concat(
            "Error at ", instr_name, " instruction: operand must be relocatable symbol or an absolute immediate value"),
        range);
}

diagnostic_op diagnostic_op::error_M114(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M114",
        concat("Error at ", instr_name, " instruction: operand must be an absolute vector register value"),
        range);
}

diagnostic_op diagnostic_op::error_M120(
    std::string_view instr_name, const range& range, long long from, long long to, std::string_view qual)
{
    return diagnostic_op(diagnostic_severity::error,
        "M120",
        concat("Error at ",
            instr_name,
            " instruction: register operand absolute value must be ",
            qual,
            std::string_view(" ", +!qual.empty()),
            "between ",
            from,
            " and ",
            to),
        range);
}

diagnostic_op diagnostic_op::error_M121(std::string_view instr_name, long long from, long long to, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M121",
        concat(
            "Error at ", instr_name, " instruction: mask operand absolute value must be between ", from, " and ", to),
        range);
}

diagnostic_op diagnostic_op::error_M122(std::string_view instr_name, long long from, long long to, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M122",
        concat("Error at ",
            instr_name,
            " instruction: immediate operand absolute value must be between ",
            from,
            " and ",
            to),
        range);
}

diagnostic_op diagnostic_op::error_M123(std::string_view instr_name, long long from, long long to, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M123",
        concat("Error at ",
            instr_name,
            " instruction: relocatable symbol or immediate absolute value must be between ",
            from,
            " and ",
            to),
        range);
}

diagnostic_op diagnostic_op::error_M124(std::string_view instr_name, long long from, long long to, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M124",
        concat("Error at ",
            instr_name,
            " instruction: vector register operand absolute value must be between ",
            from,
            " and ",
            to),
        range);
}

diagnostic_op diagnostic_op::error_M130(std::string_view instr_name, long long from, long long to, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M130",
        concat("Error at ",
            instr_name,
            " instruction: value of the address operand displacement value must be between ",
            from,
            " and ",
            to),
        range);
}

diagnostic_op diagnostic_op::error_M131(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M131",
        concat("Error at ",
            instr_name,
            " instruction: value of the address operand base register parameter must be between 0 and 15"),
        range);
}

diagnostic_op diagnostic_op::error_M132(std::string_view instr_name, long long from, long long to, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M132",
        concat("Error at ",
            instr_name,
            " instruction: value of the address operand length parameter must be between ",
            from,
            " and ",
            to),
        range);
}

diagnostic_op diagnostic_op::error_M133(std::string_view instr_name, long long from, long long to, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M133",
        concat("Error at ",
            instr_name,
            " instruction: value of the address operand register parameter must be between ",
            from,
            " and ",
            to),
        range);
}

diagnostic_op diagnostic_op::error_M134(std::string_view instr_name, long long from, long long to, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M134",
        concat("Error at ",
            instr_name,
            " instruction: value of the address operand vector register parameter must be between ",
            from,
            " and ",
            to),
        range);
}

diagnostic_op diagnostic_op::error_D001(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "D001", "Integer out of range", range);
}

diagnostic_op diagnostic_op::error_D002(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "D002", "Expected an integer", range);
}

diagnostic_op diagnostic_op::error_D003(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "D003", "Expected an integer or an expression after modifier", range);
}

// diagnostic_op diagnostic_op::error_D004(const range& range)
// {
//     return diagnostic_op(diagnostic_severity::error, "D004", "Wrong order of modifiers", range);
// }

// diagnostic_op diagnostic_op::error_D005(const range& range)
// {
//     return diagnostic_op(diagnostic_severity::error, "D005", "Unexpected '.' in modifier other than length", range);
// }

diagnostic_op diagnostic_op::error_D006(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "D006", "Unexpected character. Nominal value or modifier expected", range);
}

diagnostic_op diagnostic_op::error_D007(const range& range, std::string_view type, std::string_view suffix)
{
    return diagnostic_op(
        diagnostic_severity::error, "D007", concat("Bit length not allowed with type ", type, suffix), range);
}

diagnostic_op diagnostic_op::error_D008(
    const range& range, std::string_view type, std::string_view modifier, int min, int max, std::string_view suffix)
{
    if (min == max)
        return diagnostic_op(diagnostic_severity::error,
            "D008",
            concat("The ", modifier, " modifier of type ", type, suffix, " must be ", min),
            range);
    else
        return diagnostic_op(diagnostic_severity::error,
            "D008",
            concat("The ", modifier, " modifier of type ", type, suffix, " must be between ", min, " and ", max),
            range);
}

diagnostic_op diagnostic_op::error_D009(const range& range, std::string_view type, std::string_view modifier)
{
    return diagnostic_op(
        diagnostic_severity::error, "D009", concat("The ", modifier, " modifier not allowed with type ", type), range);
}

diagnostic_op diagnostic_op::error_D010(const range& range, std::string_view type)
{
    return diagnostic_op(
        diagnostic_severity::error, "D010", concat("Wrong format of nominal value of type ", type), range);
}

diagnostic_op diagnostic_op::error_D011(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "D011", "Duplication factor must be non-negative", range);
}

diagnostic_op diagnostic_op::error_D012(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "D012", "Unknown type of data definition", range);
}

diagnostic_op diagnostic_op::error_D013(const range& range, std::string_view type)
{
    return diagnostic_op(diagnostic_severity::error, "D013", concat("Invalid type extension for type ", type), range);
}

diagnostic_op diagnostic_op::error_D014(const range& range, std::string_view type)
{
    return diagnostic_op(
        diagnostic_severity::error, "D014", concat("The length modifier must be even with type ", type), range);
}

diagnostic_op diagnostic_op::error_D015(const range& range, std::string_view type)
{
    return diagnostic_op(diagnostic_severity::error,
        "D015",
        concat("Only hexadecimal digits allowed in nominal value of type ", type),
        range);
}

diagnostic_op diagnostic_op::error_D016(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "D016", "Nominal value expected", range);
}

diagnostic_op diagnostic_op::error_D017(const range& range, std::string_view type)
{
    return diagnostic_op(diagnostic_severity::error,
        "D017",
        concat("Nominal value enclosed in parentheses expected with type ", type),
        range);
}

diagnostic_op diagnostic_op::error_D018(const range& range, std::string_view type)
{
    return diagnostic_op(diagnostic_severity::error,
        "D018",
        concat("Nominal value enclosed in apostrophes expected with type ", type),
        range);
}

diagnostic_op diagnostic_op::error_D019(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "D019", "Name of a symbol expected", range);
}

diagnostic_op diagnostic_op::error_D020(const range& range, std::string_view type)
{
    return diagnostic_op(
        diagnostic_severity::error, "D020", concat("Address in form D(B) is not allowed with type ", type), range);
}

diagnostic_op diagnostic_op::error_D021(const range& range, std::string_view type)
{
    return diagnostic_op(
        diagnostic_severity::error, "D021", concat("Only lengths 3, 4 or 8 are allowed with type ", type), range);
}

diagnostic_op diagnostic_op::error_D022(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "D022", "Displacement out of range", range);
}

diagnostic_op diagnostic_op::error_D023(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "D023", "Base register must be between 0 and 15", range);
}

diagnostic_op diagnostic_op::error_D024(const range& range, std::string_view type)
{
    return diagnostic_op(diagnostic_severity::error,
        "D024",
        concat("With type ", type, " only lengths 2 to 4 and 8 are allowed"),
        range);
}

diagnostic_op diagnostic_op::warn_D025(const range& range, std::string_view type, std::string_view modifier)
{
    return diagnostic_op(
        diagnostic_severity::warning, "D025", concat("The ", modifier, " modifier is ignored with type ", type), range);
}

diagnostic_op diagnostic_op::error_D026(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "D026", "Invalid round mode", range);
}

diagnostic_op diagnostic_op::error_D027(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "D027", "Graphic constant must be enclosed in SI '<' and SO '>'", range);
}

diagnostic_op diagnostic_op::error_D028(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "D028", "Data definition operand is longer than 2^31 bytes", range);
}

diagnostic_op diagnostic_op::error_D029(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "D029", "Data definition operands are longer than 2^31 bytes", range);
}

diagnostic_op diagnostic_op::error_D030(const range& range, std::string_view type)
{
    return diagnostic_op(
        diagnostic_severity::error, "D030", concat("Only single symbol expected with type ", type), range);
}

diagnostic_op diagnostic_op::error_D031(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "D031", "Duplication factor in literals must be positive", range);
}

diagnostic_op diagnostic_op::warn_D032(const range& range, std::string_view operand_value)
{
    return diagnostic_op(diagnostic_severity::warning,
        "D032",
        concat("Using absolute value '", operand_value, "' as relative immediate value"),
        range);
}

diagnostic_op diagnostic_op::error_D033(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "D033",
        "Address in form D(B) or a simple relocatable expression resolvable by USING expected",
        range);
}

diagnostic_op diagnostic_op::error_D034(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "D034", "Expression must evaluate into an absolute value", range);
}

diagnostic_op diagnostic_op::error_D035(const range& range, bool goff)
{
    return diagnostic_op(diagnostic_severity::error,
        "D035",
        goff ? "Only DXD, DSECT and external symbols allowed" : "Only DXD and DSECT symbols allowed",
        range);
}

diagnostic_op diagnostic_op::error_M135(std::string_view instr_name, long long from, long long to, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M135",
        concat("Error at ",
            instr_name,
            " instruction: value of the address operand displacement register parameter must be between ",
            from,
            " and ",
            to),
        range);
}

diagnostic_op diagnostic_op::warn_M136(const range& range)
{
    return diagnostic_op(diagnostic_severity::warning,
        "M136",
        "Relative Immediate reference across sections used in non-GOFF object",
        range);
}

diagnostic_op diagnostic_op::warn_M137(std::string_view instr_name, long long from, long long to, const range& range)
{
    return diagnostic_op(diagnostic_severity::warning,
        "M137",
        concat(instr_name, " instruction: immediate operand absolute value should be between ", from, " and ", to),
        range);
}

diagnostic_op diagnostic_op::error_optional_number_of_operands(
    std::string_view instr_name, size_t optional_no, size_t operands_no, const range& range)
{
    if (optional_no == 0)
        return error_M000(instr_name, operands_no, range);
    else if (optional_no == 1)
        return error_M001(instr_name, operands_no - 1, operands_no, range);
    else
        return error_M002(instr_name, operands_no - optional_no, operands_no, range);
}

diagnostic_op hlasm_plugin::parser_library::diagnostic_op::error_M000(
    std::string_view instr_name, size_t number, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M000",
        concat("Incorrect number of operands at ", instr_name, " instruction: number of operands has to be ", number),
        range);
}

diagnostic_op diagnostic_op::error_M001(std::string_view instr_name, size_t one, size_t two, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M001",
        concat("Incorrect number of operands at ",
            instr_name,
            " instruction: number of operands has to be either ",
            one,
            " or ",
            two),
        range);
}

diagnostic_op hlasm_plugin::parser_library::diagnostic_op::error_M010(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M010",
        concat("Error at ", instr_name, " instruction: address operand is not valid"),
        range);
}

diagnostic_op diagnostic_op::error_M002(std::string_view instr_name, size_t one, size_t two, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M002",
        concat("Incorrect number of operands at ",
            instr_name,
            " instruction: number of operands has to be from ",
            one,
            " to ",
            two),
        range);
}

diagnostic_op diagnostic_op::error_M003(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M003",
        concat("Error at ", instr_name, " instruction: operand cannot be empty"),
        range);
}

diagnostic_op diagnostic_op::error_M004(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "M004", concat("Operand format D(X,) not allowed"), range);
}

diagnostic_op diagnostic_op::warning_M041(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::warning,
        "M041",
        concat("Warning at ", instr_name, " instruction: non-standard address format"),
        range);
}

diagnostic_op diagnostic_op::error_M200(std::string_view instr_name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "M200",
        concat("Error at ", instr_name, " instruction: second and third operand must be equal"),
        range);
}

diagnostic_op diagnostic_op::error_E001(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "E001", "Continued line does not begin with required number of blanks", range);
}

diagnostic_op diagnostic_op::error_E010(std::string_view type, std::string_view name, const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E010", concat("Unknown ", type, ": ", name), range);
}

diagnostic_op diagnostic_op::error_E011(std::string_view message, const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E011", concat(message, " already specified"), range);
}

diagnostic_op diagnostic_op::error_E012(std::string_view message, const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E012", concat("Wrong format: ", message), range);
}

diagnostic_op diagnostic_op::error_E013(std::string_view message, const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E013", concat("Inconsistent format: ", message), range);
}

diagnostic_op diagnostic_op::error_E014(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E014", "Variable name expected", range);
}

diagnostic_op diagnostic_op::error_E015(std::span<const std::string_view> expected, const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "E015", concat("Unexpected operand type. Allowed types: ", expected), range);
}

diagnostic_op diagnostic_op::error_E016(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E016", "Unable to evaluate operand", range);
}

diagnostic_op diagnostic_op::error_E020(std::string_view message, const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "E020", concat("Error at ", message, " - too many operands"), range);
}

diagnostic_op diagnostic_op::error_E021(std::string_view message, const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "E021", concat("Error at ", message, " - operand number too low"), range);
}

diagnostic_op diagnostic_op::error_E022(std::string_view message, const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E022", concat("Error at ", message, " - operand missing"), range);
}

diagnostic_op diagnostic_op::error_E030(std::string_view message, const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E030", concat("Can't assign value to ", message), range);
}

diagnostic_op diagnostic_op::error_E031(std::string_view message, const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "E031", concat("Cannot declare ", message, " with the same name"), range);
}

diagnostic_op diagnostic_op::error_E032(std::string_view message, const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E032", concat("Undefined symbol - ", message), range);
}

diagnostic_op diagnostic_op::error_E033(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E033", "Cyclic symbol definition", range);
}

diagnostic_op diagnostic_op::error_E042(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E042", "Macro name omitted - ASPACE instead", range);
}

diagnostic_op diagnostic_op::error_E043(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E043", "Invalid name of variable in macro prototype", range);
}

diagnostic_op diagnostic_op::error_E044(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E044", "Illegal name field in macro prototype, discarded", range);
}

diagnostic_op diagnostic_op::error_E045(std::string_view message, const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "E045", concat("Sequence symbol already defined - ", message), range);
}

diagnostic_op diagnostic_op::error_E046(std::string_view message, const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E046", concat("Missing MEND in ", message), range);
}

diagnostic_op diagnostic_op::error_E047(std::string_view message, const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "E047", concat("Lookahead failed, symbol not found - ", message), range);
}

diagnostic_op diagnostic_op::error_E048(std::string_view message, const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "E048", concat("Undefined sequence symbol, macro aborted - ", message), range);
}

diagnostic_op diagnostic_op::error_E049(std::string_view message, const range& range)
{
    // E049 code utilized in the extension code
    return diagnostic_op(diagnostic_severity::error, "E049", concat("Operation code not found - ", message), range);
}

diagnostic_op diagnostic_op::error_E050(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E050", "Illegal set symbol name", range);
}

diagnostic_op diagnostic_op::error_E051(std::string_view message, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "E051",
        concat("Duplicate SET symbol declaration, first is retained - ", message),
        range);
}

diagnostic_op diagnostic_op::error_E052(std::string_view message, const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "E052", concat("Illegal use of symbolic parameter - ", message), range);
}

diagnostic_op diagnostic_op::error_E053(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E053", "Required name missing", range);
}

diagnostic_op diagnostic_op::error_E054(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E054", "Illegal statement outside macro definition", range);
}

diagnostic_op diagnostic_op::error_E055(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E055", "Too many nested macro calls, continuing opencode", range);
}

diagnostic_op diagnostic_op::error_E056(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E056", "ACTR counter exceeded", range);
}

diagnostic_op diagnostic_op::error_E057(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E057", "Symbol is not an ordinary or a sequence symbol", range);
}

diagnostic_op diagnostic_op::error_E058(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E058", "Copy member not found", range);
}

diagnostic_op diagnostic_op::error_E059(std::string_view message, const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "E059", concat("First statement not MACRO in library ", message), range);
}

diagnostic_op diagnostic_op::error_E060(std::string_view message, const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "E060", concat("Library macro name incorrect, expected ", message), range);
}

diagnostic_op diagnostic_op::error_E061(std::string_view message, const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "E061", concat("Unbalanced MACRO MEND statements in copy member ", message), range);
}

diagnostic_op diagnostic_op::error_E062(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E062", "Recursive COPY", range);
}

diagnostic_op diagnostic_op::error_W063(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::warning, "W063", "Too many ACTR calls, review the use of the ACTR instruction", range);
}

diagnostic_op diagnostic_op::error_E064(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E064", "Illegal use of ampersand", range);
}

diagnostic_op diagnostic_op::error_E065(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E065", "Invalid symbol name", range);
}

diagnostic_op diagnostic_op::error_E066(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E066", "Illegal attribute reference", range);
}

diagnostic_op diagnostic_op::error_E067(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E067", "Blank symbol in instruction", range);
}

diagnostic_op diagnostic_op::error_E068(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "E068", "Operand value falls outside of current section/LOCTR", range);
}

diagnostic_op diagnostic_op::error_E069(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "E069", "AREAD instruction can only be called from within a macro", range);
}

diagnostic_op diagnostic_op::error_E070(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "E070", "Invalid AREAD operand. Use AREAD [NOSTMT|NOPRINT|CLOCKB|CLOCKD].", range);
}

diagnostic_op diagnostic_op::error_E071(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E071", "Macro prototype expected.", range);
}

diagnostic_op diagnostic_op::error_E072(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E072", "SYSNDX limit reached, macro call suppressed.", range);
}

diagnostic_op diagnostic_op::error_E073(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "E073", "Illegal START instruction - CSECT already exists.", range);
}

diagnostic_op diagnostic_op::error_E074(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E074", "NULL operation code has been generated.", range);
}

diagnostic_op diagnostic_op::error_E075(std::string_view message, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "E075",
        concat("The name field ",
            message,
            " contains unexpected characters. Valid characters are A-Z, 0-9, $, #, @ and _"),
        range);
}

diagnostic_op diagnostic_op::error_E076(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "E076", "SYSLIST must be subscripted; using default subscript is 1", range);
}

diagnostic_op diagnostic_op::error_E077(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E077", "Statement count limit reached", range);
}

diagnostic_op diagnostic_op::error_E078(std::string_view message, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "E078",
        concat("Global variable re-declared with an incorrect type - ", message),
        range);
}

diagnostic_op diagnostic_op::error_E079(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E079", "Argument limit (2GB) reached", range);
}

diagnostic_op diagnostic_op::error_E080(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E080", "Array size limit reached", range);
}

diagnostic_op diagnostic_op::error_E081(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "E081", "SYSLIST limit (2G) reached", range);
}

diagnostic_op diagnostic_op::warning_W010(std::string_view message, const range& range)
{
    return diagnostic_op(diagnostic_severity::warning, "W010", concat(message, " not expected"), range);
}

diagnostic_op diagnostic_op::warning_W011(const range& range)
{
    return diagnostic_op(diagnostic_severity::warning,
        "W011",
        "Wrong type of constant for S or I attribute reference, default is 0",
        range);
}

diagnostic_op diagnostic_op::warning_W012(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::warning, "W012", "Length of EQUated symbol undefined, default is 1", range);
}

diagnostic_op diagnostic_op::warning_W013(const range& range)
{
    return diagnostic_op(diagnostic_severity::warning, "W013", "Undefined symbol attribute, default used", range);
}

diagnostic_op diagnostic_op::warning_W014(const range& range)
{
    return diagnostic_op(diagnostic_severity::warning,
        "W013",
        "Undefined keyword parameter, default to positional including keyword",
        range);
}

diagnostic_op diagnostic_op::warning_W015(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::warning, "W015", "End of source input reached, batch mode is not supported yet", range);
}

diagnostic_op diagnostic_op::warning_W016(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::warning, "W016", "Multiple TITLE instructions with a non-empty name field", range);
}

diagnostic_op diagnostic_op::warning_W017(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::warning, "W017", "Non-UTF-8 characters detected and replaced [server]", range);
}

diagnostic_op diagnostic_op::warning_W018(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::warning, "W018", "Non-UTF-8 characters detected and replaced [client]", range);
}

diagnostic_op diagnostic_op::error_EQU1(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "EQU1", "Constant redefinition", range);
}

diagnostic_op diagnostic_op::error_EQU2(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "EQU2", "Label redefinition", range);
}

diagnostic_op diagnostic_op::error_ME001(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "ME001", "Constant number overflow.", range);
}

diagnostic_op diagnostic_op::error_ME002(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "ME002", "multiplication or division of address", range);
}

diagnostic_op diagnostic_op::error_ME003(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "ME003", "Relative Immediate operand must evaluate into an even offset.", range);
}

diagnostic_op diagnostic_op::error_ME004(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "ME004", "Absolute expression cannot be qualified with a label.", range);
}

diagnostic_op diagnostic_op::error_ME005(std::string_view label, std::string_view sect, const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "ME005",
        concat("Labeled USING '", label, "' does not map section '", sect, "'."),
        range);
}

diagnostic_op diagnostic_op::error_ME006(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "ME006", "Complex relocatable expression cannot be qualified with a label.", range);
}

diagnostic_op diagnostic_op::error_ME007(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "ME007", "No active USING.", range);
}

diagnostic_op diagnostic_op::error_ME008(long missed_by, const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "ME008", concat("Beyond active USING range by ", missed_by, "."), range);
}

diagnostic_op diagnostic_op::error_ME009(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "ME009", "A simple relocatable expression resolvable by USING expected.", range);
}

diagnostic_op diagnostic_op::error_ME010(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "ME010", "Expression must evaluate to an absolute value.", range);
}

diagnostic_op diagnostic_op::error_ME011(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "ME011",
        "Cannot combine relocatable displacement with an explicit base register.",
        range);
}

diagnostic_op diagnostic_op::error_CE001(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "CE001", "Operator expected", range);
}

diagnostic_op diagnostic_op::error_CE002(std::string_view message, const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "CE002", concat("Undefined operator - ", message), range);
}

diagnostic_op diagnostic_op::error_CE003(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "CE003", "Operand expected", range);
}

diagnostic_op diagnostic_op::error_CE004(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "CE004", "Invalid use of operator - different return type expected", range);
}

diagnostic_op diagnostic_op::error_CE005(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "CE005", "Illegal duplication factor", range);
}

diagnostic_op diagnostic_op::error_CE006(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "CE006", "Bad number of operands", range);
}

diagnostic_op diagnostic_op::error_CE007(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "CE007", "Bad operand value", range);
}

diagnostic_op diagnostic_op::error_CE008(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "CE008", "Bad substring expression", range);
}

diagnostic_op diagnostic_op::error_CE009(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "CE009", "Substring start points past string end", range);
}

diagnostic_op diagnostic_op::error_CE010(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "CE010", "Negative duplication factor", range);
}

diagnostic_op diagnostic_op::error_CE011(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "CE011", "Maximum string size exceeded", range);
}

diagnostic_op diagnostic_op::error_CE012(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "CE012", "Only absolute and defined symbols allowed", range);
}

diagnostic_op diagnostic_op::error_CE013(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "CE013", "Arithmetic overflow", range);
}

diagnostic_op diagnostic_op::error_CE014(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "CE014", "Arithmetic underflow", range);
}

diagnostic_op diagnostic_op::error_CE015(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "CE015", "Invalid self-defining term", range);
}

diagnostic_op diagnostic_op::error_CE016_logical_expression_parenthesis(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "CE016", "Logical expression must be enclosed in parenthesis.", range);
}

diagnostic_op diagnostic_op::error_CE017_character_expression_expected(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "CE017", "Character expression expected", range);
}

diagnostic_op diagnostic_op::error_CW001(const range& range)
{
    return diagnostic_op(diagnostic_severity::warning, "CW001", "Substring count points past string end", range);
}

diagnostic_op diagnostic_op::error_DB001(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "DB001", "DB2 preprocessor - invalid line continuation", range);
}

diagnostic_op diagnostic_op::error_DB002(const range& range, std::string_view lib)
{
    return diagnostic_op(diagnostic_severity::error,
        "DB002",
        std::string("DB2 preprocessor - unable to find library '").append(lib).append("'"),
        range);
}

diagnostic_op diagnostic_op::error_DB003(const range& range, std::string_view lib)
{
    return diagnostic_op(diagnostic_severity::error,
        "DB003",
        std::string("DB2 preprocessor - nested include '").append(lib).append("' requested"),
        range);
}

diagnostic_op diagnostic_op::error_DB004(const range& range)
{
    return diagnostic_op(diagnostic_severity::error,
        "DB004",
        std::string(
            "DB2 preprocessor - requested 'SQL TYPE IS' not recognized (operands either missing or not recognized)"),
        range);
}

diagnostic_op diagnostic_op::warn_DB005(const range& range)
{
    return diagnostic_op(diagnostic_severity::warning,
        "DB005",
        std::string("DB2 preprocessor - continuation detected on 'SQL TYPE' statement"),
        range);
}

diagnostic_op diagnostic_op::warn_DB006(const range& range)
{
    return diagnostic_op(diagnostic_severity::warning,
        "DB006",
        std::string("DB2 preprocessor - requested 'SQL TYPE' not recognized (operand 'IS' either missing or split)"),
        range);
}

diagnostic_op diagnostic_op::warn_DB007(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::warning, "DB007", std::string("DB2 preprocessor - missing INCLUDE member"), range);
}

diagnostic_op diagnostic_op::warn_CIC001(const range& range)
{
    return diagnostic_op(diagnostic_severity::warning,
        "CIC001",
        std::string("CICS preprocessor - continuation ignored on ASM statement"),
        range);
}

diagnostic_op diagnostic_op::warn_CIC002(const range& range, std::string_view variable_name)
{
    return diagnostic_op(diagnostic_severity::warning,
        "CIC002",
        concat("CICS preprocessor - ", variable_name, " argument cannot be NULL"),
        range);
}

diagnostic_op diagnostic_op::warn_CIC003(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::warning, "CIC003", std::string("CICS preprocessor - missing CICS command"), range);
}

diagnostic_op diagnostic_op::error_END001(const range& range, std::string_view lib)
{
    return diagnostic_op(diagnostic_severity::error,
        "END001",
        std::string("ENDEVOR preprocessor - unable to find library '").append(lib).append("'"),
        range);
}

diagnostic_op diagnostic_op::error_END002(const range& range, std::string_view lib)
{
    return diagnostic_op(diagnostic_severity::error,
        "END002",
        std::string("ENDEVOR preprocessor - cycle detected while expanding library '").append(lib).append("'"),
        range);
}

diagnostic_op diagnostic_op::warn_U001_drop_had_no_effect(const range& range, std::string_view arg)
{
    return diagnostic_op(
        diagnostic_severity::warning, "U001", concat("USING - label '", arg, "' currently not used."), range);
}

diagnostic_op diagnostic_op::warn_U001_drop_had_no_effect(const range& range, int arg)
{
    return diagnostic_op(
        diagnostic_severity::warning, "U001", concat("USING - register ", arg, " currently not used."), range);
}

diagnostic_op diagnostic_op::error_U002_label_not_allowed(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "U002", "USING - label is not allowed in this context", range);
}

diagnostic_op diagnostic_op::error_U003_drop_label_or_reg(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "U003", "DROP - label or register expected", range);
}

diagnostic_op diagnostic_op::error_U004_no_active_using(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "U004", "No active USING found.", range);
}

diagnostic_op diagnostic_op::error_U005_invalid_range(
    const range& s_range, const range& e_range, std::string_view s_sect, int s_off, std::string_view e_sect, int e_off)
{
    using namespace std::string_view_literals;
    return diagnostic_op(diagnostic_severity::error,
        "U005",
        concat("USING - expression (",
            s_sect,
            s_sect.empty() ? ""sv : "+"sv,
            s_off,
            ",",
            e_sect,
            e_sect.empty() ? ""sv : "+"sv,
            e_off,
            ") is not a valid non-empty range."),
        union_range(s_range, e_range));
}

diagnostic_op diagnostic_op::error_U006_duplicate_base_specified(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "U006", "Base registers must be distinct.", range);
}

diagnostic_op diagnostic_op::mnote_diagnostic(unsigned level, std::string_view message, const range& range)
{
    const auto lvl = level >= 8 ? diagnostic_severity::error
        : level >= 4            ? diagnostic_severity::warning
        : level >= 2            ? diagnostic_severity::info
                                : diagnostic_severity::hint;
    const auto tag = level >= 2 ? diagnostic_tag::none : diagnostic_tag::unnecessary;
    return diagnostic_op(lvl, "MNOTE", std::string(message), range, tag);
}

diagnostic_op diagnostic_op::error_S0001(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "S0001", "C++ error", range);
}

diagnostic_op diagnostic_op::error_S0002(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "S0002", "Syntax error", range);
}

diagnostic_op diagnostic_op::error_S0003(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "S0003", "Unexpected end of statement", range);
}

diagnostic_op diagnostic_op::error_S0004(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "S0004", "Unfinished statement, the label cannot be alone on a line", range);
}

diagnostic_op diagnostic_op::error_S0005(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "S0005", "Expected an apostrophe", range);
}

diagnostic_op diagnostic_op::error_S0006(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "S0006", "Unexpected sign in an expression", range);
}

diagnostic_op diagnostic_op::error_S0007(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "S0007", "A sign needs to be preceded by an expression", range);
}

diagnostic_op diagnostic_op::error_S0008(const range& range)
{
    return diagnostic_op(
        diagnostic_severity::error, "S0008", "Ampersand has to be followed by a name of a variable", range);
}

diagnostic_op diagnostic_op::error_S0009(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "S0009", "A sign has to be followed by an expression", range);
}

diagnostic_op diagnostic_op::error_S0010(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "S0010", "Only left and right parenthesis present", range);
}

diagnostic_op diagnostic_op::error_S0011(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "S0011", "Left parenthesis has no right match", range);
}

diagnostic_op diagnostic_op::error_S0012(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "S0012", "Right parenthesis has no left match", range);
}

diagnostic_op diagnostic_op::error_S0013(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "S0013", "Invalid literal usage", range);
}

diagnostic_op diagnostic_op::error_S0014(const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "S0014", "Invalid attribute reference", range);
}

diagnostic_op diagnostic_op::error_S100(std::string_view message, const range& range)
{
    return diagnostic_op(diagnostic_severity::error, "S100", concat("Long ordinary symbol name - ", message), range);
}

std::string diagnostic_decorate_message(std::string_view field, std::string_view message)
{
    static const std::string_view prefix = "While evaluating the result of substitution '";
    static const std::string_view arrow = "' => ";
    std::string result;
    result.reserve(prefix.size() + field.size() + arrow.size() + message.size());

    result.append(prefix);
    utils::append_utf8_sanitized(result, field);
    result.append(arrow);
    result.append(message);

    return result;
}

} // namespace hlasm_plugin::parser_library
