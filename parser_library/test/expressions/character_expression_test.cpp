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
// arithmetic SETC expressions

#define SETCEQ(X, Y)                                                                                                   \
    EXPECT_EQ(a.hlasm_ctx()                                                                                            \
                  .get_var_sym(context::id_index(X))                                                                   \
                  ->access_set_symbol_base()                                                                           \
                  ->access_set_symbol<C_t>()                                                                           \
                  ->get_value(),                                                                                       \
        Y)

TEST(character_expression, operator_priority)
{
    std::string input =
        R"(
&C1 SETC 'ABC'.(3)'ABCDEF'(4,3)
&C2 SETC 'ABC'.(NOT -3)'ABCDEF'(NOT -5,NOT -4)
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETCEQ("C1", "ABCDEFDEFDEF");
    SETCEQ("C2", "ABCDEFDEF");
}

TEST(character_expression, substring_notation)
{
    std::string input =
        R"(
&C1 SETC 'ABC'(1,3)
&C2 SETC '&C1'(1,2).'DEF'
&C3 SETC ''(0,0)
&C4 SETC 'XYZ'(2,*)
&C5 SETC 'XYZ'(1,0)
&C6 SETC (2)UPPER('x')
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETCEQ("C1", "ABC");
    SETCEQ("C2", "ABDEF");
    SETCEQ("C3", "");
    SETCEQ("C4", "YZ");
    SETCEQ("C5", "");
    SETCEQ("C6", "XX");
}

TEST(character_expression, invalid_substring_notation)
{
    std::string input =
        R"(
&C SETC 'ABC'(0,1)
&C SETC 'ABCDE'(7,3)
&C SETC 'ABCDE'(3,-2)
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)3);
}

/*TODO uncomment when assembler options will be implemented
TEST(character_expression, exceeds_warning)
{
    std::string input =
        R"(
&C SETC 'ABC'(2,3)
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
    EXPECT_EQ(a.diags().front().severity, diagnostic_severity::warning);
}*/

TEST(character_expression, invalid_string)
{
    std::string input =
        R"(
&C SETC '&'
&C SETC (5000)'A'
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0008", "CE004", "CE011" })); // CE004 is artifact of S0008
}

TEST(character_expression, escaping)
{
    std::string input =
        R"(
&C1 SETC 'L''SYMBOL'
&C2 SETC '&&'(1,1)
&C3 SETC 'HALF&&'
&C4 SETC '&C1..S'
&DOT SETC '.'
&C5 SETC 'A&DOT.&DOT'
&C6 SETC '&C2.A'
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETCEQ("C1", "L'SYMBOL");
    SETCEQ("C2", "&");
    SETCEQ("C3", "HALF&&");
    SETCEQ("C4", "L'SYMBOL.S");
    SETCEQ("C5", "A..");
    SETCEQ("C6", "&A");
}

TEST(character_expression, single_operand_with_spaces)
{
    std::string input =
        R"(
&C1 SETC UPPER( 'A' )
&C2 SETC UPPER( 'A')
&C3 SETC UPPER('A' )
&C4 SETC UPPER('&C1') 
&C5 SETC (UPPER '&C1')
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);

    SETCEQ("C1", "A");
    SETCEQ("C2", "A");
    SETCEQ("C3", "A");
    SETCEQ("C4", "A");
    SETCEQ("C5", "A");
}

TEST(character_expression, single_operand_fail)
{
    for (std::string input : {
             "&C SETC UPPER(&C)",
             "&C SETC (UPPER &C)",
         })
    {
        analyzer a(input);
        a.analyze();

        a.collect_diags();
        EXPECT_FALSE(a.diags().empty());
    }
}

TEST(character_expression, zero_length_substring)
{
    std::string input = R"(
     LCLC &EMPTY
&C1  SETC '&EMPTY'(0,0)
&C2  SETC '&EMPTY'(1,0)
&C3  SETC '&EMPTY'(2,0)
&C4  SETC 'ABCDE'(6,*)
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(character_expression, dots)
{
    for (const auto& [input, ok] : {
             std::pair<std::string, bool> { "&C SETC &C", false },
             std::pair<std::string, bool> { "&C. SETC &C", false },
             std::pair<std::string, bool> { "&C SETC &C.", false },
             std::pair<std::string, bool> { "&C. SETC &C.", false },
             std::pair<std::string, bool> { "&C SETC T'&C", true },
             std::pair<std::string, bool> { "&C SETC T'&C.", true },
         })
    {
        analyzer a(input);
        a.analyze();

        a.collect_diags();
        ASSERT_EQ(a.diags().empty(), ok);
    }
}

TEST(character_expression, valid_subscript_expression)
{
    std::string input =
        R"(
&A SETC 'XYZ'
&X SETC '&A'((0 OR 1),1).'&A'((3 AND 7),1)
&Y SETC '&A'(1,(NOT -2))
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "X"), "XZ");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "Y"), "X");
}

TEST(character_expression, invalid_subscript_expression)
{
    std::string input =
        R"(
&C SETC 'ABCDEF'(1,(DCVAL('A')))
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "CE004" }));
}

TEST(character_expression, valid_dupl_expression)
{
    std::string input =
        R"(
&A  SETC 'ABC'
&C1 SETC (1)'&A'
&C2 SETC (+5)'&A'
&C3 SETC ((DCLEN('XYZ')))'&A'
&C4 SETC ((NOT -X'03'))'&A'
&C5 SETC (((('ABC' FIND 'BC'))))'&A'
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C1"), "ABC");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C2"), "ABCABCABCABCABC");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C3"), "ABCABCABC");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C4"), "ABCABC");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C5"), "ABCABC");
}

TEST(character_expression, invalid_dupl_expression)
{
    std::string input =
        R"(
&A  SETC 'ABCDEF'
&B SETC ((1 AND 1))'&A'
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "CE005" }));
}

TEST(character_expression, subscripted_concat_evaluation)
{
    std::string input =
        R"(
&A    SETA 2
&B    SETA 3
&L(1) SETB 1,0,1
&X    SETC '&L((&A OR &B))'.'&L((&A AND &B))'
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "X"), "10");
}

TEST(character_expression, invalid_expression)
{
    std::string input =
        R"(
&A SETC DCLEN('ABC')
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "CE004" }));
}

// TODO - DCLEN implementation needs to be changed to support this behavior
// TEST(character_expression, string_concat_dclen)
//{
//    std::string input =
//        R"(
//&A SETC 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
//&C SETC '&A'(3,(DCLEN('SEVEN')))
//)";
//    analyzer a(input);
//    a.analyze();
//    a.collect_diags();
//
//    EXPECT_TRUE(a.diags().empty());
//    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C"), "VENFG");
//}

TEST(character_expression, valid_function)
{
    std::string input =
        R"(
&A      SETA  210
&C      SETC (BYTE &A)
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C"), "K");
}

TEST(character_expression, invalid_function)
{
    std::string input =
        R"(
&A      SETC  'X'
&C      SETC (BYTE &A)
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "CE012" }));
}

TEST(character_expression, multi_byte_substr)
{
    std::string input = (const char*)u8"&C SETC '\u00A6'(1,1)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C"), std::string((const char*)u8"\u00A6"));
}

TEST(character_expression, bounds)
{
    const std::string input_template = R"(
&C  SETC  'A'      
&C  SETC  '&C'%
)";
    for (const auto& [replacement, err] : std::initializer_list<std::pair<std::string_view, std::string_view>> {
             { "(0,*)", "CE008" },
             { "(1,*)", "" },
             { "(2,*)", "" },
             { "(2,1)", "CE009" },
             { "(3,*)", "CE009" },
         })
    {
        std::string input = input_template;
        input.replace(input.find('%'), 1, replacement);

        analyzer a(input);
        a.analyze();
        a.collect_diags();

        if (err.empty())
            EXPECT_TRUE(a.diags().empty());
        else
            EXPECT_TRUE(matches_message_codes(a.diags(), { std::string(err) }));
    }
}
