#pragma once
#include "common_testing.h"

TEST(lookahead, forward_jump_success)
{
	std::string input(
		R"( 
   AGO .A  
&new seta 1 
.A ANOP
)"
	);

	analyzer a(input);
	a.analyze();
	
	auto id = a.context().ids().add("new");
	auto var = a.context().get_var_sym(id);
	EXPECT_FALSE(var);
}

TEST(lookahead, forward_jump_to_continued)
{
	std::string input(
		R"( 
      AGO      .HERE
&bad seta 1
.HERE LR                                                               x
               1,1
&good seta 1
      LR 1,1
)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(a.diags().size(), (size_t)0);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

	EXPECT_FALSE(a.context().get_var_sym(a.context().ids().add("bad")));
	EXPECT_TRUE(a.context().get_var_sym(a.context().ids().add("good")));
}

TEST(lookahead, forward_jump_from_continued)
{
	std::string input(
		R"( 
      AGO                                                              x
               .HERE
&bad seta 1
.HERE LR                                                               x
               1,1
&good seta 1
      LR 1,1
)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(a.diags().size(), (size_t)0);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

	EXPECT_FALSE(a.context().get_var_sym(a.context().ids().add("bad")));
	EXPECT_TRUE(a.context().get_var_sym(a.context().ids().add("good")));
}

TEST(lookahead, forward_jump_success_valid_input)
{
	std::string input(
		R"( 
   AGO .A  
&new seta 1 
das cvx
tr9023-22
=f2 **
.A ANOP)"
);

	analyzer a(input);
	a.analyze();

	auto id = a.context().ids().add("new");
	auto var = a.context().get_var_sym(id);
	EXPECT_FALSE(var);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(),(size_t)0);
}

TEST(lookahead, forward_jump_fail)
{
	std::string input(
		R"( 
   AGO .A  
&new seta 1 
.B ANOP
)"
);

	analyzer a(input);
	a.analyze();

	auto id = a.context().ids().add("new");
	auto var = a.context().get_var_sym(id);
	EXPECT_TRUE(var);
}

TEST(lookahead, rewinding_from_last_line)
{
	std::string input(
		R"( 
 ACTR 2 
.A ANOP
 AGO .A)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(lookahead, rewinding_from_one_from_last_line)
{
	std::string input(
		R"( 
 ACTR 2 
.A ANOP
 AGO .A
)"
	);

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.diags().size(), (size_t)1);
}