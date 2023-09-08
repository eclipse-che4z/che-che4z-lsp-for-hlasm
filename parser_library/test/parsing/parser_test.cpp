/*
 * Copyright (c) 2019 Broadcom.
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
#include "utils/platform.h"

// tests for
// parsing various files
// parsing CA expressions

class library_test : public testing::TestWithParam<std::string>
{
public:
    void SetUp() override
    {
        if (hlasm_plugin::utils::platform::is_web())
            GTEST_SKIP() << "Direct I/O not available in Web mode";

        input = hlasm_plugin::utils::platform::read_file("test/library/input/" + GetParam() + ".in").value();
        holder = std::make_unique<analyzer>(input);
    }

    static constexpr const size_t size_t_zero = static_cast<size_t>(0);

protected:
    std::unique_ptr<analyzer> holder;
    std::string input;
};


INSTANTIATE_TEST_SUITE_P(library_test,
    library_test,
    ::testing::Values(
        // 3 instruction statements
        "simple",
        // 5 instruction statements that have 1,1,1,2 and 4 operands respectively
        "operand",
        // 3 alternative forms of instructions
        "continuation",
        // finding 3 variable symbols in model statement
        "model_statement",
        // simply parse correctly
        "comment",
        // simply parse correctly
        "long_macro",
        "process",
        "macro_model"

        ));

// 3 instruction statements
TEST_P(library_test, syntax_errors)
{
    holder->analyze();
    // no errors found while parsing
    EXPECT_EQ(get_syntax_errors(*holder), size_t_zero);
}
