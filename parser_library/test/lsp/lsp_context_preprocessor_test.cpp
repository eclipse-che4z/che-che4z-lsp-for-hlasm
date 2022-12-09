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

#include <string>

#include "gtest/gtest.h"

#include "../mock_parse_lib_provider.h"
#include "analyzer.h"
#include "context/instruction.h"
#include "location.h"
#include "range.h"
#include "utils/resource_location.h"
#include "workspaces/parse_lib_provider.h"


using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::utils::resource;
using namespace hlasm_plugin::parser_library::lsp;

namespace {
const resource_location mac_loc("MAC");
const resource_location source_loc("OPEN");
const resource_location member_loc("MEMBER");
const resource_location member2_loc("MEMBER2");
} // namespace

class lsp_context_endevor_preprocessor_test : public testing::Test
{
public:
    lsp_context_endevor_preprocessor_test()
        : lib_provider({ { "MEMBER", R"(R2 EQU 2
            LR R2,R2)" },
            { "MEMBER2", R"(R5 EQU 5
            LR R5,R5)" } })
        , a(contents, analyzer_options { source_loc, &lib_provider, endevor_preprocessor_options() }) {};

    void SetUp() override { a.analyze(); }
    void TearDown() override {}

protected:
    const std::string contents =
        R"(
-INC  MEMBER blabla
++INCLUDE  MEMBER blabla
-INC  MEMBER2 blabla
)";
    std::string content;
    mock_parse_lib_provider lib_provider;
    analyzer a;
};

TEST_F(lsp_context_endevor_preprocessor_test, go_to)
{
    // no jump, instr -INC
    EXPECT_EQ(location(position(1, 1), source_loc), a.context().lsp_ctx->definition(source_loc, position(1, 1)));
    // no jump, instr ++INCLUDE
    EXPECT_EQ(location(position(2, 5), source_loc), a.context().lsp_ctx->definition(source_loc, position(2, 5)));
    // no jump, instr -INC
    EXPECT_EQ(location(position(3, 1), source_loc), a.context().lsp_ctx->definition(source_loc, position(3, 1)));

    // jump from source to included file
    EXPECT_EQ(location(position(0, 0), member_loc), a.context().lsp_ctx->definition(source_loc, position(1, 8)));
    // jump from source to included file
    EXPECT_EQ(location(position(0, 0), member_loc), a.context().lsp_ctx->definition(source_loc, position(2, 14)));
    // jump from source to included file
    EXPECT_EQ(location(position(0, 0), member2_loc), a.context().lsp_ctx->definition(source_loc, position(3, 8)));

    // no jump
    EXPECT_EQ(location(position(1, 15), source_loc), a.context().lsp_ctx->definition(source_loc, position(1, 15)));
    // no jump
    EXPECT_EQ(location(position(2, 21), source_loc), a.context().lsp_ctx->definition(source_loc, position(2, 21)));
    // no jump
    EXPECT_EQ(location(position(3, 15), source_loc), a.context().lsp_ctx->definition(source_loc, position(3, 15)));
}

TEST_F(lsp_context_endevor_preprocessor_test, refs)
{
    location_list expected_inc_locations {
        location(position(1, 0), source_loc), location(position(2, 0), source_loc), location(position(3, 0), source_loc)
    };
    location_list expected_member_locations { location(position(1, 6), source_loc),
        location(position(2, 11), source_loc) };
    location_list expected_member2_locations { location(position(3, 6), source_loc) };
    location_list expected_blabla_locations {};

    // -INC/++INCLUDE reference
    EXPECT_EQ(expected_inc_locations, a.context().lsp_ctx->references(source_loc, position(1, 1)));
    // -INC/++INCLUDE reference
    EXPECT_EQ(expected_inc_locations, a.context().lsp_ctx->references(source_loc, position(2, 5)));
    // -INC/++INCLUDE reference
    EXPECT_EQ(expected_inc_locations, a.context().lsp_ctx->references(source_loc, position(3, 2)));

    // MEMBER reference
    EXPECT_EQ(expected_member_locations, a.context().lsp_ctx->references(source_loc, position(1, 8)));
    // MEMBER reference
    EXPECT_EQ(expected_member_locations, a.context().lsp_ctx->references(source_loc, position(2, 14)));
    // MEMBER reference
    EXPECT_EQ(expected_member2_locations, a.context().lsp_ctx->references(source_loc, position(3, 8)));

    // blabla reference
    EXPECT_EQ(expected_blabla_locations, a.context().lsp_ctx->references(source_loc, position(1, 15)));
    // blabla reference
    EXPECT_EQ(expected_blabla_locations, a.context().lsp_ctx->references(source_loc, position(2, 21)));
    // blabla reference
    EXPECT_EQ(expected_blabla_locations, a.context().lsp_ctx->references(source_loc, position(3, 15)));
}
