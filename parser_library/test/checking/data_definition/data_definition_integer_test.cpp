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

#include "context/ordinary_assembly/symbol_attributes.h"
#include "data_definition_common.h"
#include "expressions/data_definition.h"

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library::checking;
using namespace hlasm_plugin::parser_library;

namespace {
auto compute_integer(const data_def_type& t,
    const data_def_length_t& length,
    const scale_modifier_t& scale,
    const reduced_nominal_value_t& nominal)
{
    const auto l = t.get_length_attribute(length, nominal);
    const auto s = t.get_scale_attribute(scale, nominal);

    return context::compute_integer_attribute(t.get_int_type(), l, s);
}
} // namespace

TEST(data_def_integer_attribute, H)
{
    data_def_type_H t;

    data_definition_operand op = setup_data_def_op('H', '\0', "-25.93");
    op.scale = 6;

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));
    EXPECT_EQ(compute_integer(t, op.length, op.scale, reduce_nominal_value(op.nominal_value)), 9);
}

TEST(data_def_integer_attribute, F)
{
    data_def_type_F t;

    data_definition_operand op = setup_data_def_op('F', '\0', "100.3E-2");
    op.scale = 8;

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));
    EXPECT_EQ(compute_integer(t, op.length, op.scale, reduce_nominal_value(op.nominal_value)), 23);
}

TEST(data_def_integer_attribute, E)
{
    data_def_type_E t;

    data_definition_operand op = setup_data_def_op('E', '\0', "46.415");
    op.scale = 2;

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));
    EXPECT_EQ(compute_integer(t, op.length, op.scale, reduce_nominal_value(op.nominal_value)), 4);
}

TEST(data_def_integer_attribute, D)
{
    data_def_type_D t;

    data_definition_operand op = setup_data_def_op('D', '\0', "-3.729");
    op.scale = 5;

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));
    EXPECT_EQ(compute_integer(t, op.length, op.scale, reduce_nominal_value(op.nominal_value)), 9);
}

TEST(data_def_integer_attribute, L)
{
    data_def_type_L t;

    data_definition_operand op = setup_data_def_op('L', '\0', "5.312");
    op.scale = 10;

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));
    EXPECT_EQ(compute_integer(t, op.length, op.scale, reduce_nominal_value(op.nominal_value)), 18);
}

TEST(data_def_integer_attribute, P)
{
    data_def_type_P t;

    data_definition_operand op = setup_data_def_op('P', '\0', "+3.513");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));
    EXPECT_EQ(compute_integer(t, op.length, op.scale, reduce_nominal_value(op.nominal_value)), 2);
}

TEST(data_def_integer_attribute, Z)
{
    data_def_type_Z t;

    data_definition_operand op = setup_data_def_op('Z', '\0', "3.513");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));
    EXPECT_EQ(compute_integer(t, op.length, op.scale, reduce_nominal_value(op.nominal_value)), 1);
}

TEST(data_def_integer_attribute, LD)
{
    data_def_type_LD t;

    data_definition_operand op = setup_data_def_op('L', 'D', "3.513");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));
    EXPECT_EQ(compute_integer(t, op.length, op.scale, reduce_nominal_value(op.nominal_value)), 28);
}

TEST(data_def_integer_attribute, LB)
{
    data_def_type_LB t;

    data_definition_operand op = setup_data_def_op('L', 'B', "3.513");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));
    EXPECT_EQ(compute_integer(t, op.length, op.scale, reduce_nominal_value(op.nominal_value)), 28);
}
