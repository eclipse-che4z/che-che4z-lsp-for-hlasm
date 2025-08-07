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

#include "data_definition_common.h"
#include "expressions/data_definition.h"
#include "processing/instruction_sets/data_def_postponed_statement.h"
#include "semantics/operand.h"


using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library::checking;
using namespace hlasm_plugin::parser_library;


TEST(data_def_scale_attribute, P)
{
    data_def_type_P t;

    data_definition_operand op = setup_data_def_op('P', '\0', "456.1234,-12,4.587");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_scale_attribute(op.scale, reduce_nominal_value(op.nominal_value)), 4);
}

TEST(data_def_scale_attribute, P_no_integral)
{
    data_def_type_P t;

    data_definition_operand op = setup_data_def_op('P', '\0', ".1234");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_scale_attribute(op.scale, reduce_nominal_value(op.nominal_value)), 4);
}

TEST(data_def_scale_attribute, P_no_fraction)
{
    data_def_type_P t;

    data_definition_operand op = setup_data_def_op('P', '\0', "3.");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_scale_attribute(op.scale, reduce_nominal_value(op.nominal_value)), 0);
}

TEST(data_def_scale_attribute, P_simple_number)
{
    data_def_type_P t;

    data_definition_operand op = setup_data_def_op('P', '\0', "3");

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_scale_attribute(op.scale, reduce_nominal_value(op.nominal_value)), 0);
}

TEST(data_def_scale_attribute, H_explicit)
{
    data_def_type_H t;

    data_definition_operand op = setup_data_def_op('H', '\0', "3");
    op.scale.present = true;
    op.scale.value = 5;

    diag_collector col;
    EXPECT_TRUE(t.check(op, data_instr_type::DC, ADD_DIAG(col)));

    EXPECT_EQ(t.get_scale_attribute(op.scale, reduce_nominal_value(op.nominal_value)), 5);
}
