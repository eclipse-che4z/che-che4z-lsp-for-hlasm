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
#include "lsp_context_test_helper.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lsp;

struct lsp_context_seq_sym : public analyzer_fixture
{
    const static inline std::string input =
        R"(
       MACRO
       MAC
       
       AGO .INMAC
.INMAC ANOP
.
       MEND

       MAC

       AGO .OUTMAC
.OUTMAC ANOP
.
       AGO .NOTEXIST
)";


    lsp_context_seq_sym()
        : analyzer_fixture(input)
    {}
};


TEST_F(lsp_context_seq_sym, definition_in_macro)
{
    location res = a.context().lsp_ctx->definition(opencode_loc, { 4, 12 });
    check_location_with_position(res, opencode_loc, 5, 0);
}

TEST_F(lsp_context_seq_sym, definition_out_of_macro)
{
    location res = a.context().lsp_ctx->definition(opencode_loc, { 11, 12 });
    check_location_with_position(res, opencode_loc, 12, 0);
}

TEST_F(lsp_context_seq_sym, references_in_macro)
{
    auto res = a.context().lsp_ctx->references(opencode_loc, { 4, 12 });

    ASSERT_EQ(res.size(), 2U);

    check_location_with_position(res[0], opencode_loc, 4, 11);
    check_location_with_position(res[1], opencode_loc, 5, 0);
}

TEST_F(lsp_context_seq_sym, references_out_of_macro)
{
    auto res = a.context().lsp_ctx->references(opencode_loc, { 11, 12 });

    ASSERT_EQ(res.size(), 2U);

    check_location_with_position(res[0], opencode_loc, 11, 11);
    check_location_with_position(res[1], opencode_loc, 12, 0);
}

TEST_F(lsp_context_seq_sym, hover)
{
    auto res = a.context().lsp_ctx->hover(opencode_loc, { 4, 12 });

    EXPECT_EQ(res, "Sequence symbol");
}

TEST_F(lsp_context_seq_sym, completion_in_macro)
{
    auto res = a.context().lsp_ctx->completion(opencode_loc, { 6, 1 }, '\0', completion_trigger_kind::invoked);


    lsp::completion_item_s expected(".INMAC", "Sequence symbol", ".INMAC", "", completion_item_kind::seq_sym);

    ASSERT_EQ(res.size(), 1U);
    EXPECT_EQ(res[0], expected);
}

TEST_F(lsp_context_seq_sym, completion_out_of_macro)
{
    auto res = a.context().lsp_ctx->completion(opencode_loc, { 13, 1 }, '\0', completion_trigger_kind::invoked);


    lsp::completion_item_s expected(".OUTMAC", "Sequence symbol", ".OUTMAC", "", completion_item_kind::seq_sym);

    ASSERT_EQ(res.size(), 1U);
    EXPECT_EQ(res[0], expected);
}



TEST_F(lsp_context_seq_sym, definition_no_definition)
{
    location res = a.context().lsp_ctx->definition(opencode_loc, { 14, 12 });
    check_location_with_position(res, opencode_loc, 14, 12);
}

TEST_F(lsp_context_seq_sym, references_no_definition)
{
    auto res = a.context().lsp_ctx->references(opencode_loc, { 14, 12 });

    ASSERT_EQ(res.size(), 1U);

    check_location_with_position(res[0], opencode_loc, 14, 11);
}
