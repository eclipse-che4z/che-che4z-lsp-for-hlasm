#include "../src/context/hlasm_context.h"

#include <string>

using namespace hlasm_plugin::parser_library::context;
using namespace std;

TEST(context, in_open_code)
{
	hlasm_context ctx;
	ASSERT_FALSE(ctx.is_in_macro());
}


TEST(context_id_storage, add)
{
	hlasm_context ctx;

	ASSERT_FALSE(!ctx.ids().find(""));

	auto it1 = ctx.ids().add("var");
	auto it2 = ctx.ids().find("var");
	auto it3 = ctx.ids().add("var");
	ASSERT_TRUE(it1==it2);
	ASSERT_TRUE(it1 == it3);
}

TEST(context_id_storage, case_insensitive)
{
	hlasm_context ctx;

	auto it1 = ctx.ids().add("var");
	auto it2 = ctx.ids().add("vaR");
	auto it3 = ctx.ids().add("Var");
	ASSERT_TRUE(it1 == it2);
	ASSERT_TRUE(it1 == it3);
}

TEST(context, create_global_var)
{
	hlasm_context ctx;


	auto idx = ctx.ids().add("var");

	auto glob = ctx.create_global_variable<int>(idx, true);

	auto found = ctx.get_var_sym(idx);


	ASSERT_TRUE(glob == found);
	ASSERT_TRUE(glob == ctx.globals().find(idx)->second);
}

TEST(context, create_local_var)
{
	hlasm_context ctx;


	auto idx = ctx.ids().add("var");

	auto loc = ctx.create_local_variable<int>(idx, true);

	auto found = ctx.get_var_sym(idx);


	ASSERT_TRUE(loc == found);
	ASSERT_TRUE(ctx.globals().find(idx) == ctx.globals().end());
}


TEST(context, OPSYN)
{
	hlasm_context ctx;


	auto lr = ctx.ids().add("LR");
	auto mvc = ctx.ids().add("MVC");
	auto st = ctx.ids().add("ST");
	auto mv = ctx.ids().add("MV");

	ctx.add_mnemonic(lr, st);
	EXPECT_EQ(ctx.get_mnemonic_opcode(lr), st);

	ctx.add_mnemonic(mv, lr);
	EXPECT_EQ(ctx.get_mnemonic_opcode(mv), st);

	ctx.remove_mnemonic(lr);
	EXPECT_EQ(ctx.get_mnemonic_opcode(lr), ctx.ids().empty_id);

	EXPECT_EQ(ctx.get_mnemonic_opcode(mvc), mvc);
}

TEST(context_set_vars, set_scalar)
{
	hlasm_context ctx;


	auto idx = ctx.ids().add("var");

	set_symbol<int> var(idx, true);

	EXPECT_EQ(var.get_value(1), 0);

	EXPECT_EQ(var.get_value(),0);

	var.set_value(5);

	EXPECT_EQ(var.get_value(), 5);

	EXPECT_EQ(var.get_value(1), 0);


	set_symbol<string> str_var(idx, true);

	EXPECT_EQ(str_var.get_value(), "");
	EXPECT_EQ(str_var.get_value(1), "");
}

TEST(context_set_vars, set_non_scalar)
{
	hlasm_context ctx;


	auto idx = ctx.ids().add("var");

	set_symbol<string> var(idx, false);

	EXPECT_EQ(var.get_value(), "");

	var.set_value("dac", 10);

	EXPECT_EQ(var.get_value(10), "dac");
	EXPECT_EQ(var.get_value(9), "");
	EXPECT_EQ(var.get_value(11), "");
	EXPECT_EQ(var.get_value(5), "");

	var.set_value("dac8", 8);
	var.set_value("dac10", 10);

	EXPECT_EQ(var.get_value(10), "dac10");
	EXPECT_EQ(var.get_value(8), "dac8");

	var.set_value("daco", 1000);
	EXPECT_EQ(var.get_value(1000), "daco");

	var.set_value("", 1000);
	EXPECT_EQ(var.get_value(1000), "");
}


TEST(context_macro_param, param_data)
{
	hlasm_context ctx;

	//creating macro param data
	auto s0 = make_unique< macro_param_data_single>("first");
	auto s1 = make_unique< macro_param_data_single>("second");
	auto s2 = make_unique< macro_param_data_single>("third");

	//asserting behaviour of data classes
	ASSERT_TRUE(s0->get_value() == "first");
	EXPECT_EQ(s1->get_ith(0)->get_value(), s1->get_value());
	EXPECT_EQ(s1->get_ith(0)->get_ith(0)->get_value(), s1->get_value());
	EXPECT_EQ(s1->get_ith(1)->get_value(), "");
	EXPECT_EQ(s1->get_ith(5)->get_ith(3)->get_value(), "");

	//creating composite data
	vector<macro_data_ptr> v;
	v.push_back(move(s0));
	v.push_back(move(s1));
	v.push_back(move(s2));
	macro_param_data_composite c(move(v));
	macro_param_data_component* ptr(&c);

	EXPECT_EQ(ptr->get_value(), "(first,second,third)");
}

//testing more complex composite data
TEST(context_macro_param, param_data_composite)
{
	vector<macro_data_ptr> v;
	v.push_back(unique_ptr<macro_param_data_component>(make_unique < macro_param_data_single>("first")));
	v.push_back(unique_ptr<macro_param_data_component>(make_unique < macro_param_data_single>("second")));

	vector<macro_data_ptr> v2;
	v2.push_back(unique_ptr<macro_param_data_component>(make_unique < macro_param_data_single>("second")));
	v2.push_back(unique_ptr<macro_param_data_component>(make_unique < macro_param_data_single>("third")));

	vector<macro_data_ptr> v3;
	v3.push_back(unique_ptr<macro_param_data_component>(make_unique < macro_param_data_single>("third")));

	macro_data_ptr c1 (make_unique< macro_param_data_composite>(move(v)));
	macro_data_ptr c2 (make_unique< macro_param_data_composite>(move(v2)));
	macro_data_ptr c3 (make_unique< macro_param_data_composite>(move(v3)));

	vector<macro_data_ptr> v4;
	v4.push_back(move(c1));
	v4.push_back(move(c2));
	v4.push_back(move(c3));

	macro_param_data_composite  c(move(v4));

	EXPECT_EQ(c.get_value(),"((first,second),(second,third),(third))");
	EXPECT_EQ(c.get_ith(0)->get_value(), "(first,second)");
	EXPECT_EQ(c.get_ith(3)->get_value(), "");
	EXPECT_EQ(c.get_ith(2)->get_value(), "(third)");
	EXPECT_EQ(c.get_ith(2)->get_ith(0)->get_value(), "third");
}

//test of adding macro to context class
TEST(context_macro, add_macro)
{
	hlasm_context ctx;


	//creating names of params
	auto idx = ctx.ids().add("mac");
	auto lbl = ctx.ids().add("lbl");
	auto key = ctx.ids().add("key");
	auto op1 = ctx.ids().add("op1");
	auto op3 = ctx.ids().add("op3");

	//creating data of params
	macro_data_ptr p1(make_unique< macro_param_data_single>(""));

	//creating vector of params for add_macro
	macro_arg a(move(p1), key);
	vector<macro_arg> args;
	args.push_back(move(a));
	args.push_back({ nullptr,op1 });
	args.push_back({ nullptr });
	args.push_back({ nullptr,op3 });

	//prototype->|&LBL		MAC		&KEY=,&OP1,,&OP3
	auto& m = ctx.add_macro(idx, lbl, move(args), {}, {}, {});

	EXPECT_EQ(m.named_params().size(), (size_t)4);
	EXPECT_NE(m.named_params().find(key), m.named_params().end());
	EXPECT_NE(m.named_params().find(op1), m.named_params().end());
	EXPECT_NE(m.named_params().find(op3), m.named_params().end());
	EXPECT_NE(m.named_params().find(lbl), m.named_params().end());
}

TEST(context_macro, call_and_leave_macro)
{
	hlasm_context ctx;


	//creating names of params
	auto idx = ctx.ids().add("mac");
	auto key = ctx.ids().add("key");
	auto op1 = ctx.ids().add("op1");
	auto op3 = ctx.ids().add("op3");

	//creating data of params
	macro_data_ptr p1(make_unique< macro_param_data_single>(""));

	//creating vector of params for add_macro
	macro_arg a(move(p1), key);
	vector<macro_arg> args;
	args.push_back(move(a));
	args.push_back({ nullptr,op1 });
	args.push_back({ nullptr });
	args.push_back({ nullptr,op3 });

	//prototype->|		MAC		&KEY=,&OP1,,&OP3
	auto& m = ctx.add_macro(idx, nullptr, move(args), {}, {}, {});

	//creating param data
	macro_data_ptr p2(make_unique < macro_param_data_single>("ada"));
	macro_data_ptr p3(make_unique < macro_param_data_single>("mko"));
	macro_data_ptr p4(make_unique < macro_param_data_single>(""));

	//creating vector of param data for entering macro
	vector<macro_arg> params;
	params.push_back({ move(p2) });
	params.push_back({ move(p3) });
	params.push_back({ move(p4) });

	//call->|		MAC		ada,mko,
	auto m2 = ctx.enter_macro(idx,nullptr, move(params));

	ASSERT_TRUE(m.id == m2->id);
	ASSERT_TRUE(ctx.is_in_macro());
	ASSERT_TRUE(ctx.this_macro()==m2);

	//testing syslist
	EXPECT_EQ(m2->SYSLIST(0), "");
	EXPECT_EQ(m2->SYSLIST(1),"ada");
	EXPECT_EQ(m2->SYSLIST(2), "mko");
	EXPECT_EQ(m2->SYSLIST(3), "");

	//testing named params
	EXPECT_EQ(m2->named_params.find(op1)->second->get_value(), "ada");
	EXPECT_EQ(m2->named_params.find(op3)->second->get_value(), "");
	EXPECT_EQ(m2->named_params.find(key)->second->get_value(), "");

	ctx.leave_macro();

	ASSERT_FALSE(ctx.is_in_macro());
}

TEST(context_macro, repeat_call_same_macro)
{
	hlasm_context ctx;


	//creating names of params
	auto idx = ctx.ids().add("mac");
	auto key = ctx.ids().add("key");
	auto op1 = ctx.ids().add("op1");
	auto op3 = ctx.ids().add("op3");
	auto lbl = ctx.ids().add("lbl");

	//creating data of params
	macro_data_ptr p1(make_unique< macro_param_data_single>(""));

	//creating vector of params for add_macro
	macro_arg a(move(p1), key);
	vector<macro_arg> args;
	args.push_back(move(a));
	args.push_back({ nullptr,op1 });
	args.push_back({ nullptr });
	args.push_back({ nullptr,op3 });

	//prototype->|&LBL		MAC		&KEY=,&OP1,,&OP3
	ctx.add_macro(idx, lbl, move(args), {}, {}, {});

	//creating param data
	macro_data_ptr lb(make_unique < macro_param_data_single>("lbl"));
	macro_data_ptr p2(make_unique < macro_param_data_single>("ada"));
	macro_data_ptr p3(make_unique < macro_param_data_single>("mko"));
	macro_data_ptr p4(make_unique < macro_param_data_single>(""));

	//creating vector of param data for entering macro
	vector<macro_arg> params;
	params.push_back({ move(p2) });
	params.push_back({ move(p3) });
	params.push_back({ move(p4) });

	//calling macro
	//call->|lbl		MAC		ada,mko,
	auto m2 = ctx.enter_macro(idx, move(lb), move(params));

	EXPECT_EQ(m2->named_params.find(lbl)->second->data->get_value(), "lbl");

	//leaving macro
	ctx.leave_macro();
	params.clear();

	//creating data for another macro call
	macro_data_ptr np2(make_unique < macro_param_data_single>(""));
	macro_data_ptr np3(make_unique < macro_param_data_single>(""));
	macro_data_ptr np4(make_unique < macro_param_data_single>("cas"));

	//creating data for one complex macro param
	auto s0 = make_unique < macro_param_data_single>("first");
	auto s1 = make_unique < macro_param_data_single>("second");
	auto s2 = make_unique < macro_param_data_single>("third");
	vector<macro_data_ptr> v;
	v.push_back(move(s0));
	v.push_back(move(s1));
	v.push_back(move(s2));
	macro_data_ptr dat(make_unique< macro_param_data_composite>(move(v)));

	//creating another vector for macro call
	params.push_back({ move(np2) });
	params.push_back({ move(np4), key });
	params.push_back({ move(np3) });
	params.push_back({ move(dat) });

	//call->|		MAC		,KEY=cas,,(first,second,third)
	auto m3 = ctx.enter_macro(idx, nullptr, move(params));

	ASSERT_TRUE(m2 != m3);

	for (size_t i = 0; i < 3; i++)
	{
		EXPECT_EQ(m3->SYSLIST(i), "");
	}

	EXPECT_EQ(m3->named_params.find(lbl)->second->get_value(), "");
	EXPECT_EQ(m3->named_params.find(op1)->second->get_value(), "");
	EXPECT_EQ(m3->named_params.find(op3)->second->get_value(), "(first,second,third)");
	EXPECT_EQ(m3->named_params.find(key)->second->get_value(), "cas");

	EXPECT_EQ(m3->SYSLIST({2,2}), "");
	EXPECT_EQ(m3->SYSLIST({ 3 }), "(first,second,third)");
	EXPECT_EQ(m3->SYSLIST({ 3,1 }), "second");
	EXPECT_EQ(m3->SYSLIST({ 3,1,0,0 }), "second");
	EXPECT_EQ(m3->SYSLIST({ 3,1,0,0,1 }), "");
}

TEST(context_macro, recurr_call)
{
	hlasm_context ctx;


	//creating names of params
	auto idx = ctx.ids().add("mac");
	auto key = ctx.ids().add("key");
	auto op1 = ctx.ids().add("op1");
	auto op3 = ctx.ids().add("op3");
	auto lbl = ctx.ids().add("lbl");

	//creating data of params
	macro_data_ptr p1(make_unique< macro_param_data_single>(""));

	//creating vector of params for add_macro
	macro_arg a(move(p1), key);
	vector<macro_arg> args;
	args.push_back(move(a));
	args.push_back({ nullptr,op1 });
	args.push_back({ nullptr });
	args.push_back({ nullptr,op3 });

	//prototype->|&LBL		MAC		&KEY=,&OP1,,&OP3
	ctx.add_macro(idx, lbl, move(args), {}, {}, {});

	//creating param data
	macro_data_ptr lb(make_unique < macro_param_data_single>("lbl"));
	macro_data_ptr p2(make_unique < macro_param_data_single>("ada"));
	macro_data_ptr p3(make_unique < macro_param_data_single>("mko"));
	macro_data_ptr p4(make_unique < macro_param_data_single>(""));
	macro_data_ptr p5(make_unique < macro_param_data_single>("as"));

	//creating vector of param data for entering macro
	vector<macro_arg> params;
	params.push_back({ move(p2) });
	params.push_back({ move(p3) });
	params.push_back({ move(p4) });

	//calling macro
	//call->|lbl		MAC		ada,mko,
	auto m2 = ctx.enter_macro(idx, move(lb), move(params));

	//*****created first macro call

	ASSERT_TRUE(ctx.this_macro() == m2);
	ASSERT_TRUE(ctx.is_in_macro());

	params.clear();

	//creating data for another macro call
	macro_data_ptr np2(make_unique < macro_param_data_single>(""));
	macro_data_ptr np3(make_unique < macro_param_data_single>(""));
	macro_data_ptr np4(make_unique < macro_param_data_single>("cas"));

	//creating data for one complex macro param
	auto s0 = make_unique < macro_param_data_single>("first");
	auto s1 = make_unique < macro_param_data_single>("second");
	auto s2 = make_unique < macro_param_data_single>("third");
	vector<macro_data_ptr> v;
	v.push_back(move(s0));
	v.push_back(move(s1));
	v.push_back(move(s2));
	macro_data_ptr dat(make_unique< macro_param_data_composite>(move(v)));

	//creating another vector for macro call
	params.push_back({ move(np2) });
	params.push_back({ move(np4), key });
	params.push_back({ move(np3) });
	params.push_back({ move(dat) });

	//call->|		MAC		,KEY=cas,,(first,second,third)
	auto m3 = ctx.enter_macro(idx, nullptr, move(params));
	
	//********called again the same macro without calling leave
	ASSERT_TRUE(ctx.this_macro() == m3);
	ASSERT_TRUE(ctx.is_in_macro());
	ASSERT_FALSE(m2 == m3);

	for (size_t i = 0; i < 2; i++)
	{
		EXPECT_EQ(m3->SYSLIST(i), "");
	}

	//testing inner macro
	EXPECT_EQ(m3->named_params.find(lbl)->second->get_value(), "");
	EXPECT_EQ(m3->named_params.find(op1)->second->get_value(), "");
	EXPECT_EQ(m3->named_params.find(op3)->second->get_value(), "(first,second,third)");
	EXPECT_EQ(m3->named_params.find(key)->second->get_value(), "cas");

	EXPECT_EQ(m3->SYSLIST(0), "");
	EXPECT_EQ(m3->SYSLIST({ 2,2 }), "");
	EXPECT_EQ(m3->SYSLIST({ 3 }), "(first,second,third)");
	EXPECT_EQ(m3->SYSLIST({ 3,1 }), "second");
	EXPECT_EQ(m3->SYSLIST({ 3,1,0,0 }), "second");
	EXPECT_EQ(m3->SYSLIST({ 3,1,0,0,1 }), "");

	//testing outer macro
	EXPECT_EQ(m2->SYSLIST(0), "lbl");
	EXPECT_EQ(m2->SYSLIST(1), "ada");
	EXPECT_EQ(m2->SYSLIST(2), "mko");
	EXPECT_EQ(m2->SYSLIST(3), "");

	EXPECT_EQ(m2->named_params.find(op1)->second->get_value(), "ada");
	EXPECT_EQ(m2->named_params.find(op3)->second->get_value(), "");
	EXPECT_EQ(m2->named_params.find(key)->second->get_value(), "");


	ctx.leave_macro();
	ASSERT_TRUE(ctx.is_in_macro());
	ASSERT_TRUE(ctx.this_macro() == m2);

	ctx.leave_macro();
	ASSERT_FALSE(ctx.is_in_macro());
}