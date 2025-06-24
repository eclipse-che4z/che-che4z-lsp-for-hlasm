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
#include "context/hlasm_context.h"
#include "instruction_set_version.h"

// clang-format off
std::unordered_map<std::string, const std::set<instruction_set_version>> instruction_compatibility_matrix = {
    { "ADDFRR", { instruction_set_version::ESA, instruction_set_version::XA } },
    { "VACD", { instruction_set_version::ESA, instruction_set_version::XA, instruction_set_version::_370 } },
    { "CLRCH", { instruction_set_version::UNI, instruction_set_version::_370 } },
    { "CLRIO", { instruction_set_version::UNI, instruction_set_version::_370, instruction_set_version::DOS } },
    { "VEVAL", { instruction_set_version::UNI, instruction_set_version::Z17 } },
    { "NNPA", { instruction_set_version::UNI, instruction_set_version::Z17, instruction_set_version::Z16 } },
    { "DFLTCC", { instruction_set_version::UNI, instruction_set_version::Z17, instruction_set_version::Z16, instruction_set_version::Z15 } },
    { "VLER", { instruction_set_version::UNI, instruction_set_version::Z17, instruction_set_version::Z16, instruction_set_version::Z15, instruction_set_version::ESA, instruction_set_version::XA, instruction_set_version::_370 } },
    { "AGH", { instruction_set_version::UNI, instruction_set_version::Z17, instruction_set_version::Z16, instruction_set_version::Z15, instruction_set_version::Z14 } },
    { "CDPT", { instruction_set_version::UNI, instruction_set_version::Z17, instruction_set_version::Z16, instruction_set_version::Z15, instruction_set_version::Z14, instruction_set_version::Z13 } },
    { "VA", { instruction_set_version::UNI, instruction_set_version::Z17, instruction_set_version::Z16, instruction_set_version::Z15, instruction_set_version::Z14, instruction_set_version::Z13, instruction_set_version::ESA, instruction_set_version::XA, instruction_set_version::_370 } },
    { "BPP", { instruction_set_version::UNI, instruction_set_version::Z17, instruction_set_version::Z16, instruction_set_version::Z15, instruction_set_version::Z14, instruction_set_version::Z13, instruction_set_version::Z12 } },
    { "ADTRA", { instruction_set_version::UNI, instruction_set_version::Z17, instruction_set_version::Z16, instruction_set_version::Z15, instruction_set_version::Z14, instruction_set_version::Z13, instruction_set_version::Z12, instruction_set_version::Z11 } },
    { "AGSI", { instruction_set_version::UNI, instruction_set_version::Z17, instruction_set_version::Z16, instruction_set_version::Z15, instruction_set_version::Z14, instruction_set_version::Z13, instruction_set_version::Z12, instruction_set_version::Z11, instruction_set_version::Z10 } },
    { "ADTR", { instruction_set_version::UNI, instruction_set_version::Z17, instruction_set_version::Z16, instruction_set_version::Z15, instruction_set_version::Z14, instruction_set_version::Z13, instruction_set_version::Z12, instruction_set_version::Z11, instruction_set_version::Z10, instruction_set_version::Z9 } },
    { "CDSY", { instruction_set_version::UNI, instruction_set_version::Z17, instruction_set_version::Z16, instruction_set_version::Z15, instruction_set_version::Z14, instruction_set_version::Z13, instruction_set_version::Z12, instruction_set_version::Z11, instruction_set_version::Z10, instruction_set_version::Z9, instruction_set_version::YOP } },
    { "KIMD", { instruction_set_version::UNI, instruction_set_version::Z17, instruction_set_version::Z16, instruction_set_version::Z15, instruction_set_version::Z14, instruction_set_version::Z13, instruction_set_version::Z12, instruction_set_version::Z11, instruction_set_version::Z10, instruction_set_version::Z9, instruction_set_version::YOP } },
    { "AG", { instruction_set_version::UNI, instruction_set_version::Z17, instruction_set_version::Z16, instruction_set_version::Z15, instruction_set_version::Z14, instruction_set_version::Z13, instruction_set_version::Z12, instruction_set_version::Z11, instruction_set_version::Z10, instruction_set_version::Z9, instruction_set_version::YOP, instruction_set_version::ZOP } },
    { "ADB", { instruction_set_version::UNI, instruction_set_version::Z17, instruction_set_version::Z16, instruction_set_version::Z15, instruction_set_version::Z14, instruction_set_version::Z13, instruction_set_version::Z12, instruction_set_version::Z11, instruction_set_version::Z10, instruction_set_version::Z9, instruction_set_version::YOP, instruction_set_version::ZOP, instruction_set_version::ESA } },
    { "BASSM", { instruction_set_version::UNI, instruction_set_version::Z17, instruction_set_version::Z16, instruction_set_version::Z15, instruction_set_version::Z14, instruction_set_version::Z13, instruction_set_version::Z12, instruction_set_version::Z11, instruction_set_version::Z10, instruction_set_version::Z9, instruction_set_version::YOP, instruction_set_version::ZOP, instruction_set_version::ESA, instruction_set_version::XA } },
    { "BAS", { instruction_set_version::UNI, instruction_set_version::Z17, instruction_set_version::Z16, instruction_set_version::Z15, instruction_set_version::Z14, instruction_set_version::Z13, instruction_set_version::Z12, instruction_set_version::Z11, instruction_set_version::Z10, instruction_set_version::Z9, instruction_set_version::YOP, instruction_set_version::ZOP, instruction_set_version::ESA, instruction_set_version::XA, instruction_set_version::_370 } },
    { "A", { instruction_set_version::UNI, instruction_set_version::Z17, instruction_set_version::Z16, instruction_set_version::Z15, instruction_set_version::Z14, instruction_set_version::Z13, instruction_set_version::Z12, instruction_set_version::Z11, instruction_set_version::Z10, instruction_set_version::Z9, instruction_set_version::YOP, instruction_set_version::ZOP, instruction_set_version::ESA, instruction_set_version::XA, instruction_set_version::_370, instruction_set_version::DOS } },
};
// clang-format on

namespace {
struct instruction_sets_compatibility_params
{
    instruction_set_version instr_set;

    static instruction_sets_compatibility_params set_instr_set(instruction_set_version instr_set)
    {
        instruction_sets_compatibility_params params {};
        params.instr_set = instr_set;

        return params;
    }
};

class instruction_sets_fixture : public ::testing::TestWithParam<instruction_sets_compatibility_params>
{};

} // namespace

INSTANTIATE_TEST_SUITE_P(instruction_test,
    instruction_sets_fixture,
    ::testing::Values(instruction_sets_compatibility_params::set_instr_set(instruction_set_version::ZOP),
        instruction_sets_compatibility_params::set_instr_set(instruction_set_version::YOP),
        instruction_sets_compatibility_params::set_instr_set(instruction_set_version::Z9),
        instruction_sets_compatibility_params::set_instr_set(instruction_set_version::Z10),
        instruction_sets_compatibility_params::set_instr_set(instruction_set_version::Z11),
        instruction_sets_compatibility_params::set_instr_set(instruction_set_version::Z12),
        instruction_sets_compatibility_params::set_instr_set(instruction_set_version::Z13),
        instruction_sets_compatibility_params::set_instr_set(instruction_set_version::Z14),
        instruction_sets_compatibility_params::set_instr_set(instruction_set_version::Z15),
        instruction_sets_compatibility_params::set_instr_set(instruction_set_version::Z16),
        instruction_sets_compatibility_params::set_instr_set(instruction_set_version::Z17),
        instruction_sets_compatibility_params::set_instr_set(instruction_set_version::UNI),
        instruction_sets_compatibility_params::set_instr_set(instruction_set_version::DOS),
        instruction_sets_compatibility_params::set_instr_set(instruction_set_version::_370),
        instruction_sets_compatibility_params::set_instr_set(instruction_set_version::XA),
        instruction_sets_compatibility_params::set_instr_set(instruction_set_version::ESA)));

TEST_P(instruction_sets_fixture, instruction_set_loading)
{
    auto instr_set = GetParam().instr_set;
    std::string dummy_input;
    analyzer a(dummy_input, analyzer_options { asm_option { "", "", instr_set } });

    for (const auto& instr : instruction_compatibility_matrix)
    {
        auto id = a.hlasm_ctx().find_id(instr.first);

        if (!instr.second.contains(instr_set))
        {
            EXPECT_TRUE(
                !id.has_value() || !a.hlasm_ctx().find_opcode_mnemo(id.value(), context::opcode_generation::zero))
                << "For instructions: " << instr.first;
        }
        else
        {
            EXPECT_TRUE(id.has_value() && a.hlasm_ctx().find_opcode_mnemo(id.value(), context::opcode_generation::zero))
                << "For instructions: " << instr.first;
        }
    }
}

namespace {
struct test_case
{
    instruction_set_version instr_set;
    int expected_var_value;
};
} // namespace

TEST(instruction_sets_fixture, identical_macro_name_inline_definition)
{
    std::string input = R"(
        MACRO
        SAM31
        GBLA &VAR
&VAR    SETA   1        
        MEND
        
        GBLA &VAR
&VAR    SETA   0    
        SAM31
)";

    test_case cases[] = { { instruction_set_version::_370, 1 }, { instruction_set_version::Z11, 1 } };

    for (const auto& c : cases)
    {
        analyzer a(input, analyzer_options { asm_option { "", "", c.instr_set } });
        a.analyze();
        EXPECT_EQ(a.diags().size(), 0);

        EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "VAR"), c.expected_var_value);
    }
}

TEST(instruction_sets_fixture, identical_macro_name_linked_definition)
{
    std::string input = R"(
        GBLA &VAR
&VAR    SETA   0    
        SAM31
)";

    std::string macro =
        R"( MACRO
        SAM31
        GBLA &VAR
&VAR    SETA   2        
        MEND
)";

    test_case cases[] = { { instruction_set_version::_370, 2 }, { instruction_set_version::Z11, 0 } };

    mock_parse_lib_provider lib_provider { { "SAM31", macro } };

    for (const auto& c : cases)
    {
        analyzer a(input, analyzer_options { asm_option { "", "", c.instr_set }, &lib_provider });
        a.analyze();
        EXPECT_EQ(a.diags().size(), 0);

        EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "VAR"), c.expected_var_value);
    }
}

TEST(instruction_sets_fixture, identical_macro_name_inline_and_linked_definition)
{
    std::string input = R"(
        MACRO
        SAM31
        GBLA &VAR
&VAR    SETA   1        
        MEND

        GBLA &VAR
&VAR    SETA   0    
        SAM31
)";

    std::string macro =
        R"( MACRO
        SAM31
        GBLA &VAR
&VAR    SETA   2        
        MEND
)";

    test_case cases[] = { { instruction_set_version::_370, 1 }, { instruction_set_version::Z11, 1 } };

    mock_parse_lib_provider lib_provider { { "SAM31", macro } };

    for (const auto& c : cases)
    {
        analyzer a(input, analyzer_options { asm_option { "", "", c.instr_set }, &lib_provider });
        a.analyze();
        EXPECT_EQ(a.diags().size(), 0);

        EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "VAR"), c.expected_var_value);
    }
}
