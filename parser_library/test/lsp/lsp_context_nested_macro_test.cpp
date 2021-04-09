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

#include "analyzer_fixture.h"


using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lsp;

struct lsp_context_nested_macro : public analyzer_fixture
{
    const static inline std::string input =
        R"(
       MACRO
       MAC 
       LCLA &VAR

         MACRO
         NESTED &P
           LCLA &VAR
           LR &VAR,&P
         MEND
       LR &VAR,1
       MEND
       MAC
)";


    lsp_context_nested_macro()
        : analyzer_fixture(input)
    {}
};


TEST_F(lsp_context_nested_macro, definition_nested)
{
    location res = a.context().lsp_ctx->definition(opencode_file_name, { 8, 15 });
    EXPECT_EQ(res.file, opencode_file_name);
    EXPECT_EQ(res.pos, position(7, 16));
}

TEST_F(lsp_context_nested_macro, references_inner_var)
{
    auto res = a.context().lsp_ctx->references(opencode_file_name, { 8, 15 });

    ASSERT_EQ(res.size(), 2U);

    EXPECT_EQ(res[0].file, opencode_file_name);
    EXPECT_EQ(res[0].pos, position(7, 16));
    EXPECT_EQ(res[1].file, opencode_file_name);
    EXPECT_EQ(res[1].pos, position(8, 14));
}

TEST_F(lsp_context_nested_macro, references_inner_macro_param)
{
    auto res = a.context().lsp_ctx->references(opencode_file_name, { 8, 20 });

    ASSERT_EQ(res.size(), 2U);

    EXPECT_EQ(res[0].file, opencode_file_name);
    EXPECT_EQ(res[0].pos, position(6, 16));
    EXPECT_EQ(res[1].file, opencode_file_name);
    EXPECT_EQ(res[1].pos, position(8, 19));
}

TEST_F(lsp_context_nested_macro, definition_outer)
{
    location res = a.context().lsp_ctx->definition(opencode_file_name, { 10, 11 });
    EXPECT_EQ(res.file, opencode_file_name);
    EXPECT_EQ(res.pos, position(3, 12));
}

TEST_F(lsp_context_nested_macro, references_outer)
{
    auto res = a.context().lsp_ctx->references(opencode_file_name, { 10, 11 });

    ASSERT_EQ(res.size(), 2U);

    EXPECT_EQ(res[0].file, opencode_file_name);
    EXPECT_EQ(res[0].pos, position(3, 12));
    EXPECT_EQ(res[1].file, opencode_file_name);
    EXPECT_EQ(res[1].pos, position(10, 10));
}
