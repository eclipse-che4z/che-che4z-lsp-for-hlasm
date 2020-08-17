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
#include "expressions/conditional_assembly/terms/ca_expr_list.h"
#include "expressions/conditional_assembly/terms/ca_function.h"
#include "expressions/conditional_assembly/terms/ca_string.h"
#include "expressions/conditional_assembly/terms/ca_symbol.h"
#include "expressions/conditional_assembly/terms/ca_symbol_attribute.h"
#include "expressions/evaluation_context.h"

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library;

TEST(ca_expr_list, unknown_function_to_operator)
{
    context::hlasm_context ctx;
    attr_prov_mock attr;
    lib_prov_mock lib;
    evaluation_context eval_ctx { ctx, attr, lib };

    std::string name = "AND";
    auto c = std::make_unique<ca_constant>(1, range());
    auto dupl = std::make_unique<ca_constant>(1, range());
    std::vector<ca_expr_ptr> params;
    params.emplace_back(std::move(c));
    auto unknown_func =
        std::make_unique<ca_function>(&name, ca_expr_funcs::UNKNOWN, std::move(params), std::move(dupl), range());

    std::vector<ca_expr_ptr> list;
    list.emplace_back(std::move(unknown_func));
    ca_expr_list expr_list(std::move(list), range());
    expr_list.resolve_expression_tree(context::SET_t_enum::B_TYPE);
    auto res = expr_list.evaluate(eval_ctx);

    ASSERT_TRUE(expr_list.diags().empty());
    ASSERT_TRUE(eval_ctx.diags().empty());
    ASSERT_EQ(res.access_b(), true);
}

TEST(ca_expr_list, resolve_C_type)
{
    context::hlasm_context ctx;
    attr_prov_mock attr;
    lib_prov_mock lib;
    evaluation_context eval_ctx { ctx, attr, lib };

    std::string name = "UPPER";
    auto sym = std::make_unique<ca_symbol>(&name, range());

    concat_chain value;
    value.push_back(std::make_unique<char_str_conc>("low"));
    auto str = std::make_unique<ca_string>(std::move(value), nullptr, ca_string::substring_t(), range());

    std::vector<ca_expr_ptr> list;
    list.emplace_back(std::move(sym));
    list.emplace_back(std::move(str));
    ca_expr_list expr_list(std::move(list), range());
    expr_list.resolve_expression_tree(context::SET_t_enum::C_TYPE);
    auto res = expr_list.evaluate(eval_ctx);

    ASSERT_TRUE(expr_list.diags().empty());
    ASSERT_TRUE(eval_ctx.diags().empty());
    ASSERT_EQ(res.access_c(), "LOW");
}

TEST(ca_expr_list, get_undefined_attributed_symbols)
{
    std::string name = "X";
    auto sym = std::make_unique<ca_symbol_attribute>(&name,context::data_attr_kind::L, range());

    concat_chain value;
    value.push_back(std::make_unique<char_str_conc>("low"));
    auto str = std::make_unique<ca_string>(std::move(value), nullptr, ca_string::substring_t(), range());

    std::vector<ca_expr_ptr> list;
    list.emplace_back(std::move(sym));
    list.emplace_back(std::move(str));
    ca_expr_list expr_list(std::move(list), range());

    dep_sol_mock m;
    auto res = expr_list.get_undefined_attributed_symbols(m);

    ASSERT_TRUE(res.size());
}

TEST(ca_expr_list, is_character_expression)
{
    concat_chain value;
    value.push_back(std::make_unique<char_str_conc>("low"));
    auto str = std::make_unique<ca_string>(std::move(value), nullptr, ca_string::substring_t(), range());

    std::vector<ca_expr_ptr> list;
    list.emplace_back(std::move(str));
    ca_expr_list expr_list(std::move(list), range());

    ASSERT_FALSE(expr_list.is_character_expression());
}
