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

// tests for
// AREAD handling

TEST(aread, only_from_macro)
{
    std::string input("&VAR AREAD");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();

    ASSERT_EQ(diags.size(), 1);
    EXPECT_TRUE(std::any_of(diags.begin(), diags.end(), [](const auto& msg) { return msg.code == "E069"; }));
}

TEST(aread, basic_test)
{
    std::string input(R"(
          MACRO
          M
          GBLC &VAR
&VAR      AREAD
          MEND

          GBLC &VAR
          M
This is a raw text
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    auto it = ctx.ids().find("var");

    auto var = ctx.get_var_sym(it);
    ASSERT_TRUE(var);

    ASSERT_EQ(var->var_kind, context::variable_kind::SET_VAR_KIND);
    auto var_ = var->access_set_symbol_base();
    ASSERT_EQ(var_->type, context::SET_t_enum::C_TYPE);
    ASSERT_TRUE(var_->is_scalar);
    auto symbol = var_->access_set_symbol<C_t>();
    ASSERT_TRUE(symbol);
    auto var_value = symbol->get_value();
    std::string expected_value = "This is a raw text";
    expected_value.resize(80, ' ');

    EXPECT_EQ(var_value, expected_value);
}

TEST(aread, array_test)
{
    std::string input(R"(
          MACRO
          M
          GBLC &VAR(10)
&VAR(1)   AREAD
&VAR(2)   AREAD
          MEND

          GBLC &VAR(10)
          M
This is a raw text 1
This is a raw text 2
&PROCESSED SETA 1
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();


    auto processed_id = ctx.ids().find("PROCESSED");
    auto processed = ctx.get_var_sym(processed_id);
    ASSERT_TRUE(processed);
    ASSERT_EQ(processed->var_kind, context::variable_kind::SET_VAR_KIND);
    auto processed_ = processed->access_set_symbol_base();
    ASSERT_TRUE(processed_->is_scalar);
    EXPECT_EQ(processed_->access_set_symbol<A_t>()->get_value(), 1);



    auto it = ctx.ids().find("var");
    auto var = ctx.get_var_sym(it);
    ASSERT_TRUE(var);

    ASSERT_EQ(var->var_kind, context::variable_kind::SET_VAR_KIND);
    auto var_ = var->access_set_symbol_base();
    ASSERT_EQ(var_->type, context::SET_t_enum::C_TYPE);
    ASSERT_FALSE(var_->is_scalar);
    auto symbol = var_->access_set_symbol<C_t>();
    ASSERT_TRUE(symbol);
    ASSERT_EQ(symbol->size(), 2);

    auto var_value_1 = symbol->get_value(1 - 1);
    auto var_value_2 = symbol->get_value(2 - 1);
    std::string expected_value_1 = "This is a raw text 1";
    expected_value_1.resize(80, ' ');
    std::string expected_value_2 = "This is a raw text 2";
    expected_value_2.resize(80, ' ');

    EXPECT_EQ(var_value_1, expected_value_1);
    EXPECT_EQ(var_value_2, expected_value_2);
}

TEST(aread, operand_support)
{
    std::string input(R"(
          MACRO
          M
          GBLC &VAR(10)
&VAR(1)   AREAD NOPRINT
&VAR(2)   AREAD NOSTMT
&VAR(3)   AREAD CLOCKB
&VAR(4)   AREAD CLOCKD
          MEND

          GBLC &VAR(10)
          M
This is a raw text 1
This is a raw text 2
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    auto it = ctx.ids().find("var");
    auto var = ctx.get_var_sym(it);
    ASSERT_TRUE(var);

    ASSERT_EQ(var->var_kind, context::variable_kind::SET_VAR_KIND);
    auto var_ = var->access_set_symbol_base();
    ASSERT_EQ(var_->type, context::SET_t_enum::C_TYPE);
    ASSERT_FALSE(var_->is_scalar);
    auto symbol = var_->access_set_symbol<C_t>();
    ASSERT_TRUE(symbol);
    ASSERT_EQ(symbol->size(), 4);

    auto var_value_1 = symbol->get_value(1 - 1);
    auto var_value_2 = symbol->get_value(2 - 1);
    auto var_value_3 = symbol->get_value(3 - 1);
    auto var_value_4 = symbol->get_value(4 - 1);
    std::string expected_value_1 = "This is a raw text 1";
    expected_value_1.resize(80, ' ');
    std::string expected_value_2 = "This is a raw text 2";
    expected_value_2.resize(80, ' ');

    EXPECT_EQ(var_value_1, expected_value_1);
    EXPECT_EQ(var_value_2, expected_value_2);
    EXPECT_EQ(var_value_3.size(), 8);
    EXPECT_EQ(var_value_4.size(), 8);
}

TEST(aread, empty_input)
{
    std::string input(R"(
          MACRO
          M
          GBLC &VAR
&VAR      AREAD 
          MEND

          GBLC &VAR
          M
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    auto it = ctx.ids().find("var");
    auto var = ctx.get_var_sym(it);
    ASSERT_TRUE(var);

    ASSERT_EQ(var->var_kind, context::variable_kind::SET_VAR_KIND);
    auto var_ = var->access_set_symbol_base();
    ASSERT_EQ(var_->type, context::SET_t_enum::C_TYPE);
    ASSERT_TRUE(var_->is_scalar);
    auto symbol = var_->access_set_symbol<C_t>();
    ASSERT_TRUE(symbol);

    EXPECT_EQ(symbol->get_value(), "");
}

TEST(aread, invalid_operands)
{
    std::string input(R"(
          MACRO
          M
          GBLC &VAR
&VAR      AREAD A
&VAR      AREAD 1
&VAR      AREAD &VAR
&VAR      AREAD 'A'
          MEND

          GBLC &VAR
          M
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 4);

    EXPECT_TRUE(std::all_of(diags.begin(), diags.end(), [](const auto& d) { return d.code == "E070"; }));
}
