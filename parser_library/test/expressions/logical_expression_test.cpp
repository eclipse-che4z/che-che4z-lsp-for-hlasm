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

// test for
// arithmetic SETB expressions

#define SETBEQ(X, Y)                                                                                                   \
    EXPECT_EQ(a.hlasm_ctx()                                                                                            \
                  .get_var_sym(a.hlasm_ctx().ids().add(X))                                                             \
                  ->access_set_symbol_base()                                                                           \
                  ->access_set_symbol<B_t>()                                                                           \
                  ->get_value(),                                                                                       \
        Y)

TEST(logical_expressions, valid_assignment)
{
    std::string input =
        R"(
&A1 SETB 1
&A2 SETB 0
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETBEQ("A1", 1);
    SETBEQ("A2", 0);
}

TEST(logical_expressions, empty_string_conversion)
{
    std::string input = R"(
&C1 SETC ''
&B1 SETB (&C1)
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), 0);

    SETBEQ("B1", false);
}

TEST(logical_expressions, invalid_assignment)
{
    std::string input =
        R"(
&A1 SETB (C'D')
&A2 SETB (&A2)
&A3 SETB (10)
&A4 SETB (-1)
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)4);
}

TEST(logical_expressions, invalid_expression)
{
    std::string input =
        R"(
NOT EQU 1
&A1 SETB (AND AND 1)
&A2 SETB (1 AND NOT)
&C3 SETB (1 OR AND 1)
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)3);
}

TEST(logical_expressions, valid_expression)
{
    std::string input =
        R"(
AND EQU 1
&A1 SETB (AND AND 1)
&A2 SETB (1 OR NOT 0)
&A3 SETB (1 OR AND)
&A4 SETB (NOT 1 AND NOT NOT 1)
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETBEQ("A1", 1);
    SETBEQ("A2", 1);
    SETBEQ("A3", 1);
    SETBEQ("A4", 0);
}

TEST(logical_expressions, valid_relational_expression)
{
    std::string input =
        R"(
AND EQU 1
&A1 SETB ('a' EQ 'A')
&A2 SETB ('a' LT 'A')
&A3 SETB ('abc' GT 'b')
&A4 SETB ('A' EQ UPPER('a'))
&A5 SETB (D2A('10') EQ 10)
&A6 SETB ('CCCC' EQ (4)'C')
&A7 SETB ((4)'C' EQ 'CCCC')
&A8 SETB ('A' EQ BYTE(X'C1'))
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETBEQ("A1", 0);
    SETBEQ("A2", 1);
    SETBEQ("A3", 1);
    SETBEQ("A4", 1);
    SETBEQ("A5", 1);
    SETBEQ("A6", 1);
    SETBEQ("A7", 1);
    SETBEQ("A8", 1);
}

TEST(logical_expressions, invalid_relational_expression)
{
    std::string input =
        R"(
&A1 SETB (UPPER('a') EQ 'A')
&A2 SETB (13 LT 'A')
&A3 SETB ('a' LT 12)
&A4 SETB (BYTE(X'C1') EQ 'A')
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)6);
}

TEST(logical_expressions, priority)
{
    std::string input =
        R"(
&A1 SETB (1 OR 1 EQ 0 OR 0)  
&A2 SETB (1 AND NOT 0)  
&A3 SETB (1 XOR 1 AND 0)  
&A4 SETB (1 XOR 0 OR 1) 0
&A5 SETB (1 AND 0 OR 1 AND 1)  
&A6 SETB (NOT 0 AND NOT 0 AND 50 GT 126+4)
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETBEQ("A1", 1);
    SETBEQ("A2", 1);
    SETBEQ("A3", 1);
    SETBEQ("A4", 0);
    SETBEQ("A5", 1);
    SETBEQ("A6", 0);
}

TEST(logical_expressions, arithmetic_logical_clash)
{
    std::string input =
        R"(
&A1 SETB (NOT 0 AND NOT 0 AND 50 GT 126+4)
&A2 SETB (1 AND 10 EQ (10 OR 2))
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETBEQ("A1", 0);
    SETBEQ("A2", 1);
}

TEST(logical_expressions, no_spaces)
{
    std::string input =
        R"(
&A1 SETB (NOT(0))
&A2 SETB ((1)AND(1))
&A3 SETB ('3'ge'2')
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETBEQ("A1", 1);
    SETBEQ("A2", 1);
    SETBEQ("A3", 1);
}

TEST(logical_expressions, bad_number_of_operands)
{
    std::string input =
        R"(
&A SETA FIND('A','B','C')
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
    EXPECT_EQ(a.diags().front().code, "CE006");
}

TEST(logical_expressions, is_functions)
{
    std::string input =
        R"(
&A SETB (ISBIN('0000'))
&B SETB (ISBIN('a0a0'))
&C SETB (ISHEX('abcd'))
&D SETB (ISHEX('hhhh'))
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETBEQ("A", 1);
    SETBEQ("B", 0);
    SETBEQ("C", 1);
    SETBEQ("D", 0);
}

TEST(logical_expressions, parenthesis_around_expressions)
{
    std::string input =
        R"(
&A SETB 0
&B SETB &A
&B SETB (&A)
&C SETB ISBIN('0000')
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    const auto& diags = a.diags();
    EXPECT_EQ(diags.size(), (size_t)2);
    EXPECT_TRUE(std::all_of(diags.begin(), diags.end(), [](const auto& d) { return d.code == "CE016"; }));
}
