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
#include "parsing/parser_tools.h"
#include "utils/platform.h"

// tests for
// parsing various files
// parsing CA expressions

class library_test : public testing::Test
{
public:
    virtual void setup(std::string param)
    {
        input = hlasm_plugin::utils::platform::read_file("test/library/input/" + param + ".in").value();
        holder = std::make_unique<analyzer>(input);
    }

protected:
    std::unique_ptr<analyzer> holder;
    std::string input;
};

// 3 instruction statements
TEST_F(library_test, simple)
{
    std::string tcase = "simple";

    setup(tcase);

    // compare tokens with output file

    holder->analyze();
    // no errors found while parsing
    ASSERT_EQ(holder->debug_syntax_errors(), size_t_zero);
}

// 5 instruction statements that have 1,1,1,2 and 4 operands respectively
TEST_F(library_test, operand)
{
    std::string tcase = "operand";

    setup(tcase);

    // compare tokens with output file

    holder->analyze();
    // no errors found while parsing
    ASSERT_EQ(holder->debug_syntax_errors(), size_t_zero);
}

// 3 alternative forms of instructions
TEST_F(library_test, continuation)
{
    std::string tcase = "continuation";

    setup(tcase);

    holder->analyze();
    // no errors found while parsing
    ASSERT_EQ(holder->debug_syntax_errors(), size_t_zero);
}

// finding 3 variable symbols in model statement
TEST_F(library_test, model_statement)
{
    std::string tcase = "model_statement";

    setup(tcase);


    holder->analyze();
    // no errors found while parsing
    ASSERT_EQ(holder->debug_syntax_errors(), size_t_zero);
}

// simply parse correctly
TEST_F(library_test, comment)
{
    std::string tcase = "comment";

    setup(tcase);

    // compare tokens with output file

    holder->analyze();
    // no errors found while parsing
    ASSERT_EQ(holder->debug_syntax_errors(), size_t_zero);
}

// simply parse correctly
TEST_F(library_test, empty_string)
{
    std::string es_input = " LR ''";

    analyzer a(es_input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)2);
}

TEST_F(library_test, long_macro)
{
    std::string tcase = "long_macro";

    setup(tcase);

    // compare tokens with output file

    holder->analyze();
    // no errors found while parsing
    ASSERT_EQ(holder->debug_syntax_errors(), size_t_zero);
}

TEST_F(library_test, process_statement)
{
    std::string tcase = "process";

    setup(tcase);

    // compare tokens with output file

    holder->analyze();
    // no errors found while parsing
    ASSERT_EQ(holder->debug_syntax_errors(), size_t_zero);
}

TEST_F(library_test, macro_model)
{
    std::string tcase = "macro_model";

    setup(tcase);

    // compare tokens with output file

    holder->analyze();
    // no errors found while parsing
    ASSERT_EQ(holder->debug_syntax_errors(), size_t_zero);
}
