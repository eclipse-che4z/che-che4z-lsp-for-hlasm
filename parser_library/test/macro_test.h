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

	id_index id = a.context()->ids.add("m1");
	auto& macros = a.context()->macros();

	auto tmp = macros.find(id);
	ASSERT_TRUE(tmp !=macros.end());

	auto& m = tmp->second;
	
	auto op = a.context()->ids.add("op");

	EXPECT_EQ(m->named_params().find(op)->second->access_positional_param()->position, (size_t)1);

	auto op2 = a.context()->ids.add("op2");

	EXPECT_EQ(m->named_params().find(op2)->second->access_positional_param()->position, (size_t)2);

	auto l = a.context()->ids.add("l");

	EXPECT_EQ(m->named_params().find(l)->second->access_positional_param()->position, (size_t)0);

	auto k = a.context()->ids.add("k");

	EXPECT_EQ(m->named_params().find(k)->second->access_keyword_param()->get_value(), "5");

	auto k2 = a.context()->ids.add("k2");

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

	ASSERT_EQ(a.context()->macros().size(),(size_t)2);

	id_index id;

	id = a.context()->ids.add("M1");
	EXPECT_TRUE(a.context()->macros().find(id) != a.context()->macros().end());

	id = a.context()->ids.add("m2");
	EXPECT_TRUE(a.context()->macros().find(id) != a.context()->macros().end());
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
 ANOP
 MEND

 MEND

 M2
)";
	analyzer a(input);
	a.analyze();

	ASSERT_EQ(a.context()->macros().size(), (size_t)3);

	id_index id;

	id = a.context()->ids.add("M1");
	EXPECT_TRUE(a.context()->macros().find(id) != a.context()->macros().end());

	id = a.context()->ids.add("M2");
	EXPECT_TRUE(a.context()->macros().find(id) != a.context()->macros().end());

	id = a.context()->ids.add("INNER_M");
	EXPECT_TRUE(a.context()->macros().find(id) != a.context()->macros().end());
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

	ASSERT_EQ(a.context()->macros().size(), (size_t)1);

	id_index id;

	id = a.context()->ids.add("M1");
	EXPECT_TRUE(a.context()->macros().find(id) != a.context()->macros().end());
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

	ASSERT_EQ(a.context()->macros().size(), (size_t)2);

	id_index id;

	id = a.context()->ids.add("M1");
	EXPECT_TRUE(a.context()->macros().find(id) != a.context()->macros().end());

	id = a.context()->ids.add("INNER_M");
	EXPECT_TRUE(a.context()->macros().find(id) != a.context()->macros().end());
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

	auto& m1 = a.context()->macros().find(a.context()->ids.add("m1"))->second;
	auto& m2 = a.context()->macros().find(a.context()->ids.add("m2"))->second;
	auto& m3 = a.context()->macros().find(a.context()->ids.add("m3"))->second;

	{
		std::vector<macro_arg> args;
		args.push_back({ std::make_unique<macro_param_data_single>("2"),nullptr });
		args.push_back({ std::make_unique<macro_param_data_single>("3"),nullptr });
		auto invo = m1->call(std::make_unique<macro_param_data_single>("1"), std::move(args));
		auto n = a.context()->ids.add("n");
		auto b = a.context()->ids.add("b");
		EXPECT_EQ(invo->named_params.find(n)->second->data->get_value(), "1");
		EXPECT_EQ(invo->named_params.find(b)->second->data->get_value(), "3");
	}

	{
		std::vector<macro_arg> args;
		args.push_back({ std::make_unique<macro_param_data_single>("1"),nullptr });
		args.push_back({ std::make_unique<macro_param_data_single>("2"),nullptr });
		auto invo = m2->call(nullptr, std::move(args));
		auto n = a.context()->ids.add("a");
		auto b = a.context()->ids.add("b");
		EXPECT_EQ(invo->named_params.find(n)->second->data->get_value(), "1");
		EXPECT_EQ(invo->named_params.find(b)->second->data->get_value(), "2");
		EXPECT_EQ(invo->SYSLIST(2), "2");
	}

	{
		std::vector<macro_arg> args;
		args.push_back({ std::make_unique<macro_param_data_single>("1"),nullptr });
		args.push_back({ std::make_unique<macro_param_data_single>("2"),nullptr });
		args.push_back({ std::make_unique<macro_param_data_single>("3"),nullptr });
		auto invo = m3->call(nullptr, std::move(args));
		auto n = a.context()->ids.add("a");
		auto b = a.context()->ids.add("b");
		EXPECT_EQ(invo->named_params.find(n)->second->access_keyword_param()->get_value(), "5");
		EXPECT_EQ(invo->named_params.find(b)->second->data->get_value(), "2");
	}

}



#endif
