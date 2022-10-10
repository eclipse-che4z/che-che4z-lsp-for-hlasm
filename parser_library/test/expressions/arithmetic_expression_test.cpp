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
    EXPECT_EQ(a.hlasm_ctx()                                                                                            \
                  .get_var_sym(a.hlasm_ctx().ids().add(X))                                                             \
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
&A5 SETA 0000000000
&A6 SETA -0000000000
&A7 SETA X'00000000'
&A8 SETA -X'00000000'
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETAEQ("A1", 1);
    SETAEQ("A2", 196);
    SETAEQ("A3", 386);
    SETAEQ("A4", 0);
    SETAEQ("A5", 0);
    SETAEQ("A6", 0);
    SETAEQ("A7", 0);
    SETAEQ("A8", 0);
}

TEST(arithmetic_expressions, valid_expressions)
{
    std::string input =
        R"(
&A1 SETA (+1)
&A2 SETA (-1)
&A3 SETA (1+0)
&A4 SETA (1-1)
&A5 SETA (NOT(2))
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETAEQ("A1", 1);
    SETAEQ("A2", -1);
    SETAEQ("A3", 1);
    SETAEQ("A4", 0);
    SETAEQ("A5", -3);
}

TEST(arithmetic_expressions, empty_string_conversion)
{
    std::string input = R"(
&C1 SETC ''
&A1 SETA &C1
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETAEQ("A1", 0);
}

TEST(arithmetic_expressions, invalid_self_defining_term)
{
    std::string input =
        R"(
&C1 SETC 'D'
&A1 SETA C'&C1'
&A2 SETA CA'A'
&A3 SETA 00000000000
&A4 SETA -00000000000
&A5 SETA X'000000000'
&A6 SETA X'0000000000'
&A7 SETA -X'000000000'
&A8 SETA -X'0000000000'
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(
        a.diags(), { "S0002", "S0002", "CE012", "CE012", "CE007", "CE007", "CE007", "CE007", "CE007", "CE007" }));
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

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C1"), "5-10*10");

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C2"), "5-10*10");
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

    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002", "E013" }));
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
* &F SETA 1+NOT+NOT -5 AND 5
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
    // SETAEQ("F", -4); // TODO Resolve when CA expression parsing is extended
}

TEST(arithmetic_expressions, operator_priorities_invalid)
{
    std::string input =
        R"(
&A SETA ('A' INDEX 'A' AND 'A' INDEX 'A')
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_NE(a.diags().size(), (size_t)0);
}

TEST(arithmetic_expressions, not_operator)
{
    std::string input =
        R"(
&VAR  SETA -7
&A1 SETA NOT 80
&A2 SETA (NOT 87)
&A3 SETA (6 AND NOT &VAR)
&A4 SETA NoT NOT NOT (6 AND 2)
&A5 SETA not NOT NOT -5    REMARK
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETAEQ("A1", -81);
    SETAEQ("A2", -88);
    SETAEQ("A3", 6);
    SETAEQ("A4", -3);
    SETAEQ("A5", 4);
}

TEST(arithmetic_expressions, not_operator_precedence)
{
    std::string input =
        R"(
&A1   SETA   (-1 AND NOT 0 AND -1)
&A2   SETA   (-1 AND (NOT 0) AND -1)
&A3   SETA   (NOT 0 AND NOT 0)
&A4   SETA   ((NOT 0) AND (NOT 0))
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A1"), -1);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A2"), -1);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A3"), -1);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A4"), -1);
}

TEST(arithmetic_expressions, not_operator_continuation)
{
    std::string input =
        R"(
&A       SETA NOT&A
* TODO Grammar for NOT needs to be adjusted
*&A       SETA                                                         NX
               OT&A                                                 
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
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

TEST(arithmetic_expressions, illegal_dupl_factor)
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


TEST(arithmetic_expressions, multiple_operand_with_spaces)
{
    std::string input =
        R"(
&A1 SETA FIND( 'A','B' )
&A2 SETA FIND( 'A','B')
&A3 SETA FIND('A','B' )
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002", "S0002", "S0002", "CE012", "CE012" }));
}

TEST(arithmetic_expressions, conversion_from_binary)
{
    std::string input =
        R"(
&A SETA ISBIN('0000')
&B SETA ISBIN('a0a0')
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETAEQ("A", 1);
    SETAEQ("B", 0);
}

TEST(arithmetic_expressions, dots)
{
    for (const auto& [input, ok] : {
             std::pair<std::string, bool> { "&A SETA &A", true },
             std::pair<std::string, bool> { "&A SETA NOT &A", true },
             std::pair<std::string, bool> { "&A. SETA &A", false },
             std::pair<std::string, bool> { "&A SETA &A.", false },
             std::pair<std::string, bool> { "&A. SETA &A.", false },
         })
    {
        analyzer a(input);
        a.analyze();

        a.collect_diags();
        ASSERT_EQ(a.diags().empty(), ok);
    }
}

TEST(arithmetic_expressions, limits)
{
    std::string input =
        R"(
&A SETA 2147483647
&B SETA -2147483648
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), SET_t(2147483647));
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "B"), SET_t(static_cast<A_t>(-2147483648)));
}

TEST(arithmetic_expressions, bit_shift)
{
    std::string input =
        R"(
&A SETA (5 SLA 1)
&B SETA (5 SLA 1 SRA 1)
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 10);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "B"), 5);
}

TEST(arithmetic_expressions, subscript_evaluation)
{
    std::string input =
        R"(
&A    SETA 2                   
&B    SETA 3                   
&C    SETA (&A AND &B)
&L(1) SETB 0,1              
&X    SETA (&L((&A AND &B)))
&Y    SETA (&L(&C))         
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "X"), 1);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "Y"), 1);
}

TEST(arithmetic_expressions, subscripted_concat_evaluation)
{
    std::string input =
        R"(
&A    SETA 2
&B    SETA 3
&L(1) SETB 1,0,1
&X    SETA C2A('&L((&A OR &B))'.'&L((&A AND &B))')
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "X"), 61936); // C2A('10') = 61936
}

TEST(arithmetic_expressions, different_var_types)
{
    std::string input =
        R"(
&C SETC 'XYZ'
&A SETA '&C'((0 OR 1),1)
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "CE004" }));
}