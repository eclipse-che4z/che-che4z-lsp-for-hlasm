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
#include "../mock_parse_lib_provider.h"

namespace {
struct system_variable_params
{
    std::string input;
    std::vector<std::string> exp_results;

    static system_variable_params create_input(std::string system_variable, std::vector<std::string> expected_results)
    {
        assert(5 == expected_results.size());

        system_variable_params params;
        params.input = R"(
          MACRO
          NESTED
          GBLC &VAR2
NSTSCTN   RSECT
&VAR2     SETC '&)"
            + system_variable + R"('
          MEND
          
          MACRO
          MAC_1
          GBLC &VAR1,&VAR3
MAC1SCTN  DSECT
&VAR1     SETC '&)"
            + system_variable + R"('
          NESTED
&VAR3     SETC '&)"
            + system_variable + R"('
          MEND

          MACRO
          MAC_2
          GBLC &VAR4
MAC2SCTN  COM
&VAR4     SETC '&)"
            + system_variable + R"('
          MEND

          MACRO
          MAC_3
          GBLC &VAR5
MAC3SCTN  CSECT
&VAR5     SETC '&)"
            + system_variable + R"('
          MEND
          
          GBLC &VAR1,&VAR2,&VAR3,&VAR4,&VAR5
MAIN      START
          MAC_1
LOC       LOCTR ,
          MAC_2
          MAC_3
          END
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
    ::testing::Values(
        system_variable_params::create_input("SYSECT", { "MAIN", "MAC1SCTN", "MAIN", "NSTSCTN", "MAC2SCTN" }),
        system_variable_params::create_input("SYSLOC", { "MAIN", "MAC1SCTN", "MAIN", "LOC", "MAC2SCTN" }),
        system_variable_params::create_input("SYSMAC", { "MAC_1", "NESTED", "MAC_1", "MAC_2", "MAC_3" }),
        system_variable_params::create_input("SYSNDX", { "0001", "0002", "0001", "0003", "0004" }),
        system_variable_params::create_input("SYSNEST", { "1", "2", "1", "1", "1" }),
        system_variable_params::create_input("SYSSTYP", { "CSECT", "DSECT", "CSECT", "RSECT", "COM" })));

TEST_P(system_variable_standard_behavior_fixture, standard_behavior)
{
    std::string input(GetParam().input);
    std::vector<std::string> exp_behavior(GetParam().exp_results);

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    for (size_t i = 0; i < exp_behavior.size(); ++i)
    {
        EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "VAR" + std::to_string(i + 1)), exp_behavior[i])
            << "for i = " << i;
    }
}

TEST(system_variable, sysstmt)
{
    std::string input = R"(
&STRING SETC  '&SYSSTMT'
&VAR    SETA   &SYSSTMT
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "STRING"), "00000003");
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "VAR"), 4);
}

TEST(system_variable, sysstmt_macros)
{
    std::string input = R"(
        GBLC &VAR1,&VAR2

        MACRO
        MAC
        
        MACRO
        NESTED
        GBLC &VAR2
&VAR2   SETC '&SYSSTMT'
        MEND

        GBLC &VAR1,&VAR2
&VAR1   SETC '&SYSSTMT'
        NESTED
        MEND
        
        MAC
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "VAR1"), "00000027");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "VAR2"), "00000030");
}

TEST(system_variable, sysstmt_copy)
{
    std::string input = R"(
        GBLC &VAR2
        COPY COPY1

        MACRO
        MAC
        GBLC &VAR2
        COPY COPY2
        MEND
        
        MAC
)";

    std::string copy1_filename = "COPY1";
    std::string copy1_source = R"(
&VAR1   SETC '&SYSSTMT'
)";

    std::string copy2_filename = "COPY2";
    std::string copy2_source = R"(
&VAR2   SETC '&SYSSTMT'
)";

    mock_parse_lib_provider lib_prov_instance { { copy1_filename, copy1_source }, { copy2_filename, copy2_source } };
    analyzer a(input, analyzer_options { "ipnut", &lib_prov_instance });
    a.analyze();
    a.collect_diags();
    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "VAR1"), "00000006");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "VAR2"), "00000019");
}

TEST(system_variable, sysstmt_aread)
{
    std::string input = R"(
    MACRO
    MAC_AREAD
&AR AREAD
    AINSERT '&AR',BACK
    MEND
    
    MAC_AREAD
&&A SETA &&SYSSTMT
    END
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 13);
}

TEST(system_variable, sysstyp_csect)
{
    std::string input = R"(
        MACRO
        MAC
        GBLC &VAR
&VAR    SETC '&SYSSTYP'
        MEND
        
        GBLC &VAR
MAIN    CSECT
        MAC
        END
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "VAR"), "CSECT");
}

TEST(system_variable, sysstyp_empty)
{
    std::string input = R"(
        MACRO
        MAC
        GBLC &VAR
&VAR    SETC '&SYSSTYP'
        MEND
        
        GBLC &VAR
        MAC
        END
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "VAR"), "");
}