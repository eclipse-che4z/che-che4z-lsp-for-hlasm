/*
 * Copyright (c) 2025 Broadcom.
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
#include <initializer_list>
#include <string_view>

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "../mock_parse_lib_provider.h"

inline const hlasm_plugin::utils::resource::resource_location opencode("opencode");

inline bool matches_diagnostic_stack(const diagnostic& d, std::initializer_list<std::string_view> stack_)
{
    std::span<const std::string_view> stack = stack_;
    assert(!stack.empty());

    static constexpr auto uri = [](const auto& ri) -> std::string_view { return ri.location.uri; };

    return d.file_uri == stack.front() && std::ranges::equal(d.related, stack.subspan(1), {}, uri);
}

TEST(macro_processing_stack, no_macro)
{
    std::string input = R"(
    MNOTE 'Hello'
)";
    analyzer a(input, analyzer_options { opencode });
    a.analyze();

    ASSERT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
    EXPECT_TRUE(matches_diagnostic_stack(a.diags().front(), { "opencode" }));
}

TEST(macro_processing_stack, plain_inline)
{
    std::string input = R"(
    MACRO
    MAC
    MNOTE 'Hello'
    MEND

    MAC
)";
    analyzer a(input, analyzer_options { opencode });
    a.analyze();

    ASSERT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
    EXPECT_TRUE(matches_diagnostic_stack(a.diags().front(), { "opencode", "opencode" }));
}

TEST(macro_processing_stack, plain_external)
{
    mock_parse_lib_provider lib({
        { "MAC",
            R"(.*
    MACRO
    MAC
    MNOTE 'Hello'
    MEND
)" },
    });
    std::string input = R"(
    MAC
)";
    analyzer a(input, analyzer_options { opencode, &lib });
    a.analyze();

    ASSERT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
    EXPECT_TRUE(matches_diagnostic_stack(a.diags().front(), { "MAC", "opencode" }));
}

TEST(macro_processing_stack, copy_in_macro)
{
    mock_parse_lib_provider lib({
        { "MAC",
            R"(.*
    MACRO
    MAC
    COPY COPYBOOK
    MEND
)" },
        { "COPYBOOK", " MNOTE 'Hello'" },
    });
    std::string input = R"(
    MAC
)";
    analyzer a(input, analyzer_options { opencode, &lib });
    a.analyze();

    ASSERT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
    EXPECT_TRUE(matches_diagnostic_stack(a.diags().front(), { "COPYBOOK", "MAC", "opencode" }));
}

TEST(macro_processing_stack, macro_copy_copy)
{
    mock_parse_lib_provider lib({
        { "MAC",
            R"(.*
    MACRO
    MAC
    MNOTE 'Hello'
    MEND
)" },
        { "COPYBOOK", " MAC" },
    });
    std::string input = R"(
    COPY COPYBOOK
)";
    analyzer a(input, analyzer_options { opencode, &lib });
    a.analyze();

    ASSERT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
    EXPECT_TRUE(matches_diagnostic_stack(a.diags().front(), { "MAC", "COPYBOOK", "opencode" }));
}

TEST(macro_processing_stack, nested_macro)
{
    mock_parse_lib_provider lib({
        { "MAC",
            R"(.*
    MACRO
    MAC
    MACRO
    NESTED
    MNOTE 'Hello'
    MEND
    MEND
)" },
    });
    std::string input = R"(
    MAC
    NESTED
)";
    analyzer a(input, analyzer_options { opencode, &lib });
    a.analyze();

    ASSERT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
    EXPECT_TRUE(matches_diagnostic_stack(a.diags().front(), { "MAC", "opencode" }));
}

TEST(macro_processing_stack, nested_macro_with_copy)
{
    mock_parse_lib_provider lib({
        { "MAC",
            R"(.*
    MACRO
    MAC
    MACRO
    NESTED
    COPY COPYBOOK
    MEND
    MEND
)" },
        { "COPYBOOK", " MNOTE 'Hello'" },
    });
    std::string input = R"(
    MAC
    NESTED
)";
    analyzer a(input, analyzer_options { opencode, &lib });
    a.analyze();

    ASSERT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
    EXPECT_TRUE(matches_diagnostic_stack(a.diags().front(), { "COPYBOOK", "MAC", "opencode" }));
}

TEST(macro_processing_stack, inline_nested_macro_with_copy)
{
    mock_parse_lib_provider lib({
        { "COPYBOOK", " MNOTE 'Hello'" },
    });
    std::string input = R"(
    MACRO
    MAC
    MACRO
    NESTED
    COPY COPYBOOK
    MEND
    MEND
*
    MAC
    NESTED
)";
    analyzer a(input, analyzer_options { opencode, &lib });
    a.analyze();

    ASSERT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
    EXPECT_TRUE(matches_diagnostic_stack(a.diags().front(), { "COPYBOOK", "opencode", "opencode" }));
}

TEST(macro_processing_stack, copy_whole_macro)
{
    mock_parse_lib_provider lib({
        { "COPYBOOK", R"(
    MACRO
    MAC
    MNOTE 'Hello'
    MEND
)" },
    });
    std::string input = R"(
    COPY COPYBOOK
    MAC
)";
    analyzer a(input, analyzer_options { opencode, &lib });
    a.analyze();

    ASSERT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
    EXPECT_TRUE(matches_diagnostic_stack(a.diags().front(), { "COPYBOOK", "opencode" }));
}

TEST(macro_processing_stack, copy_whole_macro_with_nested)
{
    mock_parse_lib_provider lib({
        { "COPYBOOK", R"(
    MACRO
    MAC
    MACRO
    NESTED
    MNOTE 'Hello'
    MEND
    MEND
)" },
    });
    std::string input = R"(
    COPY COPYBOOK
    MAC
    NESTED
)";
    analyzer a(input, analyzer_options { opencode, &lib });
    a.analyze();

    ASSERT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
    EXPECT_TRUE(matches_diagnostic_stack(a.diags().front(), { "COPYBOOK", "opencode" }));
}

TEST(macro_processing_stack, copy_whole_macro_with_copybook)
{
    mock_parse_lib_provider lib({
        { "ACTION", " MNOTE 'Hello'" },
        { "COPYBOOK", R"(
    MACRO
    MAC
    COPY ACTION
    MEND
)" },
    });
    std::string input = R"(
    COPY COPYBOOK
    MAC
)";
    analyzer a(input, analyzer_options { opencode, &lib });
    a.analyze();

    ASSERT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
    EXPECT_TRUE(matches_diagnostic_stack(a.diags().front(), { "ACTION", "COPYBOOK", "opencode" }));
}

TEST(macro_processing_stack, copy_whole_macro_with_nested_macro_in_copybook)
{
    mock_parse_lib_provider lib({
        { "NESTCPY", R"(
    MACRO
    NESTED
    MNOTE 'Hello'
    MEND
)" },
        { "COPYBOOK", R"(
    MACRO
    MAC
    COPY NESTCPY
    MEND
)" },
    });
    std::string input = R"(
    COPY COPYBOOK
    MAC
    NESTED
)";
    analyzer a(input, analyzer_options { opencode, &lib });
    a.analyze();

    ASSERT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
    EXPECT_TRUE(matches_diagnostic_stack(a.diags().front(), { "NESTCPY", "opencode" }));
}

TEST(macro_processing_stack, copy_whole_macro_with_nested_macro_in_copybook_with_copybook)
{
    mock_parse_lib_provider lib({
        { "ACTION", " MNOTE 'Hello'" },
        { "NESTCPY", R"(
    MACRO
    NESTED
    COPY ACTION
    MEND
)" },
        { "COPYBOOK", R"(
    MACRO
    MAC
    COPY NESTCPY
    MEND
)" },
    });
    std::string input = R"(
    COPY COPYBOOK
    MAC
    NESTED
)";
    analyzer a(input, analyzer_options { opencode, &lib });
    a.analyze();

    ASSERT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
    EXPECT_TRUE(matches_diagnostic_stack(a.diags().front(), { "ACTION", "NESTCPY", "opencode" }));
}

TEST(macro_processing_stack, double_copy_with_nested_0)
{
    mock_parse_lib_provider lib({
        { "COPY1", " COPY COPY2" },
        { "COPY2", R"(
    MACRO
    MAC
    MACRO
    NESTED
    MACRO
    INNER
    MNOTE 'Hello'
    MEND
    MEND
    MEND
)" },
    });
    std::string input = R"(
    COPY COPY1
    MAC
    NESTED
    INNER
)";
    analyzer a(input, analyzer_options { opencode, &lib });
    a.analyze();

    ASSERT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
    EXPECT_TRUE(matches_diagnostic_stack(a.diags().front(), { "COPY2", "opencode" }));
}

TEST(macro_processing_stack, double_copy_with_nested_1)
{
    mock_parse_lib_provider lib({
        { "COPY1", " COPY COPY2" },
        { "COPY2", R"(
    MACRO
    NESTED
    MACRO
    INNER
    MNOTE 'Hello'
    MEND
    MEND
)" },
        { "MAC", R"(.*
    MACRO
    MAC
    COPY COPY1
    MEND
)" },
    });
    std::string input = R"(
    MAC
    NESTED
    INNER
)";
    analyzer a(input, analyzer_options { opencode, &lib });
    a.analyze();

    ASSERT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
    EXPECT_TRUE(matches_diagnostic_stack(a.diags().front(), { "COPY2", "opencode" }));
}

TEST(macro_processing_stack, double_copy_with_nested_2)
{
    mock_parse_lib_provider lib({
        { "COPY1", " COPY COPY2" },
        { "COPY2", R"(
    MACRO
    INNER
    MNOTE 'Hello'
    MEND
)" },
        { "MAC", R"(.*
    MACRO
    MAC
    MACRO
    NESTED
    COPY COPY1
    MEND
    MEND
)" },
    });
    std::string input = R"(
    MAC
    NESTED
    INNER
)";
    analyzer a(input, analyzer_options { opencode, &lib });
    a.analyze();

    ASSERT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
    EXPECT_TRUE(matches_diagnostic_stack(a.diags().front(), { "COPY2", "opencode" }));
}

TEST(macro_processing_stack, double_copy_with_nested_3)
{
    mock_parse_lib_provider lib({
        { "COPY1", " COPY COPY2" },
        { "COPY2", R"(
    MNOTE 'Hello'
)" },
        { "MAC", R"(.*
    MACRO
    MAC
    MACRO
    NESTED
    MACRO
    INNER
    COPY COPY1
    MEND
    MEND
    MEND
)" },
    });
    std::string input = R"(
    MAC
    NESTED
    INNER
)";
    analyzer a(input, analyzer_options { opencode, &lib });
    a.analyze();

    ASSERT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
    EXPECT_TRUE(matches_diagnostic_stack(a.diags().front(), { "COPY2", "COPY1", "MAC", "opencode" }));
}
