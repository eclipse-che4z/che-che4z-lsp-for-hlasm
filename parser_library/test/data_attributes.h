#include "common_testing.h"


TEST(data_attributes, N)
{
	hlasm_context ctx;


	auto id = ctx.ids().add("id");
	auto id2 = ctx.ids().add("id2");

	macro_data_ptr data = std::make_unique < macro_param_data_single>("ada");
	EXPECT_EQ(positional_param(id, 0, *data).number(), 1);
	data = std::make_unique < macro_param_data_dummy>();
	EXPECT_EQ(positional_param(id, 0, *data).number(), 0);


	auto s0 = std::make_unique < macro_param_data_single>("first");
	auto s1 = std::make_unique < macro_param_data_single>("second");
	auto s2 = std::make_unique < macro_param_data_single>("third");
	std::vector<macro_data_ptr> v;
	v.push_back(move(s0));
	v.push_back(move(s1));
	v.push_back(move(s2));

	EXPECT_EQ(keyword_param(id, std::make_unique < macro_param_data_dummy>(), nullptr).number(), 0);
	data = std::make_unique< macro_param_data_composite>(move(v));
	auto kp = keyword_param(id, std::make_unique < macro_param_data_dummy>(), std::move(data));
	EXPECT_EQ(kp.number(), 3);
	EXPECT_EQ(kp.number({1}), 1);
	EXPECT_EQ(kp.number({4}), 0);

	auto var = ctx.create_local_variable<A_t>(id, true)->access_set_symbol<A_t>();

	EXPECT_EQ(var->number(), 0);
	EXPECT_EQ(var->number({ 1 }), 0);

	var->set_value(1);
	EXPECT_EQ(var->number(), 0);
	EXPECT_EQ(var->number({ 1 }), 0);

	auto var_ns = ctx.create_local_variable<A_t>(id2, false)->access_set_symbol<A_t>();

	EXPECT_EQ(var_ns->number(), 0);
	EXPECT_EQ(var_ns->number({ 1 }), 0);

	var_ns->set_value(1, 0);
	EXPECT_EQ(var_ns->number(), 1);
	EXPECT_EQ(var_ns->number({ 1 }), 1);

	var_ns->set_value(1, 14);
	EXPECT_EQ(var_ns->number(), 15);
	EXPECT_EQ(var_ns->number({ 1 }), 15);

}

TEST(data_attributes, K)
{
	hlasm_context ctx;


	auto idA = ctx.ids().add("id1");
	auto idB = ctx.ids().add("id2");
	auto idC = ctx.ids().add("id3");

	macro_data_ptr data = std::make_unique < macro_param_data_single>("ada");
	EXPECT_EQ(positional_param(idA, 0, *data).count(), 3);
	data = std::make_unique < macro_param_data_dummy>();
	EXPECT_EQ(positional_param(idA, 0, *data).count(), 0);


	auto s0 = std::make_unique < macro_param_data_single>("first");
	auto s1 = std::make_unique < macro_param_data_single>("second");
	auto s2 = std::make_unique < macro_param_data_single>("third");
	std::vector<macro_data_ptr> v;
	v.push_back(move(s0));
	v.push_back(move(s1));
	v.push_back(move(s2));

	EXPECT_EQ(keyword_param(idA, std::make_unique < macro_param_data_dummy>(), nullptr).count(), 0);
	auto kp = keyword_param(idA, std::make_unique < macro_param_data_dummy>(), std::make_unique< macro_param_data_composite>(move(v)));
	EXPECT_EQ(kp.count(), 20);
	EXPECT_EQ(kp.count({ 1 }), 6);
	EXPECT_EQ(kp.count({ 4 }), 0);

	auto varA = ctx.create_local_variable<A_t>(idA, true)->access_set_symbol<A_t>();
	auto varB = ctx.create_local_variable<B_t>(idB, true)->access_set_symbol<B_t>();
	auto varC = ctx.create_local_variable<C_t>(idC, true)->access_set_symbol<C_t>();

	EXPECT_EQ(varA->count(), 1);
	EXPECT_EQ(varA->count({ 1 }), 1);
	EXPECT_EQ(varB->count(), 1);
	EXPECT_EQ(varB->count({ 1 }), 1);
	EXPECT_EQ(varC->count(), 0);
	EXPECT_EQ(varC->count({ 1 }), 0);

	varA->set_value(1);
	EXPECT_EQ(varA->count(), 1);
	varA->set_value(10);
	EXPECT_EQ(varA->count(), 2);
	varA->set_value(-10);
	EXPECT_EQ(varA->count(), 3);

}
