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
#include "../mock_parse_lib_provider.h"

// tests for macro feature:
// definition parsing
// instantiation
// arguments passing
// external macro libraries

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

    id_index id = a.hlasm_ctx().ids().add("m1");
    auto& macros = a.hlasm_ctx().macros();

    auto tmp = macros.find(id);
    ASSERT_TRUE(tmp != macros.end());

    auto& m = tmp->second;

    auto op = a.hlasm_ctx().ids().add("op");

    EXPECT_EQ(m->named_params().find(op)->second->access_positional_param()->position, (size_t)1);

    auto op2 = a.hlasm_ctx().ids().add("op2");

    EXPECT_EQ(m->named_params().find(op2)->second->access_positional_param()->position, (size_t)2);

    auto l = a.hlasm_ctx().ids().add("l");

    EXPECT_EQ(m->named_params().find(l)->second->access_positional_param()->position, (size_t)0);

    auto k = a.hlasm_ctx().ids().add("k");

    EXPECT_EQ(m->named_params().find(k)->second->access_keyword_param()->get_value(), "5");

    auto k2 = a.hlasm_ctx().ids().add("k2");

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

    ASSERT_EQ(a.hlasm_ctx().macros().size(), (size_t)2);

    id_index id;

    id = a.hlasm_ctx().ids().add("M1");
    EXPECT_TRUE(a.hlasm_ctx().macros().find(id) != a.hlasm_ctx().macros().end());

    id = a.hlasm_ctx().ids().add("m2");
    EXPECT_TRUE(a.hlasm_ctx().macros().find(id) != a.hlasm_ctx().macros().end());
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

    ASSERT_EQ(a.hlasm_ctx().macros().size(), (size_t)3);

    id_index id;

    id = a.hlasm_ctx().ids().add("M1");
    EXPECT_TRUE(a.hlasm_ctx().macros().find(id) != a.hlasm_ctx().macros().end());

    id = a.hlasm_ctx().ids().add("M2");
    EXPECT_TRUE(a.hlasm_ctx().macros().find(id) != a.hlasm_ctx().macros().end());

    id = a.hlasm_ctx().ids().add("INNER_M");
    EXPECT_TRUE(a.hlasm_ctx().macros().find(id) != a.hlasm_ctx().macros().end());
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

    ASSERT_EQ(a.hlasm_ctx().macros().size(), (size_t)1);

    id_index id;

    id = a.hlasm_ctx().ids().add("M1");
    EXPECT_TRUE(a.hlasm_ctx().macros().find(id) != a.hlasm_ctx().macros().end());
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

    ASSERT_EQ(a.hlasm_ctx().macros().size(), (size_t)2);

    id_index id;

    id = a.hlasm_ctx().ids().add("M1");
    EXPECT_TRUE(a.hlasm_ctx().macros().find(id) != a.hlasm_ctx().macros().end());

    id = a.hlasm_ctx().ids().add("INNER_M");
    EXPECT_TRUE(a.hlasm_ctx().macros().find(id) != a.hlasm_ctx().macros().end());

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), 1U);
    EXPECT_EQ(a.diags()[0].code, "E047");
    ;
    EXPECT_EQ(a.diags()[0].diag_range, range({ 2, 5 }, { 2, 7 }));
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
    EXPECT_EQ(a.diags().size(), (size_t)1);
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
    EXPECT_EQ(a.diags().size(), (size_t)1);
    EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}

TEST(macro, macro_undefined_keyword_param)
{
    std::string input =
        R"( MACRO
   M1 &a
   aif ('&a' eq 'x=1').a
   err
.a mend

 M1 x=1
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
    ASSERT_EQ(a.diags().front().severity, diagnostic_severity::warning);
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
    EXPECT_EQ(a.diags().size(), (size_t)2);
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
    EXPECT_EQ(a.diags().size(), (size_t)0);
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
    EXPECT_EQ(a.diags().size(), (size_t)1);
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
    EXPECT_EQ(a.diags().size(), (size_t)0);
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
    EXPECT_EQ(a.diags().size(), (size_t)3);
    EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

    auto& m1 = a.hlasm_ctx().macros().find(a.hlasm_ctx().ids().add("m1"))->second;
    auto& m2 = a.hlasm_ctx().macros().find(a.hlasm_ctx().ids().add("m2"))->second;
    auto& m3 = a.hlasm_ctx().macros().find(a.hlasm_ctx().ids().add("m3"))->second;

    {
        std::vector<macro_arg> args;
        args.push_back({ std::make_unique<macro_param_data_single>("2"), nullptr });
        args.push_back({ std::make_unique<macro_param_data_single>("3"), nullptr });
        auto invo = m1->call(
            std::make_unique<macro_param_data_single>("1"), std::move(args), a.hlasm_ctx().ids().add("SYSLIST"));
        auto n = a.hlasm_ctx().ids().add("n");
        auto b = a.hlasm_ctx().ids().add("b");
        EXPECT_EQ(invo->named_params.find(n)->second->get_value(), "1");
        EXPECT_EQ(invo->named_params.find(b)->second->get_value(), "3");
    }

    {
        std::vector<macro_arg> args;
        args.push_back({ std::make_unique<macro_param_data_single>("1"), nullptr });
        args.push_back({ std::make_unique<macro_param_data_single>("2"), nullptr });
        auto invo = m2->call(nullptr, std::move(args), a.hlasm_ctx().ids().add("SYSLIST"));
        auto n = a.hlasm_ctx().ids().add("a");
        auto b = a.hlasm_ctx().ids().add("b");
        EXPECT_EQ(invo->named_params.find(n)->second->get_value(), "1");
        EXPECT_EQ(invo->named_params.find(b)->second->get_value(), "2");
        EXPECT_EQ(invo->named_params.find(a.hlasm_ctx().ids().add("SYSLIST"))->second->get_value(1), "1");
    }

    {
        std::vector<macro_arg> args;
        args.push_back({ std::make_unique<macro_param_data_single>("1"), nullptr });
        args.push_back({ std::make_unique<macro_param_data_single>("2"), nullptr });
        args.push_back({ std::make_unique<macro_param_data_single>("3"), nullptr });
        auto invo = m3->call(nullptr, std::move(args), a.hlasm_ctx().ids().add("SYSLIST"));
        auto n = a.hlasm_ctx().ids().add("a");
        auto b = a.hlasm_ctx().ids().add("b");
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
    EXPECT_EQ(a.diags().size(), (size_t)1);
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
    EXPECT_EQ(a.diags().size(), (size_t)1);
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
    EXPECT_EQ(a.diags().size(), (size_t)10);
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

    auto it = a.hlasm_ctx().globals().find(a.hlasm_ctx().ids().add("V"));

    ASSERT_NE(it, a.hlasm_ctx().globals().end());

    EXPECT_EQ(it->second->access_set_symbol_base()->access_set_symbol<C_t>()->get_value(), "(B-C)+(A-D)");

    EXPECT_EQ(a.diags().size(), (size_t)0);
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

    auto Q = a.hlasm_ctx().globals().find(a.hlasm_ctx().ids().add("Q"));
    auto W = a.hlasm_ctx().globals().find(a.hlasm_ctx().ids().add("W"));

    ASSERT_NE(Q, a.hlasm_ctx().globals().end());
    ASSERT_NE(W, a.hlasm_ctx().globals().end());

    EXPECT_EQ(Q->second->access_set_symbol_base()->access_set_symbol<C_t>()->get_value(), "X");
    EXPECT_EQ(W->second->access_set_symbol_base()->access_set_symbol<C_t>()->get_value(), "Y");

    EXPECT_EQ(a.diags().size(), (size_t)0);
    EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}
TEST(external_macro, bad_name)
{
    std::string input =
        R"(
 MAC
 MAC
)";
    std::string content_bad_name =
        R"(   MACRO
       MACC   &VAR
       LR    &VAR,&VAR
       MEND
)";
    mock_parse_lib_provider lib_provider { { "MAC", content_bad_name } };
    analyzer a(input, analyzer_options { &lib_provider });
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(lib_provider.analyzers.count("MAC"), 1U);
    EXPECT_EQ(lib_provider.analyzers["MAC"]->diags().size(), 1U);
    EXPECT_EQ(a.diags().size(), 2U);
    EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), 0U);
}

TEST(external_macro, bad_begin)
{
    std::string input =
        R"(
 MAC
 MAC
)";
    std::string content_bad_begin =
        R"(  aMACRO
       MAC   &VAR
       LR    &VAR,&VAR
       MEND
)";

    mock_parse_lib_provider lib_provider { { "MAC", content_bad_begin } };
    analyzer a(input, analyzer_options { &lib_provider });
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(lib_provider.analyzers.count("MAC"), 1U);
    EXPECT_EQ(lib_provider.analyzers["MAC"]->diags().size(), 1U);
    EXPECT_EQ(a.diags().size(), 2U);
    EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), 0U);
}

TEST(external_macro, library_with_begin_comment)
{
    std::string input =
        R"(
 MAC 1
 MAC 1
)";
    std::string content_comment =
        R"(**********  
       MACRO
       MAC   &VAR
       LR    &VAR,&VAR
       MEND
)";
    mock_parse_lib_provider lib_provider { { "MAC", content_comment } };
    analyzer a(input, analyzer_options { &lib_provider });
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(lib_provider.analyzers.count("MAC"), 1U);
    EXPECT_EQ(lib_provider.analyzers["MAC"]->diags().size(), 0U);
    EXPECT_EQ(a.diags().size(), 0U);
    EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), 0U);
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

TEST(macro, seq_numbers)
{
    std::string input = R"(
         MACRO                                                          00010000
         M                                                              00020000
         MNOTE 8,'Long continued string spanning multiple lines of the X00030000
               file with sequence symbols.'                             00040000
         MEND                                                           00050000
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);
    EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}

TEST(macro, apostrophe_in_substitution)
{
    std::string input = R"(
         MACRO                                                          00010000
         M     &VAR                                    Comment          00020000
         MNOTE 8,'Message that uses the variable''s VAR value ''&VAR'' X00030000
               and is continued '' .'                                   00040000
         MEND                                                           00050000
                                                                        00060000
         M     TEST                                                     00070000
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);
    EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}

TEST(macro, macro_call_reparse_range)
{
    std::string input = R"(
         MACRO                                                          00010000
         M     &VAR                                    Comment          00020000
         MEND                                                           00050000
                                                                        00060000
         M     op1,               remark                               X00070000
               (
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), 1U);
    EXPECT_EQ(a.diags()[0].code, "S0003");
    EXPECT_EQ(a.diags()[0].diag_range, range({ 6, 16 }, { 6, 16 }));
}

TEST(macro, skip_invalid)
{
    std::string input(R"(
      MACRO
      MAC
      AGO .SKIP
      2 a
.SKIP ANOP
      MEND
      MAC
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
}

TEST(macro, invalid_not_invoked)
{
    std::string input(R"(
      MACRO
      MAC
      2 a
      MEND
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
}

TEST(macro, invalid_invoked)
{
    std::string input(R"(
      MACRO
      MAC
      2 a
      MEND
      MAC
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002", "E049" }));
}

TEST(macro, invalid_prototype)
{
    std::string input = R"(
     MACRO
&    &LABEL &a=15

     MEND
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    const auto& d = a.diags();

    EXPECT_NE(std::find_if(d.begin(), d.end(), [](const auto& diag) { return diag.code == "E071"; }), d.end());
}

TEST(macro, early_macro_errors)
{
    std::string input = R"(
     MACRO
     MAC
     SAM31                                                             X
A
AAA
     MEND
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E001", "S0003" }));
}

TEST(macro, missing_mend)
{
    std::string input = R"(
        MACRO
        MACO
        SAM31                                                          X
            X

        MACRO
        MAC
        SAM31
        MEND
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E001", "E046" }));
}

TEST(macro, dynamic_instruction_types)
{
    std::string input = R"(
         MACRO
&LABEL   MAC   &OP,&FIRST,&SECOND,&THIRD
         LCLC  &OPERAND
&OPERAND SETC  '&FIRST'
         AIF   ('&SECOND' EQ '').SKIP
&OPERAND SETC  '&FIRST.,&SECOND.'
         AIF   ('&THIRD' EQ '').SKIP
&OPERAND SETC  '&FIRST.,&SECOND.,&THIRD.'
.SKIP    ANOP
&LABEL   &OP   &OPERAND
         MEND

         MAC   SAM64,this_is_comment
         MAC   LA,0,0
         MAC   ARK,0,0,0
         EXTRN EXT
EXT      MAC   ALIAS,C'extalias'
ALIAS    OPSYN SAM64
         MAC   ALIAS,this_is_comment
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
}

TEST(macro, operand_string_substitution)
{
    std::string input = R"(
        MACRO
        MAC  &PAR
        GBLC &VAR
&VAR    SETC '&PAR'
        MEND

        GBLC &VAR
        MAC  'test string &SYSPARM'
)";
    analyzer a(input, analyzer_options(asm_option { .sysparm = "ABC" }));
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "VAR"), "'test string ABC'");
}

TEST(macro, operand_string_substitution_continuation)
{
    for (int i = 0; i < 256; ++i)
    {
        std::string input = R"(
        MACRO
        MAC  &PAR
        GBLC &VAR
&VAR    SETC '&PAR'
        MEND

        GBLC &VAR
&AAA(1) SETC 'ABC'
)";
        std::string suffix_ = " MAC " + std::string(i, ' ') + "'test string &(AAA)(1)'";
        std::string_view suffix = suffix_;
        size_t limit = 71;
        while (suffix.size() > limit)
        {
            input.append(suffix.substr(0, limit)).append("X\n               ");
            suffix.remove_prefix(limit);
            limit = 71 - 15;
        }
        input += std::move(suffix);
        analyzer a(input);
        a.analyze();
        a.collect_diags();

        EXPECT_TRUE(a.diags().empty()) << i;
        EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "VAR"), "'test string ABC'") << i;
    }
}

TEST(macro, operand_sublist_continuation)
{
    for (int i = 0; i < 256; ++i)
    {
        std::string input = R"(
        MACRO
        MAC
        MEND
)";
        std::string suffix_ =
            " MAC " + std::string(i, ' ') + "(" + std::string(i / 2, 'J') + "," + std::string(i / 2, 'K') + ")";
        std::string_view suffix = suffix_;
        size_t limit = 71;
        while (suffix.size() > limit)
        {
            input.append(suffix.substr(0, limit)).append("X\n               ");
            suffix.remove_prefix(limit);
            limit = 71 - 15;
        }
        input += std::move(suffix);
        analyzer a(input);
        a.analyze();
        a.collect_diags();

        ASSERT_TRUE(a.diags().empty()) << i;
    }
}

TEST(macro, pass_empty_operand_components)
{
    std::string input = R"(
        MACRO
        MAC2 &PAR=
        GBLA &VAR
&VAR    SETA N'&PAR
        MEND

        MACRO
        MAC  &PAR=
        MAC2 PAR=&PAR
        MEND

        GBLA &VAR
        MAC  PAR=(E,)
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "VAR"), 2);
}

TEST(macro, multiline_comment)
{
    std::string input = R"(
        MACRO
        MAC  &PAR=
        MEND

        MAC  PAR='test'                                        comment X
                                                               comment
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
}

TEST(macro, attribute_and_multiline_comment)
{
    std::string input = R"(
        MACRO
        MAC  &F,&L
TEST    EQU  L'&F-&L
        MEND
LABEL   DS   CL80

        MAC  LABEL,L'LABEL                                     comment X
                                                               comment
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "TEST"), 0);
}

TEST(macro, attribute_string_combo)
{
    std::string input = R"(
        MACRO
        MAC  
TEST    EQU  &SYSLIST(1,2)
        MEND

LABEL   DS   A

        MAC  (0,l'LABEL),'-',(l'LABEL,0),                              X
               '-'
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "TEST"), 4);
}

TEST(macro, empty_parms)
{
    std::string input = R"(
        MACRO
        MAC  &PAR1,&PAR2,&PAR3
        MEND

        MAC  ,A,B
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
}

TEST(macro, empty_parms_after_continuation)
{
    std::string input = R"(
        MACRO
        MAC  &PAR1,&PAR2,&PAR3
        GBLB &RESULT
        AIF  ('&PAR2' NE '').SKIP
&RESULT SETB 1
.SKIP   ANOP ,
        MEND

        GBLB &RESULT

        MAC    A,                                                      X
               ,B
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "RESULT"), true);
}

TEST(macro, syslist_size)
{
    using namespace std::string_literals;

    std::string input = R"(
     MACRO
&L   MAC
&A   SETA N'&SYSLIST
&L   EQU  &A
     MEND
T0   MAC
T1   MAC 
T2   MAC A 
T3   MAC A,
T4   MAC A, 
T5   MAC ,A
T6   MAC ,A 
T7   MAC ,
T8   MAC , 
T9   MAC ,,
T10  MAC ,, 
T11  MAC ,, A
     END
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    auto expected = { 0, 0, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3 };

    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), std::string("T") + std::to_string(i)), expected.begin()[i]) << i;
    }
}
