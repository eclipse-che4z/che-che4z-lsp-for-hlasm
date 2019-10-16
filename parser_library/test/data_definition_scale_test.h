#include "data_definition_common.h"
#include "../src/processing/instruction_sets/data_def_postponed_statement.h"
#include "../src/expressions/data_definition.h"
#include "../src/semantics/operand.h"


using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library::checking;
using namespace hlasm_plugin::parser_library;


TEST(data_def_scale_attribute, P)
{
	data_def_type_P t;

	data_definition_operand op = setup_data_def_op('P', '\0', "456.1234,-12,4.587");

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	EXPECT_EQ(t.get_scale_attribute(op.scale, op.nominal_value), 4);
}

TEST(data_def_scale_attribute, P_no_integral)
{
	data_def_type_P t;

	data_definition_operand op = setup_data_def_op('P', '\0', ".1234");

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	EXPECT_EQ(t.get_scale_attribute(op.scale, op.nominal_value), 4);
}

TEST(data_def_scale_attribute, P_no_fraction)
{
	data_def_type_P t;

	data_definition_operand op = setup_data_def_op('P', '\0', "3.");

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	EXPECT_EQ(t.get_scale_attribute(op.scale, op.nominal_value), 0);
}

TEST(data_def_scale_attribute, P_simple_number)
{
	data_def_type_P t;

	data_definition_operand op = setup_data_def_op('P', '\0', "3");

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	EXPECT_EQ(t.get_scale_attribute(op.scale, op.nominal_value), 0);
}

TEST(data_def_scale_attribute, H_explicit)
{
	data_def_type_H t;

	data_definition_operand op = setup_data_def_op('H', '\0', "3");
	op.scale.present = true;
	op.scale.value = 5;

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	EXPECT_EQ(t.get_scale_attribute(op.scale, op.nominal_value), 5);
}

