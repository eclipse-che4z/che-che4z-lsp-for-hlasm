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

using namespace hlasm_plugin::parser_library;


TEST(special_lines, process)
{
    std::string input("*PROCESS");
    analyzer a(input, analyzer_options { file_is_opencode::yes });
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();

    ASSERT_EQ(diags.size(), 1);
    EXPECT_TRUE(std::any_of(diags.begin(), diags.end(), [](const auto& msg) { return msg.code == "A010"; }));
}

TEST(special_lines, process_in_macro)
{
    std::string input("*PROCESS");
    analyzer a(input, analyzer_options { file_is_opencode::no });
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();

    ASSERT_EQ(diags.size(), 0);
}

TEST(special_lines, process_after_code_encountered)
{
    std::string input(R"(
*PROCESS)");
    analyzer a(input, analyzer_options { file_is_opencode ::yes });
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();

    ASSERT_EQ(diags.size(), 0);
}

TEST(special_lines, process_multiple)
{
    std::string input(R"(*PROCESS override(using(nowarn))
*PROCESS)");
    analyzer a(input, analyzer_options { file_is_opencode ::yes });
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();

    ASSERT_EQ(diags.size(), 1);
    EXPECT_TRUE(std::any_of(diags.begin(), diags.end(), [](const auto& msg) { return msg.code == "A010"; }));
}

TEST(special_lines, process_after_limit)
{
    std::string input(R"(*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS)");
    analyzer a(input, analyzer_options { file_is_opencode::yes });
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();

    ASSERT_EQ(diags.size(), 0);
}

TEST(special_lines, process_after_ictl)
{
    std::string input(R"( ICTL 1,71,16
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS)");
    analyzer a(input, analyzer_options { file_is_opencode::yes });
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();

    ASSERT_EQ(diags.size(), 1);
    EXPECT_TRUE(std::any_of(diags.begin(), diags.end(), [](const auto& msg) { return msg.code == "A010"; }));
}

TEST(special_lines, process_after_limit_and_ictl)
{
    std::string input(R"( ICTL 1,71,16
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS override(using(nowarn))
*PROCESS)");
    analyzer a(input, analyzer_options { file_is_opencode::yes });
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();

    ASSERT_EQ(diags.size(), 0);
}
