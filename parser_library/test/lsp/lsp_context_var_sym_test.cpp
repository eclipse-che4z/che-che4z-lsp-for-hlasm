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
#include "lsp/lsp_context.h"
#include "workspaces/workspace.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lsp;


struct lsp_context_var_symbol_SET : public analyzer_fixture
{
    const static inline std::string input =
        R"(
&VAR SETA 1
 LR 1,&VAR
 &
)";

    lsp_context_var_symbol_SET()
        : analyzer_fixture(input)
    {}
};

TEST_F(lsp_context_var_symbol_SET, definition)
{
    location res = a.context().lsp_ctx->definition(opencode_loc, { 2, 7 });
    EXPECT_EQ(res, location(position(1, 0), opencode_loc));
}

TEST_F(lsp_context_var_symbol_SET, references)
{
    auto res = a.context().lsp_ctx->references(opencode_loc, { 2, 7 });
    ASSERT_EQ(res.size(), 2U);

    EXPECT_EQ(res[0], location(position(1, 0), opencode_loc));
    EXPECT_EQ(res[1], location(position(2, 6), opencode_loc));
}

TEST_F(lsp_context_var_symbol_SET, hover)
{
    auto res = a.context().lsp_ctx->hover(opencode_loc, { 2, 7 });


    EXPECT_EQ(res, "SETA variable");
}

TEST_F(lsp_context_var_symbol_SET, completion)
{
    auto res_v =
        a.context().lsp_ctx->completion(opencode_loc, { 3, 2 }, '&', completion_trigger_kind::trigger_character);

    ASSERT_TRUE(std::holds_alternative<const lsp::vardef_storage*>(res_v));

    const auto& res = *std::get<const lsp::vardef_storage*>(res_v);

    ASSERT_EQ(res.size(), 1U);
    EXPECT_EQ(res.begin()->name.to_string_view(), "VAR");
}


struct lsp_context_var_symbol_GBL : public analyzer_fixture
{
    const static inline std::string input =
        R"(
 GBLC &VAR
 LR 1,&VAR
 &
)";

    lsp_context_var_symbol_GBL()
        : analyzer_fixture(input)
    {}
};


TEST_F(lsp_context_var_symbol_GBL, definition)
{
    location res = a.context().lsp_ctx->definition(opencode_loc, { 2, 7 });
    EXPECT_EQ(res, location(position(1, 6), opencode_loc));
}

TEST_F(lsp_context_var_symbol_GBL, references)
{
    auto res = a.context().lsp_ctx->references(opencode_loc, { 2, 7 });
    ASSERT_EQ(res.size(), 2U);

    EXPECT_EQ(res[0], location(position(1, 6), opencode_loc));
    EXPECT_EQ(res[1], location(position(2, 6), opencode_loc));
}

TEST_F(lsp_context_var_symbol_GBL, hover)
{
    auto res = a.context().lsp_ctx->hover(opencode_loc, { 2, 7 });

    EXPECT_EQ(res, "SETC variable");
}

TEST_F(lsp_context_var_symbol_GBL, completion)
{
    auto res_v =
        a.context().lsp_ctx->completion(opencode_loc, { 3, 2 }, '&', completion_trigger_kind::trigger_character);

    ASSERT_TRUE(std::holds_alternative<const vardef_storage*>(res_v));

    const auto& res = *std::get<const vardef_storage*>(res_v);

    ASSERT_EQ(res.size(), 1U);
    EXPECT_EQ(res.begin()->name.to_string_view(), "VAR");
}


struct lsp_context_var_symbol_LCL : public analyzer_fixture
{
    const static inline std::string input =
        R"(
 LCLB &VAR
 LR 1,&VAR
 &
)";

    lsp_context_var_symbol_LCL()
        : analyzer_fixture(input)
    {}
};

TEST_F(lsp_context_var_symbol_LCL, definition)
{
    auto res = a.context().lsp_ctx->definition(opencode_loc, { 2, 7 });
    EXPECT_EQ(res, location(position(1, 6), opencode_loc));
}

TEST_F(lsp_context_var_symbol_LCL, references)
{
    auto res = a.context().lsp_ctx->references(opencode_loc, { 2, 7 });
    ASSERT_EQ(res.size(), 2U);

    EXPECT_EQ(res[0], location(position(1, 6), opencode_loc));
    EXPECT_EQ(res[1], location(position(2, 6), opencode_loc));
}

TEST_F(lsp_context_var_symbol_LCL, hover)
{
    auto res = a.context().lsp_ctx->hover(opencode_loc, { 2, 7 });

    EXPECT_EQ(res, "SETB variable");
}

TEST_F(lsp_context_var_symbol_LCL, completion)
{
    auto res_v =
        a.context().lsp_ctx->completion(opencode_loc, { 3, 2 }, '&', completion_trigger_kind::trigger_character);

    ASSERT_TRUE(std::holds_alternative<const vardef_storage*>(res_v));

    const auto& res = *std::get<const vardef_storage*>(res_v);

    ASSERT_EQ(res.size(), 1U);
    EXPECT_EQ(res.begin()->name.to_string_view(), "VAR");
}

struct lsp_context_var_symbol_no_definition : public analyzer_fixture
{
    const static inline std::string input = " LR &VAR,1";

    lsp_context_var_symbol_no_definition()
        : analyzer_fixture(input)
    {}
};

TEST_F(lsp_context_var_symbol_no_definition, definition)
{
    auto res = a.context().lsp_ctx->definition(opencode_loc, { 0, 6 });
    EXPECT_EQ(res, location(position(0, 6), opencode_loc));
}

TEST_F(lsp_context_var_symbol_no_definition, references)
{
    auto res = a.context().lsp_ctx->references(opencode_loc, { 0, 6 });
    ASSERT_EQ(res.size(), 1U);

    EXPECT_EQ(res[0], location(position(0, 4), opencode_loc));
}

TEST_F(lsp_context_var_symbol_no_definition, hover)
{
    auto res = a.context().lsp_ctx->hover(opencode_loc, { 0, 6 });

    EXPECT_EQ(res, "");
}

TEST_F(lsp_context_var_symbol_no_definition, definition_no_occurrence)
{
    auto res = a.context().lsp_ctx->definition(opencode_loc, { 0, 9 });
    EXPECT_EQ(res, location(position(0, 9), opencode_loc));
}

TEST_F(lsp_context_var_symbol_no_definition, references_no_occurrence)
{
    auto res = a.context().lsp_ctx->references(opencode_loc, { 0, 9 });
    ASSERT_EQ(res.size(), 0U);
}

TEST_F(lsp_context_var_symbol_no_definition, hover_no_occurrence)
{
    auto res = a.context().lsp_ctx->hover(opencode_loc, { 0, 9 });

    EXPECT_EQ(res, "");
}
