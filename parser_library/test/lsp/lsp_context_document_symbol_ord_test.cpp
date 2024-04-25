/*
 * Copyright (c) 2021 Broadcom.
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

#include "../common_testing.h"
#include "../mock_parse_lib_provider.h"
#include "document_symbol_item.h"
#include "lsp/lsp_context.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lsp;
using namespace hlasm_plugin::utils::resource;

namespace {
const auto empty_loc = resource_location("");
const auto COPYSECT_loc = resource_location("COPYSECT");
const auto MACSECT_loc = resource_location("MACSECT");
const auto TITLE_loc = resource_location("$TITLE");
} // namespace

TEST(lsp_context_document_symbol, basics)
{
    std::string opencode =
        R"(
    MACRO
&L  MAC1
E   EQU 1
&L  LR  1,1
    MEND              

    TITLE 'PART 1'
SECT CSECT
    SAM31
    SAM31
LBL DS 0H
    SAM31
    SAM31
LB2 EQU *
    SAM31
    SAM31

LBM MAC1

    $TITLE 'PART 2'
PARM DSECT
VAL DS   F

    MACSECT
    COPY COPYSECT

T   TITLE 'EXTRA'
C   CSECT
C   AMODE 31
    DC   A(0)
.SEQ ANOP
)";
    mock_parse_lib_provider mock({
        { "COPYSECT", R"(
        TITLE 'SECT_CPY DSECT'
SECT_CPY DSECT
CPYFLD1  DS  F
        )" },
        { "MACSECT", R"( MACRO
        MACSECT
        TITLE 'SECT_MAC DSECT'
SECT_MAC DSECT
MACFLD1  DS  F
        MEND
)" },
        { "$TITLE", R"( MACRO
        $TITLE &TITLE
        AIF (T'&TITLE EQ 'O').SKIP
&TEXT   SETC '&TITLE'(2,K'&TITLE-2)
        TITLE 'PREFIX: &TEXT'
.SKIP   ANOP   ,
.EXTRA  ANOP   ,
        MEND
)" },
    });
    analyzer a(opencode, analyzer_options { &mock });
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    using enum document_symbol_kind;
    auto outline = a.context().lsp_ctx->document_symbol(empty_loc);
    auto outlineM = a.context().lsp_ctx->document_symbol(MACSECT_loc);
    auto outlineC = a.context().lsp_ctx->document_symbol(COPYSECT_loc);
    auto outlineT = a.context().lsp_ctx->document_symbol(TITLE_loc);

    auto expected = std::vector<document_symbol_item> {
        document_symbol_item {
            "MAC1",
            MACRO,
            range { { 2, 0 }, { 5, position::max_value } },
        },
        document_symbol_item {
            "PART 1",
            TITLE,
            range { { 7, 0 }, { 19, position::max_value } },
            std::vector<document_symbol_item> {
                document_symbol_item {
                    "SECT",
                    EXECUTABLE,
                    range { { 8, 0 }, { 10, position::max_value } },
                },
                document_symbol_item {
                    "LBL",
                    DAT,
                    range { { 11, 0 }, { 13, position::max_value } },
                },
                document_symbol_item {
                    "LB2",
                    EQU,
                    range { { 14, 0 }, { 17, position::max_value } },
                },
                document_symbol_item {
                    "LBM",
                    MACH,
                    range { { 18, 0 }, { 19, position::max_value } },
                },
            },
        },
        document_symbol_item {
            "PREFIX: PART 2",
            TITLE,
            range { { 20, 0 }, { 23, position::max_value } },
            std::vector<document_symbol_item> {
                document_symbol_item {
                    "PARM",
                    DUMMY,
                    range { { 21, 0 }, { 21, position::max_value } },
                },
                document_symbol_item {
                    "VAL",
                    DAT,
                    range { { 22, 0 }, { 23, position::max_value } },
                },
            },
        },
        document_symbol_item {
            "SECT_MAC DSECT",
            TITLE,
            range { { 24, 0 }, { 24, position::max_value } },
        },
        document_symbol_item {
            "SECT_CPY DSECT",
            TITLE,
            range { { 25, 0 }, { 26, position::max_value } },
        },
        document_symbol_item {
            "EXTRA",
            TITLE,
            range { { 27, 0 }, { 32, position::max_value } },
            std::vector<document_symbol_item> {
                document_symbol_item {
                    "T",
                    UNKNOWN,
                    range { { 27, 0 }, { 27, position::max_value } },
                },
                document_symbol_item {
                    "C",
                    EXECUTABLE,
                    range { { 28, 0 }, { 30, position::max_value } },
                },
                document_symbol_item {
                    ".SEQ",
                    SEQ,
                    range { { 31, 0 }, { 32, position::max_value } },
                },
            },
        },
    };
    auto expectedM = std::vector<document_symbol_item> {
        document_symbol_item {
            "MACSECT",
            MACRO,
            range { { 1, 0 }, { 5, position::max_value } },
        },
    };
    auto expectedC = std::vector<document_symbol_item> {
        document_symbol_item {
            "SECT_CPY DSECT",
            TITLE,
            range { { 1, 0 }, { 4, position::max_value } },
            std::vector<document_symbol_item> {
                document_symbol_item {
                    "SECT_CPY",
                    DUMMY,
                    range { { 2, 0 }, { 2, position::max_value } },
                },
                document_symbol_item {
                    "CPYFLD1",
                    DAT,
                    range { { 3, 0 }, { 4, position::max_value } },
                },
            },
        },
    };
    auto expectedT = std::vector<document_symbol_item> {
        document_symbol_item {
            "$TITLE",
            MACRO,
            range { { 1, 0 }, { 7, position::max_value } },
            std::vector<document_symbol_item> {
                document_symbol_item {
                    ".SKIP",
                    SEQ,
                    range { { 5, 0 }, { 5, position::max_value } },
                },
                document_symbol_item {
                    ".EXTRA",
                    SEQ,
                    range { { 6, 0 }, { 7, position::max_value } },
                },
            },
        },
    };

    EXPECT_TRUE(is_similar(outline, expected));
    EXPECT_TRUE(is_similar(outlineM, expectedM));
    EXPECT_TRUE(is_similar(outlineC, expectedC));
    EXPECT_TRUE(is_similar(outlineT, expectedT));
}
