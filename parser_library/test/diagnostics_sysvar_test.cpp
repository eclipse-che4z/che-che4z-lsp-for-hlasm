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
 
TEST CSECT
 MAC
)";

        return result;
    }
};

class diagnostics_sysvar_invalid_opcode_fixture : public ::testing::TestWithParam<diagnostics_sysvar_params>
{};

class diagnostics_sysvar_null_opcode_invalid_subscript_fixture
    : public ::testing::TestWithParam<diagnostics_sysvar_params>
{};

class diagnostics_sysvar_null_opcode_fixture : public ::testing::TestWithParam<diagnostics_sysvar_params>
{};

} // namespace

INSTANTIATE_TEST_SUITE_P(diagnostics_sysvar,
    diagnostics_sysvar_invalid_opcode_fixture,
    ::testing::Values(diagnostics_sysvar_params::create_input("SYSDATC", ""),
        diagnostics_sysvar_params::create_input("SYSDATC", "(1)"),
        diagnostics_sysvar_params::create_input("SYSDATC", "(1,1)"),
        diagnostics_sysvar_params::create_input("SYSDATC", "(1,1,1)"),
        diagnostics_sysvar_params::create_input("SYSECT", ""),
        diagnostics_sysvar_params::create_input("SYSECT", "(1)"),
        diagnostics_sysvar_params::create_input("SYSECT", "(1,1)"),
        diagnostics_sysvar_params::create_input("SYSECT", "(1,1,1)"),
        diagnostics_sysvar_params::create_input("SYSLOC", ""),
        diagnostics_sysvar_params::create_input("SYSLOC", "(1)"),
        diagnostics_sysvar_params::create_input("SYSLOC", "(1,1)"),
        diagnostics_sysvar_params::create_input("SYSLOC", "(1,1,1)"),
        diagnostics_sysvar_params::create_input("SYSNDX", ""),
        diagnostics_sysvar_params::create_input("SYSNDX", "(1)"),
        diagnostics_sysvar_params::create_input("SYSNDX", "(1,1)"),
        diagnostics_sysvar_params::create_input("SYSNDX", "(1,1,1)"),
        diagnostics_sysvar_params::create_input("SYSNEST", ""),
        diagnostics_sysvar_params::create_input("SYSNEST", "(1)"),
        diagnostics_sysvar_params::create_input("SYSNEST", "(1,1)"),
        diagnostics_sysvar_params::create_input("SYSNEST", "(1,1,1)"),
        diagnostics_sysvar_params::create_input("SYSOPT_RENT", ""),
        diagnostics_sysvar_params::create_input("SYSOPT_RENT", "(1)"),
        diagnostics_sysvar_params::create_input("SYSOPT_RENT", "(1,1)"),
        diagnostics_sysvar_params::create_input("SYSOPT_RENT", "(1,1,1)")));

INSTANTIATE_TEST_SUITE_P(diagnostics_sysvar,
    diagnostics_sysvar_null_opcode_invalid_subscript_fixture,
    ::testing::Values(
        diagnostics_sysvar_params::create_input("SYSDATC", "(0)"),
        diagnostics_sysvar_params::create_input("SYSDATC", "(1,0)"),
        diagnostics_sysvar_params::create_input("SYSDATC", "(1,1,0)"),
        diagnostics_sysvar_params::create_input("SYSECT", "(0)"),
        diagnostics_sysvar_params::create_input("SYSECT", "(1,0)"),
        diagnostics_sysvar_params::create_input("SYSECT", "(1,1,0)"),
        diagnostics_sysvar_params::create_input("SYSLOC", "(0)"),
        diagnostics_sysvar_params::create_input("SYSLOC", "(1,0)"),
        diagnostics_sysvar_params::create_input("SYSLOC", "(1,1,0)"),
        diagnostics_sysvar_params::create_input("SYSNDX", "(0)"),
        diagnostics_sysvar_params::create_input("SYSNDX", "(1,0)"),
        diagnostics_sysvar_params::create_input("SYSNDX", "(1,1,0)"),
        diagnostics_sysvar_params::create_input("SYSNEST", "(0)"),
        diagnostics_sysvar_params::create_input("SYSNEST", "(1,0)"),
        diagnostics_sysvar_params::create_input("SYSNEST", "(1,1,0)"),
        diagnostics_sysvar_params::create_input("SYSOPT_RENT", "(0)"),
        diagnostics_sysvar_params::create_input("SYSOPT_RENT", "(1,0)"),
        diagnostics_sysvar_params::create_input("SYSOPT_RENT", "(1,1,0)")));

INSTANTIATE_TEST_SUITE_P(diagnostics_sysvar,
    diagnostics_sysvar_null_opcode_fixture,
    ::testing::Values(
        diagnostics_sysvar_params::create_input("SYSDATC", "(2)"),
        diagnostics_sysvar_params::create_input("SYSDATC", "(2,0)"),
        diagnostics_sysvar_params::create_input("SYSDATC", "(2,1)"),
        diagnostics_sysvar_params::create_input("SYSDATC", "(2,2,2)"),
        diagnostics_sysvar_params::create_input("SYSECT", "(2)"),
        diagnostics_sysvar_params::create_input("SYSECT", "(2,0)"),
        diagnostics_sysvar_params::create_input("SYSECT", "(2,1)"),
        diagnostics_sysvar_params::create_input("SYSECT", "(2,2,2)"),
        diagnostics_sysvar_params::create_input("SYSLOC", "(2)"),
        diagnostics_sysvar_params::create_input("SYSLOC", "(2,0)"),
        diagnostics_sysvar_params::create_input("SYSLOC", "(2,1)"),
        diagnostics_sysvar_params::create_input("SYSLOC", "(2,2,2)"),
        diagnostics_sysvar_params::create_input("SYSNDX", "(2)"),
        diagnostics_sysvar_params::create_input("SYSNDX", "(2,0)"),
        diagnostics_sysvar_params::create_input("SYSNDX", "(2,1)"),
        diagnostics_sysvar_params::create_input("SYSNDX", "(2,2,2)"),
        diagnostics_sysvar_params::create_input("SYSNEST", "(2)"),
        diagnostics_sysvar_params::create_input("SYSNEST", "(2,0)"),
        diagnostics_sysvar_params::create_input("SYSNEST", "(2,1)"),
        diagnostics_sysvar_params::create_input("SYSNEST", "(2,2,2)"),
        diagnostics_sysvar_params::create_input("SYSOPT_RENT", "(2)"),
        diagnostics_sysvar_params::create_input("SYSOPT_RENT", "(2,0)"),
        diagnostics_sysvar_params::create_input("SYSOPT_RENT", "(2,1)"),
        diagnostics_sysvar_params::create_input("SYSOPT_RENT", "(2,2,2)")
        ));



TEST_P(diagnostics_sysvar_invalid_opcode_fixture, invalid_opcode)
{
    std::string input(GetParam().input);

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)1);
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E049" }));
}

TEST_P(diagnostics_sysvar_null_opcode_invalid_subscript_fixture, null_opcode_invalid_subscript)
{
    std::string input(GetParam().input);

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)2);
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E012", "E074" }));
}

TEST_P(diagnostics_sysvar_null_opcode_fixture, null_opcode)
{
    std::string input(GetParam().input);

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    ASSERT_EQ(a.diags().size(), (size_t)1);
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E074" }));
}