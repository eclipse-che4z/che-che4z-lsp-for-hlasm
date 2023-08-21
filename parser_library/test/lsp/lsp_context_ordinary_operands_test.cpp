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

#include "../common_testing.h"
#include "analyzer_fixture.h"
#include "context/ordinary_assembly/symbol.h"
#include "lsp/lsp_context.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lsp;
using namespace ::testing;

struct lsp_context_ordinary_operands : public analyzer_fixture
{
    const static inline std::string input =
        R"(
D        DSECT
DA       DS    F
DB       DS    F
DL       EQU   *-D
*
C        CSECT
         LARL  
 
         USING D,1              
         LARL                                                          X
               
         DROP
L        USING D,1
         LARL                                                          X
                                                                       X
               
         DROP
         LARL  
)";
    const context::symbol* c;
    const context::symbol* d;
    const context::symbol* da;
    const context::symbol* db;
    const context::symbol* dl;

    lsp_context_ordinary_operands()
        : analyzer_fixture(input)
    {
        c = get_symbol(a.hlasm_ctx(), "C");
        d = get_symbol(a.hlasm_ctx(), "D");
        da = get_symbol(a.hlasm_ctx(), "DA");
        db = get_symbol(a.hlasm_ctx(), "DB");
        dl = get_symbol(a.hlasm_ctx(), "DL");

        assert(c && d && da && db && dl);
    }
};

constexpr context::id_index empty;
constexpr context::id_index L("L");

TEST_F(lsp_context_ordinary_operands, non_continued)
{
    auto res = a.context().lsp_ctx->completion(opencode_loc, { 7, 15 }, 0, completion_trigger_kind::invoked);
    auto md_sec_p = std::get_if<
        std::pair<const context::macro_definition*, std::vector<std::pair<const context::symbol*, context::id_index>>>>(
        &res);
    ASSERT_TRUE(md_sec_p);

    EXPECT_THAT(md_sec_p->second, UnorderedElementsAre(std::pair(c, empty), std::pair(dl, empty)));
}

TEST_F(lsp_context_ordinary_operands, empty_line_after_instr)
{
    auto res = a.context().lsp_ctx->completion(opencode_loc, { 11, 15 }, 0, completion_trigger_kind::invoked);
    auto md_sec_p = std::get_if<
        std::pair<const context::macro_definition*, std::vector<std::pair<const context::symbol*, context::id_index>>>>(
        &res);
    ASSERT_TRUE(md_sec_p);

    EXPECT_THAT(md_sec_p->second,
        UnorderedElementsAre(std::pair(c, empty),
            std::pair(d, empty),
            std::pair(da, empty),
            std::pair(db, empty),
            std::pair(dl, empty)));
}

TEST_F(lsp_context_ordinary_operands, continued_line_edge)
{
    auto res = a.context().lsp_ctx->completion(opencode_loc, { 16, 15 }, 0, completion_trigger_kind::invoked);
    auto md_sec_p = std::get_if<
        std::pair<const context::macro_definition*, std::vector<std::pair<const context::symbol*, context::id_index>>>>(
        &res);
    ASSERT_TRUE(md_sec_p);

    EXPECT_THAT(md_sec_p->second,
        UnorderedElementsAre(
            std::pair(c, empty), std::pair(d, L), std::pair(da, L), std::pair(db, L), std::pair(dl, empty)));
}

TEST_F(lsp_context_ordinary_operands, second_continued_line_edge)
{
    auto res = a.context().lsp_ctx->completion(opencode_loc, { 18, 15 }, 0, completion_trigger_kind::invoked);
    auto md_sec_p = std::get_if<
        std::pair<const context::macro_definition*, std::vector<std::pair<const context::symbol*, context::id_index>>>>(
        &res);
    ASSERT_TRUE(md_sec_p);

    EXPECT_THAT(md_sec_p->second, UnorderedElementsAre(std::pair(c, empty), std::pair(dl, empty)));
}
