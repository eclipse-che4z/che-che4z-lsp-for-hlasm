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

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "../common_testing.h"
#include "../mock_parse_lib_provider.h"
#include "analyzer.h"
#include "diagnostic_consumer.h"
#include "document.h"
#include "preprocessor_options.h"
#include "processing/preprocessor.h"
#include "semantics/source_info_processor.h"
#include "utils/resource_location.h"

using namespace hlasm_plugin::parser_library::processing;
using namespace hlasm_plugin::utils::resource;

class endevor_preprocessor_test : public testing::Test
{
public:
    endevor_preprocessor_test()
        : m_src_info(false)
    {}

    std::unique_ptr<preprocessor> create_preprocessor(library_fetcher libs)
    {
        return preprocessor::create(endevor_preprocessor_options(), libs, &m_diags, m_src_info);
    }

protected:
    semantics::source_info_processor m_src_info;
    diagnostic_op_consumer_container m_diags;
    int m_callback_count = 0;
};

TEST_F(endevor_preprocessor_test, basic_inc)
{
    auto p = create_preprocessor([&callback_count = m_callback_count](std::string_view s) {
        EXPECT_EQ(s, "AAA");
        ++callback_count;
        return std::pair<std::string, hlasm_plugin::utils::resource::resource_location>(
            "TEST", hlasm_plugin::utils::resource::resource_location());
    });

    auto result = p->generate_replacement(document("-INC AAA"));

    EXPECT_EQ(m_callback_count, 1);
    EXPECT_TRUE(m_diags.diags.empty());
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result.at(0).text(), "TEST");
}

TEST_F(endevor_preprocessor_test, basic_include)
{
    auto p = create_preprocessor([&callback_count = m_callback_count](std::string_view s) {
        EXPECT_EQ(s, "AAA");
        ++callback_count;
        return std::pair<std::string, hlasm_plugin::utils::resource::resource_location>(
            "TEST", hlasm_plugin::utils::resource::resource_location());
    });

    auto result = p->generate_replacement(document("++INCLUDE AAA"));

    EXPECT_EQ(m_callback_count, 1);
    EXPECT_TRUE(m_diags.diags.empty());
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result.at(0).text(), "TEST");
}

TEST_F(endevor_preprocessor_test, missing_member)
{
    auto p = create_preprocessor([&callback_count = m_callback_count](std::string_view s) {
        EXPECT_EQ(s, "AAA");
        ++callback_count;
        return std::nullopt;
    });

    auto result = p->generate_replacement(document("++INCLUDE AAA\nBBB"));

    EXPECT_EQ(m_callback_count, 1);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result.at(0).text(), "BBB");
    EXPECT_TRUE(matches_message_codes(m_diags.diags, { "END001" }));
}

TEST_F(endevor_preprocessor_test, cycle)
{
    auto p = create_preprocessor([&callback_count = m_callback_count](std::string_view s) {
        EXPECT_EQ(s, "AAA");
        ++callback_count;
        return std::pair<std::string, hlasm_plugin::utils::resource::resource_location>(
            "-INC AaA", hlasm_plugin::utils::resource::resource_location());
    });

    auto result = p->generate_replacement(document("++INCLUDE AAA"));

    EXPECT_EQ(m_callback_count, 1);
    EXPECT_EQ(result.size(), 0);
    EXPECT_TRUE(matches_message_codes(m_diags.diags, { "END002" }));
}

TEST_F(endevor_preprocessor_test, nested)
{
    auto p = create_preprocessor(
        [&callback_count = m_callback_count](std::string_view s)
            -> std::optional<std::pair<std::string, hlasm_plugin::utils::resource::resource_location>> {
            ++callback_count;
            if (s == "MEMBER")
                return std::pair<std::string, hlasm_plugin::utils::resource::resource_location>(
                    "BBB\n-INC NESTED\nDDD", hlasm_plugin::utils::resource::resource_location());
            if (s == "NESTED")
                return std::pair<std::string, hlasm_plugin::utils::resource::resource_location>(
                    "CCC", hlasm_plugin::utils::resource::resource_location());
            return std::nullopt;
        });

    auto result = p->generate_replacement(document("AAA\n-INC MEMBER\nEEE"));

    EXPECT_EQ(m_callback_count, 2);
    EXPECT_TRUE(m_diags.diags.empty());
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
-INC MeMbEr
)";

    analyzer a(input, analyzer_options { &libs, endevor_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "TESTVAL"), 42);
}
