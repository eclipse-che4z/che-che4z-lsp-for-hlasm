#ifndef HLASMPLUGIN_PARSERLIBARY_MACROTEST_H
#define HLASMPLUGIN_PARSERLIBARY_MACROTEST_H
#include "common_testing.h"

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

	EXPECT_EQ(m->definition.size(), (size_t)4);
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
		EXPECT_EQ(invo->named_params.find(a.context().ids().add("SYSLIST"))->second->get_value(1), "2");
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

class bad_mock : public parse_lib_provider
{
public:
	bad_mock(bool bad_name) : current_content(bad_name ? &content_bad_name : &content_bad_begin) {}

	virtual parse_result parse_library(const std::string& library, context::hlasm_context& hlasm_ctx, const library_data data)
	{
		a = std::make_unique<analyzer>(*current_content, "/tmp/MAC", hlasm_ctx,*this, data);
		a->analyze();
		a->collect_diags();
		return true;
	}
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


};

TEST(external_macro, bad_library)
{
	std::string input =
		R"(
 MAC
 MAC
)";
	bad_mock m1(true);
	analyzer a1(input,"",m1);
	a1.analyze();
	a1.collect_diags();
	EXPECT_EQ(dynamic_cast<diagnosable*>(&*m1.a)->diags().size(), (size_t)1);
	EXPECT_EQ(dynamic_cast<diagnosable*>(&a1)->diags().size(), (size_t)0);
	EXPECT_EQ(a1.parser().getNumberOfSyntaxErrors(), (size_t)0);

	bad_mock m2(false);
	analyzer a2(input, "", m2);
	a2.analyze();
	a2.collect_diags();
	EXPECT_EQ(dynamic_cast<diagnosable*>(&*m2.a)->diags().size(), (size_t)1);
	EXPECT_EQ(dynamic_cast<diagnosable*>(&a2)->diags().size(), (size_t)0);
	EXPECT_EQ(a2.parser().getNumberOfSyntaxErrors(), (size_t)0);
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

	data = macro_processor::string_to_macrodata("=A(((TDXENTPL+TBXT001EntryLen+7)/8)*8)");

	ASSERT_TRUE(dynamic_cast<macro_param_data_single*>(data.get()));
	ASSERT_EQ(data->get_value(), "=A(((TDXENTPL+TBXT001EntryLen+7)/8)*8)");

	data = macro_processor::string_to_macrodata("(a(1)");

	ASSERT_TRUE(dynamic_cast<macro_param_data_single*>(data.get()));
	ASSERT_EQ(data->get_value(), "(a(1)");

	data = macro_processor::string_to_macrodata("(a(1)))");

	ASSERT_TRUE(dynamic_cast<macro_param_data_single*>(data.get()));
	ASSERT_EQ(data->get_value(), "(a(1)))");
}

#endif
