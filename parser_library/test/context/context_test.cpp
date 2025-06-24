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

#include <array>
#include <string>

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "analyzer.h"
#include "context/hlasm_context.h"
#include "context/id_storage.h"
#include "context/variables/set_symbol.h"
#include "context/variables/system_variable.h"
#include "context/well_known.h"

// tests for hlasm_ctx class:
// id_storage
// variable creation and assignment
// mnemonics (OPSYN)
// macro invocations

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;

TEST(context, in_open_code)
{
    hlasm_context ctx;
    ASSERT_FALSE(ctx.is_in_macro());
}


TEST(context_id_storage, add)
{
    hlasm_context ctx;

    ASSERT_TRUE(ctx.find_id("").has_value());

    auto it1 = ctx.add_id(std::string_view("var"));
    auto it2 = ctx.find_id("var");
    auto it3 = ctx.add_id(std::string_view("var"));
    ASSERT_TRUE(it1 == it2);
    ASSERT_TRUE(it1 == it3);
}

TEST(context_id_storage, case_insensitive)
{
    hlasm_context ctx;

    auto it1 = ctx.add_id(std::string_view("var"));
    auto it2 = ctx.add_id(std::string_view("vaR"));
    auto it3 = ctx.add_id(std::string_view("Var"));
    ASSERT_TRUE(it1 == it2);
    ASSERT_TRUE(it1 == it3);
}

TEST(context, create_global_var)
{
    hlasm_context ctx;
    diagnostic_op_consumer_container diag;

    auto idx = ctx.add_id(std::string_view("var"));

    auto glob = ctx.create_global_variable<C_t>(idx, true);

    auto found = ctx.get_var_sym(idx);

    ASSERT_TRUE(found);
    EXPECT_TRUE(glob == found);

    EXPECT_TRUE(ctx.globals().find(idx) != ctx.globals().end());
    EXPECT_TRUE(glob
        == std::visit([](const auto& e) -> const set_symbol_base* { return &e; }, ctx.globals().find(idx)->second));
    EXPECT_TRUE(dynamic_cast<set_symbol<C_t>*>(glob));
}

TEST(context, create_global_var_different_types)
{
    hlasm_context ctx;
    diagnostic_op_consumer_container diag;

    auto idx = ctx.add_id(std::string_view("var"));
    auto glob_a = ctx.create_global_variable<A_t>(idx, true);
    auto glob_b = ctx.create_global_variable<B_t>(idx, true);
    auto found = ctx.get_var_sym(idx);

    ASSERT_TRUE(found);
    EXPECT_TRUE(glob_a == found);
    EXPECT_FALSE(glob_b);

    EXPECT_NE(ctx.globals().find(idx), ctx.globals().end());
    EXPECT_TRUE(glob_a
        == std::visit([](const auto& e) -> const set_symbol_base* { return &e; }, ctx.globals().find(idx)->second));
    EXPECT_TRUE(dynamic_cast<set_symbol<A_t>*>(glob_a));
}

TEST(context, find_system_var)
{
    hlasm_context ctx;


    auto idx = ctx.find_id("SYSDATC");

    ASSERT_TRUE(idx.has_value());

    auto scope_var_ptr = ctx.get_var_sym(idx.value());


    EXPECT_TRUE(scope_var_ptr);
    EXPECT_EQ(ctx.globals().find(idx.value()), ctx.globals().end());
}

TEST(context, create_local_var)
{
    hlasm_context ctx;


    auto idx = ctx.add_id(std::string_view("var"));

    auto loc = ctx.create_local_variable<int>(idx, true);

    auto found = ctx.get_var_sym(idx);


    EXPECT_EQ(loc, found);
    EXPECT_EQ(ctx.globals().find(idx), ctx.globals().end());
}


TEST(context, OPSYN)
{
    hlasm_context ctx;


    auto lr = ctx.add_id(std::string_view("LR"));
    auto mvc = ctx.add_id(std::string_view("MVC"));
    auto st = ctx.add_id(std::string_view("ST"));
    auto mv = ctx.add_id(std::string_view("MV"));

    EXPECT_TRUE(ctx.add_mnemonic(lr, st));
    EXPECT_EQ(ctx.get_operation_code(lr).opcode, st);

    EXPECT_TRUE(ctx.add_mnemonic(mv, lr));
    EXPECT_EQ(ctx.get_operation_code(mv).opcode, st);

    EXPECT_TRUE(ctx.remove_mnemonic(lr));
    EXPECT_EQ(ctx.get_operation_code(lr).opcode, id_index());

    EXPECT_EQ(ctx.get_operation_code(mvc).opcode, mvc);
}

TEST(context_set_vars, set_scalar)
{
    hlasm_context ctx;


    auto idx = ctx.add_id(std::string_view("var"));

    set_symbol<int> var(idx, true);

    EXPECT_EQ(var.get_value(1), 0);

    EXPECT_EQ(var.get_value(), 0);

    var.set_value(5);

    EXPECT_EQ(var.get_value(), 5);

    EXPECT_EQ(var.get_value(1), 0);


    set_symbol<std::string> str_var(idx, true);

    EXPECT_EQ(str_var.get_value(), "");
    EXPECT_EQ(str_var.get_value(1), "");
}

TEST(context_set_vars, set_non_scalar)
{
    hlasm_context ctx;


    auto idx = ctx.add_id(std::string_view("var"));

    set_symbol<std::string> var(idx, false);

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

    // creating macro param data
    auto s0 = std::make_unique<macro_param_data_single>("first");
    auto s1 = std::make_unique<macro_param_data_single>("second");
    auto s2 = std::make_unique<macro_param_data_single>("third");

    // asserting behaviour of data classes
    ASSERT_TRUE(s0->get_value() == "first");
    EXPECT_EQ(s1->get_ith(1)->get_value(), s1->get_value());
    EXPECT_EQ(s1->get_ith(1)->get_ith(1)->get_value(), s1->get_value());
    EXPECT_EQ(s1->get_ith(2)->get_value(), "");
    EXPECT_EQ(s1->get_ith(6)->get_ith(4)->get_value(), "");

    // creating composite data
    std::vector<macro_data_ptr> v;
    v.push_back(std::move(s0));
    v.push_back(std::move(s1));
    v.push_back(std::move(s2));
    macro_param_data_composite c(std::move(v));
    macro_param_data_component* ptr(&c);

    EXPECT_EQ(ptr->get_value(), "(first,second,third)");
}

TEST(context_macro_param, param_data_composite_empty)
{
    std::vector<macro_data_ptr> v;
    macro_param_data_composite c(std::move(v));

    EXPECT_EQ(c.get_value(), "()");
}

// testing more complex composite data
TEST(context_macro_param, param_data_composite)
{
    std::vector<macro_data_ptr> v;
    v.push_back(std::make_unique<macro_param_data_single>("first"));
    v.push_back(std::make_unique<macro_param_data_single>("second"));

    std::vector<macro_data_ptr> v2;
    v2.push_back(std::make_unique<macro_param_data_single>("second"));
    v2.push_back(std::make_unique<macro_param_data_single>("third"));

    std::vector<macro_data_ptr> v3;
    v3.push_back(std::make_unique<macro_param_data_single>("third"));

    macro_data_ptr c1(std::make_unique<macro_param_data_composite>(std::move(v)));
    macro_data_ptr c2(std::make_unique<macro_param_data_composite>(std::move(v2)));
    macro_data_ptr c3(std::make_unique<macro_param_data_composite>(std::move(v3)));

    std::vector<macro_data_ptr> v4;
    v4.push_back(std::move(c1));
    v4.push_back(std::move(c2));
    v4.push_back(std::move(c3));

    macro_param_data_composite c(std::move(v4));

    EXPECT_EQ(c.get_value(), "((first,second),(second,third),(third))");
    EXPECT_EQ(c.get_ith(1)->get_value(), "(first,second)");
    EXPECT_EQ(c.get_ith(4)->get_value(), "");
    EXPECT_EQ(c.get_ith(3)->get_value(), "(third)");
    EXPECT_EQ(c.get_ith(3)->get_ith(1)->get_value(), "third");
}

// test of adding macro to context class
TEST(context_macro, add_macro)
{
    hlasm_context ctx;


    // creating names of params
    auto idx = ctx.add_id(std::string_view("mac"));
    auto lbl = ctx.add_id(std::string_view("lbl"));
    auto key = ctx.add_id(std::string_view("key"));
    auto op1 = ctx.add_id(std::string_view("op1"));
    auto op3 = ctx.add_id(std::string_view("op3"));

    // creating data of params
    macro_data_ptr p1(std::make_unique<macro_param_data_single>(""));

    // creating vector of params for add_macro
    macro_arg a(std::move(p1), key);
    std::vector<macro_arg> args;
    args.push_back(std::move(a));
    args.emplace_back(nullptr, op1);
    args.emplace_back(nullptr);
    args.emplace_back(nullptr, op3);

    // prototype->|&LBL        MAC        &KEY=,&OP1,,&OP3
    auto& m = *ctx.add_macro(idx, lbl, std::move(args), {}, {}, {}, {}, {}, false);

    EXPECT_EQ(m.named_params().size(), (size_t)4);
    EXPECT_NE(m.named_params().find(key), m.named_params().end());
    EXPECT_NE(m.named_params().find(op1), m.named_params().end());
    EXPECT_NE(m.named_params().find(op3), m.named_params().end());
    EXPECT_NE(m.named_params().find(lbl), m.named_params().end());
}

TEST(context_macro, call_and_leave_macro)
{
    hlasm_context ctx;


    // creating names of params
    auto idx = ctx.add_id(std::string_view("mac"));
    auto key = ctx.add_id(std::string_view("key"));
    auto op1 = ctx.add_id(std::string_view("op1"));
    auto op3 = ctx.add_id(std::string_view("op3"));

    // creating data of params
    macro_data_ptr p1(std::make_unique<macro_param_data_single>(""));

    // creating vector of params for add_macro
    macro_arg a(std::move(p1), key);
    std::vector<macro_arg> args;
    args.push_back(std::move(a));
    args.emplace_back(nullptr, op1);
    args.emplace_back(nullptr);
    args.emplace_back(nullptr, op3);

    // prototype->|        MAC        &KEY=,&OP1,,&OP3
    auto m = ctx.add_macro(idx, context::id_index(), std::move(args), {}, {}, {}, {}, {}, false);

    // creating param data
    macro_data_ptr p2(std::make_unique<macro_param_data_single>("ada"));
    macro_data_ptr p3(std::make_unique<macro_param_data_single>("mko"));
    macro_data_ptr p4(std::make_unique<macro_param_data_single>(""));

    // creating vector of param data for entering macro
    std::vector<macro_arg> params;
    params.emplace_back(std::move(p2));
    params.emplace_back(std::move(p3));
    params.emplace_back(std::move(p4));

    // call->|        MAC        ada,mko,
    auto [m2, t2] = ctx.enter_macro(m.get(), nullptr, std::move(params));

    EXPECT_FALSE(t2);
    ASSERT_TRUE(m->id == m2->id);
    ASSERT_TRUE(ctx.is_in_macro());
    ASSERT_TRUE(ctx.current_macro() == m2);

    auto SYSLIST = m2->named_params.find(well_known::SYSLIST)->second->access_system_variable();
    ASSERT_TRUE(SYSLIST);
    // testing syslist
    EXPECT_EQ(SYSLIST->get_value((size_t)0), "");
    EXPECT_EQ(SYSLIST->get_value((size_t)1), "ada");
    EXPECT_EQ(SYSLIST->get_value((size_t)2), "mko");
    EXPECT_EQ(SYSLIST->get_value((size_t)3), "");

    // testing named params
    EXPECT_EQ(m2->named_params.find(op1)->second->get_value(), "ada");
    EXPECT_EQ(m2->named_params.find(op3)->second->get_value(), "");
    EXPECT_EQ(m2->named_params.find(key)->second->get_value(), "");

    ctx.leave_macro();

    ASSERT_FALSE(ctx.is_in_macro());
}

TEST(context_macro, repeat_call_same_macro)
{
    hlasm_context ctx;


    // creating names of params
    auto idx = ctx.add_id(std::string_view("mac"));
    auto key = ctx.add_id(std::string_view("key"));
    auto op1 = ctx.add_id(std::string_view("op1"));
    auto op3 = ctx.add_id(std::string_view("op3"));
    auto lbl = ctx.add_id(std::string_view("lbl"));

    // creating data of params
    macro_data_ptr p1(std::make_unique<macro_param_data_single>(""));

    // creating vector of params for add_macro
    macro_arg a(std::move(p1), key);
    std::vector<macro_arg> args;
    args.push_back(std::move(a));
    args.emplace_back(nullptr, op1);
    args.emplace_back(nullptr);
    args.emplace_back(nullptr, op3);

    // prototype->|&LBL        MAC        &KEY=,&OP1,,&OP3
    auto m = ctx.add_macro(idx, lbl, std::move(args), {}, {}, {}, {}, {}, false);

    // creating param data
    macro_data_ptr lb(std::make_unique<macro_param_data_single>("lbl"));
    macro_data_ptr p2(std::make_unique<macro_param_data_single>("ada"));
    macro_data_ptr p3(std::make_unique<macro_param_data_single>("mko"));
    macro_data_ptr p4(std::make_unique<macro_param_data_single>(""));

    // creating vector of param data for entering macro
    std::vector<macro_arg> params;
    params.emplace_back(std::move(p2));
    params.emplace_back(std::move(p3));
    params.emplace_back(std::move(p4));

    // calling macro
    // call->|lbl        MAC        ada,mko,
    auto [m2, t2] = ctx.enter_macro(m.get(), std::move(lb), std::move(params));

    EXPECT_FALSE(t2);
    EXPECT_EQ(m2->named_params.find(lbl)->second->get_value(), "lbl");

    // leaving macro
    ctx.leave_macro();
    params.clear();

    // creating data for another macro call
    macro_data_ptr np2(std::make_unique<macro_param_data_single>(""));
    macro_data_ptr np3(std::make_unique<macro_param_data_single>(""));
    macro_data_ptr np4(std::make_unique<macro_param_data_single>("cas"));

    // creating data for one complex macro param
    std::vector<macro_data_ptr> v;
    v.push_back(std::make_unique<macro_param_data_single>("first"));
    v.push_back(std::make_unique<macro_param_data_single>("second"));
    v.push_back(std::make_unique<macro_param_data_single>("third"));
    macro_data_ptr dat(std::make_unique<macro_param_data_composite>(std::move(v)));

    // creating another vector for macro call
    params.emplace_back(std::move(np2));
    params.emplace_back(std::move(np4), key);
    params.emplace_back(std::move(np3));
    params.emplace_back(std::move(dat));

    // call->|        MAC        ,KEY=cas,,(first,second,third)
    auto [m3, t3] = ctx.enter_macro(m.get(), nullptr, std::move(params));

    EXPECT_FALSE(t3);

    auto SYSLIST = m3->named_params.find(well_known::SYSLIST)->second->access_system_variable();
    ASSERT_TRUE(SYSLIST);

    for (auto i = 0; i < 3; i++)
    {
        EXPECT_EQ(SYSLIST->get_value(i), "");
    }

    EXPECT_EQ(m3->named_params.find(lbl)->second->get_value(), "");
    EXPECT_EQ(m3->named_params.find(op1)->second->get_value(), "");
    EXPECT_EQ(m3->named_params.find(op3)->second->get_value(), "(first,second,third)");
    EXPECT_EQ(m3->named_params.find(key)->second->get_value(), "cas");

    EXPECT_EQ(SYSLIST->get_value(std::array<context::A_t, 2> { 2, 3 }), "");
    EXPECT_EQ(SYSLIST->get_value(3), "(first,second,third)");
    EXPECT_EQ(SYSLIST->get_value(std::array<context::A_t, 2> { 3, 2 }), "second");
    EXPECT_EQ(SYSLIST->get_value(std::array<context::A_t, 4> { 3, 2, 1, 1 }), "second");
    EXPECT_EQ(SYSLIST->get_value(std::array<context::A_t, 5> { 3, 2, 1, 1, 2 }), "");
}

TEST(context_macro, recurr_call)
{
    hlasm_context ctx;


    // creating names of params
    auto idx = ctx.add_id(std::string_view("mac"));
    auto key = ctx.add_id(std::string_view("key"));
    auto op1 = ctx.add_id(std::string_view("op1"));
    auto op3 = ctx.add_id(std::string_view("op3"));
    auto lbl = ctx.add_id(std::string_view("lbl"));

    // creating data of params
    macro_data_ptr p1(std::make_unique<macro_param_data_single>(""));

    // creating vector of params for add_macro
    macro_arg a(std::move(p1), key);
    std::vector<macro_arg> args;
    args.push_back(std::move(a));
    args.emplace_back(nullptr, op1);
    args.emplace_back(nullptr);
    args.emplace_back(nullptr, op3);

    // prototype->|&LBL        MAC        &KEY=,&OP1,,&OP3
    auto m = ctx.add_macro(idx, lbl, std::move(args), {}, {}, {}, {}, {}, false);

    // creating param data
    macro_data_ptr lb(std::make_unique<macro_param_data_single>("lbl"));
    macro_data_ptr p2(std::make_unique<macro_param_data_single>("ada"));
    macro_data_ptr p3(std::make_unique<macro_param_data_single>("mko"));
    macro_data_ptr p4(std::make_unique<macro_param_data_single>(""));
    macro_data_ptr p5(std::make_unique<macro_param_data_single>("as"));

    // creating vector of param data for entering macro
    std::vector<macro_arg> params;
    params.emplace_back(std::move(p2));
    params.emplace_back(std::move(p3));
    params.emplace_back(std::move(p4));

    // calling macro
    // call->|lbl        MAC        ada,mko,
    auto [m2, t2] = ctx.enter_macro(m.get(), std::move(lb), std::move(params));

    //*****created first macro call

    EXPECT_FALSE(t2);
    ASSERT_TRUE(ctx.current_macro() == m2);
    ASSERT_TRUE(ctx.is_in_macro());

    params.clear();

    // creating data for another macro call
    macro_data_ptr np2(std::make_unique<macro_param_data_single>(""));
    macro_data_ptr np3(std::make_unique<macro_param_data_single>(""));
    macro_data_ptr np4(std::make_unique<macro_param_data_single>("cas"));

    // creating data for one complex macro param
    std::vector<macro_data_ptr> v;
    v.push_back(std::make_unique<macro_param_data_single>("first"));
    v.push_back(std::make_unique<macro_param_data_single>("second"));
    v.push_back(std::make_unique<macro_param_data_single>("third"));
    macro_data_ptr dat(std::make_unique<macro_param_data_composite>(std::move(v)));

    // creating another vector for macro call
    params.emplace_back(std::move(np2));
    params.emplace_back(std::move(np4), key);
    params.emplace_back(std::move(np3));
    params.emplace_back(std::move(dat));

    // call->|        MAC        ,KEY=cas,,(first,second,third)
    auto [m3, t3] = ctx.enter_macro(m.get(), nullptr, std::move(params));

    //********called again the same macro without calling leave
    EXPECT_FALSE(t3);
    ASSERT_TRUE(ctx.current_macro() == m3);
    ASSERT_TRUE(ctx.is_in_macro());
    ASSERT_FALSE(m2 == m3);

    auto SYSLIST2 = m2->named_params.find(well_known::SYSLIST)->second->access_system_variable();
    ASSERT_TRUE(SYSLIST2);
    auto SYSLIST3 = m3->named_params.find(well_known::SYSLIST)->second->access_system_variable();
    ASSERT_TRUE(SYSLIST3);

    for (context::A_t i = 0; i < 2; i++)
    {
        EXPECT_EQ(SYSLIST3->get_value(i), "");
    }

    // testing inner macro
    EXPECT_EQ(m3->named_params.find(lbl)->second->get_value(), "");
    EXPECT_EQ(m3->named_params.find(op1)->second->get_value(), "");
    EXPECT_EQ(m3->named_params.find(op3)->second->get_value(), "(first,second,third)");
    EXPECT_EQ(m3->named_params.find(key)->second->get_value(), "cas");

    EXPECT_EQ(SYSLIST3->get_value(0), "");
    EXPECT_EQ(SYSLIST3->get_value(std::array<context::A_t, 2> { 2, 3 }), "");
    EXPECT_EQ(SYSLIST3->get_value(3), "(first,second,third)");
    EXPECT_EQ(SYSLIST3->get_value(std::array<context::A_t, 2> { 3, 2 }), "second");
    EXPECT_EQ(SYSLIST3->get_value(std::array<context::A_t, 4> { 3, 2, 1, 1 }), "second");
    EXPECT_EQ(SYSLIST3->get_value(std::array<context::A_t, 5> { 3, 2, 1, 1, 2 }), "");

    // testing outer macro
    EXPECT_EQ(SYSLIST2->get_value(0), "lbl");
    EXPECT_EQ(SYSLIST2->get_value(1), "ada");
    EXPECT_EQ(SYSLIST2->get_value(2), "mko");
    EXPECT_EQ(SYSLIST2->get_value(3), "");

    EXPECT_EQ(m2->named_params.find(op1)->second->get_value(), "ada");
    EXPECT_EQ(m2->named_params.find(op3)->second->get_value(), "");
    EXPECT_EQ(m2->named_params.find(key)->second->get_value(), "");


    ctx.leave_macro();
    ASSERT_TRUE(ctx.is_in_macro());
    ASSERT_TRUE(ctx.current_macro() == m2);

    ctx.leave_macro();
    ASSERT_FALSE(ctx.is_in_macro());
}

TEST(context, id_check)
{
    hlasm_context ctx;

    EXPECT_TRUE(ctx.try_get_symbol_name("LIST").first);
    EXPECT_TRUE(ctx.try_get_symbol_name("T_A").first);
    EXPECT_TRUE(ctx.try_get_symbol_name("T1").first);

    EXPECT_FALSE(ctx.try_get_symbol_name("a1-").first);
    EXPECT_FALSE(ctx.try_get_symbol_name("*1").first);
    EXPECT_FALSE(ctx.try_get_symbol_name("1av").first);
}

TEST(context, create_global_var_identical_types_macro)
{
    std::string input =
        R"(
      MACRO
      MAC
      GBLA &A
      MEND

      GBLA &A
      MAC

      END
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(context, create_global_var_different_types_macro)
{
    std::string input =
        R"(
      MACRO
      MAC
      GBLB &A
      MEND

      GBLA &A
      MAC

      END
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E078" }));
}

TEST(context, create_local_var_different_types)
{
    std::string input =
        R"(
      LCLA &A
      LCLB &A

      END
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E051" }));
}

TEST(context_system_variables, SYSNEST_SYSMAC)
{
    std::string input =
        R"(
 MACRO
 M1
 GBLA V1
 GBLC V2,V3,V5,V6,V7
 GBLC T1,T2,T3
 GBLA K1,K2,K3
&V1 SETA &SYSNEST
&V2 SETC '&SYSMAC(&SYSNEST)'
&V3 SETC '&SYSMAC(1)'
&V5 SETC '&SYSMAC(0)'
&V6 SETC '&SYSMAC'
&V7 SETC '&SYSMAC(0,1,2,3,4,5,6,7,8,9,1)'
&T1 SETC T'&SYSMAC
&T2 SETC T'&SYSMAC(0)
&T3 SETC T'&SYSMAC(10)
&K1 SETA K'&SYSMAC
&K2 SETA K'&SYSMAC(0)
&K3 SETA K'&SYSMAC(10)
 MEND

 MACRO
 M2
 GBLA V4
&V4 SETA &SYSNEST
 M1
 MEND
 
 GBLA V1,V4
 GBLC V2,V3,V5,V6,V7
 GBLC T1,T2,T3
 GBLA K1,K2,K3

 M2
)";
    analyzer a(input);
    a.analyze();


    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(get_var_value<context::A_t>(a.hlasm_ctx(), "v1"), 2);
    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "v2"), "OPEN CODE");
    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "v3"), "M2");
    EXPECT_EQ(get_var_value<context::A_t>(a.hlasm_ctx(), "v4"), 1);
    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "v5"), "M1");
    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "v6"), "M1");
    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "v7"), "M2");

    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "t1"), "U");
    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "t2"), "U");
    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "t3"), "O");

    EXPECT_EQ(get_var_value<context::A_t>(a.hlasm_ctx(), "k1"), 2);
    EXPECT_EQ(get_var_value<context::A_t>(a.hlasm_ctx(), "k2"), 2);
    EXPECT_EQ(get_var_value<context::A_t>(a.hlasm_ctx(), "k3"), 0);
}

TEST(context_system_variables, SYSVER)
{
    std::string input =
        R"(
&V      SETC '&SYSVER'
&T      SETC T'&SYSVER
&K      SETA K'&SYSVER
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "V"), "1.6.0");
    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "T"), "U");
    EXPECT_EQ(get_var_value<context::A_t>(a.hlasm_ctx(), "K"), 5);
}
