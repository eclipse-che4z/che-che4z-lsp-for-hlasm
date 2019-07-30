#pragma once
#include "common_testing.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library::semantics;

TEST(var_subs, gbl_instr_only)
{
	std::string input("   gbla var");
	analyzer a(input);
	a.analyze();

	auto& ctx = a.context();

	auto it = ctx.ids().find("var");

	ASSERT_TRUE(ctx.get_var_sym(it));

}

TEST(var_subs, lcl_instr_only)
{
	std::string input("   lcla var");
	analyzer a(input);
	a.analyze();

	auto& ctx = a.context();

	auto it = ctx.ids().find("var");

	ASSERT_TRUE(ctx.get_var_sym(it));

}

TEST(var_subs, gbl_instr_more)
{
	std::string input("   gbla var,var2,var3");
	analyzer a(input);
	a.analyze();

	auto& ctx = a.context();

	auto it = ctx.ids().find("var");
	auto it2 = ctx.ids().find("var2");
	auto it3 = ctx.ids().find("var3");

	ASSERT_TRUE(ctx.get_var_sym(it));
	ASSERT_TRUE(ctx.get_var_sym(it2));
	ASSERT_TRUE(ctx.get_var_sym(it3));

}

TEST(var_subs, lcl_instr_more)
{
	std::string input("   lcla var,var2,var3");
	analyzer a(input);
	a.analyze();

	auto& ctx = a.context();

	auto it = ctx.ids().find("var");
	auto it2 = ctx.ids().find("var2");
	auto it3 = ctx.ids().find("var3");

	ASSERT_TRUE(ctx.get_var_sym(it));
	ASSERT_TRUE(ctx.get_var_sym(it2));
	ASSERT_TRUE(ctx.get_var_sym(it3));

}

TEST(var_subs, set_to_var)
{
	std::string input("&var seta 3");
	analyzer a(input);
	a.analyze();

	auto& ctx = a.context();
	processing::context_manager m(a.context());

	auto it = ctx.ids().find("var");

	ASSERT_TRUE(ctx.get_var_sym(it));

	int tmp = m.get_var_sym_value(semantics::basic_var_sym(it, {}, {})).access_a();
	EXPECT_EQ(tmp, 3);

}

TEST(var_subs, set_to_var_idx)
{
	std::string input("&var(2) seta 3");
	std::string input_s("2");
	analyzer a(input);
	analyzer s(input_s);
	a.analyze();

	auto& ctx = a.context();
	processing::context_manager m(a.context());

	auto it = ctx.ids().find("var");

	ASSERT_TRUE(ctx.get_var_sym(it));
	std::vector<antlr4::ParserRuleContext*> subscript1;
	subscript1.push_back(s.parser().expr());
	int tmp = m.get_var_sym_value(semantics::basic_var_sym(it, std::move(subscript1), {})).access_a();
	EXPECT_EQ(tmp, 3);

}

TEST(var_subs, set_to_var_idx_many)
{
	std::string input("&var(2) seta 3,4,5");
	std::string input_s1("2");
	std::string input_s2("3");
	std::string input_s3("4");
	analyzer s1(input_s1);
	analyzer s2(input_s2);
	analyzer s3(input_s3);

	analyzer a(input);
	a.analyze();

	auto& ctx = a.context();
	processing::context_manager m(a.context());

	auto it = ctx.ids().find("var");

	ASSERT_TRUE(ctx.get_var_sym(it));

	int tmp;
	std::vector<antlr4::ParserRuleContext*> subscript1;
	subscript1.push_back(s1.parser().expr());
	tmp = m.get_var_sym_value(semantics::basic_var_sym(it, std::move(subscript1), {})).access_a();
	EXPECT_EQ(tmp, 3);
	std::vector<antlr4::ParserRuleContext*> subscript2;
	subscript2.push_back(s2.parser().expr());
	tmp = m.get_var_sym_value(semantics::basic_var_sym(it, std::move(subscript2), {})).access_a();
	EXPECT_EQ(tmp, 4);
	std::vector<antlr4::ParserRuleContext*> subscript3;
	subscript3.push_back(s3.parser().expr());
	tmp = m.get_var_sym_value(semantics::basic_var_sym(it, std::move(subscript3), {})).access_a();
	EXPECT_EQ(tmp, 5);

}

TEST(var_subs, var_sym_reset)
{
	std::string input("&var setc 'avc'   \n&var setc 'XXX'");
	analyzer a(input);
	a.analyze();

	auto& ctx = a.context();
	processing::context_manager m(a.context());

	auto it = ctx.ids().find("var");

	ASSERT_TRUE(ctx.get_var_sym(it));

	std::string tmp = m.get_var_sym_value(semantics::basic_var_sym(it, {}, {})).access_c();
	EXPECT_EQ(tmp, "XXX");

}

TEST(var_subs, created_set_sym)
{
	std::string input("&var setc 'avc'   \n&var2 setb 0  \n&(ab&var.cd&var2) seta 11");
	analyzer a(input);
	a.analyze();

	auto& ctx = a.context();
	processing::context_manager m(a.context());

	auto it = ctx.ids().find("abavccd0");

	ASSERT_TRUE(ctx.get_var_sym(it));

	auto tmp = m.get_var_sym_value(semantics::basic_var_sym(it, {}, {})).access_a();
	EXPECT_EQ(tmp, 11);

}

TEST(AGO, extended)
{
	std::string input(R"(
 AGO (2).a,.b,.c
.a anop   
&var1 setb 0
.b anop
&var2 setb 0
.c anop
&var3 setb 0
)");
	analyzer a(input);
	a.analyze();

	auto& ctx = a.context();

	auto it1 = ctx.ids().add("var1");
	auto it2 = ctx.ids().add("var2");
	auto it3 = ctx.ids().add("var3");

	EXPECT_FALSE(ctx.get_var_sym(it1));
	EXPECT_TRUE(ctx.get_var_sym(it2));
	EXPECT_TRUE(ctx.get_var_sym(it3));
}

TEST(AGO, extended_fail)
{
	std::string input(R"(
 AGO (8).a,.b,.c
.a anop   
&var1 setb 0
.b anop
&var2 setb 0
.c anop
&var3 setb 0
)");
	analyzer a(input);
	a.analyze();

	auto& ctx = a.context();

	auto it1 = ctx.ids().add("var1");
	auto it2 = ctx.ids().add("var2");
	auto it3 = ctx.ids().add("var3");

	EXPECT_TRUE(ctx.get_var_sym(it1));
	EXPECT_TRUE(ctx.get_var_sym(it2));
	EXPECT_TRUE(ctx.get_var_sym(it3));
}

TEST(AIF, extended)
{
	std::string input(R"(
 AIF (0).a,(1).b,(1).c
.a anop   
&var1 setb 0
.b anop
&var2 setb 0
.c anop
&var3 setb 0
)");
	analyzer a(input);
	a.analyze();

	auto& ctx = a.context();

	auto it1 = ctx.ids().add("var1");
	auto it2 = ctx.ids().add("var2");
	auto it3 = ctx.ids().add("var3");

	EXPECT_FALSE(ctx.get_var_sym(it1));
	EXPECT_TRUE(ctx.get_var_sym(it2));
	EXPECT_TRUE(ctx.get_var_sym(it3));
}

TEST(AIF, extended_fail)
{
	std::string input(R"(
 AIF (0).a,(0).b,(0).c
.a anop   
&var1 setb 0
.b anop
&var2 setb 0
.c anop
&var3 setb 0
)");
	analyzer a(input);
	a.analyze();

	auto& ctx = a.context();

	auto it1 = ctx.ids().add("var1");
	auto it2 = ctx.ids().add("var2");
	auto it3 = ctx.ids().add("var3");

	EXPECT_TRUE(ctx.get_var_sym(it1));
	EXPECT_TRUE(ctx.get_var_sym(it2));
	EXPECT_TRUE(ctx.get_var_sym(it3));
}