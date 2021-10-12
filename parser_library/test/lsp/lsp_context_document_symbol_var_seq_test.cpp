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

#include "../mock_parse_lib_provider.h"
#include "analyzer_fixture.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lsp;

TEST(lsp_context_document_symbol_var_seq, 1)
{
    std::string opencode =
        R"(&V1 SETA    5
&V2 SETA    &V1)";

    analyzer a(opencode, analyzer_options {});
    a.analyze();

    const auto limit = 1000LL;
    document_symbol_list_s outline_opencode = a.context().lsp_ctx->document_symbol("", limit);
    document_symbol_list_s expected_opencode = document_symbol_list_s {
        document_symbol_item_s { "V1", document_symbol_kind::VAR, range { { 0, 0 }, { 0, 0 } } },
        document_symbol_item_s { "V2", document_symbol_kind::VAR, range { { 1, 0 }, { 1, 0 } } }
    };

    EXPECT_TRUE(is_permutation_with_permutations(outline_opencode, expected_opencode));
}

TEST(lsp_context_document_symbol_var_seq, 2)
{
    std::string opencode =
        R"(&V1 SETA    5
&V2 SETA    &V1
    MAC1)";
    mock_parse_lib_provider mock({
        { "MAC1", R"(      MACRO
      MAC1
&VM11 SETA    1
&VM12 SETA    &VM11
      AGO     .SM1
.SM1  ANOP
EM1   EQU     1
      MEND
)" },
    });

    analyzer a(opencode, analyzer_options { &mock });
    a.analyze();

    const auto limit = 1000LL;
    document_symbol_list_s outline_opencode = a.context().lsp_ctx->document_symbol("", limit);
    document_symbol_list_s outline_mac1 = a.context().lsp_ctx->document_symbol("MAC1", limit);
    document_symbol_list_s expected_opencode = document_symbol_list_s {
        document_symbol_item_s { "MAC1",
            document_symbol_kind::MACRO,
            range { { 2, 4 }, { 2, 4 } },
            document_symbol_list_s {
                document_symbol_item_s { "EM1", document_symbol_kind::EQU, range { { 2, 4 }, { 2, 4 } } },
                document_symbol_item_s { "VM11", document_symbol_kind::VAR, range { { 2, 4 }, { 2, 4 } } },
                document_symbol_item_s { "VM12", document_symbol_kind::VAR, range { { 2, 4 }, { 2, 4 } } },
                document_symbol_item_s { "SM1", document_symbol_kind::SEQ, range { { 2, 4 }, { 2, 4 } } } } },
        document_symbol_item_s { "V1", document_symbol_kind::VAR, range { { 0, 0 }, { 0, 0 } } },
        document_symbol_item_s { "V2", document_symbol_kind::VAR, range { { 1, 0 }, { 1, 0 } } }
    };
    document_symbol_list_s expected_mac1 = document_symbol_list_s {
        document_symbol_item_s { "VM11", document_symbol_kind::VAR, range { { 2, 0 }, { 2, 0 } } },
        document_symbol_item_s { "VM12", document_symbol_kind::VAR, range { { 3, 0 }, { 3, 0 } } },
        document_symbol_item_s { "SM1", document_symbol_kind::SEQ, range { { 5, 0 }, { 5, 0 } } }
    };

    EXPECT_TRUE(is_permutation_with_permutations(outline_opencode, expected_opencode));
    EXPECT_TRUE(is_permutation_with_permutations(outline_mac1, expected_mac1));
}

TEST(lsp_context_document_symbol_var_seq, 3)
{
    std::string opencode =
        R"(&V1 SETA    5
&V2 SETA    &V1
    MAC1)";
    mock_parse_lib_provider mock({
        { "MAC1", R"(      MACRO
      MAC1
&VM11 SETA    1
&VM12 SETA    &VM11
      AGO     .SM1
.SM1  ANOP
EM1   EQU     1
      MAC2
      MEND
)" },
        { "MAC2", R"(      MACRO
      MAC2
&VM21 SETA    1
&VM22 SETA    &VM21
      AGO     .SM2
.SM2  ANOP
EM2   EQU     1
      MEND)" },
    });

    analyzer a(opencode, analyzer_options { &mock });
    a.analyze();

    const auto limit = 1000LL;
    document_symbol_list_s outline_opencode = a.context().lsp_ctx->document_symbol("", limit);
    document_symbol_list_s outline_mac1 = a.context().lsp_ctx->document_symbol("MAC1", limit);
    document_symbol_list_s outline_mac2 = a.context().lsp_ctx->document_symbol("MAC2", limit);
    document_symbol_list_s expected_opencode = document_symbol_list_s {
        document_symbol_item_s { "MAC1",
            document_symbol_kind::MACRO,
            range { { 2, 4 }, { 2, 4 } },
            document_symbol_list_s {
                document_symbol_item_s { "MAC2",
                    document_symbol_kind::MACRO,
                    range { { 2, 4 }, { 2, 4 } },
                    document_symbol_list_s {
                        document_symbol_item_s { "EM2", document_symbol_kind::EQU, range { { 2, 4 }, { 2, 4 } } },
                        document_symbol_item_s { "VM21", document_symbol_kind::VAR, range { { 2, 4 }, { 2, 4 } } },
                        document_symbol_item_s { "VM22", document_symbol_kind::VAR, range { { 2, 4 }, { 2, 4 } } },
                        document_symbol_item_s { "SM2", document_symbol_kind::SEQ, range { { 2, 4 }, { 2, 4 } } } } },
                document_symbol_item_s { "EM1", document_symbol_kind::EQU, range { { 2, 4 }, { 2, 4 } } },
                document_symbol_item_s { "VM11", document_symbol_kind::VAR, range { { 2, 4 }, { 2, 4 } } },
                document_symbol_item_s { "VM12", document_symbol_kind::VAR, range { { 2, 4 }, { 2, 4 } } },
                document_symbol_item_s { "SM1", document_symbol_kind::SEQ, range { { 2, 4 }, { 2, 4 } } } } },
        document_symbol_item_s { "V1", document_symbol_kind::VAR, range { { 0, 0 }, { 0, 0 } } },
        document_symbol_item_s { "V2", document_symbol_kind::VAR, range { { 1, 0 }, { 1, 0 } } }
    };
    document_symbol_list_s expected_mac1 = document_symbol_list_s {
        document_symbol_item_s { "VM11", document_symbol_kind::VAR, range { { 2, 0 }, { 2, 0 } } },
        document_symbol_item_s { "VM12", document_symbol_kind::VAR, range { { 3, 0 }, { 3, 0 } } },
        document_symbol_item_s { "SM1", document_symbol_kind::SEQ, range { { 5, 0 }, { 5, 0 } } }
    };
    document_symbol_list_s expected_mac2 = document_symbol_list_s {
        document_symbol_item_s { "VM21", document_symbol_kind::VAR, range { { 2, 0 }, { 2, 0 } } },
        document_symbol_item_s { "VM22", document_symbol_kind::VAR, range { { 3, 0 }, { 3, 0 } } },
        document_symbol_item_s { "SM2", document_symbol_kind::SEQ, range { { 5, 0 }, { 5, 0 } } }
    };

    EXPECT_TRUE(is_permutation_with_permutations(outline_opencode, expected_opencode));
    EXPECT_TRUE(is_permutation_with_permutations(outline_mac1, expected_mac1));
    EXPECT_TRUE(is_permutation_with_permutations(outline_mac2, expected_mac2));
}

TEST(lsp_context_document_symbol_var_seq, 4)
{
    std::string opencode =
        R"(&V1 SETA    5
&V2 SETA    &V1
    MAC1
    COPY COPYFILE1)";
    mock_parse_lib_provider mock({
        { "MAC1", R"(      MACRO
      MAC1
&VM11 SETA    1
&VM12 SETA    &VM11
      AGO     .SM1
.SM1  ANOP
EM1   EQU     1
      MEND
)" },
        { "COPYFILE1", R"(&VC11 SETA    1
&VC12 SETA    &VC11
      AGO     .SC1
.SC1  ANOP
EC1   EQU     1)" },
    });

    analyzer a(opencode, analyzer_options { &mock });
    a.analyze();

    const auto limit = 1000LL;
    document_symbol_list_s outline_opencode = a.context().lsp_ctx->document_symbol("", limit);
    document_symbol_list_s outline_mac1 = a.context().lsp_ctx->document_symbol("MAC1", limit);
    document_symbol_list_s outline_copyfile1 = a.context().lsp_ctx->document_symbol("COPYFILE1", limit);
    document_symbol_list_s expected_opencode = document_symbol_list_s {
        document_symbol_item_s { "COPYFILE1",
            document_symbol_kind::MACRO,
            range { { 3, 4 }, { 3, 4 } },
            document_symbol_list_s {
                document_symbol_item_s { "EC1", document_symbol_kind::EQU, range { { 3, 4 }, { 3, 4 } } },
                document_symbol_item_s { "VC11", document_symbol_kind::VAR, range { { 3, 4 }, { 3, 4 } } },
                document_symbol_item_s { "VC12", document_symbol_kind::VAR, range { { 3, 4 }, { 3, 4 } } },
                document_symbol_item_s { "SC1", document_symbol_kind::SEQ, range { { 3, 4 }, { 3, 4 } } } } },
        document_symbol_item_s { "MAC1",
            document_symbol_kind::MACRO,
            range { { 2, 4 }, { 2, 4 } },
            document_symbol_list_s {
                document_symbol_item_s { "EM1", document_symbol_kind::EQU, range { { 2, 4 }, { 2, 4 } } },
                document_symbol_item_s { "VM11", document_symbol_kind::VAR, range { { 2, 4 }, { 2, 4 } } },
                document_symbol_item_s { "VM12", document_symbol_kind::VAR, range { { 2, 4 }, { 2, 4 } } },
                document_symbol_item_s { "SM1", document_symbol_kind::SEQ, range { { 2, 4 }, { 2, 4 } } } } },
        document_symbol_item_s { "V1", document_symbol_kind::VAR, range { { 0, 0 }, { 0, 0 } } },
        document_symbol_item_s { "V2", document_symbol_kind::VAR, range { { 1, 0 }, { 1, 0 } } }
    };
    document_symbol_list_s expected_mac1 = document_symbol_list_s {
        document_symbol_item_s { "VM11", document_symbol_kind::VAR, range { { 2, 0 }, { 2, 0 } } },
        document_symbol_item_s { "VM12", document_symbol_kind::VAR, range { { 3, 0 }, { 3, 0 } } },
        document_symbol_item_s { "SM1", document_symbol_kind::SEQ, range { { 5, 0 }, { 5, 0 } } }
    };
    document_symbol_list_s expected_copyfile1 = document_symbol_list_s {
        document_symbol_item_s { "VC11", document_symbol_kind::VAR, range { { 0, 0 }, { 0, 5 } } },
        document_symbol_item_s { "VC12", document_symbol_kind::VAR, range { { 1, 0 }, { 1, 5 } } },
        document_symbol_item_s { "SC1", document_symbol_kind::SEQ, range { { 3, 0 }, { 3, 4 } } }
    };

    EXPECT_TRUE(is_permutation_with_permutations(outline_opencode, expected_opencode));
    EXPECT_TRUE(is_permutation_with_permutations(outline_mac1, expected_mac1));
    EXPECT_TRUE(is_permutation_with_permutations(outline_copyfile1, expected_copyfile1));
}

TEST(lsp_context_document_symbol_var_seq, 5)
{
    std::string opencode =
        R"(&V1 SETA    5
&V2 SETA    &V1
    COPY    COPYFILE1)";
    mock_parse_lib_provider mock({
        { "MAC1", R"(      MACRO
      MAC1
&VM11 SETA    1
&VM12 SETA    &VM11
      AGO     .SM1
.SM1  ANOP
EM1   EQU     1
      MEND
)" },
        { "COPYFILE1", R"(&VC11 SETA    1
&VC12 SETA    &VC11
      AGO     .SC1
.SC1  ANOP
EC1   EQU     1
      MAC1)" },
    });

    analyzer a(opencode, analyzer_options { &mock });
    a.analyze();

    const auto limit = 1000LL;
    document_symbol_list_s outline_opencode = a.context().lsp_ctx->document_symbol("", limit);
    document_symbol_list_s outline_mac1 = a.context().lsp_ctx->document_symbol("MAC1", limit);
    document_symbol_list_s outline_copyfile1 = a.context().lsp_ctx->document_symbol("COPYFILE1", limit);
    document_symbol_list_s expected_opencode = document_symbol_list_s {
        document_symbol_item_s { "COPYFILE1",
            document_symbol_kind::MACRO,
            range { { 2, 4 }, { 2, 4 } },
            document_symbol_list_s {
                document_symbol_item_s { "MAC1",
                    document_symbol_kind::MACRO,
                    range { { 2, 4 }, { 2, 4 } },
                    document_symbol_list_s {
                        document_symbol_item_s { "EM1", document_symbol_kind::EQU, range { { 2, 4 }, { 2, 4 } } },
                        document_symbol_item_s { "VM11", document_symbol_kind::VAR, range { { 2, 4 }, { 2, 4 } } },
                        document_symbol_item_s { "VM12", document_symbol_kind::VAR, range { { 2, 4 }, { 2, 4 } } },
                        document_symbol_item_s { "SM1", document_symbol_kind::SEQ, range { { 2, 4 }, { 2, 4 } } } } },
                document_symbol_item_s { "EC1", document_symbol_kind::EQU, range { { 2, 4 }, { 2, 4 } } },
                document_symbol_item_s { "VC11", document_symbol_kind::VAR, range { { 2, 4 }, { 2, 4 } } },
                document_symbol_item_s { "VC12", document_symbol_kind::VAR, range { { 2, 4 }, { 2, 4 } } },
                document_symbol_item_s { "SC1", document_symbol_kind::SEQ, range { { 2, 4 }, { 2, 4 } } } } },
        document_symbol_item_s { "V1", document_symbol_kind::VAR, range { { 0, 0 }, { 0, 0 } } },
        document_symbol_item_s { "V2", document_symbol_kind::VAR, range { { 1, 0 }, { 1, 0 } } }
    };
    document_symbol_list_s expected_mac1 = document_symbol_list_s {
        document_symbol_item_s { "VM11", document_symbol_kind::VAR, range { { 2, 0 }, { 2, 0 } } },
        document_symbol_item_s { "VM12", document_symbol_kind::VAR, range { { 3, 0 }, { 3, 0 } } },
        document_symbol_item_s { "SM1", document_symbol_kind::SEQ, range { { 5, 0 }, { 5, 0 } } }
    };
    document_symbol_list_s expected_copyfile1 = document_symbol_list_s {
        document_symbol_item_s { "VC11", document_symbol_kind::VAR, range { { 0, 0 }, { 0, 5 } } },
        document_symbol_item_s { "VC12", document_symbol_kind::VAR, range { { 1, 0 }, { 1, 5 } } },
        document_symbol_item_s { "SC1", document_symbol_kind::SEQ, range { { 3, 0 }, { 3, 4 } } }
    };

    EXPECT_TRUE(is_permutation_with_permutations(outline_opencode, expected_opencode));
    EXPECT_TRUE(is_permutation_with_permutations(outline_mac1, expected_mac1));
    EXPECT_TRUE(is_permutation_with_permutations(outline_copyfile1, expected_copyfile1));
}

TEST(lsp_context_document_symbol_var_seq, 6)
{
    std::string opencode =
        R"(&V1 SETA    5
&V2 SETA    &V1
    MAC1)";
    mock_parse_lib_provider mock({
        { "MAC1", R"(      MACRO
      MAC1
&VM11 SETA    1
&VM12 SETA    &VM11
      AGO     .SM1
.SM1  ANOP
EM1   EQU     1
      COPY    COPYFILE1
      MEND
)" },
        { "COPYFILE1", R"(&VC11 SETA    1
&VC12 SETA    &VC11
      AGO     .SC1
.SC1  ANOP
EC1   EQU     1
)" },
    });

    analyzer a(opencode, analyzer_options { &mock });
    a.analyze();

    const auto limit = 1000LL;
    document_symbol_list_s outline_opencode = a.context().lsp_ctx->document_symbol("", limit);
    document_symbol_list_s outline_mac1 = a.context().lsp_ctx->document_symbol("MAC1", limit);
    document_symbol_list_s outline_copyfile1 = a.context().lsp_ctx->document_symbol("COPYFILE1", limit);
    document_symbol_list_s expected_opencode = document_symbol_list_s {
        document_symbol_item_s { "MAC1",
            document_symbol_kind::MACRO,
            range { { 2, 4 }, { 2, 4 } },
            document_symbol_list_s {
                document_symbol_item_s { "COPYFILE1",
                    document_symbol_kind::MACRO,
                    range { { 2, 4 }, { 2, 4 } },
                    document_symbol_list_s {
                        document_symbol_item_s { "EC1", document_symbol_kind::EQU, range { { 2, 4 }, { 2, 4 } } },
                        document_symbol_item_s { "VC11", document_symbol_kind::VAR, range { { 2, 4 }, { 2, 4 } } },
                        document_symbol_item_s { "VC12", document_symbol_kind::VAR, range { { 2, 4 }, { 2, 4 } } },
                        document_symbol_item_s { "SC1", document_symbol_kind::SEQ, range { { 2, 4 }, { 2, 4 } } } } },
                document_symbol_item_s { "EM1", document_symbol_kind::EQU, range { { 2, 4 }, { 2, 4 } } },
                document_symbol_item_s { "VM11", document_symbol_kind::VAR, range { { 2, 4 }, { 2, 4 } } },
                document_symbol_item_s { "VM12", document_symbol_kind::VAR, range { { 2, 4 }, { 2, 4 } } },
                document_symbol_item_s { "SM1", document_symbol_kind::SEQ, range { { 2, 4 }, { 2, 4 } } } } },
        document_symbol_item_s { "V1", document_symbol_kind::VAR, range { { 0, 0 }, { 0, 0 } } },
        document_symbol_item_s { "V2", document_symbol_kind::VAR, range { { 1, 0 }, { 1, 0 } } }
    };
    document_symbol_list_s expected_mac1 = document_symbol_list_s {
        document_symbol_item_s { "VM11", document_symbol_kind::VAR, range { { 2, 0 }, { 2, 0 } } },
        document_symbol_item_s { "VM12", document_symbol_kind::VAR, range { { 3, 0 }, { 3, 0 } } },
        document_symbol_item_s { "COPYFILE1",
            document_symbol_kind::MACRO,
            range { { 7, 14 }, { 7, 23 } },
            document_symbol_list_s {
                document_symbol_item_s { "VC11", document_symbol_kind::VAR, range { { 7, 14 }, { 7, 23 } } },
                document_symbol_item_s { "VC12", document_symbol_kind::VAR, range { { 7, 14 }, { 7, 23 } } },
                document_symbol_item_s { "SC1", document_symbol_kind::SEQ, range { { 7, 14 }, { 7, 23 } } } } },
        document_symbol_item_s { "SM1", document_symbol_kind::SEQ, range { { 5, 0 }, { 5, 0 } } }
    };
    document_symbol_list_s expected_copyfile1 = document_symbol_list_s {
        document_symbol_item_s { "VC11", document_symbol_kind::VAR, range { { 0, 0 }, { 0, 5 } } },
        document_symbol_item_s { "VC12", document_symbol_kind::VAR, range { { 1, 0 }, { 1, 5 } } },
        document_symbol_item_s { "SC1", document_symbol_kind::SEQ, range { { 3, 0 }, { 3, 4 } } }
    };

    EXPECT_TRUE(is_permutation_with_permutations(outline_opencode, expected_opencode));
    EXPECT_TRUE(is_permutation_with_permutations(outline_mac1, expected_mac1));
    EXPECT_TRUE(is_permutation_with_permutations(outline_copyfile1, expected_copyfile1));
}
