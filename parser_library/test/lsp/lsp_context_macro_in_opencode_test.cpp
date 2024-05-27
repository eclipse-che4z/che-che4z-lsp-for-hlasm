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
#include "completion_item.h"
#include "completion_trigger_kind.h"
#include "lsp/lsp_context.h"
#include "workspaces/workspace.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lsp;

struct lsp_context_macro_in_opencode : public analyzer_fixture
{
    const static inline std::string input =
        R"(
       MACRO
&LABEL MAC &POS_PAR,&KEY_PAR=1
&LABEL LR &POS_PAR,&KEY_PAR
&
       MEND

LAB    MAC 1,KEY_PAR=1

 LCLA &KEY_PAR
 LR &KEY_PAR,1
&
       UNKNOWN

       LARL 0,LAB
)";

    lsp_context_macro_in_opencode()
        : analyzer_fixture(input)
    {}
};

TEST_F(lsp_context_macro_in_opencode, definition_macro)
{
    location res = a.context().lsp_ctx->definition(opencode_loc, { 7, 8 });
    EXPECT_EQ(res, location(position(2, 7), opencode_loc));
}

TEST_F(lsp_context_macro_in_opencode, references_macro)
{
    auto res = a.context().lsp_ctx->references(opencode_loc, { 7, 8 });
    ASSERT_EQ(res.size(), 2U);

    EXPECT_EQ(res[0], location(position(2, 7), opencode_loc));
    EXPECT_EQ(res[1], location(position(7, 7), opencode_loc));
}

TEST_F(lsp_context_macro_in_opencode, hover_macro)
{
    auto res = a.context().lsp_ctx->hover(opencode_loc, { 7, 8 });

    EXPECT_EQ(res, R"(```hlasm
&LABEL MAC &POS_PAR,&KEY_PAR=1
```
)");
}


TEST_F(lsp_context_macro_in_opencode, definition_macro_param_positional)
{
    location res = a.context().lsp_ctx->definition(opencode_loc, { 3, 11 });
    EXPECT_EQ(res, location(position(2, 11), opencode_loc));
}

TEST_F(lsp_context_macro_in_opencode, references_macro_param_positional)
{
    auto res = a.context().lsp_ctx->references(opencode_loc, { 3, 11 });
    ASSERT_EQ(res.size(), 2U);

    EXPECT_EQ(res[0], location(position(2, 11), opencode_loc));
    EXPECT_EQ(res[1], location(position(3, 10), opencode_loc));
}

TEST_F(lsp_context_macro_in_opencode, hover_macro_param)
{
    auto res = a.context().lsp_ctx->hover(opencode_loc, { 3, 11 });

    EXPECT_EQ(res, "MACRO parameter");
}


TEST_F(lsp_context_macro_in_opencode, definition_macro_param_keyword)
{
    location res = a.context().lsp_ctx->definition(opencode_loc, { 3, 20 });
    EXPECT_EQ(res, location(position(2, 20), opencode_loc));
}

TEST_F(lsp_context_macro_in_opencode, references_macro_param_keyword)
{
    auto res = a.context().lsp_ctx->references(opencode_loc, { 3, 20 });
    ASSERT_EQ(res.size(), 2U);

    EXPECT_EQ(res[0], location(position(2, 20), opencode_loc));
    EXPECT_EQ(res[1], location(position(3, 19), opencode_loc));
}

TEST_F(lsp_context_macro_in_opencode, definition_macro_param_label)
{
    location res = a.context().lsp_ctx->definition(opencode_loc, { 3, 1 });
    EXPECT_EQ(res, location(position(2, 0), opencode_loc));
}

TEST_F(lsp_context_macro_in_opencode, references_macro_param_label)
{
    auto res = a.context().lsp_ctx->references(opencode_loc, { 3, 1 });
    ASSERT_EQ(res.size(), 2U);

    EXPECT_EQ(res[0], location(position(2, 0), opencode_loc));
    EXPECT_EQ(res[1], location(position(3, 0), opencode_loc));
}

TEST_F(lsp_context_macro_in_opencode, definition_local_var_same_name)
{
    location res = a.context().lsp_ctx->definition(opencode_loc, { 10, 5 });
    EXPECT_EQ(res, location(position(9, 6), opencode_loc));
}

TEST_F(lsp_context_macro_in_opencode, references_local_var_same_name)
{
    auto res = a.context().lsp_ctx->references(opencode_loc, { 10, 5 });
    ASSERT_EQ(res.size(), 2U);

    EXPECT_EQ(res[0], location(position(9, 6), opencode_loc));
    EXPECT_EQ(res[1], location(position(10, 4), opencode_loc));
}

TEST_F(lsp_context_macro_in_opencode, completion_var_in_macro)
{
    auto res_v = a.context().lsp_ctx->completion(opencode_loc, { 4, 1 }, '\0', completion_trigger_kind::invoked);

    ASSERT_TRUE(std::holds_alternative<const vardef_storage*>(res_v));

    const auto& res_map = *std::get<const vardef_storage*>(res_v);
    std::vector<std::string_view> res;
    std::ranges::transform(res_map, std::back_inserter(res), [](const auto& v) { return v.name.to_string_view(); });

    const std::vector<std::string_view> expected { "KEY_PAR", "LABEL", "POS_PAR" };

    EXPECT_TRUE(std::ranges::is_permutation(res, expected));
    EXPECT_TRUE(std::ranges::all_of(res_map, [](const auto& v) { return v.macro_param; }));
}

TEST_F(lsp_context_macro_in_opencode, completion_var_outside_macro)
{
    auto res_v = a.context().lsp_ctx->completion(opencode_loc, { 11, 1 }, '\0', completion_trigger_kind::invoked);

    ASSERT_TRUE(std::holds_alternative<const vardef_storage*>(res_v));

    const auto& res_map = *std::get<const vardef_storage*>(res_v);
    std::vector<std::string_view> res;
    std::ranges::transform(res_map, std::back_inserter(res), [](const auto& v) { return v.name.to_string_view(); });

    const std::vector<std::string_view> expected { "KEY_PAR" };

    EXPECT_TRUE(std::ranges::is_permutation(res, expected));
    EXPECT_TRUE(std::ranges::none_of(res_map, &variable_symbol_definition::macro_param));
}

TEST_F(lsp_context_macro_in_opencode, hover_unknown_macro)
{
    auto res = a.context().lsp_ctx->hover(opencode_loc, { 12, 10 });

    EXPECT_EQ(res, "");
}

TEST_F(lsp_context_macro_in_opencode, definition_macro_label)
{
    location macro_call = a.context().lsp_ctx->definition(opencode_loc, { 14, 15 });
    EXPECT_EQ(macro_call, location(position(7, 0), opencode_loc));

    location in_macro = a.context().lsp_ctx->definition(opencode_loc, { 7, 0 });
    EXPECT_EQ(in_macro, location(position(3, 0), opencode_loc));
}
