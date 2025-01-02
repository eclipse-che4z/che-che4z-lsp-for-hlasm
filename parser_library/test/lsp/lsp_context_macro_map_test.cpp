/*
 * Copyright (c) 2024 Broadcom.
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
#include "lsp/lsp_context.h"

TEST(lsp_context, macro_map)
{
    mock_parse_lib_provider libs {
        { "MAC", R"(.*
    MACRO
    MAC
    COPY E
)" },
        { "E", " MEND" },
        { "COPYM", R"(
    MACRO
    MACE
    MEND)" },
    };
    std::string_view input = R"(
    MACRO
    MACOP
    MEND

    MAC

    COPY  COPYM)";
    analyzer a(input, analyzer_options { &libs });

    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    using hlasm_plugin::utils::resource::resource_location;

    const auto* opencode = a.context().lsp_ctx->get_file_info(resource_location());
    const auto* mac = a.context().lsp_ctx->get_file_info(resource_location("MAC"));
    const auto* e = a.context().lsp_ctx->get_file_info(resource_location("E"));
    const auto* copym = a.context().lsp_ctx->get_file_info(resource_location("COPYM"));

    ASSERT_TRUE(opencode);
    ASSERT_TRUE(mac);
    ASSERT_TRUE(e);
    ASSERT_TRUE(copym);

    EXPECT_EQ(opencode->macro_map(), (std::vector<bool> { false, false, true, true }));
    EXPECT_EQ(mac->macro_map(), (std::vector<bool> { false, false, true, true }));
    EXPECT_EQ(e->macro_map(), (std::vector<bool> {})); // used only as copybook in macro
    EXPECT_EQ(copym->macro_map(), (std::vector<bool> { false, false, true, true }));
}

TEST(lsp_context, macro_map_with_nested_macro)
{
    mock_parse_lib_provider libs {
        { "F", R"(
         MACRO
         INNER2
         COPY E
         MEND)" },
        { "E", " SAM31" },
    };
    std::string_view input = R"(
* This empty area is part of the test





         MACRO
         MAC
         MACRO
         INNER
         COPY F
         MEND
         MEND


         MAC
)";
    analyzer a(input, analyzer_options { &libs });

    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    using hlasm_plugin::utils::resource::resource_location;

    const auto* opencode = a.context().lsp_ctx->get_file_info(resource_location());

    ASSERT_TRUE(opencode);

    EXPECT_EQ(opencode->macro_map(),
        (std::vector<bool> {
            false, false, false, false, false, false, false, false, true, true, true, true, true, true }));
}
