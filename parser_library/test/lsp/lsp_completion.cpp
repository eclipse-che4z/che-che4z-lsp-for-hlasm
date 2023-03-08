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

#include <algorithm>

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "lsp/completion_item.h"
#include "lsp/item_convertors.h"
#include "lsp/lsp_context.h"

TEST(lsp_completion, completion_list_instr)
{
    const std::string input = R"(
    MACRO
    AAAA
    MEND
)";
    analyzer a(input);
    a.analyze();

    auto aaaa = a.context().lsp_ctx->get_macro_info(context::id_index("AAAA"));
    ASSERT_TRUE(aaaa);
    std::unordered_map<context::macro_def_ptr, lsp::macro_info_ptr> m;
    m.try_emplace(aaaa->macro_definition, aaaa);

    auto result = lsp::generate_completion(lsp::completion_list_source(
        lsp::completion_list_instructions { "AAAAA", 1, &m, a.context().lsp_ctx.get(), { "AAAA", "ADATA" } }));

    EXPECT_TRUE(std::any_of(
        result.begin(), result.end(), [](const auto& e) { return e.label == "ADATA" && e.suggestion_for == "AAAAA"; }));
    EXPECT_TRUE(std::any_of(
        result.begin(), result.end(), [](const auto& e) { return e.label == "AAAA" && e.suggestion_for == "AAAAA"; }));
}

TEST(lsp_completion, completion_list_vars)
{
    lsp::vardef_storage vars(1, lsp::variable_symbol_definition(context::id_index("VARNAME"), 0, {}));

    auto result = lsp::generate_completion(&vars);

    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::count_if(result.begin(), result.end(), [](const auto& e) { return e.label == "&VARNAME"; }), 1);
}

TEST(lsp_completion, completion_list_labels)
{
    context::label_storage labels;
    labels.try_emplace(context::id_index("LABEL"),
        std::make_unique<context::macro_sequence_symbol>(context::id_index("LABEL"), location(), 0));

    auto result = lsp::generate_completion(&labels);

    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::count_if(result.begin(), result.end(), [](const auto& e) { return e.label == ".LABEL"; }), 1);
}

TEST(lsp_completion, completion_list_empty)
{
    auto result = lsp::generate_completion(std::monostate());

    EXPECT_TRUE(result.empty());
}
