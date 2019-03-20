#pragma once
#include "common_testing.h"

TEST(parser_var_sub, one_op)
{
	parser_holder h("   LR  &var,1");

	auto id = h.ctx->ids.add("var");
	h.ctx->create_local_variable<B_t>(id, true);
	h.parser->analyzer.set_var_sym_value<B_t>(semantics::var_sym("var", {}, {}), { false });

	h.parser->ordinary_instruction_statement();

	auto& op_rem = h.parser->analyzer.current_operands_and_remarks();
	
	ASSERT_EQ(op_rem.operands.size(), (size_t)2);
	ASSERT_EQ(op_rem.remarks.size(), (size_t)0);

	EXPECT_TRUE(op_rem.operands[0]->access_model_op());
	EXPECT_TRUE(op_rem.operands[1]->access_subs_op());
}

/*
TEST(parser_var_sub, more_ops_all)
{
	parser_holder h("   LR  &var");
	
	auto id = h.ctx->ids.add("var");
	h.ctx->create_local_variable<C_t>(id, true);
	h.parser->analyzer.set_var_sym_value<C_t>(id, {}, "1,1", {});

	h.parser->ordinary_instruction_statement();

	auto& op_rem = h.parser->analyzer.current_operands_and_remarks();

	ASSERT_EQ(op_rem.substituted_operands.size(), (size_t)2);
	ASSERT_EQ(op_rem.substituted_remarks.size(), (size_t)0);

	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 0, 0, 1), op_rem.substituted_operands[0]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 2, 0, 3), op_rem.substituted_operands[1]->range);
}

TEST(parser_var_sub, more_ops_part)
{
	parser_holder h("   LR  1&var");

	auto id = h.ctx->ids.add("var");
	h.ctx->create_local_variable<C_t>(id, true);
	h.parser->analyzer.set_var_sym_value<C_t>(id, {}, "1,1", {});

	h.parser->ordinary_instruction_statement();

	auto& op_rem = h.parser->analyzer.current_operands_and_remarks();

	ASSERT_EQ(op_rem.substituted_operands.size(), (size_t)2);
	ASSERT_EQ(op_rem.substituted_remarks.size(), (size_t)0);

	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 0, 0, 2), op_rem.substituted_operands[0]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 3, 0, 4), op_rem.substituted_operands[1]->range);
}

TEST(parser_var_sub, remark_subs)
{
	parser_holder h("   LR  1&var");

	auto id = h.ctx->ids.add("var");
	h.ctx->create_local_variable<C_t>(id, true);
	h.parser->analyzer.set_var_sym_value<C_t>(id, {}, "1,1 remark", {});

	h.parser->ordinary_instruction_statement();

	auto& op_rem = h.parser->analyzer.current_operands_and_remarks();

	ASSERT_EQ(op_rem.substituted_operands.size(), (size_t)2);
	ASSERT_EQ(op_rem.substituted_remarks.size(), (size_t)1);

	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 0, 0, 2), op_rem.substituted_operands[0]->range);
	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 3, 0, 4), op_rem.substituted_operands[1]->range);

	EXPECT_EQ(hlasm_plugin::parser_library::semantics::symbol_range(0, 5, 0,11), op_rem.substituted_remarks[0]);
}
*/
