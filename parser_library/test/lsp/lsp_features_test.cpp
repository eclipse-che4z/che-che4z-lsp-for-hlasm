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

#include "../mock_parse_lib_provider.h"
#include "analyzer.h"
#include "context/instruction.h"
#include "workspaces/parse_lib_provider.h"


using namespace hlasm_plugin::parser_library;

constexpr const char* MACRO_FILE = "MAC";
constexpr const char* SOURCE_FILE = "OPEN";
constexpr const char* COPY_FILE = "COPYFILE";

class lsp_features_test : public testing::Test
{
public:
    lsp_features_test()
        : lib_provider({ { "MAC",
                             R"(   MACRO
       MAC   &VAR
       LR    &VAR,&VAR
       MEND)" },
            { "COPYFILE", R"(R2 EQU 2
            LR R2,R2)" } })
        , a(contents, analyzer_options { SOURCE_FILE, &lib_provider })
        , instruction_count(context::instruction::machine_instructions.size()
              + context::instruction::assembler_instructions.size() + context::instruction::ca_instructions.size()
              + context::instruction::mnemonic_codes.size()) {};

    void SetUp() override { a.analyze(); }
    void TearDown() override {}

protected:
    const std::string contents =
        R"(   MAC  1
&VAR   SETA 5
       LR   &VAR,&VAR REMARK
       AGO  .HERE
       L    1,0(5)
.HERE  ANOP
* line 6
       MACRO
&LABEL MAC   &VAR
*THIS IS DOCUMENTATION
* line 10
       LR     &VAR,&LABEL
       AGO    .HERE
       AGO    .HERE2
.HERE2 ANOP
.HERE  ANOP 
       MEND
* line 17
1      MAC   2
* line 19
      COPY COPYFILE
      LR R1,R2
R1 EQU 1
R1      MAC   R2
   LCLC &SETC
* line 25
 
)";
    std::string content;
    mock_parse_lib_provider lib_provider;
    analyzer a;
    const size_t instruction_count;
};

TEST_F(lsp_features_test, go_to)
{
    // jump from source to macro, macro MAC
    EXPECT_EQ(location(position(1, 7), MACRO_FILE), a.context().lsp_ctx->definition(SOURCE_FILE, position(0, 4)));
    // no jump
    EXPECT_EQ(location(position(0, 8), SOURCE_FILE), a.context().lsp_ctx->definition(SOURCE_FILE, position(0, 8)));
    // jump in source, open code, var symbol &VAR
    EXPECT_EQ(location(position(1, 0), SOURCE_FILE), a.context().lsp_ctx->definition(SOURCE_FILE, position(2, 13)));
    // jump in source, open code, seq symbol .HERE
    EXPECT_EQ(location(position(5, 0), SOURCE_FILE), a.context().lsp_ctx->definition(SOURCE_FILE, position(3, 13)));
    // jump in source, macro, seq symbol .HERE
    EXPECT_EQ(location(position(15, 0), SOURCE_FILE), a.context().lsp_ctx->definition(SOURCE_FILE, position(12, 15)));
    // jump in source, macro, var symbol &LABEL
    EXPECT_EQ(location(position(8, 0), SOURCE_FILE), a.context().lsp_ctx->definition(SOURCE_FILE, position(11, 20)));
    // jump in source, macro, var symbol &VAR
    EXPECT_EQ(location(position(8, 13), SOURCE_FILE), a.context().lsp_ctx->definition(SOURCE_FILE, position(11, 15)));
    // forward jump in source, open code, ord symbol R1
    EXPECT_EQ(location(position(22, 0), SOURCE_FILE), a.context().lsp_ctx->definition(SOURCE_FILE, position(21, 10)));
    // jump from source to copy file, ord symbol R2 on machine instrution
    EXPECT_EQ(location(position(0, 0), COPY_FILE), a.context().lsp_ctx->definition(SOURCE_FILE, position(21, 13)));
    // jump from source to copy file, ord symbol R2 on macro MAC
    EXPECT_EQ(location(position(0, 0), COPY_FILE), a.context().lsp_ctx->definition(SOURCE_FILE, position(23, 14)));
    // jump from source to first instruction in copy file, COPY COPYFILE
    EXPECT_EQ(location(position(0, 3), COPY_FILE), a.context().lsp_ctx->definition(SOURCE_FILE, position(20, 14)));
}

TEST_F(lsp_features_test, refs)
{
    // the same as go_to test but with references
    // reference MAC, should appear once in source and once in macro file
    EXPECT_EQ((size_t)2, a.context().lsp_ctx->references(SOURCE_FILE, position(0, 4)).size());
    // no reference
    EXPECT_EQ((size_t)0, a.context().lsp_ctx->references(SOURCE_FILE, position(0, 8)).size());
    // source code references for &VAR, appeared three times
    EXPECT_EQ((size_t)3, a.context().lsp_ctx->references(SOURCE_FILE, position(2, 13)).size());
    // source code references for .HERE, appeared twice
    EXPECT_EQ((size_t)2, a.context().lsp_ctx->references(SOURCE_FILE, position(3, 13)).size());
    // references inside macro def, seq symbol .HERE
    EXPECT_EQ((size_t)2, a.context().lsp_ctx->references(SOURCE_FILE, position(12, 15)).size());
    //  references inside macro def, var symbol &LABEL
    EXPECT_EQ((size_t)2, a.context().lsp_ctx->references(SOURCE_FILE, position(11, 20)).size());
    //  references inside macro def, var symbol &VAR
    EXPECT_EQ((size_t)2, a.context().lsp_ctx->references(SOURCE_FILE, position(11, 15)).size());
}
