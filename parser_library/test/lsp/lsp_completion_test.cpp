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
#include "completion_item.h"
#include "completion_trigger_kind.h"
#include "context/hlasm_context.h"
#include "lsp/item_convertors.h"
#include "lsp/lsp_context.h"

constexpr auto zero_stmt_id = context::statement_id { 0 };

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
    std::unordered_map<const context::macro_definition*, lsp::macro_info_ptr> m;
    m.try_emplace(aaaa->macro_definition.get(), aaaa);

    auto result = lsp::generate_completion(lsp::completion_list_source(
        lsp::completion_list_instructions { "AAAAA", 1, &m, a.context().lsp_ctx.get(), { "AAAA", "ADATA" } }));

    EXPECT_TRUE(
        std::ranges::any_of(result, [](const auto& e) { return e.label == "ADATA" && e.suggestion_for == "AAAAA"; }));
    EXPECT_TRUE(
        std::ranges::any_of(result, [](const auto& e) { return e.label == "AAAA" && e.suggestion_for == "AAAAA"; }));
}

TEST(lsp_completion, completion_list_instr_exact)
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
    std::unordered_map<const context::macro_definition*, lsp::macro_info_ptr> m;
    m.try_emplace(aaaa->macro_definition.get(), aaaa);

    auto result = lsp::generate_completion(lsp::completion_list_source(
        lsp::completion_list_instructions { "AAA", 1, &m, a.context().lsp_ctx.get(), { "AAAA", "ADATA" } }));

    EXPECT_TRUE(
        std::ranges::any_of(result, [](const auto& e) { return e.label == "AAAA" && e.suggestion_for.empty(); }));
}

TEST(lsp_completion, completion_list_vars)
{
    lsp::vardef_storage vars(1, lsp::variable_symbol_definition(context::id_index("VARNAME"), zero_stmt_id, {}));

    auto result = lsp::generate_completion(&vars);

    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::ranges::count(result, "&VARNAME", &completion_item::label), 1);
}

TEST(lsp_completion, completion_list_labels)
{
    context::label_storage labels;
    labels.try_emplace(context::id_index("LABEL"),
        std::make_unique<context::macro_sequence_symbol>(context::id_index("LABEL"), location(), zero_stmt_id));

    auto result = lsp::generate_completion(&labels);

    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::ranges::count(result, ".LABEL", &completion_item::label), 1);
}

TEST(lsp_completion, completion_list_empty)
{
    auto result = lsp::generate_completion(std::monostate());

    EXPECT_TRUE(result.empty());
}

TEST(lsp_completion, macro_operands)
{
    const std::string input = R"(
         MACRO
         MAC   &A,&B=(DEFAULTB,                                        X
               1)

         MEND
)";
    analyzer a(input);
    a.analyze();

    auto mac = a.context().lsp_ctx->get_macro_info(context::id_index("MAC"));
    ASSERT_TRUE(mac);

    auto result = lsp::generate_completion(lsp::completion_list_source(
        std::pair(mac->macro_definition.get(), std::vector<std::pair<const context::symbol*, context::id_index>>())));

    EXPECT_EQ(result.size(), 2);

    EXPECT_TRUE(std::ranges::any_of(result, [](const auto& e) { return e.label == "&A" && e.documentation.empty(); }));
    EXPECT_TRUE(std::ranges::any_of(result,
        [](const auto& e) { return e.label == "&B" && e.documentation.find("&B=(DEFAULTB,1)") != std::string::npos; }));
}

TEST(lsp_completion, ordinary_operands_reloc_no_label)
{
    using namespace ::testing;

    const std::string input = R"(
D   DSECT
DA  DS  F
DL  EQU *-D
C   CSECT
)";
    analyzer a(input);
    a.analyze();

    const auto* da = get_symbol(a.hlasm_ctx(), "DA");
    ASSERT_TRUE(da);

    auto result = lsp::generate_completion(
        lsp::completion_list_source(std::pair(nullptr, std::vector { std::pair(da, context::id_index()) })));

    ASSERT_EQ(result.size(), 1);
    const auto& item = result.front();

    EXPECT_EQ(item.label, "DA");
    EXPECT_THAT(item.detail, HasSubstr("relocatable symbol"));
    EXPECT_THAT(item.documentation, HasSubstr("Relocatable Symbol"));
    EXPECT_EQ(item.insert_text, "DA");
}

TEST(lsp_completion, ordinary_operands_reloc_label)
{
    using namespace ::testing;

    const std::string input = R"(
D   DSECT
DA  DS  F
DL  EQU *-D
C   CSECT
)";
    analyzer a(input);
    a.analyze();

    constexpr context::id_index L("L");

    const auto* da = get_symbol(a.hlasm_ctx(), "DA");
    ASSERT_TRUE(da);

    auto result =
        lsp::generate_completion(lsp::completion_list_source(std::pair(nullptr, std::vector { std::pair(da, L) })));

    ASSERT_EQ(result.size(), 1);
    const auto& item = result.front();

    EXPECT_EQ(item.label, "L.DA");
    EXPECT_THAT(item.detail, HasSubstr("relocatable symbol"));
    EXPECT_THAT(item.documentation, HasSubstr("Relocatable Symbol"));
    EXPECT_EQ(item.insert_text, "L.DA");
}

TEST(lsp_completion, ordinary_operands_abs)
{
    using namespace ::testing;

    const std::string input = R"(
D   DSECT
DA  DS  F
DL  EQU *-D
C   CSECT
)";
    analyzer a(input);
    a.analyze();

    const auto* dl = get_symbol(a.hlasm_ctx(), "DL");
    ASSERT_TRUE(dl);

    auto result = lsp::generate_completion(
        lsp::completion_list_source(std::pair(nullptr, std::vector { std::pair(dl, context::id_index()) })));

    ASSERT_EQ(result.size(), 1);
    const auto& item = result.front();

    EXPECT_EQ(item.label, "DL");
    EXPECT_THAT(item.detail, HasSubstr("absolute symbol"));
    EXPECT_THAT(item.documentation, HasSubstr("Absolute Symbol"));
    EXPECT_EQ(item.insert_text, "DL");
}

TEST(lsp_completion, private_csect)
{
    using namespace ::testing;

    const std::string input = R"(
LLLLL  DS A
LABEL  DS A
)";
    analyzer a(input);
    a.analyze();
    auto loc = a.context().hlasm_ctx->opencode_location();

    auto l1 = a.context().lsp_ctx->completion(loc, position(1, 0), 0, completion_trigger_kind::invoked);
    auto l2 = a.context().lsp_ctx->completion(loc, position(2, 0), 0, completion_trigger_kind::invoked);

    ASSERT_EQ(l1.index(), l2.index());

    using T = std::pair<const macro_definition*, std::vector<std::pair<const symbol*, id_index>>>;
    ASSERT_TRUE(std::holds_alternative<T>(l1));

    const auto& t1 = std::get<T>(l1);
    const auto& t2 = std::get<T>(l2);

    EXPECT_EQ(t1.first, t2.first);
    EXPECT_THAT(t1.second, UnorderedElementsAreArray(t2.second));
}
