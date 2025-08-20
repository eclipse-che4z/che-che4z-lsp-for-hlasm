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
#include "checking/data_definition/data_def_type_base.h"
#include "context/hlasm_context.h"
#include "context/ordinary_assembly/ordinary_assembly_dependency_solver.h"
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
    const auto* t = data_def_type::access_data_def_type('B', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, "111010101,10100010,011"), (size_t)(4 * 8));
}

TEST(data_def_length_attribute, B_multiple)
{
    const auto* t = data_def_type::access_data_def_type('B', 0);

    EXPECT_EQ(t->get_length_attribute({}, "111010101,10100010,011"), 2U);
}

TEST(data_def_length_attribute, B_explicit)
{
    const auto* t = data_def_type::access_data_def_type('B', 0);

    EXPECT_EQ(t->get_length_attribute(data_def_length_t(5), "111010101,10100010,011"), 5U);
}

TEST(data_def_length, dupl_factor_bit_length)
{
    const auto* t = data_def_type::access_data_def_type('B', 0);

    EXPECT_EQ(t->get_length(3, 4, true, "111010101,10100010,011"), 12U * 3);
}

TEST(data_def_length, dupl_factor_implicit_length)
{
    const auto* t = data_def_type::access_data_def_type('A', 0);

    EXPECT_EQ(t->get_length(5, -1, false, (size_t)2), (size_t)(8 * 5 * 8));
}

TEST(data_def_length, B_one_bit)
{
    const auto* t = data_def_type::access_data_def_type('B', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, "1"), (size_t)(1 * 8));
}

TEST(data_def_length, B_explicit_length)
{
    const auto* t = data_def_type::access_data_def_type('B', 0);

    EXPECT_EQ(t->get_length(-1, 4, false, "111010101,10100010,011"), (size_t)(12 * 8));
}

TEST(data_def_length, B_no_nominal)
{
    const auto* t = data_def_type::access_data_def_type('B', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, {}), 8U);
}

TEST(data_def_length, CA)
{
    const auto* t = data_def_type::access_data_def_type('C', 'A');

    EXPECT_EQ(t->get_length(-1, -1, false, "ASCII"), 5U * 8);
}

TEST(data_def_length_attribute, CA)
{
    const auto* t = data_def_type::access_data_def_type('C', 'A');

    EXPECT_EQ(t->get_length_attribute({}, "ASCII"), 5U);
}

TEST(data_def_length_attribute, CA_bit_length)
{
    const auto* t = data_def_type::access_data_def_type('C', 'A');

    data_def_length_t len(30);
    len.len_type = data_def_length_t::BIT;

    EXPECT_EQ(t->get_length_attribute(len, "ASCII"), 4U);
}


TEST(data_def_length, CA_no_nominal)
{
    const auto* t = data_def_type::access_data_def_type('C', 'A');

    EXPECT_EQ(t->get_length(-1, -1, false, {}), 8U);
}

TEST(data_def_length, CA_explicit_length)
{
    const auto* t = data_def_type::access_data_def_type('C', 'A');

    EXPECT_EQ(t->get_length(-1, 4, false, "ASCII,ASCII"), 4U * 8);
}

TEST(data_def_length, CE)
{
    const auto* t = data_def_type::access_data_def_type('C', 'E');

    EXPECT_EQ(t->get_length(-1, -1, false, "ebcdic"), 6U * 8);
}

TEST(data_def_length, CU)
{
    const auto* t = data_def_type::access_data_def_type('C', 'U');

    EXPECT_EQ(t->get_length(-1, -1, false, "utf16"), 5U * 2 * 8);
}

TEST(data_def_length_attribute, CU)
{
    const auto* t = data_def_type::access_data_def_type('C', 'U');

    EXPECT_EQ(t->get_length_attribute({}, "utf,16"), 12U);
}

TEST(data_def_length, CU_no_nominal)
{
    const auto* t = data_def_type::access_data_def_type('C', 'U');

    EXPECT_EQ(t->get_length(-1, -1, false, {}), 16U);
}

TEST(data_def_length, CU_explicit_length)
{
    const auto* t = data_def_type::access_data_def_type('C', 'U');

    EXPECT_EQ(t->get_length(-1, 4, false, "utf,16"), 4U * 8);
}


TEST(data_def_length, G)
{
    const auto* t = data_def_type::access_data_def_type('G', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, "<.A.B>"), 4U * 8);
}

TEST(data_def_length, G_explicit_length)
{
    const auto* t = data_def_type::access_data_def_type('G', 0);

    EXPECT_EQ(t->get_length(-1, 4, false, "<.A.,.B>"), 4U * 8);
}

TEST(data_def_length, G_no_nominal)
{
    const auto* t = data_def_type::access_data_def_type('G', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, {}), 16U);
}

TEST(data_def_length, X_odd)
{
    const auto* t = data_def_type::access_data_def_type('X', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, "ABC"), 2U * 8);
}

TEST(data_def_length_attribute, X_multiple)
{
    const auto* t = data_def_type::access_data_def_type('X', 0);

    EXPECT_EQ(t->get_length_attribute({}, "ABC,CD,1234"), 2U);
}

TEST(data_def_length, X_even)
{
    const auto* t = data_def_type::access_data_def_type('X', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, "ABCD"), 2U * 8);
}

TEST(data_def_length, X_multiple)
{
    const auto* t = data_def_type::access_data_def_type('X', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, "ABCD,A,ABC,01235"), 8U * 8);
}

TEST(data_def_length, X_no_nominal)
{
    const auto* t = data_def_type::access_data_def_type('X', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, {}), 8U);
}

TEST(data_def_length, F)
{
    const auto* t = data_def_type::access_data_def_type('F', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, "1.23E3"), 4U * 8);
}

TEST(data_def_length_attribute, F)
{
    const auto* t = data_def_type::access_data_def_type('F', 0);

    EXPECT_EQ(t->get_length_attribute({}, "1.23E3,123"), 4U);
}

TEST(data_def_length, F_no_nominal)
{
    const auto* t = data_def_type::access_data_def_type('F', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, {}), 4U * 8);
}

TEST(data_def_length, F_multiple)
{
    const auto* t = data_def_type::access_data_def_type('F', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, "1.23E3,1,1.45"), 3U * 4 * 8);
}

TEST(data_def_length, FD_multiple)
{
    const auto* t = data_def_type::access_data_def_type('F', 'D');

    EXPECT_EQ(t->get_length(-1, -1, false, "1.23E3,1,1.45"), 3U * 8 * 8);
}

TEST(data_def_length, H_multiple)
{
    const auto* t = data_def_type::access_data_def_type('H', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, "1.23E3,1,1.45"), 3U * 2 * 8);
}

TEST(data_def_length, P)
{
    const auto* t = data_def_type::access_data_def_type('P', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, "456,-12,4.587"), (size_t)(2 + 2 + 3) * 8);
}

TEST(data_def_length_attribute, P_multiple)
{
    const auto* t = data_def_type::access_data_def_type('P', 0);

    EXPECT_EQ(t->get_length_attribute({}, "456.174,-12,4.587"), 4U);
}

TEST(data_def_length_attribute, P_simple)
{
    const auto* t = data_def_type::access_data_def_type('P', 0);

    EXPECT_EQ(t->get_length_attribute({}, "456"), 2U);
}

TEST(data_def_length, Z)
{
    const auto* t = data_def_type::access_data_def_type('Z', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, "456,-12,4.587"), (size_t)(3 + 2 + 4) * 8);
}

TEST(data_def_length_attribute, Z_multiple)
{
    const auto* t = data_def_type::access_data_def_type('Z', 0);

    EXPECT_EQ(t->get_length_attribute({}, "45.12,-12,4.587"), 4U);
}

TEST(data_def_length_attribute, Z_simple)
{
    const auto* t = data_def_type::access_data_def_type('Z', 0);

    EXPECT_EQ(t->get_length_attribute({}, "45"), 2U);
}

TEST(data_def_length, A)
{
    const auto* t = data_def_type::access_data_def_type('A', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, (size_t)3), (size_t)(4 + 4 + 4) * 8);
}

TEST(data_def_length, A_no_nominal)
{
    const auto* t = data_def_type::access_data_def_type('A', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, {}), 4U * 8);
}

TEST(data_def_length, A_explicit_length)
{
    const auto* t = data_def_type::access_data_def_type('A', 0);

    EXPECT_EQ(t->get_length(-1, 3, false, (size_t)3), (size_t)(3 + 3 + 3) * 8);
}

TEST(data_def_length, AD)
{
    const auto* t = data_def_type::access_data_def_type('A', 'D');

    EXPECT_EQ(t->get_length(-1, -1, false, (size_t)3), (size_t)(8 + 8 + 8) * 8);
}

TEST(data_def_length, Y)
{
    const auto* t = data_def_type::access_data_def_type('Y', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, (size_t)3), (size_t)(2 + 2 + 2) * 8);
}

TEST(data_def_length, R)
{
    const auto* t = data_def_type::access_data_def_type('R', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, (size_t)2), (size_t)(4 + 4) * 8);
}

TEST(data_def_length, RD)
{
    const auto* t = data_def_type::access_data_def_type('R', 'D');

    EXPECT_EQ(t->get_length(-1, -1, false, (size_t)2), (size_t)(8 + 8) * 8);
}

TEST(data_def_length, RD_no_nominal)
{
    const auto* t = data_def_type::access_data_def_type('R', 'D');

    EXPECT_EQ(t->get_length(-1, -1, false, {}), 8U * 8);
}

TEST(data_def_length, RD_explicit_length)
{
    const auto* t = data_def_type::access_data_def_type('R', 'D');

    EXPECT_EQ(t->get_length(-1, 4, false, (size_t)2), (size_t)(4 + 4) * 8);
}


TEST(data_def_length, S)
{
    const auto* t = data_def_type::access_data_def_type('S', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, (size_t)1), 2U * 8);
}

TEST(data_def_length, SY)
{
    const auto* t = data_def_type::access_data_def_type('S', 'Y');

    EXPECT_EQ(t->get_length(-1, -1, false, (size_t)1), 3U * 8);
}

TEST(data_def_length, J)
{
    const auto* t = data_def_type::access_data_def_type('J', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, (size_t)2), (size_t)(4 + 4) * 8);
}


TEST(data_def_length, E)
{
    const auto* t = data_def_type::access_data_def_type('E', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, "456E10,45"), (size_t)(4 + 4) * 8);
}

TEST(data_def_length, ED)
{
    const auto* t = data_def_type::access_data_def_type('E', 'D');

    EXPECT_EQ(t->get_length(-1, -1, false, "456E7R8"), 4U * 8);
}

TEST(data_def_length, D)
{
    const auto* t = data_def_type::access_data_def_type('D', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, "456E7"), 8U * 8);
}

TEST(data_def_length, DD)
{
    const auto* t = data_def_type::access_data_def_type('D', 'D');

    EXPECT_EQ(t->get_length(-1, -1, false, "456,14"), (size_t)(8 + 8) * 8);
}

TEST(data_def_length, L)
{
    const auto* t = data_def_type::access_data_def_type('L', 0);

    EXPECT_EQ(t->get_length(-1, -1, false, "+4E7"), 16U * 8);
}

TEST(data_def_length, LB)
{
    const auto* t = data_def_type::access_data_def_type('L', 'B');

    EXPECT_EQ(t->get_length(-1, -1, false, "-13.14E7"), 16U * 8);
}

TEST(data_def_length, LQ)
{
    const auto* t = data_def_type::access_data_def_type('L', 'Q');

    EXPECT_EQ(t->get_length(-1, -1, false, "-1E7"), 16U * 8);
}
