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
#include "utils/general_hashers.h"
#include "utils/task.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::utils;
using namespace ::testing;

struct async_macro_parsing_fixture : ::testing::Test, parse_lib_provider
{
    std::unordered_map<std::string, std::string, hashers::string_hasher, std::equal_to<>> m_files;
    analyzer* current = nullptr;

    // Inherited via parse_lib_provider
    value_task<bool> parse_library(std::string library, analyzing_context ctx, library_data data) override
    {
        auto it = m_files.find(library);
        if (it == m_files.end())
        {
            co_return false;
        }
        else
        {
            analyzer a(it->second,
                analyzer_options {
                    hlasm_plugin::utils::resource::resource_location(std::move(library)),
                    this,
                    std::move(ctx),
                    data,
                });

            co_await a.co_analyze();
            current->collect_diags_from_child(a);
            co_return true;
        }
    }
    bool has_library(std::string_view library, resource::resource_location* url) override
    {
        auto it = m_files.find(library);
        if (it == m_files.end())
            return false;
        if (url)
            *url = resource::resource_location(it->second);
        return true;
    }
    value_task<std::optional<std::pair<std::string, resource::resource_location>>> get_library(
        std::string library) override
    {
        if (auto it = m_files.find(library); it != m_files.end())
            co_return std::make_pair(it->second, resource::resource_location(it->first));
        else
            co_return std::nullopt;
    }

    void analyze(analyzer& a)
    {
        current = &a;
        a.co_analyze().run();
        current = nullptr;
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
