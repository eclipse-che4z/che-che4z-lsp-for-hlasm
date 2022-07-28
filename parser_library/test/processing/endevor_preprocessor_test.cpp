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

#include <unordered_map>

#include "../common_testing.h"
#include "../mock_parse_lib_provider.h"
#include "preprocessor_options.h"
#include "processing/preprocessor.h"

using namespace hlasm_plugin::parser_library::processing;
using namespace hlasm_plugin::utils::resource;

TEST(endevor_preprocessor, basic_inc)
{
    diagnostic_op_consumer_container diags;

    int callback_count = 0;

    auto p = preprocessor::create(
        endevor_preprocessor_options(),
        [&callback_count](std::string_view s) {
            EXPECT_EQ(s, "AAA");
            ++callback_count;
            return std::string("TEST");
        },
        &diags);

    auto result = p->generate_replacement(document("-INC AAA"));

    EXPECT_EQ(callback_count, 1);
    EXPECT_TRUE(diags.diags.empty());
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result.at(0).text(), "TEST");
}

TEST(endevor_preprocessor, basic_include)
{
    diagnostic_op_consumer_container diags;

    int callback_count = 0;

    auto p = preprocessor::create(
        endevor_preprocessor_options(),
        [&callback_count](std::string_view s) {
            EXPECT_EQ(s, "AAA");
            ++callback_count;
            return std::string("TEST");
        },
        &diags);

    auto result = p->generate_replacement(document("++INCLUDE AAA"));

    EXPECT_EQ(callback_count, 1);
    EXPECT_TRUE(diags.diags.empty());
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result.at(0).text(), "TEST");
}

TEST(endevor_preprocessor, missing_member)
{
    diagnostic_op_consumer_container diags;

    int callback_count = 0;

    auto p = preprocessor::create(
        endevor_preprocessor_options(),
        [&callback_count](std::string_view s) {
            EXPECT_EQ(s, "AAA");
            ++callback_count;
            return std::nullopt;
        },
        &diags);

    auto result = p->generate_replacement(document("++INCLUDE AAA\nBBB"));

    EXPECT_EQ(callback_count, 1);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result.at(0).text(), "BBB");
    EXPECT_TRUE(matches_message_codes(diags.diags, { "END001" }));
}

TEST(endevor_preprocessor, cycle)
{
    diagnostic_op_consumer_container diags;

    int callback_count = 0;

    auto p = preprocessor::create(
        endevor_preprocessor_options(),
        [&callback_count](std::string_view s) {
            EXPECT_EQ(s, "AAA");
            ++callback_count;
            return std::string("-INC AAA");
        },
        &diags);

    auto result = p->generate_replacement(document("++INCLUDE AAA"));

    EXPECT_EQ(callback_count, 1);
    EXPECT_EQ(result.size(), 0);
    EXPECT_TRUE(matches_message_codes(diags.diags, { "END002" }));
}

TEST(endevor_preprocessor, nested)
{
    diagnostic_op_consumer_container diags;

    int callback_count = 0;

    auto p = preprocessor::create(
        endevor_preprocessor_options(),
        [&callback_count](std::string_view s) -> std::optional<std::string> {
            ++callback_count;
            if (s == "MEMBER")
                return "BBB\n-INC NESTED\nDDD";
            if (s == "NESTED")
                return "CCC";
            return std::nullopt;
        },
        &diags);

    auto result = p->generate_replacement(document("AAA\n-INC MEMBER\nEEE"));

    EXPECT_EQ(callback_count, 2);
    EXPECT_TRUE(diags.diags.empty());
    EXPECT_EQ(result.size(), 5);
    EXPECT_EQ(result.text(), "AAA\nBBB\nCCC\nDDD\nEEE\n");
}

TEST(endevor_preprocessor, with_analyzer)
{
    mock_parse_lib_provider libs({
        { "MEMBER", R"(
TESTVAL EQU 42
)" },
    });
    std::string input = R"(
-INC MEMBER
)";

    analyzer a(input, analyzer_options { &libs, endevor_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "TESTVAL"), 42);
}
