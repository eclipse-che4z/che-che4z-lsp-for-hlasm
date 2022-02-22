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

#include "gtest/gtest.h"

#include "common_testing.h"

namespace {

struct diagnostics_sysvar_params
{
    std::string input;

    static diagnostics_sysvar_params create_input(std::string system_variable, std::string subscript)
    {
        diagnostics_sysvar_params result;

        result.input =
            R"( 
 MACRO
 MAC
 &)" + system_variable
            + subscript + R"(
 MEND 
 
 MAC
)";

        return result;
    }
};

class diagnostics_sysvar_without_subscript_fixture : public ::testing::TestWithParam<diagnostics_sysvar_params>
{};

class diagnostics_sysvar_with_subscript_1_fixture : public ::testing::TestWithParam<diagnostics_sysvar_params>
{};

class diagnostics_sysvar_with_subscripts_1_1_fixture : public ::testing::TestWithParam<diagnostics_sysvar_params>
{};

class diagnostics_sysvar_with_subscripts_1_1_1_fixture : public ::testing::TestWithParam<diagnostics_sysvar_params>
{};

class diagnostics_sysvar_with_subscript_0_fixture : public ::testing::TestWithParam<diagnostics_sysvar_params>
{};

class diagnostics_sysvar_with_subscripts_1_0_fixture : public ::testing::TestWithParam<diagnostics_sysvar_params>
{};

class diagnostics_sysvar_with_subscripts_1_1_0_fixture : public ::testing::TestWithParam<diagnostics_sysvar_params>
{};

class diagnostics_sysvar_with_subscript_2_fixture : public ::testing::TestWithParam<diagnostics_sysvar_params>
{};

class diagnostics_sysvar_with_subscripts_2_0_fixture : public ::testing::TestWithParam<diagnostics_sysvar_params>
{};

class diagnostics_sysvar_with_subscripts_2_1_fixture : public ::testing::TestWithParam<diagnostics_sysvar_params>
{};

class diagnostics_sysvar_with_subscripts_2_2_2_fixture : public ::testing::TestWithParam<diagnostics_sysvar_params>
{};

} // namespace

INSTANTIATE_TEST_SUITE_P(diagnostics_sysvar,
    diagnostics_sysvar_without_subscript_fixture,
    ::testing::Values(
        diagnostics_sysvar_params::create_input("SYSNDX", ""), diagnostics_sysvar_params::create_input("SYSNEST", "")));

INSTANTIATE_TEST_SUITE_P(diagnostics_sysvar,
    diagnostics_sysvar_with_subscript_1_fixture,
    ::testing::Values(diagnostics_sysvar_params::create_input("SYSNDX", "(1)"),
        diagnostics_sysvar_params::create_input("SYSNEST", "(1)")));


INSTANTIATE_TEST_SUITE_P(diagnostics_sysvar,
    diagnostics_sysvar_with_subscripts_1_1_fixture,
    ::testing::Values(diagnostics_sysvar_params::create_input("SYSNDX", "(1,1)"),
        diagnostics_sysvar_params::create_input("SYSNEST", "(1,1)")));

INSTANTIATE_TEST_SUITE_P(diagnostics_sysvar,
    diagnostics_sysvar_with_subscripts_1_1_1_fixture,
    ::testing::Values(diagnostics_sysvar_params::create_input("SYSNDX", "(1,1,1)"),
        diagnostics_sysvar_params::create_input("SYSNEST", "(1,1,1)")));

INSTANTIATE_TEST_SUITE_P(diagnostics_sysvar,
    diagnostics_sysvar_with_subscript_0_fixture,
    ::testing::Values(diagnostics_sysvar_params::create_input("SYSNDX", "(0)"),
        diagnostics_sysvar_params::create_input("SYSNEST", "(0)")));

INSTANTIATE_TEST_SUITE_P(diagnostics_sysvar,
    diagnostics_sysvar_with_subscripts_1_0_fixture,
    ::testing::Values(diagnostics_sysvar_params::create_input("SYSNDX", "(1,0)"),
        diagnostics_sysvar_params::create_input("SYSNEST", "(1,0)")));

INSTANTIATE_TEST_SUITE_P(diagnostics_sysvar,
    diagnostics_sysvar_with_subscripts_1_1_0_fixture,
    ::testing::Values(diagnostics_sysvar_params::create_input("SYSNDX", "(1,1,0)"),
        diagnostics_sysvar_params::create_input("SYSNEST", "(1,1,0)")));

INSTANTIATE_TEST_SUITE_P(diagnostics_sysvar,
    diagnostics_sysvar_with_subscript_2_fixture,
    ::testing::Values(diagnostics_sysvar_params::create_input("SYSNDX", "(2)"),
        diagnostics_sysvar_params::create_input("SYSNEST", "(2)")));

INSTANTIATE_TEST_SUITE_P(diagnostics_sysvar,
    diagnostics_sysvar_with_subscripts_2_0_fixture,
    ::testing::Values(diagnostics_sysvar_params::create_input("SYSNDX", "(2,0)"),
        diagnostics_sysvar_params::create_input("SYSNEST", "(2,0)")));

INSTANTIATE_TEST_SUITE_P(diagnostics_sysvar,
    diagnostics_sysvar_with_subscripts_2_1_fixture,
    ::testing::Values(diagnostics_sysvar_params::create_input("SYSNDX", "(2,1)"),
        diagnostics_sysvar_params::create_input("SYSNEST", "(2,1)")));

INSTANTIATE_TEST_SUITE_P(diagnostics_sysvar,
    diagnostics_sysvar_with_subscripts_2_2_2_fixture,
    ::testing::Values(diagnostics_sysvar_params::create_input("SYSNDX", "(2,2,2)"),
        diagnostics_sysvar_params::create_input("SYSNEST", "(2,2,2)")));

TEST_P(diagnostics_sysvar_without_subscript_fixture, invalid_opcode)
{
    std::string input(GetParam().input);

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)1);
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E049" }));
}



TEST_P(diagnostics_sysvar_with_subscript_1_fixture, invalid_opcode)
{
    std::string input(GetParam().input);

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)1);
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E049" }));
}


TEST_P(diagnostics_sysvar_with_subscripts_1_1_fixture, invalid_opcode)
{
    std::string input(GetParam().input);

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)1);
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E049" }));
}



TEST_P(diagnostics_sysvar_with_subscripts_1_1_1_fixture, invalid_opcode)
{
    std::string input(GetParam().input);

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)1);
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E049" }));
}



TEST_P(diagnostics_sysvar_with_subscript_0_fixture, null_opcode_invalid_subscript)
{
    std::string input(GetParam().input);

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)2);
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E012", "E074" }));
}



TEST_P(diagnostics_sysvar_with_subscripts_1_0_fixture, null_opcode_invalid_subscript)
{
    std::string input(GetParam().input);

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)2);
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E012", "E074" }));
}

TEST_P(diagnostics_sysvar_with_subscripts_1_1_0_fixture, null_opcode_invalid_subscript)
{
    std::string input(GetParam().input);

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)2);
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E012", "E074" }));
}

TEST_P(diagnostics_sysvar_with_subscript_2_fixture, null_opcode)
{
    std::string input(GetParam().input);

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)1);
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E074" }));
}



TEST_P(diagnostics_sysvar_with_subscripts_2_0_fixture, null_opcode)
{
    std::string input(GetParam().input);

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)1);
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E074" }));
}

TEST_P(diagnostics_sysvar_with_subscripts_2_1_fixture, null_opcode)
{
    std::string input(GetParam().input);

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)1);
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E074" }));
}

TEST_P(diagnostics_sysvar_with_subscripts_2_2_2_fixture, null_opcode)
{
    std::string input(GetParam().input);

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)1);
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E074" }));
}