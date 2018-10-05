#pragma once
#include "common_testing.h"

TEST(lookahead, forward_jump_success)
{
	parser_holder h(
		R"( 
   AGO .A  
&new seta 1 
.A ANOP
)"
	);

	h.parser->program();
	
	auto id = h.ctx->ids.add("new");
	auto var = h.ctx->get_var_sym(id);
	EXPECT_FALSE(var);
}

TEST(lookahead, forward_jump_success_valid_input)
{
	parser_holder h(
		R"( 
   AGO .A  
&new seta 1 
das cvx
tr9023-22
=f2 **
.A ANOP
)"
);

	h.parser->program();

	auto id = h.ctx->ids.add("new");
	auto var = h.ctx->get_var_sym(id);
	EXPECT_FALSE(var);
	EXPECT_EQ(h.parser->getNumberOfSyntaxErrors(),(size_t)0);
}

TEST(lookahead, forward_jump_fail)
{
	parser_holder h(
		R"( 
   AGO .A  
&new seta 1 
.B ANOP
)"
);

	h.parser->program();

	auto id = h.ctx->ids.add("new");
	auto var = h.ctx->get_var_sym(id);
	EXPECT_TRUE(var);
}