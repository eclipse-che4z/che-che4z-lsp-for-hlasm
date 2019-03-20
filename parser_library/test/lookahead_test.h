#pragma once
#include "common_testing.h"

TEST(lookahead, forward_jump_success)
{
	analyzer a(
		R"( 
   AGO .A  
&new seta 1 
.A ANOP
)"
	);

	a.analyze();
	
	auto id = a.context()->ids.add("new");
	auto var = a.context()->get_var_sym(id);
	EXPECT_FALSE(var);
}

TEST(lookahead, forward_jump_success_valid_input)
{
	analyzer a(
		R"( 
   AGO .A  
&new seta 1 
das cvx
tr9023-22
=f2 **
.A ANOP
)"
);

	a.analyze();

	auto id = a.context()->ids.add("new");
	auto var = a.context()->get_var_sym(id);
	EXPECT_FALSE(var);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(),(size_t)0);
}

TEST(lookahead, forward_jump_fail)
{
	analyzer a(
		R"( 
   AGO .A  
&new seta 1 
.B ANOP
)"
);

	a.analyze();

	auto id = a.context()->ids.add("new");
	auto var = a.context()->get_var_sym(id);
	EXPECT_TRUE(var);
}