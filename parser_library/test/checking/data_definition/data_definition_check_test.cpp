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

#include <span>

#include "../../common_testing.h"
#include "checking/data_check.h"
#include "checking/diagnostic_collector.h"
#include "context/hlasm_context.h"
#include "context/ordinary_assembly/ordinary_assembly_dependency_solver.h"
#include "diagnosable_ctx.h"
#include "instructions/instruction.h"
#include "library_info_transitional.h"
#include "semantics/operand_impls.h"

using namespace hlasm_plugin::parser_library;

TEST(data_def_checker, unknown_type)
{
    std::string input = R"( DS W)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D012" }));
}

// TODO: This case cannot happen anymore because the parser reports syntax error
//       Maybe the D013 should be reported from the parser directly
TEST(data_def_checker, unknown_extension)
{
    const auto& dc = instructions::get_assembler_instructions("DC");

    context::hlasm_context ctx;
    diagnosable_ctx diag_ctx(ctx);

    data_definition dd;
    dd.type = 'B';
    dd.extension = 'A';
    dd.nominal_value = std::make_unique<nominal_value_string>("", range());
    const std::unique_ptr<semantics::operand> ops[] = {
        std::make_unique<data_def_operand_inline>(std::move(dd), range()),
    };
    ordinary_assembly_dependency_solver solver(ctx.ord_ctx, library_info_transitional::empty);

    diagnostic_collector add_diagnostic(&diag_ctx);
    checking::check_data_instruction_operands(dc, std::span(ops), {}, solver, add_diagnostic);
    EXPECT_TRUE(matches_message_codes(diag_ctx.diags(), { "D013" }));
}

TEST(data_def_checker, operands_too_long)
{
    std::string input = R"( DS 1073741824CL1,1073741824CL1)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D029" }));
}

// TODO: See unknown_extension
TEST(data_def_checker, unknown_extension_analyzer)
{
    std::string input = " DC BA'1'";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002", "D016" }));
}

TEST(data_def_checker, unexpected_expr)
{
    std::string input = " DC B(0,0)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D018" }));
}

TEST(data_def_checker, unexpected_string)
{
    std::string input = R"( DC A'sth')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D017" }));
}

TEST(data_def_checker, unexpected_address)
{
    std::string input = R"( DC A(0,14(2)))";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D020" }));
}

TEST(data_def_checker, DC_nominal_expected)
{
    std::string input = R"( DC A)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D016" }));
}

TEST(data_def_checker, DS_no_nominal_ok)
{
    std::string input = R"( DS A)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, nominal_checked_when_dupl_zero)
{
    std::string input = R"( DC 0S(-1))";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D022" }));
}

TEST(data_def_checker, operand_too_long)
{
    std::string input = R"( DC 268435456CL256'char')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D028" }));
}

TEST(data_def_checker, B_correct)
{
    std::string input = R"( DC B'101011010')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, B_multiple_correct)
{
    std::string input = R"( DC B'11100101,10100010,011')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, B_multiple_wrong_end)
{
    std::string input = R"( DC B'11100101,10100010,011,')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
}

TEST(data_def_checker, DS_nominal_is_checked)
{
    std::string input = R"( DC B'12')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
}

TEST(data_def_checker, B_multiple_wrong_middle)
{
    std::string input = R"( DC B'11100101,,10100010,011')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
}

TEST(data_def_checker, B_multiple_wrong_beginning)
{
    std::string input = R"( DC B',011')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
}

TEST(data_def_checker, B_wrong)
{
    std::string input = R"( DC B'10101101023')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
}

TEST(data_def_checker, C_DC_length_too_big)
{
    std::string input = R"( DC CL30000'ASCII')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D008" }));
}

TEST(data_def_checker, C_DS_length)
{
    std::string input = R"( DS CL65535'ASCII')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, C_DS_length_too_big)
{
    std::string input = R"( DC CL65536'ASCII')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D008" }));
}

TEST(data_def_checker, CA_correct)
{
    std::string input = R"( DC C'ASCII')";
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, CE_correct)
{
    std::string input = R"( DC CE'ebcdic')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, CU_correct)
{
    std::string input = R"( DC CU'utf16')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, CU_not_even)
{
    std::string input = R"( DC CUL35'utf16')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D014" }));
}

TEST(data_def_checker, CU_bit_length_not_allowed)
{
    std::string input = R"( DC CUL.480'utf16')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D007" }));
}

TEST(data_def_checker, G_not_even)
{
    std::string input = R"( DC GL35'utf16')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D014" }));
}

TEST(data_def_checker, X_correct)
{
    std::string input = R"( DC XL35' ABCDEFabc def1 234567890 ')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, X_multiple)
{
    std::string input = R"( DC XL35'ABCDEF,abcdef12345,67890')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, X_hexa_digit_expected)
{
    std::string input = R"( DC X'G')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
}


TEST(data_def_checker, F_correct)
{
    std::string input = R"( DC F'1.23E3')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, FD_correct)
{
    std::string input = R"( DC FD'1.23E3')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, FD_correct_case_insensitive)
{
    std::string input = R"( DC FD'u1.23e3')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, H_correct)
{
    std::string input = R"( DC H'U12345')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, H_correct_no_fractional)
{
    std::string input = R"( DC H'3.')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, H_sign_after_dot)
{
    std::string input = R"( DC H'3.+0')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
}

TEST(data_def_checker, H_two_signs)
{
    std::string input = R"( DC H'U-12345')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
}

TEST(data_def_checker, H_no_exponent)
{
    std::string input = R"( DC H'U12345E')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
}

TEST(data_def_checker, H_no_fraction)
{
    std::string input = R"( DC H'U123123.')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, H_no_integral)
{
    std::string input = R"( DC H'.3,+.3,-.3,U.3')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}


TEST(data_def_checker, P_correct)
{
    std::string input = R"( DC P'456')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, Z_correct)
{
    std::string input = R"( DC Z'+456,-01453,-12.13,+10.19')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, Z_two_signs)
{
    std::string input = R"( DC Z'+-456')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
}

TEST(data_def_checker, Z_two_dots)
{
    std::string input = R"( DC Z'456..4')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
}

TEST(data_def_checker, Z_two_commas)
{
    std::string input = R"( DC Z'456,,45')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
}

TEST(data_def_checker, Z_plus_only)
{
    std::string input = R"( DC Z'+,45')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
}

TEST(data_def_checker, Z_plus_end)
{
    std::string input = R"( DC Z'45+,45')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
}

TEST(data_def_checker, Z_comma_only)
{
    std::string input = R"( DC Z',')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
}

TEST(data_def_checker, A_sym_bit_length)
{
    std::string input = R"(A DC AL.10(A,A+A))";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D007" }));
}

TEST(data_def_checker, A_sym_length_too_big)
{
    std::string input = R"(A DC AL8(A,0))";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D008" }));
}

TEST(data_def_checker, A_abs_length_ok)
{
    std::string input = R"( DC AL8(0,0))";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, RD_wrong_length)
{
    std::string input = R"(A DC RDL6(A,A))";
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D021" }));
}

TEST(data_def_checker, S_expr_out_of_range)
{
    std::string input = R"( DC S(4097))";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D022" }));
}

TEST(data_def_checker, S_expr_out_of_range_negative)
{
    std::string input = R"( DC S(-10))";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D022" }));
}

TEST(data_def_checker, S_displacement_out_of_range)
{
    std::string input = R"( DC S(-10(3)))";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D022" }));
}

TEST(data_def_checker, S_base_out_of_range)
{
    std::string input = R"( DC S(30(30)))";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D023" }));
}

TEST(data_def_checker, SY_negative_displacement_OK)
{
    std::string input = R"( DC SY(-10(3)))";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, J_wrong_length)
{
    std::string input = R"(
A   DXD F
    DC JL6(A))";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D024" }));
}

TEST(data_def_checker, J_length_ok)
{
    std::string input = R"(
A   DXD F
    DC JL2(A))";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, V_ok)
{
    std::string input = " DC V(SYMBOL)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, V_single_symbol_expected)
{
    std::string input = " DC V(SYMBOL+5)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D030" }));

    std::string input2 = " DC V(12(13))";
    analyzer a2(input);
    a2.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D030" }));
}

TEST(data_def_checker, L_two_commas)
{
    std::string input = R"( DC L'456,,45')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
}

TEST(data_def_checker, L_correct)
{
    std::string input = R"( DC L'456E10,5')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, L_correct_space_at_end)
{
    std::string input = R"( DC L'456E10,5 ')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, L_invalid_round_mode)
{
    std::string input = R"( DC L'456E10R5')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D026" }));
}

TEST(data_def_checker, LB_correct_case_insensitive)
{
    std::string input = R"( DC LB'456e10r5')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(data_def_checker, LD_invalid_round_mode)
{
    std::string input = R"( DC LD'456E7R5')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D026" }));
}

TEST(data_def_checker, LD_no_round_mode)
{
    std::string input = R"( DC LD'456E7R')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D026" }));
}

TEST(data_def_checker, LD_no_exponent)
{
    std::string input = R"( DC LD'456ER10')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
}

TEST(data_def_checker, LD_scale_ignored)
{
    std::string input = R"( DC LDS0'0')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D025" }));
}

TEST(data_def_checker, LD_scale_wrong)
{
    std::string input = R"( DC LDS1'0')";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D009" }));
}

TEST(data_def_checker, special_values_BD)
{
    const std::string_view inputs[] = {
        " DC LD'(SNAN)'",
        " DC ED'+(SNAN)'",
        " DC DD'-(SNAN)'",
        " DC LB'(QNAN)'",
        " DC EB'+(QNAN)'",
        " DC DB'-(QNAN)'",
        " DC LD'(nan)'",
        " DC ED'+(inf)'",
        " DC DD'-(max)'",
        " DC LB'(min)'",
        " DC EB'+(dmin)'",
        " DC DB'-(qnan)'",
    };
    for (auto input : inputs)
    {
        analyzer a(input);
        a.analyze();

        EXPECT_TRUE(a.diags().empty());
    }
}

TEST(data_def_checker, special_values_BD_space_after)
{
    const std::string_view inputs[] = {
        " DC LD'(SNAN) '",
        " DC ED'+(SNAN) '",
        " DC DD'-(SNAN) '",
    };
    for (auto input : inputs)
    {
        analyzer a(input);
        a.analyze();

        EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
    }
}

TEST(data_def_checker, special_values_BD_incomplete)
{
    const std::string_view inputs[] = {
        " DC LB'(QNAN),'",
        " DC EB'+(QNAN),'",
        " DC DB'-(QNAN),'",
    };
    for (auto input : inputs)
    {
        analyzer a(input);
        a.analyze();

        EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
    }
}

TEST(data_def_checker, special_values_BD_wrong)
{
    const std::string_view inputs[] = {
        " DC LB'( QNAN)'",
        " DC EB'+(QN AN)'",
        " DC DB'-(QNAN )'",
        " DC DB'-(SOMETHING)'",
        " DC DB' '",
        " DC DB'('",
        " DC DB')'",
        " DC DB'()'",
    };
    for (auto input : inputs)
    {
        analyzer a(input);
        a.analyze();

        EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
    }
}

TEST(data_def_checker, special_values_no_extension)
{
    const std::string_view inputs[] = {
        " DC L'(MAX)'",
        " DC E'+(MAX)'",
        " DC D'-(MAX)'",
    };
    for (auto input : inputs)
    {
        analyzer a(input);
        a.analyze();

        EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
    }
}

TEST(data_def_checker, special_values_H)
{
    const std::string_view inputs[] = {
        " DC LH'(MAX)'",
        " DC EH'+(MAX)'",
        " DC DH'-(MAX)'",
        " DC LQ'(MIN)'",
        " DC LQ'+(MIN)'",
        " DC LQ'-(MIN)'",
        " DC LQ'(DMIN)'",
        " DC LQ'+(DMIN)'",
        " DC LQ'-(DMIN)'",
    };
    for (auto input : inputs)
    {
        analyzer a(input);
        a.analyze();

        EXPECT_TRUE(a.diags().empty());
    }
}

TEST(data_def_checker, special_values_H_space_after)
{
    const std::string_view inputs[] = {
        " DC LH'(MAX) '",
        " DC EH'+(MAX) '",
        " DC DH'-(MAX) '",
    };
    for (auto input : inputs)
    {
        analyzer a(input);
        a.analyze();

        EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
    }
}

TEST(data_def_checker, special_values_H_incomplete)
{
    const std::string_view inputs[] = {
        " DC LH'(MAX),'",
        " DC EH'+(MAX),'",
        " DC DH'-(MAX),'",
    };
    for (auto input : inputs)
    {
        analyzer a(input);
        a.analyze();

        EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
    }
}

TEST(data_def_checker, special_values_H_wrong)
{
    const std::string_view inputs[] = {
        " DC LH'( DMIN)'",
        " DC EH'+(DM IN)'",
        " DC DH'-(DMIN )'",
        " DC LH'(SNAN)'",
        " DC EH'+(QNAN)'",
        " DC DH'-(INF)'",
        " DC LH'()'",
        " DC EH'('",
        " DC DH')'",
        " DC DH' '",
    };
    for (auto input : inputs)
    {
        analyzer a(input);
        a.analyze();

        EXPECT_TRUE(matches_message_codes(a.diags(), { "D010" }));
    }
}
