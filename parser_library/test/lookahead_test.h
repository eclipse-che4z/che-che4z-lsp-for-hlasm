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

TEST(lookahead, forward_jump_before_comment)
{
	std::string input(
		R"( 
 AGO .A
 BAD_INSTR
*COMMENT
.A ANOP
)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(lookahead, forward_jump_before_continued_comment)
{
	std::string input(
		R"( 
 AGO .A
 BAD_INSTR
*COMMENT                                                               X IGNORED
 IGNORED         COMMENT
.A ANOP
)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, lookup_triggered)
{
	std::string input("L'X");
	analyzer a(input);
	auto expr = a.parser().expr();

	empty_attribute_provider prov;

	expression_analyzer ea(evaluation_context{ a.context(),prov,empty_parse_lib_provider::instance });

	EXPECT_EQ(ea.get_undefined_symbol_references(expr).size(), (size_t)1);

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, lookup_not_triggered)
{
	std::string input("L'X");
	analyzer a(input);
	auto expr = a.parser().expr();

	//define symbol with undefined length
	auto tmp = a.context().ord_ctx.create_symbol(a.context().ids().add("X"), symbol_value(), symbol_attributes(symbol_origin::DAT, 200), {});
	ASSERT_TRUE(tmp);

	empty_attribute_provider prov;

	expression_analyzer ea(evaluation_context{ a.context(),prov,empty_parse_lib_provider::instance });

	//although length is undefined the actual symbol is defined so no lookup should happen
	EXPECT_EQ(ea.get_undefined_symbol_references(expr).size(), (size_t)0);

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, lookup_of_two_refs)
{
	std::string input("L'X+L'Y");
	analyzer a(input);
	auto expr = a.parser().expr();

	empty_attribute_provider prov;

	expression_analyzer ea(evaluation_context{ a.context(),prov,empty_parse_lib_provider::instance });

	EXPECT_EQ(ea.get_undefined_symbol_references(expr).size(), (size_t)2);

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, lookup_of_two_refs_but_one_symbol)
{
	std::string input("S'X+L'X");
	analyzer a(input);
	auto expr = a.parser().expr();

	empty_attribute_provider prov;

	expression_analyzer ea(evaluation_context{ a.context(),prov,empty_parse_lib_provider::instance });

	EXPECT_EQ(ea.get_undefined_symbol_references(expr).size(), (size_t)1);

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(EQU_attribute_lookahead, correct_attribute_refereces)
{
	std::string input(
		R"( 
&A SETC T'X
&B SETA L'X
X EQU 1,10,C'T'
&C SETA L'Y
Y EQU X+1
)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("A"))->access_set_symbol_base()->access_set_symbol<C_t>()->get_value(), "T");
	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("B"))->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 10);
	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("C"))->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 10);

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(EQU_attribute_lookahead, incorrect_attribute_reference)
{
	std::string input(
		R"( 
&A SETA S'X
X EQU 1,10,C'T'
)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("A"))->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 0);

	EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(EQU_attribute_lookahead, unresolvable_attribute_referece)
{
	std::string input(
		R"( 
&A SETA L'X
X EQU 1,Y+11,C'T'
)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("A"))->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 1);

	EXPECT_EQ(a.diags().size(), (size_t)2);
}

TEST(EQU_attribute_lookahead, errorous_but_resolable_statement_incorrect_operand)
{
	std::string input(
		R"( 
&A SETA L'X
 AGO .A
X EQU 1,2,**&
.A ANOP
)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("A"))->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 2);

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(EQU_attribute_lookahead, errorous_but_resolable_statement_last_operand_model)
{
	std::string input(
		R"( 
&A SETA L'X
 AGO .A
X EQU 1,2,&a
.A ANOP
)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("A"))->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 2);

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(EQU_attribute_lookahead, errorous_but_unresolable_statement_first_operand_model)
{
	std::string input(
		R"( 
&A SETA L'X
 AGO .A
X EQU &a,2
.A ANOP
)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("A"))->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 1);

	EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(EQU_attribute_lookahead, errorous_but_unresolable_statement_first_operand_invalid)
{
	std::string input(
		R"( 
&A SETA L'X
 AGO .A
X EQU =**)-,2
.A ANOP
)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("A"))->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 1);

	EXPECT_EQ(a.diags().size(), (size_t)1);
}

class look_parse_lib_prov : public parse_lib_provider
{
	std::unique_ptr<analyzer> a;

	std::string LIB =
		R"( 
X EQU 1,2,C'X'
&WAS_IN SETB 1
)";

	std::string LIB2 =
		R"( 
&A SETA L'X
&WAS_IN SETB 1
)";

	std::string LIB3 =
		R"( 
 MAC 
&AFTER_MAC SETB 1
)";

	virtual parse_result parse_library(const std::string& library, context::hlasm_context& hlasm_ctx, const library_data data) override
	{
		std::string* content;
		if (library == "LIB")
			content = &LIB;
		else if (library == "LIB2")
			content = &LIB2;
		else if (library == "LIB3")
			content = &LIB3;
		else
			return false;

		a = std::make_unique<analyzer>(*content, library, hlasm_ctx, *this, data);
		a->analyze();
		a->collect_diags();
		return true;
	}

	virtual bool has_library(const std::string& library, context::hlasm_context& hlasm_ctx) const { return false; }
};

TEST(attribute_lookahead, lookup_to_copy)
{
	std::string input(
		R"( 
&A SETA L'X
&WAS_BEFORE SETB 1
 COPY LIB
&WAS_AFTER SETB 1
)"
);

	look_parse_lib_prov mock;
	analyzer a(input, "", mock);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("A"))->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 2);
	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("WAS_BEFORE"))->access_set_symbol_base()->access_set_symbol<B_t>()->get_value(), true);
	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("WAS_IN"))->access_set_symbol_base()->access_set_symbol<B_t>()->get_value(), true);
	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("WAS_AFTER"))->access_set_symbol_base()->access_set_symbol<B_t>()->get_value(), true);

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, lookup_from_copy)
{
	std::string input(
		R"( 
&WAS_BEFORE SETB 1
 COPY LIB2
X EQU 1,2
&WAS_AFTER SETB 1
)"
);

	look_parse_lib_prov mock;
	analyzer a(input, "", mock);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("A"))->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 2);
	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("WAS_BEFORE"))->access_set_symbol_base()->access_set_symbol<B_t>()->get_value(), true);
	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("WAS_IN"))->access_set_symbol_base()->access_set_symbol<B_t>()->get_value(), true);
	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("WAS_AFTER"))->access_set_symbol_base()->access_set_symbol<B_t>()->get_value(), true);

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, lookup_from_macro)
{
	std::string input(
		R"(
 MACRO
 MAC
 GBLA A
&A SETA L'X
X EQU 2,3
 MEND
*
 COPY LIB3
X EQU 1,2
)"
);

	look_parse_lib_prov mock;
	analyzer a(input, "", mock);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().globals().find(a.context().ids().add("A"))->second->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 2);
	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("AFTER_MAC"))->access_set_symbol_base()->access_set_symbol<B_t>()->get_value(), true);

	EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(attribute_lookahead, lookup_from_macro_last_line)
{
	std::string input(
		R"( macro
 GETMAIN &b=,&l=
 AIF   (T'&l NE 'O' AND T'&b NE 'O').ERR14      @L1A 
 mend
         GETMAIN   b=svc)"
);

	look_parse_lib_prov mock;
	analyzer a(input, "", mock);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, lookup_from_macro_one_to_last_line)
{
	std::string input(
		R"( macro
 GETMAIN &b=,&l=
 AIF   (T'&l NE 'O' AND T'&b NE 'O').ERR14      @L1A 
 mend
         GETMAIN   b=svc
)"
	);

	look_parse_lib_prov mock;
	analyzer a(input, "", mock);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, lookup_of_two_refs_evaluation)
{
	std::string input(
		R"( 
&A SETA L'X+L'Y
X EQU 1,10
Y EQU 2,11
)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("A"))->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 21);

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, machine_label_lookahead)
{
	std::string input(
		R"( 
&A SETA L'X
&B SETC T'X
X LR 1,1
)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("A"))->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 2);
	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("B"))->access_set_symbol_base()->access_set_symbol<C_t>()->get_value(), "I");

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, section_label_lookahead)
{
	std::string input(
		R"( 
&A SETA L'X
&B SETC T'X
X CSECT
)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("A"))->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 1);
	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("B"))->access_set_symbol_base()->access_set_symbol<C_t>()->get_value(), "J");

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_def_attribute_lookahead, correct_attribute_reference)
{
	std::string input(
		R"( 
&A SETA L'X
&B SETC T'X
&C SETA S'X
X DC FS24'6'       remark
)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("A"))->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 4);
	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("B"))->access_set_symbol_base()->access_set_symbol<C_t>()->get_value(), "F");
	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("C"))->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 24);

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_def_attribute_lookahead, incorrect_attribute_reference)
{
	std::string input(
		R"( 
&A SETA S'X
X DC C'A'
)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("A"))->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 0);

	EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(attribute_lookahead, lookup_of_two_refs_but_one_symbol_evaluation)
{
	std::string input(
		R"( 
&A SETA L'X+L'Y
X EQU 1,10
Y EQU 2,11
)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("A"))->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 21);

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, failed_lookup)
{
	std::string input(
		R"( 
&A SETA L'X+L'Y
X EQU 1,10
)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("A"))->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 11);

	EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(attribute_lookahead, nested_lookup)
{
	std::string input(
		R"( 
&V(1) SETC 'A','B','C'
&A SETA L'&V(L'X)
X EQU 1,2
B EQU 2,22
)"
);

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("A"))->access_set_symbol_base()->access_set_symbol<A_t>()->get_value(), 22);

	EXPECT_EQ(a.diags().size(), (size_t)0);
}