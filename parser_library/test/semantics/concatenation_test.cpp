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

#include "semantics/concatenation_term.h"

using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library;

concat_chain create_chain()
{
    static std::string name = "n";
    auto vs = std::make_unique<basic_variable_symbol>(&name, std::vector<ca_expr_ptr>(), range());

    concat_chain created_name;
    created_name.push_back(std::make_unique<char_str_conc>("n", range()));

    auto vsc = std::make_unique<created_variable_symbol>(std::move(created_name), std::vector<ca_expr_ptr>(), range());

    concat_chain chain;

    chain.push_back(std::make_unique<char_str_conc>("ada", range()));
    chain.push_back(std::make_unique<var_sym_conc>(std::move(vs)));
    chain.push_back(std::make_unique<dot_conc>());
    chain.push_back(std::make_unique<equals_conc>());

    std::vector<concat_chain> list;
    concat_chain elem;
    elem.push_back(std::make_unique<char_str_conc>("ada", range()));
    list.push_back(std::move(elem));
    elem.push_back(std::make_unique<char_str_conc>("ada", range()));
    list.push_back(std::move(elem));
    elem.push_back(std::make_unique<var_sym_conc>(std::move(vsc)));
    list.push_back(std::move(elem));

    chain.push_back(std::make_unique<sublist_conc>(std::move(list)));

    return chain;
}


TEST(concatenation, to_string) { EXPECT_EQ(concatenation_point::to_string(create_chain()), "ada&n.=(ada,ada,&(n))"); }

TEST(concatenation, contains_var_sym)
{
    auto chain = create_chain();

    {
        auto var = concatenation_point::contains_var_sym(chain.cbegin(), chain.cend());

        auto pos = dynamic_cast<var_sym_conc*>(chain[1].get());

        EXPECT_EQ(var, pos);
    }

    chain.erase(chain.begin() + 1);

    {
        auto var = concatenation_point::contains_var_sym(chain.cbegin(), chain.cend());

        auto pos = dynamic_cast<var_sym_conc*>(chain.back()->access_sub()->list.back().front().get());

        EXPECT_EQ(var, pos);
    }

    chain.erase(chain.begin() + (chain.size() - 1));

    {
        auto var = concatenation_point::contains_var_sym(chain.cbegin(), chain.cend());

        EXPECT_EQ(var, nullptr);
    }
}