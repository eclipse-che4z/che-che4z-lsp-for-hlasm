#pragma once
#include "common_testing.h"

TEST(var_subs, gbl_instr_only)
{
	analyzer a("   gbla var");
	a.analyze();

	auto& ctx = *a.context();

	auto it = ctx.ids.find("var");

	ASSERT_TRUE(ctx.get_var_sym(it));

}

TEST(var_subs, lcl_instr_only)
{
	analyzer a("   lcla var");
	a.analyze();

	auto& ctx = *a.context();

	auto it = ctx.ids.find("var");

	ASSERT_TRUE(ctx.get_var_sym(it));

}

TEST(var_subs, gbl_instr_more)
{
	analyzer a("   gbla var,var2,var3");
	a.analyze();

	auto& ctx = *a.context();

	auto it = ctx.ids.find("var");
	auto it2 = ctx.ids.find("var2");
	auto it3 = ctx.ids.find("var3");

	ASSERT_TRUE(ctx.get_var_sym(it));
	ASSERT_TRUE(ctx.get_var_sym(it2));
	ASSERT_TRUE(ctx.get_var_sym(it3));

}

TEST(var_subs, lcl_instr_more)
{
	analyzer a("   lcla var,var2,var3");
	a.analyze();

	auto& ctx = *a.context();

	auto it = ctx.ids.find("var");
	auto it2 = ctx.ids.find("var2");
	auto it3 = ctx.ids.find("var3");

	ASSERT_TRUE(ctx.get_var_sym(it));
	ASSERT_TRUE(ctx.get_var_sym(it2));
	ASSERT_TRUE(ctx.get_var_sym(it3));

}

TEST(var_subs, set_to_var)
{
	analyzer a("&var seta 3");
	a.analyze();

	auto& ctx = *a.context();
	semantics::context_manager m(a.context());

	auto it = ctx.ids.find("var");

	ASSERT_TRUE(ctx.get_var_sym(it));

	int tmp = m.get_var_sym_value(semantics::var_sym("var", {}, {})).access_a();
	EXPECT_EQ(tmp, 3);

}

TEST(var_subs, set_to_var_idx)
{
	analyzer a("&var(2) seta 3");
	analyzer s("2");
	a.analyze();

	auto& ctx = *a.context();
	semantics::context_manager m(a.context());

	auto it = ctx.ids.find("var");

	ASSERT_TRUE(ctx.get_var_sym(it));
	std::vector<antlr4::ParserRuleContext*> subscript1;
	subscript1.push_back(s.parser().expr());
	int tmp = m.get_var_sym_value(semantics::var_sym("var", std::move(subscript1), {})).access_a();
	EXPECT_EQ(tmp, 3);

}

TEST(var_subs, set_to_var_idx_many)
{
	analyzer a("&var(2) seta 3,4,5");
	analyzer s1("2");
	analyzer s2("3");
	analyzer s3("4");
	a.analyze();

	auto& ctx = *a.context();
	semantics::context_manager m(a.context());

	auto it = ctx.ids.find("var");

	ASSERT_TRUE(ctx.get_var_sym(it));

	int tmp;
	std::vector<antlr4::ParserRuleContext*> subscript1;
	subscript1.push_back(s1.parser().expr());
	tmp = m.get_var_sym_value(semantics::var_sym("var", std::move(subscript1), {})).access_a();
	EXPECT_EQ(tmp, 3);
	std::vector<antlr4::ParserRuleContext*> subscript2;
	subscript2.push_back(s2.parser().expr());
	tmp = m.get_var_sym_value(semantics::var_sym("var", std::move(subscript2), {})).access_a();
	EXPECT_EQ(tmp, 4);
	std::vector<antlr4::ParserRuleContext*> subscript3;
	subscript3.push_back(s3.parser().expr());
	tmp = m.get_var_sym_value(semantics::var_sym("var", std::move(subscript3), {})).access_a();
	EXPECT_EQ(tmp, 5);

}

TEST(var_subs, var_sym_reset)
{
	analyzer a("&var setc 'avc'   \n&var setc 'XXX'");
	a.analyze();

	auto& ctx = *a.context();
	semantics::context_manager m(a.context());

	auto it = ctx.ids.find("var");

	ASSERT_TRUE(ctx.get_var_sym(it));

	std::string tmp = m.get_var_sym_value(semantics::var_sym("var", {}, {})).access_c();
	EXPECT_EQ(tmp, "XXX");

}

TEST(var_subs, created_set_sym)
{
	analyzer a("&var setc 'avc'   \n&var2 setb 0  \n&(ab&var.cd&var2) seta 11");
	a.analyze();

	auto& ctx = *a.context();
	semantics::context_manager m(a.context());

	auto it = ctx.ids.find("abavccd0");

	ASSERT_TRUE(ctx.get_var_sym(it));

	auto tmp = m.get_var_sym_value(semantics::var_sym("abavccd0", {}, {})).access_a();
	EXPECT_EQ(tmp, 11);

}

TEST(AGO, extended)
{
	analyzer a(R"(
 AGO (2).a,.b,.c
.a anop   
&var1 setb 0
.b anop
&var2 setb 0
.c anop
&var3 setb 0
)");
	a.analyze();

	auto& ctx = *a.context();

	auto it1 = ctx.ids.add("var1");
	auto it2 = ctx.ids.add("var2");
	auto it3 = ctx.ids.add("var3");

	EXPECT_FALSE(ctx.get_var_sym(it1));
	EXPECT_TRUE(ctx.get_var_sym(it2));
	EXPECT_TRUE(ctx.get_var_sym(it3));
}

TEST(AGO, extended_fail)
{
	analyzer a(R"(
 AGO (8).a,.b,.c
.a anop   
&var1 setb 0
.b anop
&var2 setb 0
.c anop
&var3 setb 0
)");
	a.analyze();

	auto& ctx = *a.context();

	auto it1 = ctx.ids.add("var1");
	auto it2 = ctx.ids.add("var2");
	auto it3 = ctx.ids.add("var3");

	EXPECT_TRUE(ctx.get_var_sym(it1));
	EXPECT_TRUE(ctx.get_var_sym(it2));
	EXPECT_TRUE(ctx.get_var_sym(it3));
}

TEST(AIF, extended)
{
	analyzer a(R"(
 AIF (0).a,(1).b,(1).c
.a anop   
&var1 setb 0
.b anop
&var2 setb 0
.c anop
&var3 setb 0
)");
	a.analyze();

	auto& ctx = *a.context();

	auto it1 = ctx.ids.add("var1");
	auto it2 = ctx.ids.add("var2");
	auto it3 = ctx.ids.add("var3");

	EXPECT_FALSE(ctx.get_var_sym(it1));
	EXPECT_TRUE(ctx.get_var_sym(it2));
	EXPECT_TRUE(ctx.get_var_sym(it3));
}

TEST(AIF, extended_fail)
{
	analyzer a(R"(
 AIF (0).a,(0).b,(0).c
.a anop   
&var1 setb 0
.b anop
&var2 setb 0
.c anop
&var3 setb 0
)");
	a.analyze();

	auto& ctx = *a.context();

	auto it1 = ctx.ids.add("var1");
	auto it2 = ctx.ids.add("var2");
	auto it3 = ctx.ids.add("var3");

	EXPECT_TRUE(ctx.get_var_sym(it1));
	EXPECT_TRUE(ctx.get_var_sym(it2));
	EXPECT_TRUE(ctx.get_var_sym(it3));
}