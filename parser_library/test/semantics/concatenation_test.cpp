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

#include "gmock/gmock.h"

#include "../common_testing.h"
#include "semantics/concatenation.h"

using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library;

concat_chain create_chain(context::hlasm_context& ctx)
{
    auto vs = std::make_unique<basic_variable_symbol>(ctx.ids().add("n"), std::vector<ca_expr_ptr>(), range());

    concat_chain created_name;
    created_name.emplace_back(char_str_conc("n", range()));

    auto vsc = std::make_unique<created_variable_symbol>(std::move(created_name), std::vector<ca_expr_ptr>(), range());

    concat_chain chain;

    chain.emplace_back(char_str_conc("ada", range()));
    chain.emplace_back(var_sym_conc(std::move(vs)));
    chain.emplace_back(dot_conc());
    chain.emplace_back(equals_conc());

    std::vector<concat_chain> list;
    concat_chain elem;
    elem.emplace_back(char_str_conc("ada", range()));
    list.push_back(std::move(elem));
    elem.emplace_back(char_str_conc("ada", range()));
    list.push_back(std::move(elem));
    elem.emplace_back(var_sym_conc(std::move(vsc)));
    list.push_back(std::move(elem));

    chain.emplace_back(sublist_conc(std::move(list)));

    return chain;
}


TEST(concatenation, to_string)
{
    context::hlasm_context ctx;
    EXPECT_EQ(concatenation_point::to_string(create_chain(ctx)), "ada&N.=(ada,ada,&(n))");
}

TEST(concatenation, find_var_sym)
{
    context::hlasm_context ctx;
    auto chain = create_chain(ctx);

    {
        auto var = concatenation_point::find_var_sym(chain.cbegin(), chain.cend());

        auto pos = std::get_if<var_sym_conc>(&chain[1].value);

        EXPECT_EQ(var, pos);
    }

    chain.erase(chain.begin() + 1);

    {
        auto var = concatenation_point::find_var_sym(chain.cbegin(), chain.cend());

        auto pos = std::get_if<var_sym_conc>(&std::get<sublist_conc>(chain.back().value).list.back().front().value);

        EXPECT_EQ(var, pos);
    }

    chain.erase(chain.begin() + (chain.size() - 1));

    {
        auto var = concatenation_point::find_var_sym(chain.cbegin(), chain.cend());

        EXPECT_EQ(var, nullptr);
    }
}

TEST(concatenation, no_dots)
{
    std::string input = R"(
&A SETC ' '
&A SETC '&A'(1,1)'&A'(1,1)
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "A"), "  ");
}

TEST(concatenation, with_dots)
{
    std::string input = R"(
&A SETC ' '
&A SETC '&A'(1,1).'&A'(1,1)
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "A"), "  ");
}

TEST(concatenation, with_functions)
{
    std::string input = R"(
&X SETC 'x'
&A SETC ' '.UPPER('&X')
&B SETC UPPER('&X').' '
&C SETC ' '.UPPER('&X').' '
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "A"), " X");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "B"), "X ");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C"), " X ");
}

TEST(concatenation, with_expr_operator)
{
    std::string input = R"(
&X SETC 'x'
&A SETC ' '.(UPPER '&X')
&B SETC (UPPER '&X').' '
&C SETC ' '.(UPPER '&X').' '
&D SETC ' '.(UPPER '&X'(1,1))
&E SETC (UPPER '&X'(1,1)).' '
&F SETC ' '.(UPPER '&X'(1,1)).' '
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "A"), " X");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "B"), "X ");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C"), " X ");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "D"), " X");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "E"), "X ");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "F"), " X ");
}

TEST(concatenation, no_dots_without_subscript)
{
    std::string input = R"(
&A SETC ' '
&A SETC '&A''&A'
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "A"), " ' ");
}

TEST(concatenation, identifier_after_variable_name)
{
    std::string input = R"(
&A SETC 'X'
&B SETC '&A:'
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "B"), "X:");
}
