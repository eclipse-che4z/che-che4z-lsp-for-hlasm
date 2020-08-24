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
// arithmetic SETA expressions

#define SETAEQ(X, Y)                                                                                                   \
    EXPECT_EQ(a.context()                                                                                              \
                  .get_var_sym(a.context().ids().add(X))                                                               \
                  ->access_set_symbol_base()                                                                           \
                  ->access_set_symbol<A_t>()                                                                           \
                  ->get_value(),                                                                                       \
        Y)

TEST(arithmetic_expressions, valid_self_defining_term)
{
    std::string input =
        R"(
&A1 SETA 1
&A2 SETA C'D'
&C3 SETC 'C''A'''
&A3 SETA &C3+&C3
&A4 SETA &A4
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETAEQ("A1", 1);
    SETAEQ("A2", 196);
    SETAEQ("A3", 386);
    SETAEQ("A4", 0);
}

TEST(arithmetic_expressions, invalid_self_defining_term)
{
    std::string input =
        R"(
&C1 SETC 'D'
&A1 SETA C'&C1'
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(arithmetic_expressions, substitution_to_character_expression)
{
    std::string input =
        R"(
&A1 SETA 10
&C1 SETC '5-10*&A1'
&A2 SETA -10
&C2 SETC '5-10*&A2'
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(a.context()
                  .get_var_sym(a.context().ids().add("C1"))
                  ->access_set_symbol_base()
                  ->access_set_symbol<C_t>()
                  ->get_value(),
        "5-10*10");

    EXPECT_EQ(a.context()
                  .get_var_sym(a.context().ids().add("C2"))
                  ->access_set_symbol_base()
                  ->access_set_symbol<C_t>()
                  ->get_value(),
        "5-10*10");
}

TEST(arithmetic_expressions, subscript_use)
{
    std::string input =
        R"(
&A1(10) SETA 10
&A2 SETA &A1(10 OR 2)
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(arithmetic_expressions, unary_operators)
{
    std::string input =
        R"(
&A SETA 6
&B SETA 4
&C SETA +++&A*---&B
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETAEQ("C", -24);
}

TEST(arithmetic_expressions, binary_space_separated_operator)
{
    std::string input =
        R"(
&A SETA ('ABC' INDEX 'B')
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETAEQ("A", 2);
}

// requires proper lexer token that recognises number with minus sign
#if 0
TEST(arithmetic_expressions, limits)
{
    std::string input =
        R"(
&A SETA 2147483647
&B SETA -2147483648
&C SETA &A+1
&D SETA &B-1
&E SETC '&B'
&F SETA &E
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)3);
}
#endif

TEST(arithmetic_expressions, division)
{
    std::string input =
        R"(
&A(1) SETA 0,1,2
&B1 SETA &A(1)/2
&B2 SETA &A(2)/2
&B3 SETA &A(3)/2
&B4 SETA &A(3)/0
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETAEQ("B1", 0);
    SETAEQ("B2", 0);
    SETAEQ("B3", 1);
    SETAEQ("B4", 0);
}

TEST(arithmetic_expressions, operator_priorities)
{
    std::string input =
        R"(
&A SETA 3+-4
&B SETA 3+4*2
&C SETA (10 OR 1+1)
&D SETA (10 SLL 10 AND 2)
&E SETA 4-2+5
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETAEQ("A", -1);
    SETAEQ("B", 11);
    SETAEQ("C", 10);
    SETAEQ("D", 40);
    SETAEQ("E", 7);
}

TEST(arithmetic_expressions, invalid_operator)
{
    std::string input =
        R"(
&A SETA (1 AND 1 EQ 2)
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
    EXPECT_EQ(a.diags().front().code, "CE002");
}

TEST(character_expresssion, illegal_dupl_factor)
{
    std::string input =
        R"(
&A SETA (1)FIND('A','B')
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
    EXPECT_EQ(a.diags().front().code, "CE005");
}
