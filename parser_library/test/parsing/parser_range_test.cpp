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

// Not available until parser can be tested for ranges.
#if false

#    include "gtest/gtest.h"

#    include "common_testing.h"
#    include "parser_impl.h"

//test for correctness of parsed operand and remark ranges

TEST(parser_get_op_rem, one_op)
{
	parser_holder h("   LR  1");
	h.parser->ordinary_instruction_statement();

	auto& op_rem = h.parser->analyzer.current_operands_and_remarks();

	ASSERT_EQ(op_rem.operands.size(), (size_t)1);
	ASSERT_EQ(op_rem.remarks.size(), (size_t)0);

	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 7, 0, 8), op_rem.operands[0]->range);

}

TEST(parser_get_op_rem, one_op_one_rem)
{
	parser_holder h("   LR  1  rem");
	h.parser->ordinary_instruction_statement();

	auto& op_rem = h.parser->analyzer.current_operands_and_remarks();

	ASSERT_EQ(op_rem.operands.size(), (size_t)1);
	ASSERT_EQ(op_rem.remarks.size(), (size_t)1);

	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 7, 0, 8), op_rem.operands[0]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 10, 0, 13), op_rem.remarks[0]);

}

TEST(parser_get_op_rem, more_ops_one_rem)
{
	parser_holder h("   LR  1,2,3  rem");
	h.parser->ordinary_instruction_statement();

	auto& op_rem = h.parser->analyzer.current_operands_and_remarks();

	ASSERT_EQ(op_rem.operands.size(), (size_t)3);
	ASSERT_EQ(op_rem.remarks.size(), (size_t)1);

	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 7, 0, 8), op_rem.operands[0]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 9, 0, 10), op_rem.operands[1]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 11, 0, 12), op_rem.operands[2]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 14, 0, 17), op_rem.remarks[0]);

}

TEST(parser_get_op_rem, no_op)
{
	parser_holder h("   COM  1,2,3  rem");
	h.parser->ordinary_instruction_statement();

	auto& op_rem = h.parser->analyzer.current_operands_and_remarks();

	ASSERT_EQ(op_rem.operands.size(), (size_t)0);
	ASSERT_EQ(op_rem.remarks.size(), (size_t)1);

	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 8, 0, 18), op_rem.remarks[0]);

}

TEST(parser_get_op_rem, alt_format_allowed)
{
	parser_holder h(get_content("test/library/input/op_alt_format_allowed.in"));
	h.parser->ordinary_instruction_statement();

	auto& op_rem = h.parser->analyzer.current_operands_and_remarks();

	ASSERT_EQ(op_rem.operands.size(), (size_t)2);
	ASSERT_EQ(op_rem.remarks.size(), (size_t)2);

	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 15, 0, 23), op_rem.operands[0]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(1, 15, 1, 23), op_rem.operands[1]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 43, 0, 71), op_rem.remarks[0]);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(1, 44, 1, 67), op_rem.remarks[1]);

}

TEST(parser_get_op_rem, alt_format_not_allowed)
{
	parser_holder h(get_content("test/library/input/op_alt_format_not_allowed.in"));
	h.parser->ordinary_instruction_statement();

	auto& op_rem = h.parser->analyzer.current_operands_and_remarks();

	ASSERT_EQ(op_rem.operands.size(), (size_t)2);
	ASSERT_EQ(op_rem.remarks.size(), (size_t)1);

	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 15, 0, 23), op_rem.operands[0]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 24, 0, 24), op_rem.operands[1]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 43, 1, 67), op_rem.remarks[0]);

}

TEST(parser_get_op_rem, cont_no_op)
{
	parser_holder h(get_content("test/library/input/cont_no_op.in"));
	h.parser->ordinary_instruction_statement();

	auto& op_rem = h.parser->analyzer.current_operands_and_remarks();

	ASSERT_EQ(op_rem.operands.size(), (size_t)2);
	ASSERT_EQ(op_rem.remarks.size(), (size_t)2);

	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 15, 0, 23), op_rem.operands[0]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(1, 15, 1, 15), op_rem.operands[1]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 43, 0, 71), op_rem.remarks[0]);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(1, 16, 1, 68), op_rem.remarks[1]);

}

TEST(parser_get_op_rem, empty_op)
{
	parser_holder h(" LR 1,,");
	h.parser->ordinary_instruction_statement();

	auto& op_rem = h.parser->analyzer.current_operands_and_remarks();

	ASSERT_EQ(op_rem.operands.size(), (size_t)3);
	ASSERT_EQ(op_rem.remarks.size(), (size_t)0);

	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 4, 0, 5), op_rem.operands[0]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 6, 0, 6), op_rem.operands[1]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 7, 0, 7), op_rem.operands[2]->range);

}

TEST(parser_get_op_rem, all_empty_op)
{
	parser_holder h(" LR ,,");
	h.parser->ordinary_instruction_statement();

	auto& op_rem = h.parser->analyzer.current_operands_and_remarks();

	ASSERT_EQ(op_rem.operands.size(), (size_t)3);
	ASSERT_EQ(op_rem.remarks.size(), (size_t)0);

	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 4, 0, 4), op_rem.operands[0]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 5, 0, 5), op_rem.operands[1]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 6, 0, 6), op_rem.operands[2]->range);

}

TEST(parser_get_op_rem, cont_empty_op)
{
	parser_holder h(get_content("test/library/input/cont_empty_op.in"));
	h.parser->ordinary_instruction_statement();

	auto& op_rem = h.parser->analyzer.current_operands_and_remarks();

	ASSERT_EQ(op_rem.operands.size(), (size_t)5);
	ASSERT_EQ(op_rem.remarks.size(), (size_t)1);

	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 15, 0, 23), op_rem.operands[0]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 24, 0, 32), op_rem.operands[1]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 33, 0, 41), op_rem.operands[2]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(1, 15, 1, 15), op_rem.operands[3]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(1, 16, 1, 16), op_rem.operands[4]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 44, 0, 71), op_rem.remarks[0]);

}

#endif