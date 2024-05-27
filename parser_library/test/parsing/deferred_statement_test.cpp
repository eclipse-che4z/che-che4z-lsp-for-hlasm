/*
 * Copyright (c) 2021 Broadcom.
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
#include "lsp/lsp_context.h"

using namespace hlasm_plugin::utils::resource;
TEST(deferred_statement, split_var)
{
    std::string input = R"(
   MACRO
   MAC &AA
   &AA C'                                                            &AX
               A'
   MEND

   MAC DC
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    auto aa1 = a.context().lsp_ctx->hover(resource_location(""), { 3, 71 });
    auto aa2 = a.context().lsp_ctx->hover(resource_location(""), { 4, 16 });
    EXPECT_EQ(aa1, "MACRO parameter");
    EXPECT_EQ(aa2, "MACRO parameter");
}

TEST(deferred_statement, nested_macro_def_with_continuation)
{
    std::string input = R"(
 MACRO
 MAC

 MACRO
 MAC2                                                                  X
               &OP=,                                                   X
               &OP2=

 MEND

 MEND

 MAC

 MAC2 OP=A,OP2=B
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(deferred_statement, long_unicode_characters)
{
    std::string input = (const char*)u8R"(     MACRO
     )"
                                     u8"\U0001F600\U0001F600"
                                     u8R"(    &P1
     LARL  0,&P1
     MEND

     MACRO
     )"
                                     u8"\U0001F600"
                                     u8R"(    &P1
     LARL  0,&P1
     )"
                                     u8"\U0001F600\U0001F600"
                                     u8R"( &P1
     MEND

     )"
                                     u8"\U0001F600"
                                     u8R"(        AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAX
               AAAAAA

AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA DS H
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(deferred_statement, recognize_comment)
{
    std::string input = R"(
         MACRO
         MAC   &X
         AIF   (T'&X NE 'O').A                                         X
               (T'&X NE 'O').B,
.A       ANOP
         MEND
         MAC
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(deferred_statement, navigation_for_nonexecuted)
{
    std::string input = R"(
         MACRO
         OUTER
         MEXIT
         INNER
         SAM31
         MEND
*
         OUTER
)";
    resource_location opencode("OPENCODE");
    mock_parse_lib_provider libs({ { "INNER", " MACRO\n INNER\n MEND" } });
    analyzer a(input, analyzer_options(&libs, opencode));
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(a.context().lsp_ctx->definition(opencode, position(4, 10)).get_uri(), "INNER");
    EXPECT_EQ(a.context().lsp_ctx->hover(opencode, position(4, 10)),
        "Statement not executed, macro with matching name available");
    EXPECT_NE(a.context().lsp_ctx->hover(opencode, position(5, 10)).find("Set Addressing Mode"), std::string::npos);
}

TEST(deferred_statement, share_references_for_nonexecuted)
{
    std::string input = R"(
         MACRO
         OUTER
         MEXIT
         INNER
         SAM31
         MEND
*
         OUTER
         INNER
)";
    resource_location opencode("OPENCODE");
    mock_parse_lib_provider libs({ { "INNER", " MACRO\n INNER\n MEND" } });
    analyzer a(input, analyzer_options(&libs, opencode));
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    std::vector<position> expected_positions { position(1, 1), position(4, 9), position(9, 9) };

    auto refs = a.context().lsp_ctx->references(opencode, position(4, 10));
    std::vector<position> positions;
    std::ranges::transform(refs, std::back_inserter(positions), &location::pos);
    std::ranges::sort(positions);
    EXPECT_EQ(positions, expected_positions);
}
