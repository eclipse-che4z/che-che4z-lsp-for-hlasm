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
    location res = a.context().lsp_ctx->definition(dummy_file_name, { 2, 7 });
    EXPECT_EQ(res.file, dummy_file_name);
    EXPECT_EQ(res.pos, position(1, 0));
}

TEST_F(lsp_context_var_symbol_SET, references)
{
    auto res = a.context().lsp_ctx->references(dummy_file_name, { 2, 7 });
    ASSERT_EQ(res.size(), 2U);

    EXPECT_EQ(res[0].file, dummy_file_name);
    EXPECT_EQ(res[0].pos, position(1, 0));
    EXPECT_EQ(res[1].file, dummy_file_name);
    EXPECT_EQ(res[1].pos, position(2, 6));
}

TEST_F(lsp_context_var_symbol_SET, hover)
{
    auto res = a.context().lsp_ctx->hover(dummy_file_name, { 2, 7 });


    EXPECT_EQ(res, "SETA variable");
}

TEST_F(lsp_context_var_symbol_SET, completion)
{
    auto res =
        a.context().lsp_ctx->completion(dummy_file_name, { 3, 2 }, '&', completion_trigger_kind::trigger_character);

    ASSERT_EQ(res.size(), 1U);
    lsp::completion_item_s expected("&VAR", "SETA variable", "&VAR", "", completion_item_kind::var_sym);
    EXPECT_EQ(res[0], expected);
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

    location res = a.context().lsp_ctx->definition(dummy_file_name, { 2, 7 });
    EXPECT_EQ(res.file, dummy_file_name);
    EXPECT_EQ(res.pos, position(1, 6));
}

TEST_F(lsp_context_var_symbol_GBL, references)
{
    auto res = a.context().lsp_ctx->references(dummy_file_name, { 2, 7 });
    ASSERT_EQ(res.size(), 2U);

    EXPECT_EQ(res[0].file, dummy_file_name);
    EXPECT_EQ(res[0].pos, position(1, 6));
    EXPECT_EQ(res[1].file, dummy_file_name);
    EXPECT_EQ(res[1].pos, position(2, 6));
}

TEST_F(lsp_context_var_symbol_GBL, hover)
{
    auto res = a.context().lsp_ctx->hover(dummy_file_name, { 2, 7 });

    EXPECT_EQ(res, "SETC variable");
}

TEST_F(lsp_context_var_symbol_GBL, completion)
{
    auto res =
        a.context().lsp_ctx->completion(dummy_file_name, { 3, 2 }, '&', completion_trigger_kind::trigger_character);

    ASSERT_EQ(res.size(), 1U);
    lsp::completion_item_s expected("&VAR", "SETC variable", "&VAR", "", completion_item_kind::var_sym);
    EXPECT_EQ(res[0], expected);
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
    auto res = a.context().lsp_ctx->definition(dummy_file_name, { 2, 7 });
    EXPECT_EQ(res.file, dummy_file_name);
    EXPECT_EQ(res.pos, position(1, 6));
}

TEST_F(lsp_context_var_symbol_LCL, references)
{
    auto res = a.context().lsp_ctx->references(dummy_file_name, { 2, 7 });
    ASSERT_EQ(res.size(), 2U);

    EXPECT_EQ(res[0].file, dummy_file_name);
    EXPECT_EQ(res[0].pos, position(1, 6));
    EXPECT_EQ(res[1].file, dummy_file_name);
    EXPECT_EQ(res[1].pos, position(2, 6));
}

TEST_F(lsp_context_var_symbol_LCL, hover)
{
    auto res = a.context().lsp_ctx->hover(dummy_file_name, { 2, 7 });

    EXPECT_EQ(res, "SETB variable");
}

TEST_F(lsp_context_var_symbol_LCL, completion)
{
    auto res =
        a.context().lsp_ctx->completion(dummy_file_name, { 3, 2 }, '&', completion_trigger_kind::trigger_character);

    ASSERT_EQ(res.size(), 1U);
    lsp::completion_item_s expected("&VAR", "SETB variable", "&VAR", "", completion_item_kind::var_sym);
    EXPECT_EQ(res[0], expected);
}