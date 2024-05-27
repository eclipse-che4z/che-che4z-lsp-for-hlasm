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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "../common_testing.h"
#include "analyzer_fixture.h"
#include "document_symbol_item.h"
#include "lsp/lsp_context.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lsp;
using namespace ::testing;

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

namespace {
const auto empty_loc = hlasm_plugin::utils::resource::resource_location("");
}

TEST_F(lsp_context_ord_symbol, document_symbol)
{
    auto outline = a.context().lsp_ctx->document_symbol(opencode_loc);
    std::string R1 = "R1";
    std::vector<document_symbol_item> expected = {
        document_symbol_item { R1, document_symbol_kind::EQU, range { { 2, 0 }, { 3, position::max_value } } },
    };
    EXPECT_TRUE(is_similar(outline, expected));
}

TEST_F(lsp_context_ord_symbol, definition)
{
    location res = a.context().lsp_ctx->definition(opencode_loc, { 1, 5 });
    EXPECT_EQ(res, location(position(2, 0), opencode_loc));
}

TEST_F(lsp_context_ord_symbol, references)
{
    auto res = a.context().lsp_ctx->references(opencode_loc, { 2, 0 });
    ASSERT_EQ(res.size(), 2U);

    EXPECT_EQ(res[0], location(position(1, 4), opencode_loc));
    EXPECT_EQ(res[1], location(position(2, 0), opencode_loc));
}

TEST(hover, abs_symbol)
{
    std::string input = R"(
 LR R1,1
R1 EQU 1 R1 comment                                                    X
               comment 2
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());


    EXPECT_EQ(a.context().lsp_ctx->hover(empty_loc, { 1, 5 }), R"(X'1' (1)

---

Absolute Symbol

---

L: X'1' (1)  
T: U  

```hlasm
R1 EQU 1 R1 comment                                                    X
               comment 2
```
)");
}

TEST(hover, reloc_symbol)
{
    std::string input = R"(
C  CSECT
   DS  C
R  DS  H comment
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    auto res = a.context().lsp_ctx->hover(empty_loc, { 3, 0 });

    EXPECT_EQ(res, R"(C + X'2' (2)

---

Relocatable Symbol

---

L: X'2' (2)  
T: H  

```hlasm
R  DS  H comment
```
)");
}

TEST(hover, various_bases)
{
    std::string input = R"(
C0 CSECT
C1 CSECT
C2 CSECT
   DS  C
A  EQU 0-*
B  EQU 0-C1+*
C  EQU C0-C1
D  EQU 0-C1-C1
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_TRUE(a.context().lsp_ctx->hover(empty_loc, { 5, 0 }).starts_with("-C2 + X'FFFFFFFF' (-1)"));
    EXPECT_TRUE(a.context().lsp_ctx->hover(empty_loc, { 6, 0 }).starts_with("-C1 + C2 + X'1' (1)"));
    EXPECT_TRUE(a.context().lsp_ctx->hover(empty_loc, { 7, 0 }).starts_with("C0 - C1 + X'0' (0)"));
    EXPECT_TRUE(a.context().lsp_ctx->hover(empty_loc, { 8, 0 }).starts_with("-2*C1 + X'0' (0)"));
}

TEST(hover, model_statement)
{
    std::string input = R"(
&BBB     SETC  '0'
         LA    R1,&BBB
R1       EQU   1
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_TRUE(a.context().lsp_ctx->hover(empty_loc, { 2, 16 }).starts_with("X'1'"));
}

TEST(hover, model_statement_continuation)
{
    std::string input = R"(
&BBB     SETC  'R1'
         LR                                                           RX
               1,&BBB
R1       EQU   1
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_TRUE(a.context().lsp_ctx->hover(empty_loc, { 2, 70 }).starts_with("X'1'"));
    EXPECT_TRUE(a.context().lsp_ctx->hover(empty_loc, { 3, 15 }).starts_with("X'1'"));
    EXPECT_EQ(a.context().lsp_ctx->references(empty_loc, { 2, 70 }).size(), 4);
    EXPECT_EQ(a.context().lsp_ctx->references(empty_loc, { 3, 15 }).size(), 4);
}

TEST(references, long_unicode_chars)
{
    std::string input = (const char*)u8R"(
     MACRO
     )"
                                     u8"\U0001F600"
                                     u8R"(    &P1
     LARL  0,&P1
     MEND

     )"
                                     u8"\U0001F600"
                                     u8R"(        AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAX
               AAAAAA,B

AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA DS H
B DS F
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    auto b_ = a.context().lsp_ctx->references(empty_loc, { 7, 22 });

    std::vector<position> a_pos;
    std::vector<position> b_pos;
    for (const auto& r : a.context().lsp_ctx->references(empty_loc, { 6, 15 }))
        a_pos.push_back(r.pos);
    for (const auto& r : a.context().lsp_ctx->references(empty_loc, { 7, 22 }))
        b_pos.push_back(r.pos);
    std::ranges::sort(a_pos);
    std::ranges::sort(b_pos);

    EXPECT_EQ(a_pos, (std::vector<position> { { 3, 13 }, { 6, 15 }, { 9, 0 } }));
    EXPECT_EQ(b_pos, (std::vector<position> { { 7, 22 }, { 10, 0 } }));
}

TEST(hover, using)
{
    std::string input = R"(
    USING *,15
    SAM31
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());


    EXPECT_THAT(a.context().lsp_ctx->hover(empty_loc, { 2, 5 }), StartsWith("Active USINGs: **(PC)"));
}
