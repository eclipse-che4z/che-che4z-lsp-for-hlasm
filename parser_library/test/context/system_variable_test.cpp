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

std::vector<std::string> subscripts_default = { "", "(1)", "(1,1)", "(1,1,1)", "(2)", "(2,2)", "(3)" };
std::vector<std::string> subscripts_sysmac = { "", "(0)", "(1)", "(1,1)", "(1,1,1)", "(2)", "(2,2)", "(3)" };
std::vector<std::string> subscripts_syslist = { "",
    "(0)",
    "(1)",
    "(1,0)",
    "(1,1)",
    "(1,1,1)",
    "(2)",
    "(2,0)",
    "(2,2)",
    "(3)",
    "(3,1)",
    "(3,1,1)",
    "(3,2)",
    "(3,3)",
    "(3,4)",
    "(3,4,1)",
    "(3,5)",
    "(3,5,1)",
    "(3,5,1,1)",
    "(3,5,1,2)",
    "(3,5,2)",
    "(3,5,2,1)",
    "(3,5,2,1)",
    "(4)",
    "(5)",
    "(5,0)",
    "(5,1)",
    "(7)",
    "(7,0)",
    "(7,1)",
    "(7,1,2)",
    "(7,2)",
    "(10)",
    "(14)",
    "(20)" };

struct system_variable_params
{
    std::string input;
    std::vector<std::string> exp_results;

    static system_variable_params create_input(std::string system_variable,
        std::vector<std::string> expected_results,
        std::vector<std::string>& subscripts = subscripts_default)
    {
        assert(subscripts.size() == expected_results.size());

        system_variable_params params;
        params.input = R"(
      MACRO
      MAC
)";

        for (size_t i = 0; i < subscripts.size(); ++i)
        {
            params.input += R"(  GBLC A)" + std::to_string(i) + R"(
)";
        }

        for (size_t i = 0; i < subscripts.size(); ++i)
        {
            params.input += R"(&A)" + std::to_string(i) + R"( SETC  '&)" + system_variable + subscripts[i] + R"('
)";
        }

        params.input += R"(
        MEND

TEST CSECT
)";

        for (size_t i = 0; i < subscripts.size(); ++i)
        {
            params.input += R"(  GBLC A)" + std::to_string(i) + R"(
)";
        }

        params.input += R"(
NNN   MAC WW,XX,(A,,B,,(AA,,BB,CC,,DD),8,,()),,YY,,(),,,ZZ
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
    ::testing::Values(system_variable_params::create_input("SYSECT", { "TEST", "TEST", "TEST", "TEST", "", "", "" }),
        system_variable_params::create_input("SYSLOC", { "TEST", "TEST", "TEST", "TEST", "", "", "" }),
        system_variable_params::create_input(
            "SYSMAC", { "MAC", "MAC", "OPEN CODE", "OPEN CODE", "OPEN CODE", "", "", "" }, subscripts_sysmac),
        system_variable_params::create_input("SYSNDX", { "0001", "0001", "0001", "0001", "", "", "" }),
        system_variable_params::create_input("SYSNEST", { "1", "1", "1", "1", "", "", "" }),
        system_variable_params::create_input("SYSOPT_RENT", { "0", "0", "0", "0", "", "", "" }),
        system_variable_params::create_input("SYSPARM", { "PAR", "PAR", "PAR", "PAR", "", "", "" }),
        system_variable_params::create_input("SYSSTMT", { "00000038", "00000039", "00000040", "00000041", "", "", "" }),
        system_variable_params::create_input("SYSSTYP", { "CSECT", "CSECT", "CSECT", "CSECT", "", "", "" }),
        system_variable_params::create_input(
            "SYSTEM_ID", { "z/OS 02.04.00", "z/OS 02.04.00", "z/OS 02.04.00", "z/OS 02.04.00", "", "", "" }),
        system_variable_params::create_input("SYSLIST",
            { "WW",
                "NNN",
                "WW",
                "",
                "WW",
                "WW",
                "XX",
                "",
                "",
                "(A,,B,,(AA,,BB,CC,,DD),8,,())",
                "A",
                "A",
                "",
                "B",
                "",
                "",
                "(AA,,BB,CC,,DD)",
                "AA",
                "AA",
                "",
                "",
                "",
                "",
                "",
                "YY",
                "",
                "YY",
                "()",
                "",
                "",
                "",
                "",
                "ZZ",
                "",
                "" },
            subscripts_syslist)));

TEST_P(system_variable_standard_behavior_fixture, standard_behavior)
{
    std::string input(GetParam().input);
    std::vector<std::string> exp_behavior(GetParam().exp_results);

    analyzer a(input, analyzer_options { asm_option { "PAR" } });
    a.analyze();
    a.collect_diags();

    for (size_t i = 0; i < exp_behavior.size(); ++i)
    {
        EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "A" + std::to_string(i)), exp_behavior[i])
            << "for i = " << i;
    }
}

TEST(system_variable, sysstmt)
{
    std::string input = R"(
        GBLC &STMTC,&STMTD,&VAR2
&STMTA  SETC '&SYSSTMT'
        COPY COPY1

        MACRO
        MAC
        GBLC &STMTC,&STMTD,&VAR2
        LR 1,1
        COPY COPY2
&STMTC  SETC '&SYSSTMT'
        MACRO
        NESTED
        GBLC &STMTD
&STMTD  SETC '&SYSSTMT'
        MEND
        NESTED

        MEND

&STMTB  SETC '&SYSSTMT'
10      MAC 13
        LR 1,2
&STMTE  SETC '&SYSSTMT'
&STMTF  SETA &SYSSTMT
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

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "STMTA"), "00000004");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "VAR1"), "00000007");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "STMTB"), "00000026");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "VAR2"), "00000031");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "STMTC"), "00000032");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "STMTD"), "00000040");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "STMTE"), "00000046");
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "STMTF"), 47);
}

TEST(system_variable, sysstmt_ainsert)
{
    std::string input = R"(
    MACRO
    MAC_AIN
    AINSERT '       MACRO',BACK
    AINSERT '       MAC',BACK
    AINSERT '       GBLC &&A',BACK
    AINSERT '&&A    SETC ''&&SYSSTMT''',BACK
    AINSERT '       MEND',BACK
    AINSERT '&&B    SETC ''&&SYSSTMT''',BACK
    MEND
    
    GBLC &A
    MAC_AIN
    MAC
    END
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "A"), "00000029");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "B"), "00000027");
}

// TODO uncomment when AINSERT grammer with apostrophes is fixed; Consider moving this to AINSERT test group
//TEST(system_variable, sysstmt_ainsert_02)
//{
//    std::string input = R"(
//    MACRO
//    MAC_AIN
//    AINSERT '       MACRO',BACK
//    AINSERT '       MAC',BACK
//    AINSERT '       GBLC &&A',BACK
//    AINSERT '&&A    SETC ''&SYSSTMT''',BACK
//    AINSERT '       MEND',BACK
//    AINSERT '&&B    SETC ''&SYSSTMT''',BACK
//    MEND
//    
//    GBLC &A
//    MAC_AIN
//    MAC
//    END
//)";
//
//    analyzer a(input);
//    a.analyze();
//    a.collect_diags();
//    EXPECT_EQ(a.diags().size(), (size_t)0);
//
//    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "A"), "00000017");
//    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "B"), "00000019");
//}