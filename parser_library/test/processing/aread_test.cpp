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

#include "../mock_parse_lib_provider.h"
#include "../common_testing.h"

// tests for
// AREAD handling

namespace {
std::string aread_pad(std::string s)
{
    s.resize(80, ' ');
    return s;
}
} // namespace

TEST(aread, only_from_macro)
{
    std::string input("&VAR AREAD");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();

    ASSERT_EQ(diags.size(), 1);
    EXPECT_TRUE(std::any_of(diags.begin(), diags.end(), [](const auto& msg) { return msg.code == "E069"; }));
}

TEST(aread, basic_test)
{
    std::string input(R"(
          MACRO
          M
          GBLC &VAR
&VAR      AREAD
          MEND

          GBLC &VAR
          M
This is a raw text
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    EXPECT_EQ(get_var_value<C_t>(ctx, "var").value_or(""), aread_pad("This is a raw text"));
}

TEST(aread, array_test)
{
    std::string input(R"(
          MACRO
          M
          GBLC &VAR(10)
&VAR(1)   AREAD
&VAR(2)   AREAD
          MEND

          GBLC &VAR(10)
          M
This is a raw text 1
This is a raw text 2
&PROCESSED SETA 1
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    EXPECT_EQ(get_var_value<A_t>(ctx, "PROCESSED").value_or(0), 1);

    const auto expected = std::vector<C_t> {
        aread_pad("This is a raw text 1"),
        aread_pad("This is a raw text 2"),
    };
    EXPECT_EQ(get_var_vector<C_t>(ctx, "var"), expected);
}

TEST(aread, operand_support)
{
    std::string input(R"(
          MACRO
          M
          GBLC &VAR(10)
&VAR(1)   AREAD NOPRINT
&VAR(2)   AREAD NOSTMT
&VAR(3)   AREAD CLOCKB
&VAR(4)   AREAD CLOCKD
          MEND

          GBLC &VAR(10)
          M
This is a raw text 1
This is a raw text 2
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    auto var = get_var_vector<C_t>(ctx, "var").value_or(std::vector<C_t> {});

    ASSERT_EQ(var.size(), 4);
    EXPECT_EQ(var[0], aread_pad("This is a raw text 1"));
    EXPECT_EQ(var[1], aread_pad("This is a raw text 2"));
    EXPECT_EQ(var[2].size(), 8);
    EXPECT_EQ(var[3].size(), 8);
}

TEST(aread, empty_input)
{
    std::string input(R"(
          MACRO
          M
          GBLC &VAR
&VAR      AREAD 
          MEND

          GBLC &VAR
          M
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    auto var = get_var_value<C_t>(ctx, "var");

    ASSERT_TRUE(var.has_value());
    EXPECT_EQ(var.value(), "");
}

TEST(aread, invalid_operands)
{
    std::string input(R"(
          MACRO
          M
          GBLC &VAR
&VAR      AREAD A
&VAR      AREAD 1
&VAR      AREAD &VAR
&VAR      AREAD 'A'
          MEND

          GBLC &VAR
          M
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 4);

    EXPECT_TRUE(std::all_of(diags.begin(), diags.end(), [](const auto& d) { return d.code == "E070"; }));
}

TEST(aread, from_ainsert_buffer)
{
    std::string input(R"(
          MACRO
          M
          GBLC &VAR1
          GBLC &VAR2
&VAR1     AREAD
&VAR2     AREAD
          MEND

          MACRO
          M2
          AINSERT 'test string 2',BACK
          AINSERT 'test string 1',FRONT
          AINSERT ' M',FRONT
          MEND

          GBLC &VAR1
          GBLC &VAR2

          M2
&PROCESSED SETA 1
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    EXPECT_EQ(get_var_value<A_t>(ctx, "PROCESSED").value_or(0), 1);

    EXPECT_EQ(get_var_value<C_t>(ctx, "var1").value_or(""), aread_pad("test string 1"));
    EXPECT_EQ(get_var_value<C_t>(ctx, "var2").value_or(""), aread_pad("test string 2"));
}

TEST(aread, ainsert_with_substitution)
{
    std::string input(R"(
&VAR SETC '1'
          AINSERT '&&PROCESSED SETA &VAR',FRONT
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    EXPECT_EQ(get_var_value<A_t>(ctx, "PROCESSED").value_or(0), 1);
}

TEST(aread, tolerate_extra_operands)
{
    std::string input(R"(
    MACRO
    M
&S  AREAD ,
&S  AREAD NOSTMT,
&S  AREAD NOSTMT,,
&S  AREAD NOSTMT,AAA,
&S  AREAD NOSTMT,AAA,,
    MEND
    M
Line1
Line2
Line3
Line4
Line5
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);
}

TEST(aread, empty_ainsert_record)
{
    std::string input(R"( AINSERT '',BACK)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 1);
    EXPECT_EQ(diags.front().code, "A021");
}

TEST(aread, correct_line_counting)
{
    std::string input(R"(
    MACRO
    M
&S  AREAD
&S  AREAD
&S  AREAD
&S  AREAD
&S  AREAD
    MEND
    M
Line1
Line2
Line3
Line4
Line5
    UNDEF
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 1);
    EXPECT_EQ(diags[0].diag_range.start.line, 15);
}

TEST(aread, macro_called_in_copybook)
{
    std::string input = " COPY  COPYBOOK";
    mock_parse_lib_provider lib_provider {
        { "COPYBOOK", R"(
          MACRO
          M
&VAR      AREAD
          MEND
          M
&A1       SETA  1                                                      X this line is removed by the macro
&A2       SETA  2
)" },
    };

    analyzer a(input, analyzer_options { &lib_provider });
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    auto a1 = get_var_value<A_t>(ctx, "A1");
    auto a2 = get_var_value<A_t>(ctx, "A2");

    EXPECT_FALSE(a1.has_value());
    EXPECT_EQ(a2, 2);
}

TEST(aread, aread_from_source_stack)
{
    std::string input = " COPY  COPYCOPY";
    mock_parse_lib_provider lib_provider {
        { "MAC", R"(*
          MACRO
          MAC
&VAR      AREAD
          MEND
)" },
        { "COPYCOPY", R"(
 COPY COPYBOOK
&A1       SETA  1                                                      X this line is removed by the macro
&A2       SETA  2
)" },
        { "COPYBOOK", " MAC" },
    };

    analyzer a(input, analyzer_options { &lib_provider });
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    auto a1 = get_var_value<A_t>(ctx, "A1");
    auto a2 = get_var_value<A_t>(ctx, "A2");

    EXPECT_FALSE(a1.has_value());
    EXPECT_EQ(a2, 2);
}

TEST(aread, aread_from_opencode)
{
    std::string input = R"(
          COPY  COPYCOPY
&A1       SETA  1                                                      X this line is removed by the macro
&A2       SETA  2)";
    mock_parse_lib_provider lib_provider {
        { "MAC", R"(*
          MACRO
          MAC
&VAR      AREAD
          MEND
)" },
        { "COPYCOPY", R"( COPY COPYBOOK)" },
        { "COPYBOOK", " MAC" },
    };

    analyzer a(input, analyzer_options { &lib_provider });
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    auto a1 = get_var_value<A_t>(ctx, "A1");
    auto a2 = get_var_value<A_t>(ctx, "A2");

    EXPECT_FALSE(a1.has_value());
    EXPECT_EQ(a2, 2);
}

TEST(aread, aread_across_stack)
{
    std::string input = R"(
          COPY  COPYCOPY
&A1       SETA  1                                                      X this line is removed by the macro
&A2       SETA  2
)";
    mock_parse_lib_provider lib_provider {
        { "MAC", R"(*
          MACRO
          MAC
&VAR      AREAD
&VAR      AREAD
&VAR      AREAD
          MEND
)" },
        { "COPYCOPY", R"(
 COPY COPYBOOK
removed by aread
)" },
        { "COPYBOOK", R"(
 MAC
removed by aread
)" },
    };

    analyzer a(input, analyzer_options { &lib_provider });
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    auto a1 = get_var_value<A_t>(ctx, "A1");
    auto a2 = get_var_value<A_t>(ctx, "A2");

    EXPECT_FALSE(a1.has_value());
    EXPECT_EQ(a2, 2);
}

TEST(aread, aread_from_macro_invoked_from_ainsert)
{
    std::string input = R"(
          COPY  COPYCOPY
&A1       SETA  1                                                      X this line is removed by the macro
&A2       SETA  2
)";
    mock_parse_lib_provider lib_provider {
        { "MAC", R"(*
          MACRO
          MAC
&VAR      AREAD
&VAR      AREAD
&VAR      AREAD
          MEND
)" },
        { "COPYCOPY", R"(
 COPY COPYBOOK
removed by aread
)" },
        { "COPYBOOK", R"(
 AINSERT ' MAC',FRONT
removed by aread
)" },
    };

    analyzer a(input, analyzer_options { &lib_provider });
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    auto a1 = get_var_value<A_t>(ctx, "A1");
    auto a2 = get_var_value<A_t>(ctx, "A2");

    EXPECT_FALSE(a1.has_value());
    EXPECT_EQ(a2, 2);
}

TEST(aread, last_statements_in_copy)
{
    std::string input = R"(
          COPY  COPYCOPY
&A1       SETA  1                                                      X this line is removed by the macro
&A2       SETA  2
)";
    mock_parse_lib_provider lib_provider {
        { "MAC", R"(*
          MACRO
          MAC
&VAR      AREAD
          MEND
)" },
        { "COPYCOPY", R"( COPY COPYBOOK)" },
        { "COPYBOOK", R"( MAC)" },
    };

    analyzer a(input, analyzer_options { &lib_provider });
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    auto a1 = get_var_value<A_t>(ctx, "A1");
    auto a2 = get_var_value<A_t>(ctx, "A2");

    EXPECT_FALSE(a1.has_value());
    EXPECT_EQ(a2, 2);
}

TEST(aread, interleave_aread_ainsert)
{
    std::string input = R"( COPY  COPYBOOK)";
    mock_parse_lib_provider lib_provider {
        { "MAC", R"(*
          MACRO
          MAC &CALL=1
&VAR      AREAD
          AIF (&CALL EQ 0).SKIPCALL
          AINSERT ' MAC CALL=0',FRONT
.SKIPCALL ANOP ,
          MEND
)" },
        { "COPYBOOK", R"(
          MAC
removed on first call                                                  X
&A1       SETA  1                                                      X removed by the second call
&A2       SETA  2
)" },
    };

    analyzer a(input, analyzer_options { &lib_provider });
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    auto a1 = get_var_value<A_t>(ctx, "A1");
    auto a2 = get_var_value<A_t>(ctx, "A2");

    EXPECT_FALSE(a1.has_value());
    EXPECT_EQ(a2, 2);
}

TEST(aread, copy_in_macro)
{
    std::string input = R"( COPY  COPYBOOK)";
    mock_parse_lib_provider lib_provider {
        { "MAC", R"(*
          MACRO
          MAC
          AINSERT '&&A1       SETA  1    removed by the macro',FRONT
          COPY MACINNER
          MEND
)" },
        { "MACINNER", "&VAR      AREAD" },
        { "COPYBOOK", R"(
          MAC
&A2       SETA  2
)" },
    };

    analyzer a(input, analyzer_options { &lib_provider });
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    auto a1 = get_var_value<A_t>(ctx, "A1");
    auto a2 = get_var_value<A_t>(ctx, "A2");

    EXPECT_FALSE(a1.has_value());
    EXPECT_EQ(a2, 2);
}

TEST(aread, normal_processing_recovery)
{
    std::string input = R"( COPY  COPYBOOK)";
    mock_parse_lib_provider lib_provider {
        { "MAC", R"(*
          MACRO
          MAC
&VAR      AREAD
          MEND
)" },
        { "COPYBOOK", R"(
          MAC
&A1       SETA  1 removed by the macro
&A2       SETA  2
)" },
    };

    analyzer a(input, analyzer_options { &lib_provider });
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    auto a1 = get_var_value<A_t>(ctx, "A1");
    auto a2 = get_var_value<A_t>(ctx, "A2");

    EXPECT_FALSE(a1.has_value());
    EXPECT_EQ(a2, 2);
}

TEST(aread, normal_processing_recovery_line_skipped)
{
    std::string input = R"( COPY  COPYBOOK)";
    mock_parse_lib_provider lib_provider {
        { "MAC", R"(*
          MACRO
          MAC
&VAR      AREAD
          MEND
)" },
        { "COPYBOOK", R"(
          MAC
&A1       SETA  1 removed by the macro
*
&A2       SETA  2
)" },
    };

    analyzer a(input, analyzer_options { &lib_provider });
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    auto a1 = get_var_value<A_t>(ctx, "A1");
    auto a2 = get_var_value<A_t>(ctx, "A2");

    EXPECT_FALSE(a1.has_value());
    EXPECT_EQ(a2, 2);
}
