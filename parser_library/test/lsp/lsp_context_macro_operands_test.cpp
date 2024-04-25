/*
 * Copyright (c) 2023 Broadcom.
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
#include "completion_trigger_kind.h"
#include "lsp/lsp_context.h"
#include "workspaces/workspace.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lsp;

struct lsp_context_macro_operands : public analyzer_fixture
{
    const static inline std::string input =
        R"(
         MACRO
         MAC   &A,&B=(DEFAULTB,                                        X
               1)

         MEND

         MAC   
               
         MAC                                                           X
               
         MAC                                                           X
                                                                       X
               
         LARL  
)";
    const context::macro_definition* mac;

    lsp_context_macro_operands()
        : analyzer_fixture(input)
    {
        mac = a.context().lsp_ctx->get_macro_info(context::id_index("MAC"))->macro_definition.get();
    }
};

TEST_F(lsp_context_macro_operands, non_continued)
{
    auto res = a.context().lsp_ctx->completion(opencode_loc, { 7, 15 }, 0, completion_trigger_kind::invoked);
    auto md_sec_p = std::get_if<
        std::pair<const context::macro_definition*, std::vector<std::pair<const context::symbol*, context::id_index>>>>(
        &res);
    ASSERT_TRUE(md_sec_p);
    EXPECT_EQ(md_sec_p->first, mac);
}

TEST_F(lsp_context_macro_operands, empty_line_after_macro)
{
    auto res = a.context().lsp_ctx->completion(opencode_loc, { 8, 15 }, 0, completion_trigger_kind::invoked);
    EXPECT_TRUE(std::holds_alternative<completion_list_instructions>(res));
}

TEST_F(lsp_context_macro_operands, continued_line_edge)
{
    auto res = a.context().lsp_ctx->completion(opencode_loc, { 10, 15 }, 0, completion_trigger_kind::invoked);
    auto md_sec_p = std::get_if<
        std::pair<const context::macro_definition*, std::vector<std::pair<const context::symbol*, context::id_index>>>>(
        &res);
    ASSERT_TRUE(md_sec_p);
    EXPECT_EQ(md_sec_p->first, mac);
}

TEST_F(lsp_context_macro_operands, second_continued_line_edge)
{
    auto res = a.context().lsp_ctx->completion(opencode_loc, { 13, 15 }, 0, completion_trigger_kind::invoked);
    auto md_sec_p = std::get_if<
        std::pair<const context::macro_definition*, std::vector<std::pair<const context::symbol*, context::id_index>>>>(
        &res);
    ASSERT_TRUE(md_sec_p);
    EXPECT_EQ(md_sec_p->first, mac);
}

TEST_F(lsp_context_macro_operands, machine_instruction)
{
    auto res = a.context().lsp_ctx->completion(opencode_loc, { 14, 15 }, 0, completion_trigger_kind::invoked);
    auto md_sec_p = std::get_if<
        std::pair<const context::macro_definition*, std::vector<std::pair<const context::symbol*, context::id_index>>>>(
        &res);
    ASSERT_TRUE(md_sec_p);
    EXPECT_FALSE(md_sec_p->first);
}
