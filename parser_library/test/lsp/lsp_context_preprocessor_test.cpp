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

#include "../mock_parse_lib_provider.h"
#include "analyzer.h"
#include "context/instruction.h"
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
const resource_location preproc5_loc("PREPROCESSOR:5.hlasm");
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
-INC  MEMBER2)";

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
    EXPECT_EQ(location(position(0, 3), member_loc), a.context().lsp_ctx->definition(source_loc, position(1, 8)));
    // jump from source to included file
    EXPECT_EQ(location(position(0, 3), member_loc), a.context().lsp_ctx->definition(source_loc, position(2, 14)));
    // jump from source to included file
    EXPECT_EQ(location(position(0, 3), member2_loc), a.context().lsp_ctx->definition(source_loc, position(3, 8)));

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
}

class lsp_context_cics_preprocessor_test : public testing::Test
{
public:
    lsp_context_cics_preprocessor_test()
        : a(contents, analyzer_options { source_loc, cics_preprocessor_options() }) {};

    void SetUp() override { a.analyze(); }
    void TearDown() override {}

protected:
    const std::string contents =
        R"(
A   EXEC CICS ABEND ABCODE('1234') NODUMP
  EXEC  CICS  ALLOCATE SYSID('4321') NOQUEUE
     EXEC  CICS  ABEND  ABCODE('1234')  NODUMP

B   LARL 0,DFHRESP(NORMAL)
    L   0,DFHRESP(BUSY)
    LARL  0,DFHVALUE ( BUSY ))";

    analyzer a;
};

TEST_F(lsp_context_cics_preprocessor_test, go_to_exec_cics)
{
    // jump to virtual file, label A
    EXPECT_EQ(location(position(1, 0), preproc5_loc), a.context().lsp_ctx->definition(source_loc, position(1, 1)));
    // no jump, EXEC CICS ABEND
    EXPECT_EQ(location(position(1, 16), source_loc), a.context().lsp_ctx->definition(source_loc, position(1, 16)));
    // no jump, operand ABCODE
    EXPECT_EQ(location(position(1, 23), source_loc), a.context().lsp_ctx->definition(source_loc, position(1, 23)));
    // no jump, operand 1234
    EXPECT_EQ(location(position(1, 30), source_loc), a.context().lsp_ctx->definition(source_loc, position(1, 30)));
    // no jump, operand NODUMP
    EXPECT_EQ(location(position(1, 41), source_loc), a.context().lsp_ctx->definition(source_loc, position(1, 41)));
}

TEST_F(lsp_context_cics_preprocessor_test, refs_exec_cics)
{
    const location_list expected_exec_cics_abend_locations {
        location(position(1, 4), source_loc),
        location(position(3, 5), source_loc),
    };
    const location_list expected_abcode_locations {
        location(position(1, 20), source_loc),
        location(position(3, 24), source_loc),
    };
    const location_list expected_1234_locations {
        location(position(1, 28), source_loc),
        location(position(3, 32), source_loc),
    };
    const location_list expected_nodump_locations {
        location(position(1, 35), source_loc),
        location(position(3, 40), source_loc),
    };
    const location_list expected_exec_cics_allocate_locations { location(position(2, 2), source_loc) };
    const location_list expected_sysid_locations { location(position(2, 23), source_loc) };
    const location_list expected_4321_locations { location(position(2, 30), source_loc) };
    const location_list expected_noqueue_locations { location(position(2, 37), source_loc) };

    // EXEC CICS ABEND reference
    EXPECT_EQ(expected_exec_cics_abend_locations, a.context().lsp_ctx->references(source_loc, position(1, 7)));
    // ABCODE reference
    EXPECT_EQ(expected_abcode_locations, a.context().lsp_ctx->references(source_loc, position(1, 25)));
    // '1234' reference
    EXPECT_EQ(expected_1234_locations, a.context().lsp_ctx->references(source_loc, position(1, 29)));
    // NODUMP reference
    EXPECT_EQ(expected_nodump_locations, a.context().lsp_ctx->references(source_loc, position(1, 39)));

    // ALLOCATE reference
    EXPECT_EQ(expected_exec_cics_allocate_locations, a.context().lsp_ctx->references(source_loc, position(2, 18)));
    // SYSID reference
    EXPECT_EQ(expected_sysid_locations, a.context().lsp_ctx->references(source_loc, position(2, 25)));
    // '4321' reference
    EXPECT_EQ(expected_4321_locations, a.context().lsp_ctx->references(source_loc, position(2, 31)));
    // NOQUEUE reference
    EXPECT_EQ(expected_noqueue_locations, a.context().lsp_ctx->references(source_loc, position(2, 42)));
}

TEST_F(lsp_context_cics_preprocessor_test, go_to_dfh)
{
    // jump to virtual file, label B
    EXPECT_EQ(location(position(1, 0), preproc5_loc), a.context().lsp_ctx->definition(source_loc, position(5, 1)));
    // no jump, instr LARL
    EXPECT_EQ(location(position(5, 7), source_loc), a.context().lsp_ctx->definition(source_loc, position(5, 7)));
    // no jump, operand 0
    EXPECT_EQ(location(position(5, 11), source_loc), a.context().lsp_ctx->definition(source_loc, position(5, 11)));
    // no jump, operand DFHRESP
    EXPECT_EQ(location(position(5, 17), source_loc), a.context().lsp_ctx->definition(source_loc, position(5, 17)));
    // no jump, operand NORMAL
    EXPECT_EQ(location(position(5, 25), source_loc), a.context().lsp_ctx->definition(source_loc, position(5, 25)));
}

TEST_F(lsp_context_cics_preprocessor_test, refs_dfh)
{
    const location_list expected_larl_locations {
        location(position(5, 4), source_loc),
        location(position(7, 4), source_loc),
        location(position(1, 9), preproc5_loc),
        location(position(5, 9), preproc5_loc),
    };
    const location_list expected_l_locations {
        location(position(6, 4), source_loc),
        location(position(3, 9), preproc5_loc),
    };
    const location_list expected_dfhresp_locations {
        location(position(5, 11), source_loc),
        location(position(6, 10), source_loc),
    };
    const location_list expected_dfhvalue_locations {
        location(position(7, 12), source_loc),
    };
    const location_list expected_normal_locations {
        location(position(5, 19), source_loc),
    };
    const location_list expected_busy_locations {
        location(position(6, 18), source_loc),
        location(position(7, 23), source_loc),
    };

    // LARL reference
    EXPECT_EQ(expected_larl_locations, a.context().lsp_ctx->references(source_loc, position(5, 7)));
    // L reference
    EXPECT_EQ(expected_l_locations, a.context().lsp_ctx->references(source_loc, position(6, 5)));
    // DFHRESP reference
    EXPECT_EQ(expected_dfhresp_locations, a.context().lsp_ctx->references(source_loc, position(5, 16)));
    // DFHVALUE reference
    EXPECT_EQ(expected_dfhvalue_locations, a.context().lsp_ctx->references(source_loc, position(7, 17)));
    // NORMAL reference
    EXPECT_EQ(expected_normal_locations, a.context().lsp_ctx->references(source_loc, position(5, 23)));
    // BUSY reference
    EXPECT_EQ(expected_busy_locations, a.context().lsp_ctx->references(source_loc, position(6, 20)));
}