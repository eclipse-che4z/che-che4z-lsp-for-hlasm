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

// tests parsing of hlasm strings

TEST(parser, mach_string_double_ampersand)
{
    std::string input("A EQU C'&&'");
    analyzer a(input);
    a.analyze();

    auto& ctx = a.hlasm_ctx();

    auto it = ctx.ids().find("A");

    ASSERT_EQ(ctx.ord_ctx.get_symbol(it)->value().get_abs(), 80);
}

TEST(parser, ca_string_double_ampersand)
{
    std::string input("&A SETC '&&'");
    analyzer a(input);
    a.analyze();

    auto& ctx = a.hlasm_ctx();

    auto it = ctx.ids().find("A");

    ASSERT_EQ(ctx.get_var_sym(it)->access_set_symbol_base()->access_set_symbol<context::C_t>()->get_value(), "&&");
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
        test_param { "quote_before_attr_D", "\"D'SYM 93'", "\"D'SYM 93'" },

        // Invalid inputs are currently parsed as remark, therefore the variable symbol is empty.
        test_param { "no_ending_apostrophe", "\"N'SYM", "" },
        test_param { "no_ending_apostrophe_2", "\"L'SYM' STH", "" }),
    stringer());
} // namespace

TEST_P(parser_string_fixture, basic)
{
    std::string input = R"(
 GBLC &PAR
 MACRO
 MAC &VAR
 GBLC &PAR
&PAR SETC &VAR 
 MEND
 
 MAC )" + GetParam().parameter;
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_EQ(a.diags().size(), 0U);

    auto par_value = get_var_value<C_t>(a.hlasm_ctx(), "PAR");
    ASSERT_TRUE(par_value.has_value());
    EXPECT_EQ(*par_value, GetParam().expected);
}


TEST(parser, incomplete_string)
{
    std::string input = R"(
 GBLC &PAR
 MACRO
 MAC &VAR
 GBLC &PAR
&PAR SETC &VAR 
 MEND
 
 MAC 'A 93)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_EQ(a.diags().size(), 0U);

    auto par_value = get_var_value<C_t>(a.hlasm_ctx(), "PAR");
    ASSERT_TRUE(par_value.has_value());
}
