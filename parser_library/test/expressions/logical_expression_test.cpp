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
&VAR SETA -6
&A1  SETB 1
&A2  SETB 0
&A3  SETB (&A3)
&A4  SETB (10)
&A5  SETB (&VAR)
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());

    SETBEQ("A1", 1);
    SETBEQ("A2", 0);
    SETBEQ("A3", 0);
    SETBEQ("A4", 1);
    SETBEQ("A5", 1);
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
&A1 SETB 10
&A2 SETB (-1)
&A3 SETB (+1)
&A4 SETB ((-1))
&A5 SETB ((+1))
&A6 SETB (0+1)
&A7 SETB (1-1)
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "CE016", "CE004", "CE004", "CE004", "CE004", "CE004", "CE004" }));
}

TEST(logical_expressions, invalid_expression)
{
    std::string input =
        R"(
NOT EQU  1
&A1 SETB (AND AND 1)
&A2 SETB (1 AND NOT)
&A3 SETB (1 OR AND 1)
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "CE012", "CE003", "CE003" }));
}
TEST(logical_expressions, valid_expression)
{
    std::string input =
        R"(
AND EQU  1
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
&A1  SETB ('a' EQ 'A')
&A2  SETB ('a' LT 'A')
&A3  SETB ('abc' GT 'b')
&A4  SETB ('A' EQ UPPER('a'))
&A5  SETB (D2A('10') EQ 10)
&A6  SETB ('CCCC' EQ (4)'C')
&A7  SETB ((4)'C' EQ 'CCCC')
&A8  SETB ('A' EQ BYTE(X'C1'))
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
    EXPECT_TRUE(matches_message_codes(a.diags(), { "CE004", "CE004", "CE004", "CE004" }));
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
&A7 SETB (NOT 0 OR NOT 0)
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
    SETBEQ("A7", 1);
}

TEST(logical_expressions, arithmetic_logical_clash)
{
    std::string input =
        R"(
&A1 SETB (NOT 0 AND NOT 0 AND 50 GT 126+4)
&A2 SETB (NOT 0 AND NOT 0 OR 50 GT 126+4)
&A3 SETB (1 AND 10 EQ (10 OR 2))
&A4 SETB (1 AND (10 EQ (10 OR 2)))
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETBEQ("A1", 0);
    SETBEQ("A2", 1);
    SETBEQ("A3", 0);
    SETBEQ("A4", 0);
}

TEST(logical_expressions, signs_in_arithmetic_expressions)
{
    std::string input =
        R"(
&A1 SETB ((+1 EQ +1) AND 1)
&A2 SETB (+1 EQ +1)
&A3 SETB (-1 EQ +1)
&A4 SETB ((+1) EQ 1)
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_EQ(a.diags().size(), (size_t)0);
    SETBEQ("A1", 1);
    SETBEQ("A2", 1);
    SETBEQ("A3", 0);
    SETBEQ("A4", 1);
}


TEST(logical_expressions, signs_in_logical_expressions)
{
    std::string input =
        R"(
&A1 SETB (5 AND -5)
&A2 SETB (5 AND +5)
&A3 SETB ((+1 EQ +1) AND +1)
&A4 SETB (NOT -5)
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "CE004", "CE004", "CE004", "CE004" }));
}

TEST(logical_expressions, no_parenthesis)
{
    std::string input =
        R"(
&A1 SETB NOT 0
&A2 SETB NOT NOT NOT 0
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "CE016", "CE016" }));
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

TEST(logical_expressions, not_operator_valid)
{
    std::string input =
        R"(
&F    SETA 5
&NEGF SETA -5
&B1   SETB (NOT &F)
&B2   SETB (NOT &NEGF)
&B3   SETB (NOT 6)
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "B1"), false);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "B2"), false);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "B3"), false);
}

TEST(logical_expressions, not_operator_valid_logical_expr)
{
    std::string input =
        R"(
&VAR  SETA   -7
&B1   SETB   (6 AND NOT &VAR)
&B2   SETB   (6 AND (NOT 7))
&B3   SETB   (1 AND NOT 0)
&B4   SETB   (NOT 0 AND NOT 0)
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "B1"), false);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "B2"), false);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "B3"), true);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "B4"), true);
}

TEST(logical_expressions, not_operator_valid_relational_expr)
{
    std::string input =
        R"(
&B1 SETB (-1 EQ NOT 0)
&B2 SETB (NOT -1 EQ +1)
&B3 SETB ((NOT 1) EQ -2)
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "B1"), false);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "B2"), true);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "B3"), false);
}

TEST(logical_expressions, operator_precedence)
{
    for (const auto& [args, expected] : std::initializer_list<std::pair<std::string, bool>> {
             { "0,0", true },
             { "0,1", true },
             { "1,0", false },
             { "1,1", true },
         })
    {
        std::string input =
            R"(
        MACRO
        TEST    &A,&B 
        AIF     (&A EQ 0 OR &A EQ 1 AND &B EQ 1).END    
        MNOTE   8,'ERROR'
.END    ANOP
        MEND

        TEST    )"
            + args;
        analyzer a(input);
        a.analyze();
        a.collect_diags();

        EXPECT_EQ(a.diags().empty(), expected) << args;
    }
}

TEST(logical_expressions, operator_precedence_not)
{
    std::string input =
        R"(
&B1   SETB (0 OR NOT 2 GT -4)
&B2   SETB   ((4 AND NOT -5) EQ 4)
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "B1"), false);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "B2"), false);
}
TEST(logical_expressions, relational_operator_eq)
{
    for (const auto& [input, res] : {
             std::pair<std::string, bool> { "&A SETB (0 EQ 0)", true },
             std::pair<std::string, bool> { "&A SETB (0 EQ 1)", false },
             std::pair<std::string, bool> { "&A SETB (1 EQ 0)", false },
             std::pair<std::string, bool> { "&A SETB (1 EQ 1)", true },
             std::pair<std::string, bool> { "&A SETB (0 EQ NOT 0)", false },
             std::pair<std::string, bool> { "&A SETB (0 EQ NOT 1)", true },
             std::pair<std::string, bool> { "&A SETB (1 EQ NOT 0)", true },
             std::pair<std::string, bool> { "&A SETB (1 EQ NOT 1)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 0 EQ 0)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 0 EQ 1)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 1 EQ 0)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 1 EQ 1)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 0 EQ NOT 0)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 0 EQ NOT 1)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 1 EQ NOT 0)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 1 EQ NOT 1)", true },
         })
    {
        analyzer a(input);
        a.analyze();

        a.collect_diags();
        EXPECT_TRUE(a.diags().empty()) << input;
        EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "A"), res) << input;
    }
}

TEST(logical_expressions, relational_operator_ne)
{
    for (const auto& [input, res] : {
             std::pair<std::string, bool> { "&A SETB (0 NE 0)", false },
             std::pair<std::string, bool> { "&A SETB (0 NE 1)", true },
             std::pair<std::string, bool> { "&A SETB (1 NE 0)", true },
             std::pair<std::string, bool> { "&A SETB (1 NE 1)", false },
             std::pair<std::string, bool> { "&A SETB (0 NE NOT 0)", true },
             std::pair<std::string, bool> { "&A SETB (0 NE NOT 1)", false },
             std::pair<std::string, bool> { "&A SETB (1 NE NOT 0)", false },
             std::pair<std::string, bool> { "&A SETB (1 NE NOT 1)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 0 NE 0)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 0 NE 1)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 1 NE 0)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 1 NE 1)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 0 NE NOT 0)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 0 NE NOT 1)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 1 NE NOT 0)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 1 NE NOT 1)", false },
         })
    {
        analyzer a(input);
        a.analyze();

        a.collect_diags();
        EXPECT_TRUE(a.diags().empty()) << input;
        EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "A"), res) << input;
    }
}

TEST(logical_expressions, relational_operator_le)
{
    for (const auto& [input, res] : {
             std::pair<std::string, bool> { "&A SETB (0 LE 0)", true },
             std::pair<std::string, bool> { "&A SETB (0 LE 1)", true },
             std::pair<std::string, bool> { "&A SETB (1 LE 0)", false },
             std::pair<std::string, bool> { "&A SETB (1 LE 1)", true },
             std::pair<std::string, bool> { "&A SETB (0 LE NOT 0)", true },
             std::pair<std::string, bool> { "&A SETB (0 LE NOT 1)", true },
             std::pair<std::string, bool> { "&A SETB (1 LE NOT 0)", true },
             std::pair<std::string, bool> { "&A SETB (1 LE NOT 1)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 0 LE 0)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 0 LE 1)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 1 LE 0)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 1 LE 1)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 0 LE NOT 0)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 0 LE NOT 1)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 1 LE NOT 0)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 1 LE NOT 1)", true },
         })
    {
        analyzer a(input);
        a.analyze();

        a.collect_diags();
        EXPECT_TRUE(a.diags().empty()) << input;
        EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "A"), res) << input;
    }
}

TEST(logical_expressions, relational_operator_lt)
{
    for (const auto& [input, res] : {
             std::pair<std::string, bool> { "&A SETB (0 LT 0)", false },
             std::pair<std::string, bool> { "&A SETB (0 LT 1)", true },
             std::pair<std::string, bool> { "&A SETB (1 LT 0)", false },
             std::pair<std::string, bool> { "&A SETB (1 LT 1)", false },
             std::pair<std::string, bool> { "&A SETB (0 LT NOT 0)", true },
             std::pair<std::string, bool> { "&A SETB (0 LT NOT 1)", false },
             std::pair<std::string, bool> { "&A SETB (1 LT NOT 0)", false },
             std::pair<std::string, bool> { "&A SETB (1 LT NOT 1)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 0 LT 0)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 0 LT 1)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 1 LT 0)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 1 LT 1)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 0 LT NOT 0)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 0 LT NOT 1)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 1 LT NOT 0)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 1 LT NOT 1)", true },
         })
    {
        analyzer a(input);
        a.analyze();

        a.collect_diags();
        EXPECT_TRUE(a.diags().empty()) << input;
        EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "A"), res) << input;
    }
}

TEST(logical_expressions, relational_operator_ge)
{
    for (const auto& [input, res] : {
             std::pair<std::string, bool> { "&A SETB (0 GE 0)", true },
             std::pair<std::string, bool> { "&A SETB (0 GE 1)", false },
             std::pair<std::string, bool> { "&A SETB (1 GE 0)", true },
             std::pair<std::string, bool> { "&A SETB (1 GE 1)", true },
             std::pair<std::string, bool> { "&A SETB (0 GE NOT 0)", false },
             std::pair<std::string, bool> { "&A SETB (0 GE NOT 1)", true },
             std::pair<std::string, bool> { "&A SETB (1 GE NOT 0)", true },
             std::pair<std::string, bool> { "&A SETB (1 GE NOT 1)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 0 GE 0)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 0 GE 1)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 1 GE 0)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 1 GE 1)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 0 GE NOT 0)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 0 GE NOT 1)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 1 GE NOT 0)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 1 GE NOT 1)", false },
         })
    {
        analyzer a(input);
        a.analyze();

        a.collect_diags();
        EXPECT_TRUE(a.diags().empty()) << input;
        EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "A"), res) << input;
    }
}

TEST(logical_expressions, relational_operator_gt)
{
    for (const auto& [input, res] : {
             std::pair<std::string, bool> { "&A SETB (0 GT 0)", false },
             std::pair<std::string, bool> { "&A SETB (0 GT 1)", false },
             std::pair<std::string, bool> { "&A SETB (1 GT 0)", true },
             std::pair<std::string, bool> { "&A SETB (1 GT 1)", false },
             std::pair<std::string, bool> { "&A SETB (0 GT NOT 0)", false },
             std::pair<std::string, bool> { "&A SETB (0 GT NOT 1)", false },
             std::pair<std::string, bool> { "&A SETB (1 GT NOT 0)", false },
             std::pair<std::string, bool> { "&A SETB (1 GT NOT 1)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 0 GT 0)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 0 GT 1)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 1 GT 0)", false },
             std::pair<std::string, bool> { "&A SETB (NOT 1 GT 1)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 0 GT NOT 0)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 0 GT NOT 1)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 1 GT NOT 0)", true },
             std::pair<std::string, bool> { "&A SETB (NOT 1 GT NOT 1)", false },
         })
    {
        analyzer a(input);
        a.analyze();

        a.collect_diags();
        EXPECT_TRUE(a.diags().empty()) << input;
        EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "A"), res) << input;
    }
}

TEST(logical_expressions, symbol_after_substitution)
{
    std::string input =
        R"(
AAA EQU 1
&T SETC 'AAA'
&B SETB (&T EQ &T)
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "B"), true);
}

TEST(logical_expressions, type_mismatch_1)
{
    std::string input =
        R"(
AAA EQU 1
&T SETC 'AAA'
&B SETB (&T EQ '&T')
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "CE004" }));
}

TEST(logical_expressions, type_mismatch_2)
{
    std::string input =
        R"(
AAA EQU 1
&T SETC 'AAA'
&B SETB ('&T' EQ &T)
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "CE017" }));
}

TEST(logical_expressions, simple_string_equality)
{
    std::string input =
        R"(
&T SETC 'AAA'
&B SETB ('&T' EQ '&T')
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "B"), true);
}

TEST(logical_expressions, logical_expr_and_string_function)
{
    std::string input =
        R"(
&A1   SETB   (6 AND DCLEN('hey'))
&A2   SETB   (DCLEN('hey') AND 6)
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "A1"), true);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "A2"), true);
}
