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

#include "../common_testing.h"

namespace {

struct system_variable_params
{
    std::string input;
    std::vector<std::string> exp_results;

    static system_variable_params create_input(std::string system_variable, std::vector<std::string> expected_results)
    {
        system_variable_params params;

        params.input =
            R"( 
      MACRO
      MAC
      GBLC A,B,C,D,E,F,G
&A    SETC '&)" + system_variable
            + R"('
&B    SETC '&)" + system_variable
            + R"((1)'
&C    SETC '&)" + system_variable
            + R"((1,1)'
&D    SETC '&)" + system_variable
            + R"((1,1,1)'
&E    SETC '&)" + system_variable
            + R"((2)'
&F    SETC '&)" + system_variable
            + R"((2,2)'
&G    SETC '&)" + system_variable
            + R"((3)'
 MEND 
 
TEST  CSECT
      GBLC A,B,C,D,E,F,G
NAME  MAC ONE,TWO,,(3,(4,5,6),,8),,TEN,()
)";


        params.exp_results = std::move(expected_results);

        return params;
    }
};

class system_variable_standard_behavior_fixture : public ::testing::TestWithParam<system_variable_params>
{};

} // namespace

INSTANTIATE_TEST_SUITE_P(system_variable,
    system_variable_standard_behavior_fixture,
    ::testing::Values(system_variable_params::create_input("SYSNDX", { "0001", "0001", "0001", "0001", "", "", "" })));


TEST_P(system_variable_standard_behavior_fixture, standard_behavior)
{
    std::string input(GetParam().input);
    std::vector<std::string> exp_behavior(GetParam().exp_results);

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "A"), exp_behavior[0]);
    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "B"), exp_behavior[1]);
    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "C"), exp_behavior[2]);
    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "D"), exp_behavior[3]);
    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "E"), exp_behavior[4]);
    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "F"), exp_behavior[5]);
    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "G"), exp_behavior[6]);
}
