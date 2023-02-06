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
#include "lsp_context_test_helper.h"

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
    location res = a.context().lsp_ctx->definition(opencode_loc, { 8, 15 });
    check_location_with_position(res, opencode_loc, 7, 16);
}

TEST_F(lsp_context_nested_macro, references_inner_var)
{
    auto res = a.context().lsp_ctx->references(opencode_loc, { 8, 15 });

    ASSERT_EQ(res.size(), 2U);

    check_location_with_position(res[0], opencode_loc, 7, 16);
    check_location_with_position(res[1], opencode_loc, 8, 14);
}

TEST_F(lsp_context_nested_macro, references_inner_macro_param)
{
    auto res = a.context().lsp_ctx->references(opencode_loc, { 8, 20 });

    ASSERT_EQ(res.size(), 2U);

    check_location_with_position(res[0], opencode_loc, 6, 16);
    check_location_with_position(res[1], opencode_loc, 8, 19);
}

TEST_F(lsp_context_nested_macro, definition_outer)
{
    location res = a.context().lsp_ctx->definition(opencode_loc, { 10, 11 });
    check_location_with_position(res, opencode_loc, 3, 12);
}

TEST_F(lsp_context_nested_macro, references_outer)
{
    auto res = a.context().lsp_ctx->references(opencode_loc, { 10, 11 });

    ASSERT_EQ(res.size(), 2U);

    check_location_with_position(res[0], opencode_loc, 3, 12);
    check_location_with_position(res[1], opencode_loc, 10, 10);
}
