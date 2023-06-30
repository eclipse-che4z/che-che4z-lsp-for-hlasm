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
#include "expressions/conditional_assembly/ca_operator_binary.h"
#include "expressions/conditional_assembly/terms/ca_constant.h"
#include "expressions/conditional_assembly/terms/ca_expr_list.h"
#include "expressions/conditional_assembly/terms/ca_function.h"
#include "expressions/conditional_assembly/terms/ca_string.h"
#include "expressions/conditional_assembly/terms/ca_symbol.h"
#include "expressions/conditional_assembly/terms/ca_symbol_attribute.h"
#include "expressions/evaluation_context.h"
#include "library_info_transitional.h"
#include "semantics/concatenation.h"

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library;

TEST(ca_expr_list, unknown_function_to_operator)
{
    context::hlasm_context ctx;
    diagnostic_op_consumer_container diags;
    evaluation_context eval_ctx { ctx, library_info_transitional::empty, diags };

    auto c = std::make_unique<ca_constant>(1, range());
    auto dupl = std::make_unique<ca_constant>(1, range());
    std::vector<ca_expr_ptr> params;
    params.emplace_back(std::move(c));
    auto unknown_func = std::make_unique<ca_function>(
        context::id_index("AND"), ca_expr_funcs::UNKNOWN, std::move(params), std::move(dupl), range());

    std::vector<ca_expr_ptr> list;
    list.emplace_back(std::move(unknown_func));
    ca_expr_list expr_list(std::move(list), range(), true);

    //   function  =>    operator
    // ((1)AND(1)) => ((1) AND (1))
    expr_list.resolve_expression_tree({ context::SET_t_enum::B_TYPE, context::SET_t_enum::B_TYPE, true }, diags);
    auto res = expr_list.evaluate(eval_ctx);

    EXPECT_TRUE(diags.diags.empty());
    EXPECT_EQ(res.access_b(), true);
}

TEST(ca_expr_list, resolve_C_type)
{
    context::hlasm_context ctx;
    diagnostic_op_consumer_container diags;
    evaluation_context eval_ctx { ctx, library_info_transitional::empty, diags };

    auto sym = std::make_unique<ca_symbol>(context::id_index("UPPER"), range());

    concat_chain value;
    value.emplace_back(char_str_conc("low", range()));
    auto str = std::make_unique<ca_string>(std::move(value), nullptr, ca_string::substring_t(), range());

    std::vector<ca_expr_ptr> list;
    list.emplace_back(std::move(sym));
    list.emplace_back(std::move(str));
    // (UPPER 'low')
    ca_expr_list expr_list(std::move(list), range(), true);
    expr_list.resolve_expression_tree({ context::SET_t_enum::C_TYPE, context::SET_t_enum::C_TYPE, true }, diags);
    auto res = expr_list.evaluate(eval_ctx);

    EXPECT_TRUE(diags.diags.empty());
    EXPECT_EQ(res.access_c(), "LOW");
}

TEST(ca_expr_list, get_undefined_attributed_symbols)
{
    context::hlasm_context ctx;

    auto sym =
        std::make_unique<ca_symbol_attribute>(context::id_index("X"), context::data_attr_kind::L, range(), range());

    concat_chain value;
    value.emplace_back(char_str_conc("low", range()));
    auto str = std::make_unique<ca_string>(std::move(value), nullptr, ca_string::substring_t(), range());

    std::vector<ca_expr_ptr> list;
    list.emplace_back(std::move(sym));
    list.emplace_back(std::move(str));
    // (L'X 'low')
    ca_expr_list expr_list(std::move(list), range(), true);

    diagnostic_op_consumer_container diags;
    evaluation_context eval_ctx { ctx, library_info_transitional::empty, diags };
    std::set<context::id_index> references;
    EXPECT_TRUE(expr_list.get_undefined_attributed_symbols(references, eval_ctx));

    EXPECT_GT(references.size(), 0);
}

TEST(ca_expr_list, is_character_expression)
{
    concat_chain value;
    value.emplace_back(char_str_conc("low", range()));
    auto str = std::make_unique<ca_string>(std::move(value), nullptr, ca_string::substring_t(), range());

    std::vector<ca_expr_ptr> list;
    list.emplace_back(std::move(str));
    // ('low')
    ca_expr_list expr_list(std::move(list), range(), true);

    EXPECT_TRUE(expr_list.is_character_expression(character_expression_purpose::assignment));
    EXPECT_FALSE(expr_list.is_character_expression(character_expression_purpose::left_side_of_comparison));
}

TEST(ca_expr_list, unfinished_expressions)
{
    context::hlasm_context ctx;
    // ()
    {
        diagnostic_op_consumer_container diags;
        ca_expr_list expr_list({}, range(), true);
        expr_list.resolve_expression_tree({ context::SET_t_enum::B_TYPE, context::SET_t_enum::B_TYPE, true }, diags);

        EXPECT_TRUE(matches_message_codes(diags.diags, { "CE003" }));
    }
    // (NOT)
    {
        diagnostic_op_consumer_container diags;
        auto sym = std::make_unique<ca_symbol>(context::id_index("NOT"), range());

        std::vector<ca_expr_ptr> list;
        list.emplace_back(std::move(sym));

        ca_expr_list expr_list(std::move(list), range(), true);
        expr_list.resolve_expression_tree({ context::SET_t_enum::B_TYPE, context::SET_t_enum::B_TYPE, true }, diags);

        EXPECT_TRUE(matches_message_codes(diags.diags, { "CE003" }));
    }
    // (1 AND)
    {
        diagnostic_op_consumer_container diags;
        auto c = std::make_unique<ca_constant>(1, range());

        auto sym = std::make_unique<ca_symbol>(context::id_index("AND"), range());

        std::vector<ca_expr_ptr> list;
        list.emplace_back(std::move(c));
        list.emplace_back(std::move(sym));

        ca_expr_list expr_list(std::move(list), range(), true);
        expr_list.resolve_expression_tree({ context::SET_t_enum::B_TYPE, context::SET_t_enum::B_TYPE, true }, diags);

        EXPECT_TRUE(matches_message_codes(diags.diags, { "CE003" }));
    }
    // (1 AND 1 EQ)
    {
        diagnostic_op_consumer_container diags;
        auto c = std::make_unique<ca_constant>(1, range());
        auto c2 = std::make_unique<ca_constant>(1, range());

        auto and_sym = std::make_unique<ca_symbol>(context::id_index("AND"), range());
        auto eq_sym = std::make_unique<ca_symbol>(context::id_index("EQ"), range());

        std::vector<ca_expr_ptr> list;
        list.emplace_back(std::move(c));
        list.emplace_back(std::move(and_sym));
        list.emplace_back(std::move(c2));
        list.emplace_back(std::move(eq_sym));

        ca_expr_list expr_list(std::move(list), range(), true);
        expr_list.resolve_expression_tree({ context::SET_t_enum::B_TYPE, context::SET_t_enum::B_TYPE, true }, diags);

        EXPECT_TRUE(matches_message_codes(diags.diags, { "CE003" }));
    }
}

TEST(ca_expr_list, different_return_type)
{
    // ((1 AND 1)+1)
    context::hlasm_context ctx;
    diagnostic_op_consumer_container diags;
    evaluation_context eval_ctx { ctx, library_info_transitional::empty, diags };

    std::vector<ca_expr_ptr> inner_list;
    inner_list.emplace_back(std::make_unique<ca_constant>(1, range()));
    inner_list.emplace_back(std::make_unique<ca_symbol>(context::id_index("AND"), range()));
    inner_list.emplace_back(std::make_unique<ca_constant>(1, range()));

    std::vector<ca_expr_ptr> list;
    list.emplace_back(std::make_unique<ca_basic_binary_operator<ca_add>>(
        std::make_unique<ca_expr_list>(std::move(inner_list), range(), true),
        std::make_unique<ca_constant>(1, range()),
        range()));

    ca_expr_list final_expr_list(std::move(list), range(), true);
    final_expr_list.resolve_expression_tree({ context::SET_t_enum::B_TYPE, context::SET_t_enum::B_TYPE, true }, diags);
    final_expr_list.evaluate(eval_ctx);

    EXPECT_TRUE(matches_message_codes(diags.diags, { "CE004" }));
}
