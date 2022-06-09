/*
 * Copyright (c) 2022 Broadcom.
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

using namespace hlasm_plugin::utils::resource;

TEST(ainsert, ainsert_with_substitution)
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

TEST(ainsert, empty_ainsert_record)
{
    std::string input(R"( AINSERT '',BACK)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 1);
    EXPECT_EQ(diags.front().code, "A021");
}

TEST(ainsert, lookahead_in_ainsert)
{
    std::string input = R"(
         MACRO
         MAC
         AINSERT '    AIF (1).L',BACK
         AINSERT '.L  ANOP     ',BACK
         MEND
         MAC
)";

    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    EXPECT_TRUE(diags.empty());
}

TEST(ainsert, lookahead_resumed_after_ainsert)
{
    std::string input = R"(
         MACRO
         MAC
         AINSERT '   AIF   (1).L       ',BACK
         AINSERT '.L ANOP              ',BACK
         MEND

&X       SETA L'A

         MAC

A        DS  C
)";

    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    EXPECT_TRUE(diags.empty());
}

TEST(ainsert, ainserted_macro_call_from_copybook)
{
    std::string input = R"( COPY  COPYBOOK)";
    mock_parse_lib_provider lib_provider {
        { "MAC", R"(*
         MACRO
         MAC
         AINSERT ' MAC2',BACK
         MEND
)" },
        { "MAC2", R"(*
         MACRO
         MAC2
A        DC C
         MEND
)" },
        { "COPYBOOK", R"(
         MAC
)" },
    };

    analyzer a(input, analyzer_options { &lib_provider });

    a.analyze();
    a.collect_diags();

    const auto& diags = a.diags();

    ASSERT_EQ(diags.size(), 1);

    const auto& d = diags.front();

    EXPECT_EQ(d.code, "D016");
    ASSERT_EQ(d.related.size(), 3);
    EXPECT_EQ(d.related[0].location.uri, "AINSERT:1.hlasm");
    EXPECT_EQ(d.related[1].location.uri, "COPYBOOK");
}

TEST(ainsert, argument_limit)
{
    std::string input = R"(
         MACRO
         MAC
&C       SETC (80)' '
         AINSERT '&C',BACK
         MEND
         MAC
)";

    analyzer a(input);

    a.analyze();
    a.collect_diags();

    ASSERT_TRUE(a.diags().empty());
}

TEST(ainsert, argument_limit_over)
{
    std::string input = R"(
         MACRO
         MAC
&C       SETC (81)' '
         AINSERT '&C',BACK
         MEND
         MAC
)";

    analyzer a(input);

    a.analyze();
    a.collect_diags();

    ASSERT_TRUE(matches_message_codes(a.diags(), { "A157" }));
}

TEST(ainsert, postponed_variable_evaluation)
{
    std::string input = R"(
    MACRO
    MAC
    AINSERT '       MACRO',BACK
    AINSERT '       MAC_GEN',BACK
    AINSERT '       GBLA &&A',BACK
    AINSERT '       GBLC &&C1,&&C2',BACK
    AINSERT '&&A    SETA   &&SYSSTMT',BACK
    AINSERT '&&C1   SETC ''&&SYSSTMT''',BACK
    AINSERT '&&C2   SETC ''&&SYSLIST(N''&&SYSLIST)''',BACK
    AINSERT '       MEND',BACK
    AINSERT '&&C3   SETC ''&&SYSSTMT''',BACK
    MEND
    
    GBLA &A
    GBLC &C1,&C2
    MAC 1,2,3,4,5
    MAC_GEN 6,7,8,9
    END
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 40);
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C1"), "00000041");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C2"), "9");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C3"), "00000037");
}

TEST(ainsert, immediate_variable_evaluation)
{
    std::string input = R"(
    MACRO
    MAC
    AINSERT '       MACRO',BACK
    AINSERT '       MAC_GEN',BACK
    AINSERT '       GBLA &&A',BACK
    AINSERT '       GBLC &&C1,&&C2',BACK
    AINSERT '&&A    SETA   &SYSSTMT',BACK
    AINSERT '&&C1   SETC ''&SYSSTMT''',BACK
    AINSERT '&&C2   SETC ''&SYSLIST(N'&SYSLIST)''',BACK
    AINSERT '       MEND',BACK
    AINSERT '&&C3   SETC ''&SYSSTMT''',BACK
    MEND

    GBLA &A
    GBLC &C1,&C2
    MAC 1,2,3,4,5
    MAC_GEN 6,7,8,9
    END
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 22);
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C1"), "00000023");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C2"), "5");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C3"), "00000026");
}

TEST(ainsert, grammar_valid_01)
{
    std::string input = R"(
    MACRO
    MAC
    AINSERT '&&S    SETC ''S''',BACK
    AINSERT '&&T    SETC ''T''',BACK
    AINSERT '&&R    SETC ''R''',BACK
    AINSERT '&&A    SETA 1',BACK
    AINSERT '&&B    SETA 2',BACK
    AINSERT '&&(&&S&&T&&R&&A&&B)    SETC ''&&SYSSTMT''',BACK

    AINSERT '&&C1   SETC ''&SYSLIST(N'&SYSLIST)''',BACK
    AINSERT '&&C2   SETC ''C''''''.''&SYSLIST(N'&SYSLIST)''(1,1).''''''x
               ''',BACK
    MEND
    
    MAC 1,2,3,4,5
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "STR12"), "00000032");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C1"), "5");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C2"), "C'5'");
}

TEST(ainsert, grammar_valid_02)
{
    std::string input = R"(
    MACRO
    MAC
    AINSERT '       MACRO',BACK
    AINSERT '       MAC_GEN',BACK

    AINSERT '       GBLC &&C1,&&C2,&&C3',BACK

    AINSERT '&&C1   SETC ''&&SYSLIST(N''&&SYSLIST)''',BACK
    AINSERT '&&C2   SETC ''C''''''.''&&SYSLIST(N''&&SYSLIST)''(1,1).'''x
               '''''',BACK

    AINSERT '       MEND',BACK
    MEND
    
    MAC 1,2,3,4,5
    
    GBLC &C1,&C2,&C3
    MAC_GEN 6,7,8,9
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C1"), "9");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C2"), "C'9'");
}

TEST(ainsert, grammar_unknown_label)
{
    std::string input = R"(
    MACRO
    MAC

    AINSERT '       MACRO',BACK
    AINSERT '       MAC_GEN',BACK
    AINSERT 'C1     SETC ''&SYSLIST(N''&SYSLIST)''',BACK
    AINSERT '&&&&C2 SETC ''&SYSLIST(N''&SYSLIST)''',BACK
    AINSERT '       MEND',BACK

    AINSERT 'C3     SETC ''&&SYSLIST(N''&&SYSLIST)''',BACK
    AINSERT '&&&&C4 SETC ''&&SYSLIST(N''&&SYSLIST)''',BACK

    MEND
    
    MAC 1,2,3,4,5
    MAC_GEN 6,7,8,9
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_TRUE(matches_message_codes(a.diags(),
        {
            "S0002",
            "S0002",
            "E010",
            "E076",
            "E010",
            "E076",
            "E010",
            "E076",
            "E010",
            "E076",
        }));
}

TEST(ainsert, grammar_unknown_variable)
{
    std::string input = R"(
    MACRO
    MAC

    AINSERT '&&C1   SETC ''&&SYSLIST(N''SYSLIST)''',BACK

    MEND
    
    MAC 1,2,3,4,5
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_TRUE(matches_message_codes(a.diags(),
        {
            "E010",
            "E066",
        }));
}

TEST(ainsert, grammar_invalid_string)
{
    std::string input = R"(
    MACRO
    MAC

    AINSERT '       MACRO',BACK
    AINSERT '       MAC_GEN',BACK
    AINSERT '&&C1   SETC ''SYSLIST(N''&SYSLIST)''',BACK
    AINSERT '&&C2   SETC ''&SYSLIST(N''SYSLIST)''',BACK
    AINSERT '       MEND',BACK

    AINSERT '&&C3   SETC ''SYSLIST(N''&&SYSLIST)''',BACK
    AINSERT '&&C4   SETC ''SYSLIST(N''SYSLIST)''',BACK

    MEND
    
    MAC 1,2,3,4,5
    MAC_GEN 6,7,8,9
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002", "S0002", "S0002", "S0002", "E076", "E076" }));
}

/*
Original valid strings:
    AINSERT '&&C    SETC ''C''''''.''&&SYSLIST(N''&&SYSLIST)''(1,1).'''x
               '''''''''',BACK
*/
TEST(ainsert, grammar_non_matching_apostrophes_by_two_01)
{
    std::string input = R"(
    MACRO
    MAC

    AINSERT '       MACRO',BACK
    AINSERT '       MAC_GEN',BACK
    AINSERT '&&C    SETC ''C''''''.''&&SYSLIST(N''&&SYSLIST)''(1,1).'''x
               '''''''',BACK
    AINSERT '       MEND',BACK

    MEND

    MAC 1,2,3,4,5
    MAC_GEN 6,7,8,9
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002" }));
}

/*
Original valid strings:
    AINSERT '&&C    SETC  ''C''''''.''&SYSLIST(N'&SYSLIST)''(1,1).'''''x
               '''''''',BACK
*/
TEST(ainsert, grammar_non_matching_apostrophes_by_two_02)
{
    std::string input = R"(
    MACRO
    MAC

    AINSERT '&&C    SETC  ''C''''''.''&SYSLIST(N'&SYSLIST)''(1,1).'''''x
               '''''',BACK
    
    MEND

    MAC 1,2,3,4,5
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002" }));
}

/*
Original valid strings:
    AINSERT '&&C    SETC ''C''''''.''&&SYSLIST(N''&&SYSLIST)''(1,1).'''x
               '''''''''',BACK
*/
TEST(ainsert, grammar_non_matching_apostrophes_by_one_01)
{
    std::string input = R"(
    MACRO
    MAC

    AINSERT '       MACRO',BACK
    AINSERT '       MAC_GEN',BACK
    AINSERT '&&C    SETC ''C''''''.''&&SYSLIST(N''&&SYSLIST)''(1,1).'''x
               ''''''''''',BACK
    AINSERT '       MEND',BACK

    MEND

    MAC 1,2,3,4,5
    MAC_GEN 6,7,8,9
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A011", "S0002", "S0005" }));
}

/*
Original valid strings:
    AINSERT '&&C    SETC  ''C''''''.''&SYSLIST(N'&SYSLIST)''(1,1).'''''x
               '''''''',BACK
*/
TEST(ainsert, grammar_non_matching_apostrophes_by_one_02)
{
    std::string input = R"(
    MACRO
    MAC

    AINSERT '&&C    SETC  ''C''''''.''&SYSLIST(N'&SYSLIST)''(1,1).'''''x
               ''''''''',BACK

    MEND

    MAC 1,2,3,4,5
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0003", "A011", "E076", "E076" }));
}
