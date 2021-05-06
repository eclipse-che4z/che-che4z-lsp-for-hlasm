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

namespace {
template<typename T>
std::optional<T> get_var_value(hlasm_context& ctx, std::string name)
{
    auto var = ctx.get_var_sym(ctx.ids().find(name));
    if (!var)
        return std::nullopt;

    if (var->var_kind != context::variable_kind::SET_VAR_KIND)
        return std::nullopt;
    auto var_ = var->access_set_symbol_base();
    if (var_->type != object_traits<T>::type_enum || !var_->is_scalar)
        return std::nullopt;

    auto symbol = var_->template access_set_symbol<T>();
    if (!symbol)
        return std::nullopt;

    return symbol->get_value();
}

template<typename T>
std::optional<std::vector<T>> get_var_vector(hlasm_context& ctx, std::string name)
{
    auto var = ctx.get_var_sym(ctx.ids().find(name));
    if (!var)
        return std::nullopt;

    if (var->var_kind != context::variable_kind::SET_VAR_KIND)
        return std::nullopt;
    auto var_ = var->access_set_symbol_base();
    if (var_->type != object_traits<T>::type_enum || var_->is_scalar)
        return std::nullopt;

    auto symbol = var_->template access_set_symbol<T>();
    if (!symbol)
        return std::nullopt;

    auto keys = symbol->keys();

    std::vector<T> result;
    result.reserve(keys.size());
    for (size_t i = 0; i < keys.size(); ++i)
    {
        if (i != keys[i])
            return std::nullopt;
        result.push_back(symbol->get_value(i));
    }

    return result;
}

std::string aread_pad(std::string s)
{
    s.resize(80, ' ');
    return s;
}
} // namespace

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

    EXPECT_EQ(get_var_value<C_t>(ctx, "var").value_or(""), aread_pad("This is a raw text"));
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

    EXPECT_EQ(get_var_value<A_t>(ctx, "PROCESSED").value_or(0), 1);

    const auto expected = std::vector<C_t> {
        aread_pad("This is a raw text 1"),
        aread_pad("This is a raw text 2"),
    };
    EXPECT_EQ(get_var_vector<C_t>(ctx, "var"), expected);
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

    auto var = get_var_vector<C_t>(ctx, "var").value_or(std::vector<C_t> {});

    ASSERT_EQ(var.size(), 4);
    EXPECT_EQ(var[0], aread_pad("This is a raw text 1"));
    EXPECT_EQ(var[1], aread_pad("This is a raw text 2"));
    EXPECT_EQ(var[2].size(), 8);
    EXPECT_EQ(var[3].size(), 8);
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

    auto var = get_var_value<C_t>(ctx, "var");

    ASSERT_TRUE(var.has_value());
    EXPECT_EQ(var.value(), "");
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
&VAR      AREAD NOSTMT,NOSTMT
          MEND

          GBLC &VAR
          M
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 5);

    EXPECT_TRUE(std::all_of(diags.begin(), diags.end(), [](const auto& d) { return d.code == "E070"; }));
}

TEST(aread, from_ainsert_buffer)
{
    std::string input(R"(
          MACRO
          M
          GBLC &VAR1
          GBLC &VAR2
&VAR1     AREAD
&VAR2     AREAD
          MEND

          MACRO
          M2
          AINSERT 'test string 2',BACK
          AINSERT 'test string 1',FRONT
          AINSERT ' M',FRONT
          MEND

          GBLC &VAR1
          GBLC &VAR2

          M2
&PROCESSED SETA 1
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    EXPECT_EQ(get_var_value<A_t>(ctx, "PROCESSED").value_or(0), 1);

    EXPECT_EQ(get_var_value<C_t>(ctx, "var1").value_or(""), aread_pad("test string 1"));
    EXPECT_EQ(get_var_value<C_t>(ctx, "var2").value_or(""), aread_pad("test string 2"));
}

TEST(aread, ainsert_with_substitution)
{
    std::string input(R"(
&VAR SETC '1'
          AINSERT '&&PROCESSED SETA &VAR',FRONT
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    auto& diags = a.diags();
    ASSERT_EQ(diags.size(), 0);

    auto& ctx = a.hlasm_ctx();

    EXPECT_EQ(get_var_value<A_t>(ctx, "PROCESSED").value_or(0), 1);
}
