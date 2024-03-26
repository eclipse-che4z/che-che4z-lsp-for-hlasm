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
#include "../output_handler_mock.h"
#include "analyzer.h"

using namespace hlasm_plugin::parser_library;
using namespace ::testing;

TEST(punch, parameter_count)
{
    std::string input = R"(
    PUNCH
    PUNCH A,A
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A011", "A011" }));
}

TEST(punch, string_only)
{
    std::string input = R"(
    PUNCH A
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A300" }));
}

TEST(punch, non_empty)
{
    std::string input = R"(
    PUNCH ''
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A302" }));
}

TEST(punch, limit_80)
{
    std::string input = R"(
    PUNCH 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaX
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A108" }));
}

TEST(punch, output)
{
    std::string input = " PUNCH 'test string'";

    NiceMock<output_hanler_mock> output;

    analyzer a(input, analyzer_options(&output));

    EXPECT_CALL(output, punch(StrEq("test string")));

    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
};
