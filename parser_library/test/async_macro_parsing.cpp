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

#include "gtest/gtest.h"

#include "common_testing.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::utils;
using namespace ::testing;

struct async_macro_parsing_fixture : ::testing::Test, parse_lib_provider
{
    std::unordered_map<std::string, std::string, hashers::string_hasher, std::equal_to<>> m_files;
    std::vector<std::pair<std::unique_ptr<analyzer>, std::function<void()>>> m_nested_analyzers;

    // Inherited via parse_lib_provider
    void parse_library(
        std::string_view library, analyzing_context ctx, library_data data, std::function<void(bool)> callback) override
    {
        auto it = m_files.find(library);
        if (it == m_files.end())
        {
            m_nested_analyzers.emplace_back(nullptr, [callback]() {
                if (callback)
                    callback(false);
            });
        }
        else
        {
            m_nested_analyzers.emplace_back(
                std::make_unique<analyzer>(it->second,
                    analyzer_options {
                        hlasm_plugin::utils::resource::resource_location(library), this, std::move(ctx), data }),
                [callback]() {
                    if (callback)
                        callback(true);
                });
        }
    }
    bool has_library(std::string_view library, resource::resource_location* url) const override
    {
        auto it = m_files.find(library);
        if (it == m_files.end())
            return false;
        if (url)
            *url = resource::resource_location(it->second);
        return true;
    }
    std::optional<std::pair<std::string, resource::resource_location>> get_library(
        std::string_view library) const override
    {
        if (auto it = m_files.find(library); it != m_files.end())
            return std::make_pair(it->second, resource::resource_location(it->first));
        else
            return std::nullopt;
    }

    void analyze(analyzer& a)
    {
        while (true)
        {
            if (!m_nested_analyzers.empty())
            {
                auto& [nested_a, callback] = m_nested_analyzers.back();
                if (!nested_a || !nested_a->analyze_step())
                {
                    callback();
                    if (nested_a)
                        a.collect_diags_from_child(*nested_a);
                    m_nested_analyzers.pop_back();
                }
                continue;
            }
            if (!a.analyze_step())
                break;
        }
    }
};

TEST_F(async_macro_parsing_fixture, macro_not_found)
{
    analyzer a(" MAC", analyzer_options(this));
    analyze(a);
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E049" }));
}

TEST_F(async_macro_parsing_fixture, macro_not_found_twice)
{
    analyzer a(R"(
    MAC
    MAC
)",
        analyzer_options(this));
    analyze(a);
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E049", "E049" }));
}

TEST_F(async_macro_parsing_fixture, copy_not_found)
{
    analyzer a(" COPY MAC", analyzer_options(this));
    analyze(a);
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E058" }));
}

TEST_F(async_macro_parsing_fixture, copy_not_found_twice)
{
    analyzer a(R"(
    COPY MAC
    COPY MAC
)",
        analyzer_options(this));
    analyze(a);
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E058", "E058" }));
}

TEST_F(async_macro_parsing_fixture, copy_not_found_in_macro)
{
    m_files.try_emplace("MAC", R"( MACRO
    MAC
    COPY COPYBOOK
    MEND
)");
    analyzer a(" MAC", analyzer_options(this));
    analyze(a);
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E058" }));
}

TEST_F(async_macro_parsing_fixture, copy_in_macro)
{
    m_files.try_emplace("MAC", R"( MACRO
    MAC
    COPY COPYBOOK
    MEND
)");
    m_files.try_emplace("COPYBOOK", " MNOTE 'AAA'");
    analyzer a(" MAC", analyzer_options(this));
    analyze(a);
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
}

TEST_F(async_macro_parsing_fixture, copy_from_ainsert)
{
    m_files.try_emplace("COPYBOOK", " MNOTE 'AAA'");
    analyzer a(" AINSERT ' COPY COPYBOOK',BACK", analyzer_options(this));
    analyze(a);
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
}

TEST_F(async_macro_parsing_fixture, self_calling_macro)
{
    m_files.try_emplace("MAC", R"( MACRO
    MAC  &P
    AIF  (&P EQ 1).E
    MAC  1
.E  MEND
)");
    analyzer a(" MAC 0", analyzer_options(this));
    analyze(a);
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
}

TEST_F(async_macro_parsing_fixture, delete_macro)
{
    m_files.try_emplace("MAC", R"( MACRO
    MAC
    MNOTE 'AAA'
    MEND
)");
    analyzer a(R"(
    MAC
MAC OPSYN
    MAC
)",
        analyzer_options(this));
    analyze(a);
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "MNOTE", "MNOTE" }));
}

TEST_F(async_macro_parsing_fixture, delete_inline_macro)
{
    m_files.try_emplace("MAC", R"( MACRO
    MAC
    MNOTE 'AAA'
    MEND
)");
    analyzer a(R"(
    MAC

    MACRO
    MAC
    MNOTE 'BBB'
    MEND

    MAC
MAC OPSYN
    MAC
)",
        analyzer_options(this));
    analyze(a);
    a.collect_diags();

    EXPECT_TRUE(matches_message_text(a.diags(), { "AAA", "BBB", "AAA" }));
}

TEST_F(async_macro_parsing_fixture, unknown_instruction)
{
    m_files.try_emplace("MAC", R"( MACRO
    MAC  &P
    &P
    MEND
)");
    m_files.try_emplace("PRT1", R"( MACRO
    PRT1
    MNOTE 'AAA'
    MEND
)");
    m_files.try_emplace("PRT2", R"( MACRO
    PRT2
    MNOTE 'AAA'
    MEND
)");
    m_files.try_emplace("COPYBOOK", R"(
    &P
)");
    analyzer a(R"(
    MAC  PRT1
&P  SETC 'PRT2'
    COPY COPYBOOK
)",
        analyzer_options(this));
    analyze(a);
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "MNOTE", "MNOTE" }));
}
