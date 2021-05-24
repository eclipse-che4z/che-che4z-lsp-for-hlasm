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

#include "../common_testing.h"
#include "../copy_mock.h"

// test for COPY instruction
// various cases of instruction occurence in the source

TEST(copy, copy_enter_fail)
{
    std::string input =
        R"(
 COPY A+1
 COPY UNKNOWN
)";
    copy_mock mock;
    analyzer a(input, analyzer_options { &mock });
    a.analyze();

    a.collect_diags();

    EXPECT_EQ(a.hlasm_ctx().copy_members().size(), (size_t)0);
    EXPECT_EQ(a.hlasm_ctx().whole_copy_stack().size(), (size_t)0);

    EXPECT_EQ(a.diags().size(), (size_t)2);
}

TEST(copy, copy_enter_success)
{
    std::string input =
        R"(
 COPY COPYR
)";
    copy_mock mock;
    analyzer a(input, analyzer_options { &mock });
    a.analyze();

    a.collect_diags();

    EXPECT_EQ(a.hlasm_ctx().copy_members().size(), (size_t)1);

    EXPECT_EQ(a.hlasm_ctx().macros().size(), (size_t)1);

    EXPECT_TRUE(a.hlasm_ctx().get_sequence_symbol(a.hlasm_ctx().ids().add("A")));

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(copy, copy_enter_diag_test)
{
    std::string input =
        R"(
 COPY COPYD
)";
    copy_mock mock;
    analyzer a(input, analyzer_options { "start", &mock });
    a.analyze();

    a.collect_diags();

    EXPECT_EQ(a.hlasm_ctx().copy_members().size(), (size_t)1);

    ASSERT_EQ(a.diags().size(), (size_t)1);

    auto diag = a.diags()[0];

    EXPECT_EQ(a.diags()[0].diag_range.start.line, (position_t)2);
    EXPECT_EQ(a.diags()[0].file_name, "COPYD");
    EXPECT_EQ(a.diags()[0].related.size(), (size_t)1);
    EXPECT_EQ(a.diags()[0].related[0].location.rang.start.line, (position_t)1);
    EXPECT_EQ(a.diags()[0].related[0].location.uri, "start");
}

TEST(copy, copy_jump)
{
    std::string input =
        R"(
&VARX SETA 0
&VAR SETA 0
 COPY COPYF
 AIF (&VAR LT 4).A
)";
    copy_mock mock;
    analyzer a(input, analyzer_options { &mock });
    a.analyze();

    a.collect_diags();

    EXPECT_EQ(a.hlasm_ctx().copy_members().size(), (size_t)2);

    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(a.hlasm_ctx()
                  .get_var_sym(a.hlasm_ctx().ids().add("VAR"))
                  ->access_set_symbol_base()
                  ->access_set_symbol<context::A_t>()
                  ->get_value(),
        4);
    EXPECT_EQ(a.hlasm_ctx()
                  .get_var_sym(a.hlasm_ctx().ids().add("VARX"))
                  ->access_set_symbol_base()
                  ->access_set_symbol<context::A_t>()
                  ->get_value(),
        2);
}

TEST(copy, copy_unbalanced_macro)
{
    std::string input =
        R"(
 COPY COPYU
)";
    copy_mock mock;
    analyzer a(input, analyzer_options { &mock });
    a.analyze();

    a.collect_diags();

    EXPECT_EQ(a.hlasm_ctx().copy_members().size(), (size_t)1);

    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(mock.a->diags().size(), (size_t)1);
}

TEST(copy, copy_twice)
{
    std::string input =
        R"(
 COPY COPYR
 COPY COPYR
)";
    copy_mock mock;
    analyzer a(input, analyzer_options { &mock });
    a.analyze();

    a.collect_diags();

    EXPECT_EQ(a.hlasm_ctx().copy_members().size(), (size_t)1);

    EXPECT_EQ(a.diags().size(), (size_t)2);
}

TEST(copy, macro_call_from_copy_enter)
{
    std::string input =
        R"(
 COPY COPYR
 M1
 M2
)";
    copy_mock mock;
    analyzer a(input, analyzer_options { &mock });
    a.analyze();

    a.collect_diags();

    EXPECT_EQ(a.hlasm_ctx().copy_members().size(), (size_t)1);

    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(a.hlasm_ctx().macros().size(), (size_t)2);
}

TEST(copy, copy_enter_from_macro_call)
{
    std::string input =
        R"(
 MACRO
 M
 LR 1,1
.B ANOP
 COPY COPYR
 MEND

 M
)";
    copy_mock mock;
    analyzer a(input, analyzer_options { "start", &mock });
    a.analyze();

    a.collect_diags();

    EXPECT_EQ(a.hlasm_ctx().copy_members().size(), (size_t)1);

    EXPECT_EQ(a.hlasm_ctx().macros().size(), (size_t)2);

    auto mac = a.hlasm_ctx().macros().find(a.hlasm_ctx().ids().add("M"));
    ASSERT_TRUE(mac != a.hlasm_ctx().macros().end());

    EXPECT_TRUE(mac->second->labels.find(a.hlasm_ctx().ids().add("A")) != mac->second->labels.end());
    EXPECT_TRUE(mac->second->labels.find(a.hlasm_ctx().ids().add("B")) != mac->second->labels.end());

    ASSERT_EQ(a.diags().size(), (size_t)1);

    EXPECT_EQ(a.diags()[0].diag_range.start.line, (position_t)16);
    EXPECT_EQ(a.diags()[0].file_name, "COPYR");
    ASSERT_EQ(a.diags()[0].related.size(), (size_t)1);
    EXPECT_EQ(a.diags()[0].related[0].location.rang.start.line, (position_t)5);
    EXPECT_EQ(a.diags()[0].related[0].location.uri, "start");
}

TEST(copy, copy_enter_from_lookahead)
{
    std::string input =
        R"(
&V SETA 0
 AGO .C
&V SETA &V+1
 COPY COPYL
&V SETA &V+1
 
)";
    copy_mock mock;
    analyzer a(input, analyzer_options { "start", &mock });
    a.analyze();

    a.collect_diags();

    EXPECT_EQ(a.hlasm_ctx().copy_members().size(), (size_t)1);

    EXPECT_EQ(a.hlasm_ctx()
                  .get_var_sym(a.hlasm_ctx().ids().add("V"))
                  ->access_set_symbol_base()
                  ->access_set_symbol<context::A_t>()
                  ->get_value(),
        1);

    ASSERT_EQ(a.diags().size(), (size_t)1);

    EXPECT_EQ(a.diags()[0].diag_range.start.line, (position_t)6);
    EXPECT_EQ(a.diags()[0].file_name, "COPYL");
    ASSERT_EQ(a.diags()[0].related.size(), (size_t)1);
    EXPECT_EQ(a.diags()[0].related[0].location.rang.start.line, (position_t)4);
    EXPECT_EQ(a.diags()[0].related[0].location.uri, "start");
}

TEST(copy, nested_macro_copy_call)
{
    std::string input =
        R"(
 COPY COPYN
 
)";
    copy_mock mock;
    analyzer a(input, analyzer_options { &mock });
    a.analyze();

    a.collect_diags();

    EXPECT_EQ(a.hlasm_ctx().copy_members().size(), (size_t)2);
    ASSERT_EQ(a.hlasm_ctx().macros().size(), (size_t)1);
    auto mac = a.hlasm_ctx().macros().find(a.hlasm_ctx().ids().add("MAC"));

    EXPECT_TRUE(mac->second->labels.find(a.hlasm_ctx().ids().add("A")) != mac->second->labels.end());

    EXPECT_EQ(a.hlasm_ctx()
                  .globals()
                  .find(a.hlasm_ctx().ids().add("X"))
                  ->second->access_set_symbol_base()
                  ->access_set_symbol<context::A_t>()
                  ->get_value(),
        4);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(copy, macro_from_copy_call)
{
    std::string input =
        R"(
 COPY COPYBM
 M
 
)";
    copy_mock mock;
    analyzer a(input, analyzer_options { "start", &mock });
    a.analyze();

    a.collect_diags();

    EXPECT_EQ(a.hlasm_ctx().copy_members().size(), (size_t)1);
    ASSERT_EQ(a.hlasm_ctx().macros().size(), (size_t)1);
    auto mac = a.hlasm_ctx().macros().find(a.hlasm_ctx().ids().add("M"));
    ASSERT_NE(a.hlasm_ctx().macros().end(), mac);

    ASSERT_EQ(a.diags().size(), (size_t)1);

    EXPECT_EQ(a.diags()[0].diag_range.start.line, (position_t)3);
    EXPECT_EQ(a.diags()[0].file_name, "COPYBM");
    ASSERT_EQ(a.diags()[0].related.size(), (size_t)1);
    EXPECT_EQ(a.diags()[0].related[0].location.rang.start.line, (position_t)2);
    EXPECT_EQ(a.diags()[0].related[0].location.uri, "start");
}

TEST(copy, inner_copy_jump)
{
    std::string input =
        R"(
 COPY COPYJ
 LR
 
)";
    copy_mock mock;
    analyzer a(input, analyzer_options { &mock });
    a.analyze();

    a.collect_diags();

    EXPECT_EQ(a.hlasm_ctx().copy_members().size(), (size_t)1);

    EXPECT_EQ(a.diags().size(), (size_t)1);
    EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}

TEST(copy, jump_from_copy_fail)
{
    std::string input =
        R"(
 COPY COPYJF
)";
    copy_mock mock;
    analyzer a(input, analyzer_options { "start", &mock });
    a.analyze();

    a.collect_diags();

    EXPECT_EQ(a.hlasm_ctx().copy_members().size(), (size_t)1);

    EXPECT_EQ(a.diags().size(), (size_t)2);
    EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

    EXPECT_EQ(a.diags()[1].diag_range.start.line, (position_t)2);
    EXPECT_EQ(a.diags()[1].file_name, "COPYJF");
    ASSERT_EQ(a.diags()[1].related.size(), (size_t)1);
    EXPECT_EQ(a.diags()[1].related[0].location.rang.start.line, (position_t)1);
    EXPECT_EQ(a.diags()[1].related[0].location.uri, "start");

    EXPECT_EQ(a.diags()[0].diag_range.start.line, (position_t)1);
    EXPECT_EQ(a.diags()[0].file_name, "COPYJF");
    ASSERT_EQ(a.diags()[0].related.size(), (size_t)1);
    EXPECT_EQ(a.diags()[0].related[0].location.rang.start.line, (position_t)1);
    EXPECT_EQ(a.diags()[0].related[0].location.uri, "start");
}

TEST(copy, jump_in_macro_from_copy_fail)
{
    std::string input =
        R"(
 MACRO
 m
 copy copyJF
 mend

 m
)";
    copy_mock mock;
    analyzer a(input, analyzer_options { "start", &mock });
    a.analyze();

    a.collect_diags();

    EXPECT_EQ(a.hlasm_ctx().copy_members().size(), (size_t)1);

    EXPECT_EQ(a.diags().size(), (size_t)2);
    EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

    EXPECT_EQ(a.diags()[0].diag_range.start.line, (position_t)1);
    EXPECT_EQ(a.diags()[0].file_name, "COPYJF");
    ASSERT_EQ(a.diags()[0].related.size(), (size_t)2);
    EXPECT_EQ(a.diags()[0].related[0].location.rang.start.line, (position_t)3);
    EXPECT_EQ(a.diags()[0].related[0].location.uri, "start");
    EXPECT_EQ(a.diags()[0].related[1].location.rang.start.line, (position_t)6);
    EXPECT_EQ(a.diags()[0].related[1].location.uri, "start");
}

TEST(copy, macro_nested_diagnostics)
{
    std::string input =
        R"( MACRO
 MAC

 copy COPYND1

 MEND
 
 MAC  
)";
    copy_mock mock;
    analyzer a(input, analyzer_options { "start", &mock });
    a.analyze();

    a.collect_diags();

    EXPECT_EQ(a.hlasm_ctx().copy_members().size(), (size_t)2);

    EXPECT_EQ(a.diags().size(), (size_t)1);
    EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

    EXPECT_EQ(a.diags()[0].diag_range.start.line, (position_t)4);
    EXPECT_EQ(a.diags()[0].file_name, "COPYND2");
    ASSERT_EQ(a.diags()[0].related.size(), (size_t)3);
    EXPECT_EQ(a.diags()[0].related[0].location.rang.start.line, (position_t)1);
    EXPECT_EQ(a.diags()[0].related[0].location.uri, "COPYND1");
    EXPECT_EQ(a.diags()[0].related[1].location.rang.start.line, (position_t)3);
    EXPECT_EQ(a.diags()[0].related[1].location.uri, "start");
    EXPECT_EQ(a.diags()[0].related[2].location.rang.start.line, (position_t)7);
    EXPECT_EQ(a.diags()[0].related[2].location.uri, "start");
}

TEST(copy, copy_call_with_jump_before_comment)
{
    std::string input =
        R"( 
 COPY COPYJ
***
 ANOP
)";
    copy_mock mock;
    analyzer a(input, analyzer_options { "start", &mock });
    a.analyze();

    a.collect_diags();

    EXPECT_EQ(a.hlasm_ctx().copy_members().size(), (size_t)1);

    EXPECT_EQ(a.diags().size(), (size_t)0);
    EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}
