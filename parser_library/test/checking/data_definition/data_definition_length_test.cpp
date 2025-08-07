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

#include "../../common_testing.h"
#include "context/ordinary_assembly/ordinary_assembly_dependency_solver.h"
#include "data_definition_common.h"
#include "expressions/data_definition.h"
#include "library_info_transitional.h"
#include "processing/instruction_sets/data_def_postponed_statement.h"
#include "semantics/operand.h"
#include "semantics/operand_impls.h"

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library::checking;
using namespace hlasm_plugin::parser_library;

std::unique_ptr<semantics::operand> data_def_op_from_string(std::string input)
{
    analyzer a(input);
    return std::make_unique<semantics::data_def_operand_inline>(parse_data_definition(a), range());
}

TEST(data_def_operands_length, all_bit_len)
{
    semantics::operand_list list;
    list.push_back(data_def_op_from_string("BL.3'101'"));
    list.push_back(data_def_op_from_string("BL.6'101'"));

    context::hlasm_context ctx;
    context::ordinary_assembly_dependency_solver dep_solver(ctx.ord_ctx, library_info_transitional::empty);
    diagnostic_op_consumer_container diags;

    EXPECT_EQ(processing::data_def_dependency<data_instr_type::DC>::get_operands_length(
                  list.data(), list.data() + list.size(), dep_solver, diags),
        2);
}

TEST(data_def_operands_length, byte_in_middle_len)
{
    semantics::operand_list list;
    list.push_back(data_def_op_from_string("BL.3'101'"));
    list.push_back(data_def_op_from_string("A(1)"));
    list.push_back(data_def_op_from_string("BL.6'101'"));
    context::hlasm_context ctx;
    context::ordinary_assembly_dependency_solver dep_solver(ctx.ord_ctx, library_info_transitional::empty);
    diagnostic_op_consumer_container diags;

    EXPECT_EQ(processing::data_def_dependency<data_instr_type::DC>::get_operands_length(
                  list.data(), list.data() + list.size(), dep_solver, diags),
        9);
}

TEST(data_def_operands_length, explicit_byte)
{
    semantics::operand_list list;
    list.push_back(data_def_op_from_string("BL5'101'"));
    list.push_back(data_def_op_from_string("AL4(1)"));
    list.push_back(data_def_op_from_string("C'101'"));
    context::hlasm_context ctx;
    context::ordinary_assembly_dependency_solver dep_solver(ctx.ord_ctx, library_info_transitional::empty);
    diagnostic_op_consumer_container diags;

    EXPECT_EQ(processing::data_def_dependency<data_instr_type::DC>::get_operands_length(
                  list.data(), list.data() + list.size(), dep_solver, diags),
        12);
}



TEST(data_def_length, B_multiple)
{
    data_def_type_B t;

    data_definition_operand op = setup_data_def_op('B', '\0', "111010101,10100010,011");

    diag_collector col;
    ASSERT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), (size_t)(4 * 8));
}

TEST(data_def_length_attribute, B_multiple)
{
    data_def_type_B t;

    data_definition_operand op = setup_data_def_op('B', '\0', "111010101,10100010,011");

    diag_collector col;
    ASSERT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length_attribute(op.length, reduce_nominal_value(op.nominal_value)), 2U);
}

TEST(data_def_length_attribute, B_explicit)
{
    data_def_type_B t;

    data_definition_operand op = setup_data_def_op('B', '\0', "111010101,10100010,011", 5);

    diag_collector col;
    ASSERT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length_attribute(op.length, reduce_nominal_value(op.nominal_value)), 5U);
}

TEST(data_def_length, dupl_factor_bit_length)
{
    data_def_type_B t;

    data_definition_operand op = setup_data_def_op('B', '\0', "111010101,10100010,011", 4);
    op.length.len_type = data_def_length_t::BIT;
    op.dupl_factor = 3;
    diag_collector col;
    ASSERT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 12U * 3);
}

TEST(data_def_length, dupl_factor_implicit_length)
{
    data_def_type_A t;

    data_definition_operand op = setup_data_def_expr_op('A', '\0', 12, expr_type::ABS, -300, expr_type::ABS);
    op.dupl_factor = 5;
    diag_collector col;
    ASSERT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), (size_t)(8 * 5 * 8));
}

TEST(data_def_length, B_one_bit)
{
    data_def_type_B t;

    data_definition_operand op = setup_data_def_op('B', '\0', "1");

    diag_collector col;
    ASSERT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), (size_t)(1 * 8));
}

TEST(data_def_length, B_explicit_length)
{
    data_def_type_B t;

    data_definition_operand op = setup_data_def_op('B', '\0', "111010101,10100010,011", 4);

    diag_collector col;
    ASSERT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), (size_t)(12 * 8));
}

TEST(data_def_length, B_no_nominal)
{
    data_def_type_B t;

    data_definition_operand op = setup_data_def_op('B', '\0');

    diag_collector col;
    ASSERT_TRUE(t.check(op, data_instr_type::DS, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 8U);
}

TEST(data_def_length, CA)
{
    data_def_type_CA t;

    data_definition_operand op = setup_data_def_op('C', 'A', "ASCII");

    diag_collector col;

    ASSERT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 5U * 8);
}

TEST(data_def_length_attribute, CA)
{
    data_def_type_CA t;

    data_definition_operand op = setup_data_def_op('C', 'A', "ASCII");

    diag_collector col;

    ASSERT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length_attribute(op.length, reduce_nominal_value(op.nominal_value)), 5U);
}

TEST(data_def_length_attribute, CA_bit_length)
{
    data_def_type_CA t;

    data_definition_operand op = setup_data_def_op('C', 'A', "ASCII", 30);
    op.length.len_type = data_def_length_t::BIT;
    diag_collector col;

    ASSERT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length_attribute(op.length, reduce_nominal_value(op.nominal_value)), 4U);
}


TEST(data_def_length, CA_no_nominal)
{
    data_def_type_CA t;

    data_definition_operand op = setup_data_def_op('C', 'A');

    diag_collector col;
    ASSERT_TRUE(t.check(op, data_instr_type::DS, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 8U);
}

TEST(data_def_length, CA_explicit_length)
{
    data_def_type_CA t;

    data_definition_operand op = setup_data_def_op('C', 'A', "ASCII,ASCII", 4);

    diag_collector col;
    ASSERT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 4U * 8);
}

TEST(data_def_length, CE)
{
    data_def_type_CE t;

    data_definition_operand op = setup_data_def_op('C', 'E', "ebcdic");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 6U * 8);
}

TEST(data_def_length, CU)
{
    data_def_type_CU t;

    data_definition_operand op = setup_data_def_op('C', 'U', "utf16");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 5U * 2 * 8);
}

TEST(data_def_length_attribute, CU)
{
    data_def_type_CU t;

    data_definition_operand op = setup_data_def_op('C', 'U', "utf,16");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length_attribute(op.length, reduce_nominal_value(op.nominal_value)), 12U);
}

TEST(data_def_length, CU_no_nominal)
{
    data_def_type_CU t;

    data_definition_operand op = setup_data_def_op('C', 'U');

    diag_collector col;
    ASSERT_TRUE(t.check(op, data_instr_type::DS, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 16U);
}

TEST(data_def_length, CU_explicit_length)
{
    data_def_type_CU t;

    data_definition_operand op = setup_data_def_op('C', 'U', "utf,16", 4);

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 4U * 8);
}


TEST(data_def_length, G)
{
    data_def_type_G t;

    data_definition_operand op = setup_data_def_op('G', '\0', "<.A.B>");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 4U * 8);
}

TEST(data_def_length, G_explicit_length)
{
    data_def_type_G t;

    data_definition_operand op = setup_data_def_op('G', '\0', "<.A.,.B>", 4);

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 4U * 8);
}

TEST(data_def_length, G_no_nominal)
{
    data_def_type_G t;

    data_definition_operand op = setup_data_def_op('G', '\0');

    diag_collector col;
    ASSERT_TRUE(t.check(op, data_instr_type::DS, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 16U);
}

TEST(data_def_length, X_odd)
{
    data_def_type_X t;

    data_definition_operand op = setup_data_def_op('X', '\0', "ABC");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 2U * 8);
}

TEST(data_def_length_attribute, X_multiple)
{
    data_def_type_X t;

    data_definition_operand op = setup_data_def_op('X', '\0', "ABC,CD,1234");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length_attribute(op.length, reduce_nominal_value(op.nominal_value)), 2U);
}

TEST(data_def_length, X_even)
{
    data_def_type_X t;

    data_definition_operand op = setup_data_def_op('X', '\0', "ABCD");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 2U * 8);
}

TEST(data_def_length, X_multiple)
{
    data_def_type_X t;

    data_definition_operand op = setup_data_def_op('X', '\0', "ABCD,A,ABC,01235");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 8U * 8);
}

TEST(data_def_length, X_no_nominal)
{
    data_def_type_X t;

    data_definition_operand op = setup_data_def_op('X', '\0');

    diag_collector col;
    ASSERT_TRUE(t.check(op, data_instr_type::DS, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 8U);
}

TEST(data_def_length, F)
{
    data_def_type_F t;

    data_definition_operand op = setup_data_def_op('F', '\0', "1.23E3");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 4U * 8);
}

TEST(data_def_length_attribute, F)
{
    data_def_type_F t;

    data_definition_operand op = setup_data_def_op('F', '\0', "1.23E3,123");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length_attribute(op.length, reduce_nominal_value(op.nominal_value)), 4U);
}

TEST(data_def_length, F_no_nominal)
{
    data_def_type_F t;

    data_definition_operand op = setup_data_def_op('F', '\0');

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DS, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 4U * 8);
}

TEST(data_def_length, F_multiple)
{
    data_def_type_F t;

    data_definition_operand op = setup_data_def_op('F', '\0', "1.23E3,1,1.45");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 3U * 4 * 8);
}

TEST(data_def_length, FD_multiple)
{
    data_def_type_FD t;

    data_definition_operand op = setup_data_def_op('F', 'D', "1.23E3,1,1.45");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 3U * 8 * 8);
}

TEST(data_def_length, H_multiple)
{
    data_def_type_H t;

    data_definition_operand op = setup_data_def_op('H', '\0', "1.23E3,1,1.45");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 3U * 2 * 8);
}

TEST(data_def_length, P)
{
    data_def_type_P t;

    data_definition_operand op = setup_data_def_op('P', '\0', "456,-12,4.587");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), (size_t)(2 + 2 + 3) * 8);
}

TEST(data_def_length_attribute, P_multiple)
{
    data_def_type_P t;

    data_definition_operand op = setup_data_def_op('P', '\0', "456.174,-12,4.587");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length_attribute(op.length, reduce_nominal_value(op.nominal_value)), 4U);
}

TEST(data_def_length_attribute, P_simple)
{
    data_def_type_P t;

    data_definition_operand op = setup_data_def_op('P', '\0', "456");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length_attribute(op.length, reduce_nominal_value(op.nominal_value)), 2U);
}

TEST(data_def_length, Z)
{
    data_def_type_Z t;

    data_definition_operand op = setup_data_def_op('Z', '\0', "456,-12,4.587");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), (size_t)(3 + 2 + 4) * 8);
}

TEST(data_def_length_attribute, Z_multiple)
{
    data_def_type_Z t;

    data_definition_operand op = setup_data_def_op('Z', '\0', "45.12,-12,4.587");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length_attribute(op.length, reduce_nominal_value(op.nominal_value)), 4U);
}

TEST(data_def_length_attribute, Z_simple)
{
    data_def_type_Z t;

    data_definition_operand op = setup_data_def_op('Z', '\0', "45");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length_attribute(op.length, reduce_nominal_value(op.nominal_value)), 2U);
}

TEST(data_def_length, A)
{
    data_def_type_A t;
    data_definition_operand op =
        setup_data_def_expr_op('A', '\0', 0, expr_type::RELOC, 0, expr_type::RELOC, 0, expr_type::ABS);
    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));
    EXPECT_EQ(t.get_length(op), (size_t)(4 + 4 + 4) * 8);
}

TEST(data_def_length, A_no_nominal)
{
    data_def_type_A t;
    data_definition_operand op = setup_data_def_op('A', '\0');

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DS, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 4U * 8);
}

TEST(data_def_length, A_explicit_length)
{
    data_def_type_A t;
    data_definition_operand op =
        setup_data_def_expr_op_length('A', '\0', 3, 0, expr_type::RELOC, 0, expr_type::RELOC, 0, expr_type::ABS);
    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));
    EXPECT_EQ(t.get_length(op), (size_t)(3 + 3 + 3) * 8);
}

TEST(data_def_length, AD)
{
    data_def_type_AD t;
    data_definition_operand op =
        setup_data_def_expr_op('A', 'D', 0, expr_type::RELOC, 0, expr_type::RELOC, 0, expr_type::ABS);
    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));
    EXPECT_EQ(t.get_length(op), (size_t)(8 + 8 + 8) * 8);
}

TEST(data_def_length, Y)
{
    data_def_type_Y t;
    data_definition_operand op =
        setup_data_def_expr_op('Y', '\0', 0, expr_type::RELOC, 0, expr_type::RELOC, 0, expr_type::ABS);
    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));
    EXPECT_EQ(t.get_length(op), (size_t)(2 + 2 + 2) * 8);
}

TEST(data_def_length, R)
{
    data_def_type_R t;
    data_definition_operand op = setup_data_def_expr_op('R', '\0', 0, expr_type::RELOC, 0, expr_type::RELOC);

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), (size_t)(4 + 4) * 8);
}

TEST(data_def_length, RD)
{
    data_def_type_RD t;
    data_definition_operand op = setup_data_def_expr_op('R', 'D', 0, expr_type::RELOC, 0, expr_type::RELOC);

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), (size_t)(8 + 8) * 8);
}

TEST(data_def_length, RD_no_nominal)
{
    data_def_type_RD t;
    data_definition_operand op = setup_data_def_op('R', 'D');

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DS, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 8U * 8);
}

TEST(data_def_length, RD_explicit_length)
{
    data_def_type_RD t;
    data_definition_operand op = setup_data_def_expr_op_length('R', 'D', 4, 0, expr_type::RELOC, 0, expr_type::RELOC);

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), (size_t)(4 + 4) * 8);
}


TEST(data_def_length, S)
{
    data_def_type_S t;
    data_definition_operand op = setup_data_def_expr_op('S', '\0', 4095, expr_type::ABS);

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 2U * 8);
}

TEST(data_def_length, SY)
{
    data_def_type_SY t;
    data_definition_operand op = setup_data_def_expr_op('S', 'Y', 4096, expr_type::ABS);

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 3U * 8);
}

TEST(data_def_length, J)
{
    data_def_type_J t;
    data_definition_operand op = setup_data_def_expr_op('J', '\0', 0, expr_type::RELOC, 0, expr_type::RELOC);

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), (size_t)(4 + 4) * 8);
}


TEST(data_def_length, E)
{
    data_def_type_E t;

    data_definition_operand op = setup_data_def_op('E', '\0', "456E10,45");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), (size_t)(4 + 4) * 8);
}

TEST(data_def_length, ED)
{
    data_def_type_ED t;

    data_definition_operand op = setup_data_def_op('E', 'D', "456E7R8");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 4U * 8);
}

TEST(data_def_length, D)
{
    data_def_type_D t;

    data_definition_operand op = setup_data_def_op('D', '\0', "456E7");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 8U * 8);
}

TEST(data_def_length, DD)
{
    data_def_type_DD t;

    data_definition_operand op = setup_data_def_op('D', 'D', "456,14");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), (size_t)(8 + 8) * 8);
}

TEST(data_def_length, L)
{
    data_def_type_L t;

    data_definition_operand op = setup_data_def_op('L', '\0', "+4E7");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 16U * 8);
}

TEST(data_def_length, LB)
{
    data_def_type_LB t;

    data_definition_operand op = setup_data_def_op('L', 'B', "-13.14E7");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 16U * 8);
}

TEST(data_def_length, LQ)
{
    data_def_type_LQ t;

    data_definition_operand op = setup_data_def_op('L', 'Q', "-1E7");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_length(op), 16U * 8);
}
