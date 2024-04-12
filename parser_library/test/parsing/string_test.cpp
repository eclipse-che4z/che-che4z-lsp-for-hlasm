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

#include <regex>

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "../mock_parse_lib_provider.h"

// tests parsing of hlasm strings

TEST(parser, mach_string_double_ampersand)
{
    std::string input("A EQU C'&&'");
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "A"), 80);
}

TEST(parser, ca_string_double_ampersand)
{
    std::string input("&A SETC '&&'");
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<context::C_t>(a.hlasm_ctx(), "A"), "&&");
}

TEST(parser, preconstructed_string)
{
    std::string input = R"(
         MACRO
         MAC2 &NAMELEN=,&PLIST=
         GBLC &NL,&PL
&NL      SETC '&NAMELEN'
&PL      SETC '&PLIST'
         MEND

         MACRO
         MAC &PLIST=PLIST,&STGNAME='STG'

         MAC2 NAMELEN=L'=C&STGNAME,PLIST=&PLIST
         MEND

         GBLC &NL,&PL
         MAC
         END)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "NL"), "L'=C'STG'");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "PL"), "PLIST");
}

TEST(parser, preconstructed_string_2)
{
    std::string input = R"(
         MACRO
         MAC
         MEND

&VAR     SETC 'L''SYM'

         MAC ('(&VAR)')
         MAC ('STR(&VAR)')
         MAC ('(&VAR)STR')
         MAC ('STR(&VAR)STR')

         MAC PARAMETER)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(parser, string_evaluation)
{
    std::string input = R"(
         MACRO
         MAC2
         MEND

         MACRO
         MAC &PAR
&HASH    SETC  'I''#RULE'
&NUM     SETC  'I''1RULE'
&NEG     SETC  'I''-1RULE'
&EQ      SETC  'I''=RULE'
&CHAR    SETC  'I''RULE'
&PAR2    SETC  'I''&PAR'
         MAC2  (&HASH)
         MAC2  (&NUM)
         MAC2  (&NEG)
         MAC2  (&EQ)
         MAC2  (&CHAR)
         MAC2  (&PAR2)
         MEND

         MAC PARAMETER)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

namespace {
struct test_param
{
    std::string name;
    std::string parameter;
    std::string expected;
};

struct stringer
{
    std::string operator()(::testing::TestParamInfo<test_param> p) { return p.param.name; }
};

class parser_string_fixture : public ::testing::TestWithParam<test_param>
{};

INSTANTIATE_TEST_SUITE_P(parser,
    parser_string_fixture,
    ::testing::Values(test_param { "A_no_attr", "A'SYM 93'", "A'SYM 93'" },
        test_param { "D_attr", "D'SYM 93'", "D'SYM 93'" },
        test_param { "I_attr", "I'SYM 93'", "I'SYM" },
        test_param { "K_attr", "K'SYM 93'", "K'SYM 93'" },
        test_param { "L_attr", "L'SYM 93'", "L'SYM" },
        test_param { "N_attr", "N'SYM 93'", "N'SYM 93'" },
        test_param { "O_attr", "O'SYM 93'", "O'SYM" },
        test_param { "S_attr", "S'SYM 93'", "S'SYM" },
        test_param { "T_attr", "T'SYM 93'", "T'SYM" },
        test_param { "attr_and_string", "S'SYM' STH'", "S'SYM' STH'" },
        test_param { "literal_FD", "=FD'SYM STH'", "=FD'SYM STH'" },
        test_param { "literal_FS", "=FS'SYM STH'", "=FS'SYM STH'" },
        test_param { "number_before_attr_L", "=4L'SYM 93'", "=4L'SYM 93'" },
        test_param { "quote_before_attr_L", "\"L'SYM 93'", "\"L'SYM" },
        test_param { "quote_before_attr_D", "\"D'SYM 93'", "\"D'SYM 93'" }),
    stringer());
} // namespace

TEST_P(parser_string_fixture, basic)
{
    std::string input = R"(
 GBLC &PAR
 MACRO
 MAC &VAR
 GBLC &PAR
&PAR SETC '&VAR'
 MEND
 
 MAC )" + GetParam().parameter;
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "PAR"), GetParam().expected);
}

TEST(parser, incomplete_string)
{
    std::string input = R"(
 GBLC &PAR
 MACRO
 MAC &VAR
 GBLC &PAR
&PAR SETC '&VAR'
 MEND
 
 MAC 'A 93)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0005" }));

    auto par_value = get_var_value<C_t>(a.hlasm_ctx(), "PAR");
    ASSERT_TRUE(par_value.has_value());
}

namespace {
struct test_param_parser_data_attribute
{
    std::string name;
    bool is_attribute;
    bool is_consuming;
};

struct stringer_test_param_parser_data_attribute
{
    std::string operator()(::testing::TestParamInfo<test_param_parser_data_attribute> p) { return p.param.name; }
};

class parser_data_attribute_fixture : public ::testing::TestWithParam<test_param_parser_data_attribute>
{};

INSTANTIATE_TEST_SUITE_P(parser,
    parser_data_attribute_fixture,
    ::testing::Values(test_param_parser_data_attribute { "A", false, false }, // Intentionally not a data attribute
        test_param_parser_data_attribute { "D", true, false },
        test_param_parser_data_attribute { "I", true, true },
        test_param_parser_data_attribute { "K", true, false },
        test_param_parser_data_attribute { "L", true, true },
        test_param_parser_data_attribute { "N", true, false },
        test_param_parser_data_attribute { "O", true, true },
        test_param_parser_data_attribute { "S", true, true },
        test_param_parser_data_attribute { "T", true, true }),
    stringer_test_param_parser_data_attribute());

static const std::regex x("\\$x");
static const std::regex y("\\$y");

mock_parse_lib_provider lib_provider { []() {
    std::vector<std::pair<std::string, std::string>> macros;

    for (auto i = 1; i <= 3; ++i)
    {
        std::string mac_name = "MAC_STR$x";
        std::string mac = R"(*
         MACRO
         MAC_STR$x &VAR
         GBLC &STR$x
&STR$x   SETC '&VAR'
         MEND
)";

        mac_name = std::regex_replace(mac_name, x, std::to_string(i));
        mac = std::regex_replace(mac, x, std::to_string(i));
        macros.emplace_back(mac_name, mac);
    }

    for (auto i = 1; i <= 5; ++i)
    {
        std::string mac_name = "MAC_LIST_1_ELEM_STR$x";
        std::string mac = R"(*
         MACRO
         MAC_LIST_1_ELEM_STR$x &VAR
         GBLC &STR$x
&STR$x   SETC '&VAR(1)'
         MEND
)";

        mac_name = std::regex_replace(mac_name, x, std::to_string(i));
        mac = std::regex_replace(mac, x, std::to_string(i));
        macros.emplace_back(mac_name, mac);
    }

    for (auto i = 1; i <= 2; ++i)
    {
        std::string mac_name = "MAC_LIST_2_ELEM_STR$x_STR$y";

        std::string mac = R"(*
         MACRO
         MAC_LIST_2_ELEM_STR$x_STR$y &VAR
         GBLC &STR$x,&STR$y
&STR$x   SETC '&VAR(1)'
&STR$y   SETC '&VAR(2)'
         MEND
)";

        mac_name = std::regex_replace(mac_name, x, std::to_string(i * 2 - 1));
        mac_name = std::regex_replace(mac_name, y, std::to_string(i * 2));
        mac = std::regex_replace(mac, x, std::to_string(i * 2 - 1));
        mac = std::regex_replace(mac, y, std::to_string(i * 2));
        macros.emplace_back(mac_name, mac);
    }

    return macros;
}() };

std::unique_ptr<analyzer> analyze(std::string s, std::string attr)
{
    s = std::regex_replace(s, x, attr);

    auto a = std::make_unique<analyzer>(s, analyzer_options { &lib_provider });
    a->analyze();

    return a;
}
} // namespace

TEST_P(parser_data_attribute_fixture, missing_apostrophe)
{
    std::string input = R"(
         GBLC &STR1
         MAC_STR1 $x')";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_attribute)
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0003" }));
    else
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005" }));
}

TEST_P(parser_data_attribute_fixture, no_ending_apostrophe)
{
    std::string input = R"(
         GBLC &STR1,&STR2
         MAC_STR1 "$x'SYM
         MAC_STR2 "$x'SYM' STH
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 3, 3 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), "\"" + GetParam().name + "'SYM");
    }
    else
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 2, 2 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), "\"" + GetParam().name + "'SYM'");
    }
}

TEST_P(parser_data_attribute_fixture, trailing_comma)
{
    std::string input = R"(
         GBLC &STR1
         MAC_STR1 $x'SYM                                     ,)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(a->diags().empty());
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), GetParam().name + "'SYM");
    }
    else
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005" }));
}

TEST_P(parser_data_attribute_fixture, text)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
         MAC_STR1 $x'J
         MAC_STR2 $x'J'
         MAC_STR3 $x'J''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 3, 3 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), GetParam().name + "'J");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), GetParam().name + "'J''");
    }
    else
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 2, 2 }, { 4, 4 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'J'");
    }
}

TEST_P(parser_data_attribute_fixture, text_remark)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
         MAC_STR1 $x'J          REMARK
         MAC_STR2 $x'J          REMARK'
         MAC_STR3 $x'J          REMARK''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(a->diags().empty());
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), GetParam().name + "'J");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'J");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), GetParam().name + "'J");
    }
    else
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 2, 2 }, { 4, 4 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'J          REMARK'");
    }
}

TEST_P(parser_data_attribute_fixture, number)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
         MAC_STR1 $x'9
         MAC_STR2 $x'9'
         MAC_STR3 $x'9''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_attribute)
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0003", "S0005" }));
    else
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005" }));

    EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 2, 2 }, { 4, 4 } }));

    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'9'");
}

TEST_P(parser_data_attribute_fixture, number_remark)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
         MAC_STR1 $x'9           REMARK
         MAC_STR2 $x'9           REMARK'
         MAC_STR3 $x'9           REMARK''
)";

    auto a = analyze(input, GetParam().name);

    EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005" }));
    EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 2, 2 }, { 4, 4 } }));

    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'9           REMARK'");
}

TEST_P(parser_data_attribute_fixture, negative_number)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
         MAC_STR1 $x'-9
         MAC_STR2 $x'-9'
         MAC_STR3 $x'-9''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_attribute)
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0003", "S0005" }));
    else
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005" }));

    EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 2, 2 }, { 4, 4 } }));

    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'-9'");
}

TEST_P(parser_data_attribute_fixture, negative_number_remark)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
         MAC_STR1 $x'-9          REMARK
         MAC_STR2 $x'-9          REMARK'
         MAC_STR3 $x'-9          REMARK''
)";

    auto a = analyze(input, GetParam().name);

    EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005" }));
    EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 2, 2 }, { 4, 4 } }));

    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'-9          REMARK'");
}

TEST_P(parser_data_attribute_fixture, var_instr)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
&INSTR   SETC   'J'
         MAC_STR1 $x'&INSTR
         MAC_STR2 $x'&INSTR'
         MAC_STR3 $x'&INSTR''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(a->diags().empty());
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), GetParam().name + "'J");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'J'");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), GetParam().name + "'J''");
    }
    else
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 3, 3 }, { 5, 5 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'J'");
    }
}

TEST_P(parser_data_attribute_fixture, var_instr_remark)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
&INSTR   SETC   'J'
         MAC_STR1 $x'&INSTR          REMARK
         MAC_STR2 $x'&INSTR          REMARK'
         MAC_STR3 $x'&INSTR          REMARK''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(a->diags().empty());
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), GetParam().name + "'J");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'J");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), GetParam().name + "'J");
    }
    else
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 3, 3 }, { 5, 5 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'J          REMARK'");
    }
}

TEST_P(parser_data_attribute_fixture, ampersand)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
         MAC_STR1 $x'&
         MAC_STR2 $x'&'
         MAC_STR3 $x'&''
)";

    auto a = analyze(input, GetParam().name);

    EXPECT_TRUE(matches_message_codes(a->diags(), { "S0008", "S0008", "S0008" }));
    EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 2, 2 }, { 3, 3 }, { 4, 4 } }));
}

TEST_P(parser_data_attribute_fixture, ampersand_remark)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
         MAC_STR1 $x'&          REMARK
         MAC_STR2 $x'&          REMARK'
         MAC_STR3 $x'&          REMARK''
)";

    auto a = analyze(input, GetParam().name);

    EXPECT_TRUE(matches_message_codes(a->diags(), { "S0008", "S0008", "S0008" }));
    EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 2, 2 }, { 3, 3 }, { 4, 4 } }));
}

TEST_P(parser_data_attribute_fixture, double_ampersand)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
         MAC_STR1 $x'&&
         MAC_STR2 $x'&&'
         MAC_STR3 $x'&&''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(a->diags().empty());
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), GetParam().name + "'&&");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'&&'");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), GetParam().name + "'&&''");
    }
    else
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 2, 2 }, { 4, 4 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'&&'");
    }
}

TEST_P(parser_data_attribute_fixture, double_ampersand_remark)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
         MAC_STR1 $x'&&          REMARK
         MAC_STR2 $x'&&          REMARK'
         MAC_STR3 $x'&&          REMARK''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(a->diags().empty());
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), GetParam().name + "'&&");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'&&");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), GetParam().name + "'&&");
    }
    else
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 2, 2 }, { 4, 4 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'&&          REMARK'");
    }
}

TEST_P(parser_data_attribute_fixture, var_var_concat)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
&INSTR   SETC 'J'
         MAC_STR1 $x'&INSTR.&INSTR
         MAC_STR2 $x'&INSTR.&INSTR'
         MAC_STR3 $x'&INSTR.&INSTR''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(a->diags().empty());
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), GetParam().name + "'JJ");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'JJ'");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), GetParam().name + "'JJ''");
    }
    else
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 3, 3 }, { 5, 5 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'JJ'");
    }
}

TEST_P(parser_data_attribute_fixture, var_var_concat_remark)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
&INSTR   SETC 'J'
         MAC_STR1 $x'&INSTR.&INSTR          REMARK
         MAC_STR2 $x'&INSTR.&INSTR          REMARK'
         MAC_STR3 $x'&INSTR.&INSTR          REMARK''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(a->diags().empty());
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), GetParam().name + "'JJ");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'JJ");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), GetParam().name + "'JJ");
    }
    else
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 3, 3 }, { 5, 5 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'JJ          REMARK'");
    }
}

TEST_P(parser_data_attribute_fixture, var_ord_concat)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
&INSTR   SETC 'J'
         MAC_STR1 $x'&INSTR.ORD
         MAC_STR2 $x'&INSTR.ORD'
         MAC_STR3 $x'&INSTR.ORD''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(a->diags().empty());
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), GetParam().name + "'JORD");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'JORD'");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), GetParam().name + "'JORD''");
    }
    else
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 3, 3 }, { 5, 5 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'JORD'");
    }
}

TEST_P(parser_data_attribute_fixture, var_ord_concat_remark)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
&INSTR   SETC 'J'
         MAC_STR1 $x'&INSTR.ORD          REMARK
         MAC_STR2 $x'&INSTR.ORD          REMARK'
         MAC_STR3 $x'&INSTR.ORD          REMARK''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(a->diags().empty());
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), GetParam().name + "'JORD");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'JORD");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), GetParam().name + "'JORD");
    }
    else
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 3, 3 }, { 5, 5 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'JORD          REMARK'");
    }
}

TEST_P(parser_data_attribute_fixture, ordsymbol_var_concat)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
&VAR     SETC 'J'
         MAC_STR1 $x'ORD&VAR
         MAC_STR2 $x'ORD&VAR'
         MAC_STR3 $x'ORD&VAR''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 4, 4 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), GetParam().name + "'ORDJ");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), GetParam().name + "'ORDJ''");
    }
    else
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 3, 3 }, { 5, 5 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'ORDJ'");
    }
}

TEST_P(parser_data_attribute_fixture, ordsymbol_var_concat_remark)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
&VAR     SETC 'J'
         MAC_STR1 $x'ORD&VAR    REMARK
         MAC_STR2 $x'ORD&VAR    REMARK'
         MAC_STR3 $x'ORD&VAR    REMARK''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(a->diags().empty());
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), GetParam().name + "'ORDJ");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'ORDJ");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), GetParam().name + "'ORDJ");
    }
    else
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 3, 3 }, { 5, 5 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'ORDJ    REMARK'");
    }
}

TEST_P(parser_data_attribute_fixture, ordsymbol_double_ampersand_concat)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
         MAC_STR1 $x'ORD&&
         MAC_STR2 $x'ORD&&'
         MAC_STR3 $x'ORD&&''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 3, 3 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), GetParam().name + "'ORD&&");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), GetParam().name + "'ORD&&''");
    }
    else
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 2, 2 }, { 4, 4 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'ORD&&'");
    }
}

TEST_P(parser_data_attribute_fixture, ordsymbol_double_ampersand_concat_remark)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3
         MAC_STR1 $x'ORD&&    REMARK
         MAC_STR2 $x'ORD&&    REMARK'
         MAC_STR3 $x'ORD&&    REMARK''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(a->diags().empty());
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), GetParam().name + "'ORD&&");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'ORD&&");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), GetParam().name + "'ORD&&");
    }
    else
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 2, 2 }, { 4, 4 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'ORD&&    REMARK'");
    }
}

TEST_P(parser_data_attribute_fixture, list_1_elem_text)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3,&STR4,&STR5
         MAC_LIST_1_ELEM_STR1 ($x'J)
         MAC_LIST_1_ELEM_STR2 ($x'J')
         MAC_LIST_1_ELEM_STR3 ($x'J'')
         MAC_LIST_1_ELEM_STR4 ($x'J')'
         MAC_LIST_1_ELEM_STR5 ($x'J')''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(contains_message_codes(a->diags(), { "S0003", "S0005", "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 3, 3 }, { 5, 5 }, { 6, 6 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), GetParam().name + "'J");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), GetParam().name + "'J''");
    }
    else
    {
        EXPECT_TRUE(contains_message_codes(a->diags(), { "S0005", "S0005", "S0005" }));
        EXPECT_TRUE(contains_diagnosed_line_ranges(a->diags(), { { 2, 2 }, { 4, 4 }, { 5, 5 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'J'");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR5"), "(" + GetParam().name + "'J')''");
    }
}

TEST_P(parser_data_attribute_fixture, list_1_elem_number)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3,&STR4,&STR5
         MAC_LIST_1_ELEM_STR1 ($x'9)
         MAC_LIST_1_ELEM_STR2 ($x'9')
         MAC_LIST_1_ELEM_STR3 ($x'9'')
         MAC_LIST_1_ELEM_STR4 ($x'9')'
         MAC_LIST_1_ELEM_STR5 ($x'9')''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_attribute)
    {
        EXPECT_TRUE(contains_message_codes(a->diags(), { "S0005", "S0003", "S0005", "S0003" }));
        EXPECT_TRUE(contains_diagnosed_line_ranges(a->diags(), { { 2, 2 }, { 4, 4 }, { 5, 5 } }));
    }
    else
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005", "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 2, 2 }, { 4, 4 }, { 5, 5 } }));
    }

    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'9'");
    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR5"), "(" + GetParam().name + "'9')''");
}

TEST_P(parser_data_attribute_fixture, list_1_elem_negative_number)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3,&STR4,&STR5
         MAC_LIST_1_ELEM_STR1 ($x'-9)
         MAC_LIST_1_ELEM_STR2 ($x'-9')
         MAC_LIST_1_ELEM_STR3 ($x'-9'')
         MAC_LIST_1_ELEM_STR4 ($x'-9')'
         MAC_LIST_1_ELEM_STR5 ($x'-9')''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_attribute)
    {
        EXPECT_TRUE(contains_message_codes(a->diags(), { "S0005", "S0003", "S0005", "S0003" }));
        EXPECT_TRUE(contains_diagnosed_line_ranges(a->diags(), { { 2, 2 }, { 4, 4 }, { 5, 5 } }));
    }
    else
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005", "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 2, 2 }, { 4, 4 }, { 5, 5 } }));
    }

    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'-9'");
    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR5"), "(" + GetParam().name + "'-9')''");
}

TEST_P(parser_data_attribute_fixture, list_1_elem_var_instr)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3,&STR4,&STR5
&VAR     SETC 'J'
         MAC_LIST_1_ELEM_STR1 ($x'&VAR)
         MAC_LIST_1_ELEM_STR2 ($x'&VAR')
         MAC_LIST_1_ELEM_STR3 ($x'&VAR'')
         MAC_LIST_1_ELEM_STR4 ($x'&VAR')'
         MAC_LIST_1_ELEM_STR5 ($x'&VAR')''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005", "S0005", "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 4, 4 }, { 6, 6 }, { 7, 7 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), GetParam().name + "'J");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), GetParam().name + "'J''");
    }
    else
    {
        EXPECT_TRUE(contains_message_codes(a->diags(), { "S0005", "S0005", "S0005" }));
        EXPECT_TRUE(contains_diagnosed_line_ranges(a->diags(), { { 3, 3 }, { 5, 5 }, { 6, 6 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'J'");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR5"), "(" + GetParam().name + "'J')''");
    }
}

TEST_P(parser_data_attribute_fixture, list_1_elem_var_number)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3,&STR4,&STR5
&VAR     SETC '9'
         MAC_LIST_1_ELEM_STR1 ($x'&VAR)
         MAC_LIST_1_ELEM_STR2 ($x'&VAR')
         MAC_LIST_1_ELEM_STR3 ($x'&VAR'')
         MAC_LIST_1_ELEM_STR4 ($x'&VAR')'
         MAC_LIST_1_ELEM_STR5 ($x'&VAR')''
)";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(contains_message_codes(a->diags(), { "S0005", "S0005" }));
        EXPECT_TRUE(contains_diagnosed_line_ranges(a->diags(), { { 3, 3 }, { 5, 5 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'9'");
        // EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR4"), "(" + GetParam().name + "'9')'"); // This almost seems
        // like a bug in HLASM
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR5"), "(" + GetParam().name + "'9')''");
    }
    else
    {
        EXPECT_TRUE(contains_message_codes(a->diags(), { "S0005", "S0005", "S0005" }));
        EXPECT_TRUE(contains_diagnosed_line_ranges(a->diags(), { { 3, 3 }, { 5, 5 }, { 6, 6 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'9'");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR5"), "(" + GetParam().name + "'9')''");
    }
}

TEST_P(parser_data_attribute_fixture, list_2_elem_text)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3,&STR4
         MAC_LIST_2_ELEM_STR1_STR2 (A,$x'J)
         MAC_LIST_2_ELEM_STR3_STR4 (A,$x'J'))";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 3, 3 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), "A");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'J");
    }
    else
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 2, 2 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), "A");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR4"), GetParam().name + "'J'");
    }
}

TEST_P(parser_data_attribute_fixture, list_2_elem_number)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3,&STR4
         MAC_LIST_2_ELEM_STR1_STR2 (A,$x'9)
         MAC_LIST_2_ELEM_STR3_STR4 (A,$x'9'))";

    auto a = analyze(input, GetParam().name);

    EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005" }));
    EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 2, 2 } }));

    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), "A");
    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR4"), GetParam().name + "'9'");
}

TEST_P(parser_data_attribute_fixture, list_2_elem_negative_number)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3,&STR4
         MAC_LIST_2_ELEM_STR1_STR2 (A,$x'-9)
         MAC_LIST_2_ELEM_STR3_STR4 (A,$x'-9'))";

    auto a = analyze(input, GetParam().name);

    EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005" }));
    EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 2, 2 } }));

    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), "A");
    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR4"), GetParam().name + "'-9'");
}

TEST_P(parser_data_attribute_fixture, list_2_elem_var_instr)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3,&STR4
&VAR     SETC 'J'
         MAC_LIST_2_ELEM_STR1_STR2 (A,$x'&VAR)
         MAC_LIST_2_ELEM_STR3_STR4 (A,$x'&VAR'))";

    auto a = analyze(input, GetParam().name);

    if (GetParam().is_consuming)
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 4, 4 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR1"), "A");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR2"), GetParam().name + "'J");
    }
    else
    {
        EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005" }));
        EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 3, 3 } }));

        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), "A");
        EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR4"), GetParam().name + "'J'");
    }
}

TEST_P(parser_data_attribute_fixture, list_2_elem_var_number)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3,&STR4
&VAR     SETC '9'
         MAC_LIST_2_ELEM_STR1_STR2 (A,$x'&VAR)
         MAC_LIST_2_ELEM_STR3_STR4 (A,$x'&VAR'))";

    auto a = analyze(input, GetParam().name);

    EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005" }));
    EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 3, 3 } }));

    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), "A");
    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR4"), GetParam().name + "'9'");
}


TEST_P(parser_data_attribute_fixture, list_2_elem_var_negative_number)
{
    std::string input = R"(
         GBLC &STR1,&STR2,&STR3,&STR4
&VAR     SETC '-9'
         MAC_LIST_2_ELEM_STR1_STR2 (A,$x'&VAR)
         MAC_LIST_2_ELEM_STR3_STR4 (A,$x'&VAR'))";

    auto a = analyze(input, GetParam().name);

    EXPECT_TRUE(matches_message_codes(a->diags(), { "S0005" }));
    EXPECT_TRUE(matches_diagnosed_line_ranges(a->diags(), { { 3, 3 } }));

    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR3"), "A");
    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "STR4"), GetParam().name + "'-9'");
}

TEST_P(parser_data_attribute_fixture, preserve_structured_parameter)
{
    std::string input = R"(
      GBLC  &PAR1,&PAR2
      MACRO
      MAC2
      GBLC  &PAR1,&PAR2
&PAR1 SETC  '&SYSLIST(1,1)'
&PAR2 SETC  '&SYSLIST(1,2)'
      MEND

      MACRO
      MAC   &P1
      MAC2  &P1
      MEND

      MAC   (A,$x'-9'))";

    auto a = analyze(input, GetParam().name);

    EXPECT_TRUE(a->diags().empty());
    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "PAR1"), "A");
    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "PAR2"), GetParam().name + "'-9'");
}

TEST_P(parser_data_attribute_fixture, preserve_structured_parameter_2)
{
    std::string input = R"(
      GBLC  &PAR1,&PAR2
      MACRO
      MAC2
      GBLC  &PAR1,&PAR2
&PAR1 SETC  '&SYSLIST(1,1)'
&PAR2 SETC  '&SYSLIST(1,2)'
      MEND

      MACRO
      MAC   &P1
      MAC2  &P1.
      MEND

      MAC   (A,$x'-9'))";

    auto a = analyze(input, GetParam().name);

    EXPECT_TRUE(a->diags().empty());
    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "PAR1"), "A");
    EXPECT_EQ(get_var_value<C_t>(a->hlasm_ctx(), "PAR2"), GetParam().name + "'-9'");
}
