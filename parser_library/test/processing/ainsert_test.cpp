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
    EXPECT_EQ(d.related[0].location.uri, "AINSERT:1");
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
    MAC_AIN
    AINSERT '       MACRO',BACK
    AINSERT '       MAC',BACK
    AINSERT '       GBLA &&A',BACK
    AINSERT '       GBLC &&C1',BACK
    AINSERT '&&A    SETA   &&SYSSTMT',BACK
    AINSERT '&&C1   SETC ''&&SYSSTMT''',BACK
    AINSERT '       MEND',BACK
    AINSERT '&&C2   SETC ''&&SYSSTMT''',BACK
    MEND
    
    GBLA &A
    GBLC &C1
    MAC_AIN
    MAC
    END
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 37);
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C1"), "00000038");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C2"), "00000034");
}

TEST(ainsert, immediate_variable_evaluation)
{
    std::string input = R"(
    MACRO
    MAC_AIN
    AINSERT '       MACRO',BACK
    AINSERT '       MAC',BACK
    AINSERT '       GBLA &&A',BACK
    AINSERT '       GBLC &&C1',BACK
    AINSERT '&&A    SETA   &SYSSTMT',BACK
    AINSERT '&&C1   SETC ''&SYSSTMT''',BACK
    AINSERT '       MEND',BACK
    AINSERT '&&C2   SETC ''&SYSSTMT''',BACK
    MEND

    GBLA &A
    GBLC &C1
    MAC_AIN
    MAC
    END
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 21);
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C1"), "00000022");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C2"), "00000024");
}