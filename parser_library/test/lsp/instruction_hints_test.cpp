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

#include <format>

#include "gtest/gtest.h"

#include "completion_item.h"
#include "context/instruction.h"
#include "instruction_set_version.h"
#include "lsp/instruction_completions.h"
#include "utils/concat.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lsp;
using hlasm_plugin::utils::concat;

struct instr_hint_test
{
    std::string_view name;
    std::string_view operands;
    std::string_view snippet;
    std::string_view substitution;
    instruction_set_version version = instruction_set_version::UNI;
};

struct instr_hint_test_fixture : ::testing::TestWithParam<instr_hint_test>
{};

INSTANTIATE_TEST_SUITE_P(instr_hint_test,
    instr_hint_test_fixture,
    ::testing::Values(instr_hint_test { "LARL", "R1,RI32S2", "${3:}${1:R1},${2:RI32S2}", "" },
        instr_hint_test { "JE", "RI16S2", "${2:}${1:RI16S2}", "8,RI16S2" },
        instr_hint_test { "VNOT", "V1,V2", "${3:}${1:V1},${2:V2}", "V1,V2,V2" },
        instr_hint_test {
            "RISBG", "R1,R2,I8U3,I8U4[,I8U5]", "${6:}${1:R1},${2:R2},${3:I8U3},${4:I8U4}${5: [,I8U5]}", "" },
        instr_hint_test { "RISBGZ",
            "R1,R2,I8U3,I8U4[,I8U5]",
            "${6:}${1:R1},${2:R2},${3:I8U3},${4:I8U4}${5: [,I8U5]}",
            "R1,R2,I8U3,X'80'|I8U4[,I8U5]" },
        instr_hint_test { "VONE", "V1", "${2:}${1:V1}", "V1,X'FFFF'" },
        instr_hint_test { "IPTE", "R1,R2[,R3[,M4]]", "${4:}${1:R1},${2:R2}${3: [,R3[,M4]]}", "" },
        instr_hint_test { "WCLGDB", "V1,V2,M4,M5", "${5:}${1:V1},${2:V2},${3:M4},${4:M5}", "V1,V2,3,8|M4,M5" },
        instr_hint_test { "CIJNE", "R1,I8S2,RI16S4", "${4:}${1:R1},${2:I8S2},${3:RI16S4}", "R1,I8S2,6,RI16S4" },
        instr_hint_test { "VTP", "V1", "${2:}${1:V1}", "", instruction_set_version::Z14 },
        instr_hint_test { "VTP", "V1[,I16U2]", "${3:}${1:V1}${2: [,I16U2]}", "" }),
    [](::testing::TestParamInfo<instr_hint_test> p) {
        return std::format("{}_{}", p.param.name, (int)p.param.version);
    });

TEST_P(instr_hint_test_fixture, instructions)
{
    auto [name, operands, snippet, substitution, version] = GetParam();

    const auto it = std::ranges::find_if(instruction_completion_items,
        [&](const auto& e) { return e.first.label == name && context::instruction_available(e.second, version); });
    ASSERT_NE(it, instruction_completion_items.end());

    const auto& item = it->first;

    EXPECT_TRUE(item.snippet);
    EXPECT_EQ(item.insert_text, concat(name, " ", snippet));
    EXPECT_EQ(item.detail, operands);

    if (substitution.empty())
        EXPECT_EQ(item.documentation.find("Substituted operands:"), std::string::npos);
    else
        EXPECT_NE(item.documentation.find(concat("Substituted operands: ", substitution)), std::string::npos);
}
