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

#include "expr_mocks.h"
#include "expressions/conditional_assembly/terms/ca_constant.h"
#include "expressions/conditional_assembly/terms/ca_symbol_attribute.h"
#include "expressions/evaluation_context.h"

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library;

TEST(ca_symbol_attr, undefined_attributes)
{
    dep_sol_mock m;
    std::string name = "n";
    std::vector<ca_expr_ptr> subscript;

    subscript.push_back(std::make_unique<ca_constant>(1, range()));

    auto vs = std::make_unique<basic_variable_symbol>(&name, std::move(subscript), range());

    ca_symbol_attribute attr(std::move(vs), context::data_attr_kind::D, range());

    auto res = attr.get_undefined_attributed_symbols(m);

    ASSERT_EQ(res.size(), 0U);
}

ca_symbol_attribute create_var_sym_attr(context::data_attr_kind kind)
{
    static std::string name = "n";
    std::vector<ca_expr_ptr> subscript;

    subscript.push_back(std::make_unique<ca_constant>(1, range()));

    auto vs = std::make_unique<basic_variable_symbol>(&name, std::move(subscript), range());

    return ca_symbol_attribute(std::move(vs), kind, range());
}

TEST(ca_symbol_attr, evaluate_undef_varsym)
{
    context::hlasm_context ctx;
    attr_prov_mock a;
    lib_prov_mock l;

    evaluation_context eval { ctx, a, l };

    auto res = create_var_sym_attr(context::data_attr_kind::D).evaluate(eval);

    ASSERT_EQ(eval.diags().size(), 1U);
}

TEST(ca_symbol_attr, evaluate_substituted_varsym_not_char)
{
    std::string name = "n";
    context::hlasm_context ctx;
    attr_prov_mock a;
    lib_prov_mock l;

    auto var = ctx.create_local_variable<int>(&name, true);
    var->access_set_symbol<int>()->set_value(12);

    evaluation_context eval { ctx, a, l };

    auto res = create_var_sym_attr(context::data_attr_kind::L).evaluate(eval);

    ASSERT_EQ(eval.diags().size(), 1U);
}

TEST(ca_symbol_attr, evaluate_substituted_varsym_char_not_sym)
{
    std::string name = "n";
    context::hlasm_context ctx;
    attr_prov_mock a;
    lib_prov_mock l;

    auto var = ctx.create_local_variable<context::C_t>(&name, true);
    var->access_set_symbol<context::C_t>()->set_value("(abc,a)");

    evaluation_context eval { ctx, a, l };

    auto res = create_var_sym_attr(context::data_attr_kind::L).evaluate(eval);

    ASSERT_EQ(eval.diags().size(), 1U);
}
