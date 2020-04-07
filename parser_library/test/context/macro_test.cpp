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

#include "gtest/gtest.h"

#include "../common_testing.h"

//tests for macro feature:
//definition parsing
//instantiation
//arguments passing
//external macro libraries

TEST(macro, macro_def)
{
	std::string input =
R"( MACRO
&l M1 &op,&k=5,&op2,&k2=(1,2,3)
 ago .a
 lr 1,1
 anop
.a mend
)";
	analyzer a(input);
	a.analyze();

	id_index id = a.context().ids().add("m1");
	auto& macros = a.context().macros();

	auto tmp = macros.find(id);
	ASSERT_TRUE(tmp !=macros.end());

	auto& m = tmp->second;
	
	auto op = a.context().ids().add("op");

	EXPECT_EQ(m->named_params().find(op)->second->access_positional_param()->position, (size_t)1);

	auto op2 = a.context().ids().add("op2");

	EXPECT_EQ(m->named_params().find(op2)->second->access_positional_param()->position, (size_t)2);

	auto l = a.context().ids().add("l");

	EXPECT_EQ(m->named_params().find(l)->second->access_positional_param()->position, (size_t)0);

	auto k = a.context().ids().add("k");

	EXPECT_EQ(m->named_params().find(k)->second->access_keyword_param()->get_value(), "5");

	auto k2 = a.context().ids().add("k2");

	EXPECT_EQ(m->named_params().find(k2)->second->access_keyword_param()->get_value(), "(1,2,3)");

	EXPECT_EQ(m->cached_definition.size(), (size_t)4);
}

TEST(macro, macro_def_count)
{
	std::string input = 
R"( MACRO
 M1
 ANOP
 MEND

 MACRO
 M2
 ANOP

 MACRO
 INNER_M
 ANOP
 MEND

 MEND
)";
	analyzer a(input);
	a.analyze();

	ASSERT_EQ(a.context().macros().size(),(size_t)2);

	id_index id;

	id = a.context().ids().add("M1");
	EXPECT_TRUE(a.context().macros().find(id) != a.context().macros().end());

	id = a.context().ids().add("m2");
	EXPECT_TRUE(a.context().macros().find(id) != a.context().macros().end());
}

TEST(macro, macro_def_count_inner)
{
	std::string input =
R"( MACRO
 M1
 ANOP
 MEND

 MACRO
 M2
 ANOP

 MACRO
 INNER_M
 LR 1,1 
 ANOP
 MEND

 MEND

 M2
)";
	analyzer a(input);
	a.analyze();

	ASSERT_EQ(a.context().macros().size(), (size_t)3);

	id_index id;

	id = a.context().ids().add("M1");
	EXPECT_TRUE(a.context().macros().find(id) != a.context().macros().end());

	id = a.context().ids().add("M2");
	EXPECT_TRUE(a.context().macros().find(id) != a.context().macros().end());

	id = a.context().ids().add("INNER_M");
	EXPECT_TRUE(a.context().macros().find(id) != a.context().macros().end());
}

TEST(macro, macro_lookahead_pass)
{
	std::string input =
R"( MACRO
 M1
 AGO .A

 MACRO
 INNER_M
 ANOP
 MEND

.A ANOP 

 MEND

 M1
)";
	analyzer a(input);
	a.analyze();

	ASSERT_EQ(a.context().macros().size(), (size_t)1);

	id_index id;

	id = a.context().ids().add("M1");
	EXPECT_TRUE(a.context().macros().find(id) != a.context().macros().end());
}

TEST(macro, macro_lookahead_fail)
{
	std::string input =
R"( MACRO
 M1
 AGO .A

 MACRO
 INNER_M
 ANOP
 MEND

.B ANOP 

 MEND

 M1
)";
	analyzer a(input);
	a.analyze();

	ASSERT_EQ(a.context().macros().size(), (size_t)2);

	id_index id;

	id = a.context().ids().add("M1");
	EXPECT_TRUE(a.context().macros().find(id) != a.context().macros().end());

	id = a.context().ids().add("INNER_M");
	EXPECT_TRUE(a.context().macros().find(id) != a.context().macros().end());
}

TEST(macro, macro_positional_param_subs)
{
	std::string input =
R"( MACRO
 M1 &p
 lr &p,1
 mend
 
 M1 20
)";
	analyzer a(input);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(dynamic_cast<diagnosable*>(&a)->diags().size(), (size_t)1);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}

TEST(macro, macro_keyword_param)
{
	std::string input =
R"( MACRO
 M1 &p=50
 lr &p,1
 mend
 
 M1
 M1 p=1
)";
	analyzer a(input);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(dynamic_cast<diagnosable*>(&a)->diags().size(), (size_t)1);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}

TEST(macro, macro_param_expr)
{
	std::string input =
R"( MACRO
 M1 &a,&b
&c seta &a
&d seta &b
&e seta &c+&d
 lr &e,&e
 mend
 
 M1 1,1
 M1 10,6
)";
	analyzer a(input);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(dynamic_cast<diagnosable*>(&a)->diags().size(), (size_t)2);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}

TEST(macro, macro_composite_param_no_err)
{
	std::string input =
R"( MACRO
 M1 &a,&b
 lr &a(2,3),&a(1)
 mend
 
 M1 (1,(1,2,3))
)";
	analyzer a(input);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(dynamic_cast<diagnosable*>(&a)->diags().size(), (size_t)0);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}

TEST(macro, macro_composite_param_err)
{
	std::string input = 
R"( MACRO
 M1 &a,&b
 lr &a(2,3),&a(1)
 mend
 
 M1 (100,(1,2,3))
)";
	analyzer a(input);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(dynamic_cast<diagnosable*>(&a)->diags().size(), (size_t)1);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}


TEST(macro, macro_name_param)
{
	std::string input =
R"( MACRO
&n M1 &a,&b
 lr &n,&n
 mend
 
1 M1 
)";
	analyzer a(input);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(dynamic_cast<diagnosable*>(&a)->diags().size(), (size_t)0);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}

TEST(macro, macro_name_param_repetition)
{
	std::string input =
R"( MACRO
&n M1 &n,&b
 mend
1 m1 2,3
 MACRO
 M2 &a,&a=6,&b
 mend

 m2 1,2

 MACRO
 M3 &a=5,&a,&b
 mend
 
 m3 1,2,3

)";
	analyzer a(input);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(dynamic_cast<diagnosable*>(&a)->diags().size(), (size_t)3);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

	auto& m1 = a.context().macros().find(a.context().ids().add("m1"))->second;
	auto& m2 = a.context().macros().find(a.context().ids().add("m2"))->second;
	auto& m3 = a.context().macros().find(a.context().ids().add("m3"))->second;

	{
		std::vector<macro_arg> args;
		args.push_back({ std::make_unique<macro_param_data_single>("2"),nullptr });
		args.push_back({ std::make_unique<macro_param_data_single>("3"),nullptr });
		auto invo = m1->call(std::make_unique<macro_param_data_single>("1"), std::move(args),a.context().ids().add("SYSLIST"));
		auto n = a.context().ids().add("n");
		auto b = a.context().ids().add("b");
		EXPECT_EQ(invo->named_params.find(n)->second->get_value(), "1");
		EXPECT_EQ(invo->named_params.find(b)->second->get_value(), "3");
	}

	{
		std::vector<macro_arg> args;
		args.push_back({ std::make_unique<macro_param_data_single>("1"),nullptr });
		args.push_back({ std::make_unique<macro_param_data_single>("2"),nullptr });
		auto invo = m2->call(nullptr, std::move(args), a.context().ids().add("SYSLIST"));
		auto n = a.context().ids().add("a");
		auto b = a.context().ids().add("b");
		EXPECT_EQ(invo->named_params.find(n)->second->get_value(), "1");
		EXPECT_EQ(invo->named_params.find(b)->second->get_value(), "2");
		EXPECT_EQ(invo->named_params.find(a.context().ids().add("SYSLIST"))->second->get_value(1), "1");
	}

	{
		std::vector<macro_arg> args;
		args.push_back({ std::make_unique<macro_param_data_single>("1"),nullptr });
		args.push_back({ std::make_unique<macro_param_data_single>("2"),nullptr });
		args.push_back({ std::make_unique<macro_param_data_single>("3"),nullptr });
		auto invo = m3->call(nullptr, std::move(args), a.context().ids().add("SYSLIST"));
		auto n = a.context().ids().add("a");
		auto b = a.context().ids().add("b");
		EXPECT_EQ(invo->named_params.find(n)->second->access_keyword_param()->get_value(), "5");
		EXPECT_EQ(invo->named_params.find(b)->second->get_value(), "2");
	}

}

TEST(macro, MEXIT)
{
	std::string input =
R"(
 MACRO
 M1
 LR 1
 MEXIT
 LR 1
 MEND
 
 M1 
)";
	analyzer a(input);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(dynamic_cast<diagnosable*>(&a)->diags().size(), (size_t)1);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}

TEST(macro, cyclic_call_infinite)
{
	std::string input =
R"(
 MACRO
 M1
 LR 1,1
 M1
 MEND
 
 M1 
)";
	analyzer a(input);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(dynamic_cast<diagnosable*>(&a)->diags().size(), (size_t)1);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}

TEST(macro, cyclic_call_finite)
{
	std::string input =
		R"(
 MACRO
 M1
 LR 1
 GBLA V
&V SETA &V+1
 AIF (&V GE 10).A
 M1
.A MEND

 M1
)";
	analyzer a(input);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(dynamic_cast<diagnosable*>(&a)->diags().size(), (size_t)10);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}

TEST(macro, arguments_concatenation)
{
	std::string input =
		R"(
 MACRO
 M1 &X=
 GBLC V
&V SETC '&X'
 MEND

&X SETC 'A'
 M1 X=(B-C)+(&X.-D)

)";
	analyzer a(input);
	a.analyze();
	a.collect_diags();

	auto it = a.context().globals().find(a.context().ids().add("V"));

	ASSERT_NE(it,a.context().globals().end());

	EXPECT_EQ(it->second->access_set_symbol<C_t>()->get_value(),"(B-C)+(A-D)");

	EXPECT_EQ(dynamic_cast<diagnosable*>(&a)->diags().size(), (size_t)0);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}

TEST(macro, arguments_continuation)
{
	std::string input =
		R"(
 MACRO
 M1 &A
 GBLC Q,W
&Q SETC '&A(1)'
&W SETC '&A(2)'
 MEND

 GBLC Q,W
 M1  (X,                REMARK                                         X
               Y)
)";
	analyzer a(input);
	a.analyze();
	a.collect_diags();

	auto Q = a.context().globals().find(a.context().ids().add("Q"));
	auto W = a.context().globals().find(a.context().ids().add("W"));

	ASSERT_NE(Q, a.context().globals().end());
	ASSERT_NE(W, a.context().globals().end());

	EXPECT_EQ(Q->second->access_set_symbol<C_t>()->get_value(), "X");
	EXPECT_EQ(W->second->access_set_symbol<C_t>()->get_value(), "Y");

	EXPECT_EQ(a.diags().size(), (size_t)0);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}

class bad_mock : public parse_lib_provider
{
public:
	bad_mock(int lib_code) 
		: current_content(lib_code == 0 ? &content_bad_name : lib_code == 1 ? &content_bad_begin : &content_comment) {}

	virtual parse_result parse_library(const std::string& library, context::hlasm_context& hlasm_ctx, const library_data data)
	{
		(void)library;

		a = std::make_unique<analyzer>(*current_content, "/tmp/MAC", hlasm_ctx,*this, data);
		a->analyze();
		a->collect_diags();
		return true;
	}
	virtual bool has_library(const std::string&, context::hlasm_context&) const { return true; }
	std::unique_ptr<analyzer> a;
private:
	const std::string* current_content;

	const std::string content_bad_name =
		R"(   MACRO
       MACC   &VAR
       LR    &VAR,&VAR
       MEND
)";
	const std::string content_bad_begin =
		R"(  aMACRO
       MAC   &VAR
       LR    &VAR,&VAR
       MEND
)";
	const std::string content_comment =
		R"(**********  
       MACRO
       MAC   &VAR
       LR    &VAR,&VAR
       MEND
)";


};

TEST(external_macro, bad_library)
{
	std::string input =
		R"(
 MAC
 MAC
)";
	for (int i = 0; i < 2; ++i)
	{
		bad_mock m(i);
		analyzer a(input, "", m);
		a.analyze();
		a.collect_diags();
		EXPECT_EQ(dynamic_cast<diagnosable*>(&*m.a)->diags().size(), (size_t)1);
		EXPECT_EQ(dynamic_cast<diagnosable*>(&a)->diags().size(), (size_t)2);
		EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	}
}

TEST(external_macro, library_with_begin_comment)
{
	std::string input =
		R"(
 MAC 1
 MAC 1
)";
	bad_mock m(2);
	analyzer a(input, "", m);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(dynamic_cast<diagnosable*>(&*m.a)->diags().size(), (size_t)0);
	EXPECT_EQ(dynamic_cast<diagnosable*>(&a)->diags().size(), (size_t)0);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}

TEST(variable_argument_passing, positive_sublist)
{
	auto data = macro_processor::string_to_macrodata("(a,b,c)");

	ASSERT_TRUE(dynamic_cast<macro_param_data_composite*>(data.get()));
	ASSERT_EQ(data->number, (size_t)3);
	EXPECT_EQ(data->get_ith(0)->get_value(), "a");
	EXPECT_EQ(data->get_ith(1)->get_value(), "b");
	EXPECT_EQ(data->get_ith(2)->get_value(), "c");

	data = macro_processor::string_to_macrodata("(a,(b,1),((c),1))");

	ASSERT_TRUE(dynamic_cast<macro_param_data_composite*>(data.get()));
	ASSERT_EQ(data->get_value(), "(a,(b,1),((c),1))");
	ASSERT_EQ(data->number, (size_t)3);
	EXPECT_EQ(data->get_ith(0)->get_value(), "a");
	EXPECT_EQ(data->get_ith(1)->get_value(), "(b,1)");
	EXPECT_EQ(data->get_ith(1)->get_value(), "(b,1)");
	EXPECT_EQ(data->get_ith(2)->get_value(), "((c),1)");
	EXPECT_EQ(data->get_ith(2)->get_ith(0)->get_value(), "(c)");

	data = macro_processor::string_to_macrodata("(a(1),(1,(1))b,()c())");

	ASSERT_TRUE(dynamic_cast<macro_param_data_composite*>(data.get()));
	ASSERT_EQ(data->number, (size_t)3);
	EXPECT_EQ(data->get_ith(0)->get_value(), "a(1)");
	EXPECT_TRUE(dynamic_cast<const macro_param_data_single*>(data->get_ith(0)));
	EXPECT_EQ(data->get_ith(1)->get_value(), "(1,(1))b");
	EXPECT_TRUE(dynamic_cast<const macro_param_data_single*>(data->get_ith(1)));
	EXPECT_EQ(data->get_ith(2)->get_value(), "()c()");
	EXPECT_TRUE(dynamic_cast<const macro_param_data_single*>(data->get_ith(2)));

	data = macro_processor::string_to_macrodata("(0(R2),E,C')',CLI)");

	ASSERT_TRUE(dynamic_cast<macro_param_data_composite*>(data.get()));
	ASSERT_EQ(data->number, (size_t)4);
	EXPECT_EQ(data->get_ith(0)->get_value(), "0(R2)");
	EXPECT_TRUE(dynamic_cast<const macro_param_data_single*>(data->get_ith(0)));
	EXPECT_EQ(data->get_ith(1)->get_value(), "E");
	EXPECT_TRUE(dynamic_cast<const macro_param_data_single*>(data->get_ith(1)));
	EXPECT_EQ(data->get_ith(2)->get_value(), "C')'");
	EXPECT_TRUE(dynamic_cast<const macro_param_data_single*>(data->get_ith(2)));
	EXPECT_EQ(data->get_ith(3)->get_value(), "CLI");
	EXPECT_TRUE(dynamic_cast<const macro_param_data_single*>(data->get_ith(3)));

	data = macro_processor::string_to_macrodata("(DATA1,DATA2,L'DATA3,DATA4)");

	ASSERT_TRUE(dynamic_cast<macro_param_data_composite*>(data.get()));
	ASSERT_EQ(data->number, (size_t)4);
	EXPECT_EQ(data->get_ith(0)->get_value(), "DATA1");
	EXPECT_TRUE(dynamic_cast<const macro_param_data_single*>(data->get_ith(0)));
	EXPECT_EQ(data->get_ith(1)->get_value(), "DATA2");
	EXPECT_TRUE(dynamic_cast<const macro_param_data_single*>(data->get_ith(1)));
	EXPECT_EQ(data->get_ith(2)->get_value(), "L'DATA3");
	EXPECT_TRUE(dynamic_cast<const macro_param_data_single*>(data->get_ith(2)));
	EXPECT_EQ(data->get_ith(3)->get_value(), "DATA4");
	EXPECT_TRUE(dynamic_cast<const macro_param_data_single*>(data->get_ith(3)));
}

TEST(variable_argument_passing, negative_sublist)
{
	auto data = macro_processor::string_to_macrodata("a,b,c");

	ASSERT_TRUE(dynamic_cast<macro_param_data_single*>(data.get()));
	ASSERT_EQ(data->number, (size_t)1);
	EXPECT_EQ(data->get_value(), "a,b,c");

	data = macro_processor::string_to_macrodata("(a,(b,1),((c),1)))");

	ASSERT_TRUE(dynamic_cast<macro_param_data_single*>(data.get()));
	ASSERT_EQ(data->get_value(), "(a,(b,1),((c),1)))");

	data = macro_processor::string_to_macrodata("(a,(b,1),((c),1)()");

	ASSERT_TRUE(dynamic_cast<macro_param_data_single*>(data.get()));
	ASSERT_EQ(data->get_value(), "(a,(b,1),((c),1)()");

	data = macro_processor::string_to_macrodata("=A(((TXXXXXXL+TXXXXXXXXXXXXXn+7)/8)*8)");

	ASSERT_TRUE(dynamic_cast<macro_param_data_single*>(data.get()));
	ASSERT_EQ(data->get_value(), "=A(((TXXXXXXL+TXXXXXXXXXXXXXn+7)/8)*8)");

	data = macro_processor::string_to_macrodata("(a(1)");

	ASSERT_TRUE(dynamic_cast<macro_param_data_single*>(data.get()));
	ASSERT_EQ(data->get_value(), "(a(1)");

	data = macro_processor::string_to_macrodata("(a(1)))");

	ASSERT_TRUE(dynamic_cast<macro_param_data_single*>(data.get()));
	ASSERT_EQ(data->get_value(), "(a(1)))");
}

TEST(macro, parse_args)
{
	std::string input =
		R"(
 macro
 if
 mend

         IF    PXXXXXS+L'PXXXXXS-1,O,X'01',TM
		 if    =d'01'
)";
	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.diags().size(), (size_t)0);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}
