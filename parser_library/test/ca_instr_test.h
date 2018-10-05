#pragma once
#include "common_testing.h"

TEST(var_subs, gbl_instr_only)
{
	parser_holder h("   gbla var");
	h.parser->program();

	auto& ctx = *h.ctx;

	auto it = ctx.ids.find("var");

	ASSERT_TRUE(ctx.get_var_sym(it));

}

TEST(var_subs, lcl_instr_only)
{
	parser_holder h("   lcla var");
	h.parser->program();

	auto& ctx = *h.ctx;

	auto it = ctx.ids.find("var");

	ASSERT_TRUE(ctx.get_var_sym(it));

}

TEST(var_subs, gbl_instr_more)
{
	parser_holder h("   gbla var,var2,var3");
	h.parser->program();

	auto& ctx = *h.ctx;

	auto it = ctx.ids.find("var");
	auto it2 = ctx.ids.find("var2");
	auto it3 = ctx.ids.find("var3");

	ASSERT_TRUE(ctx.get_var_sym(it));
	ASSERT_TRUE(ctx.get_var_sym(it2));
	ASSERT_TRUE(ctx.get_var_sym(it3));

}

TEST(var_subs, lcl_instr_more)
{
	parser_holder h("   lcla var,var2,var3");
	h.parser->program();

	auto& ctx = *h.ctx;

	auto it = ctx.ids.find("var");
	auto it2 = ctx.ids.find("var2");
	auto it3 = ctx.ids.find("var3");

	ASSERT_TRUE(ctx.get_var_sym(it));
	ASSERT_TRUE(ctx.get_var_sym(it2));
	ASSERT_TRUE(ctx.get_var_sym(it3));

}

TEST(var_subs, set_to_var)
{
	parser_holder h("&var seta 3");
	h.parser->program();

	auto& ctx = *h.ctx;

	auto it = ctx.ids.find("var");

	ASSERT_TRUE(ctx.get_var_sym(it));

	int tmp = h.parser->analyzer.get_var_sym_value(it, {}, {}).access_a();
	EXPECT_EQ(tmp, 3);

}

TEST(var_subs, set_to_var_idx)
{
	parser_holder h("&var(2) seta 3");
	h.parser->program();

	auto& ctx = *h.ctx;

	auto it = ctx.ids.find("var");

	ASSERT_TRUE(ctx.get_var_sym(it));
	std::vector<expr_ptr> subscript;
	subscript.push_back(std::make_unique<arithmetic_expression>(2));
	int tmp = h.parser->analyzer.get_var_sym_value(it, std::move(subscript), {}).access_a();
	EXPECT_EQ(tmp, 3);

}

TEST(var_subs, set_to_var_idx_many)
{
	parser_holder h("&var(2) seta 3,4,5");
	h.parser->program();

	auto& ctx = *h.ctx;

	auto it = ctx.ids.find("var");

	ASSERT_TRUE(ctx.get_var_sym(it));

	int tmp;
	std::vector<expr_ptr> subscript1;
	subscript1.push_back(std::make_unique<arithmetic_expression>(2));
	tmp = h.parser->analyzer.get_var_sym_value(it, std::move(subscript1), {}).access_a();
	EXPECT_EQ(tmp, 3);
	std::vector<expr_ptr> subscript2;
	subscript2.push_back(std::make_unique<arithmetic_expression>(3));
	tmp = h.parser->analyzer.get_var_sym_value(it, std::move(subscript2), {}).access_a();
	EXPECT_EQ(tmp, 4);
	std::vector<expr_ptr> subscript3;
	subscript3.push_back(std::make_unique<arithmetic_expression>(4));
	tmp = h.parser->analyzer.get_var_sym_value(it, std::move(subscript3), {}).access_a();
	EXPECT_EQ(tmp, 5);

}

TEST(var_subs, var_sym_reset)
{
	parser_holder h("&var setc 'avc'   \n&var setc 'XXX'");
	h.parser->program();

	auto& ctx = *h.ctx;

	auto it = ctx.ids.find("var");

	ASSERT_TRUE(ctx.get_var_sym(it));

	std::string tmp = h.parser->analyzer.get_var_sym_value(it, {  }, {}).access_c();
	EXPECT_EQ(tmp, "XXX");

}

TEST(var_subs, created_set_sym)
{
	parser_holder h("&var setc 'avc'   \n&var2 setb 0  \n&(ab&var.cd&var2) seta 11");
	h.parser->program();

	auto& ctx = *h.ctx;

	auto it = ctx.ids.find("abavccd0");

	ASSERT_TRUE(ctx.get_var_sym(it));

	auto tmp = h.parser->analyzer.get_var_sym_value(it, {  }, {}).access_a();
	EXPECT_EQ(tmp, 11);

}

TEST(AGO, extended)
{
	parser_holder h(R"(
 AGO (2).a,.b,.c
.a anop   
&var1 setb 0
.b anop
&var2 setb 0
.c anop
&var3 setb 0
)");
	h.parser->program();

	auto& ctx = *h.ctx;

	auto it1 = ctx.ids.add("var1");
	auto it2 = ctx.ids.add("var2");
	auto it3 = ctx.ids.add("var3");

	EXPECT_FALSE(ctx.get_var_sym(it1));
	EXPECT_TRUE(ctx.get_var_sym(it2));
	EXPECT_TRUE(ctx.get_var_sym(it3));
}

TEST(AGO, extended_fail)
{
	parser_holder h(R"(
 AGO (8).a,.b,.c
.a anop   
&var1 setb 0
.b anop
&var2 setb 0
.c anop
&var3 setb 0
)");
	h.parser->program();

	auto& ctx = *h.ctx;

	auto it1 = ctx.ids.add("var1");
	auto it2 = ctx.ids.add("var2");
	auto it3 = ctx.ids.add("var3");

	EXPECT_TRUE(ctx.get_var_sym(it1));
	EXPECT_TRUE(ctx.get_var_sym(it2));
	EXPECT_TRUE(ctx.get_var_sym(it3));
}

TEST(AIF, extended)
{
	parser_holder h(R"(
 AIF (0).a,(1).b,(1).c
.a anop   
&var1 setb 0
.b anop
&var2 setb 0
.c anop
&var3 setb 0
)");
	h.parser->program();

	auto& ctx = *h.ctx;

	auto it1 = ctx.ids.add("var1");
	auto it2 = ctx.ids.add("var2");
	auto it3 = ctx.ids.add("var3");

	EXPECT_FALSE(ctx.get_var_sym(it1));
	EXPECT_TRUE(ctx.get_var_sym(it2));
	EXPECT_TRUE(ctx.get_var_sym(it3));
}

TEST(AIF, extended_fail)
{
	parser_holder h(R"(
 AIF (0).a,(0).b,(0).c
.a anop   
&var1 setb 0
.b anop
&var2 setb 0
.c anop
&var3 setb 0
)");
	h.parser->program();

	auto& ctx = *h.ctx;

	auto it1 = ctx.ids.add("var1");
	auto it2 = ctx.ids.add("var2");
	auto it3 = ctx.ids.add("var3");

	EXPECT_TRUE(ctx.get_var_sym(it1));
	EXPECT_TRUE(ctx.get_var_sym(it2));
	EXPECT_TRUE(ctx.get_var_sym(it3));
}