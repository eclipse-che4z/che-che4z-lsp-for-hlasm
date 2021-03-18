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


struct lsp_context_ord_symbol : public analyzer_fixture
{
    const static inline std::string input =
        R"(
 LR R1,1
R1 EQU 1
)";

    lsp_context_ord_symbol()
        : analyzer_fixture(input)
    {}
};

TEST_F(lsp_context_ord_symbol, definition)
{
    location res = a.context().lsp_ctx->definition(dummy_file_name, { 1, 5 });
    EXPECT_EQ(res.file, dummy_file_name);
    EXPECT_EQ(res.pos, position(2, 0));
}

TEST_F(lsp_context_ord_symbol, references)
{
    auto res = a.context().lsp_ctx->references(dummy_file_name, { 2, 0 });
    ASSERT_EQ(res.size(), 2U);

    EXPECT_EQ(res[0].file, dummy_file_name);
    EXPECT_EQ(res[0].pos, position(1, 4));
    EXPECT_EQ(res[1].file, dummy_file_name);
    EXPECT_EQ(res[1].pos, position(2, 0));
}

TEST_F(lsp_context_ord_symbol, hover)
{
    auto res = a.context().lsp_ctx->hover(dummy_file_name, { 1, 5 });


    EXPECT_EQ(res, R"(1

---

Absolute Symbol

---

L: 1  
T: U  
)");
}
