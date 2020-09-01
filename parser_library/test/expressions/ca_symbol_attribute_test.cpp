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
    context::hlasm_context ctx;
    lib_prov_mock lib;
    evaluation_context eval_ctx { ctx, lib };

    std::string name = "n";
    std::vector<ca_expr_ptr> subscript;

    subscript.push_back(std::make_unique<ca_constant>(1, range()));

    auto vs = std::make_unique<basic_variable_symbol>(&name, std::move(subscript), range());

    ca_symbol_attribute attr(std::move(vs), context::data_attr_kind::D, range());

    auto res = attr.get_undefined_attributed_symbols(eval_ctx);

    ASSERT_EQ(res.size(), 0U);
}

ca_symbol_attribute create_var_sym_attr(context::data_attr_kind kind, context::id_storage& ids)
{
    auto name = ids.add("n");

    auto vs = std::make_unique<basic_variable_symbol>(name, std::vector<ca_expr_ptr> {}, range());

    return ca_symbol_attribute(std::move(vs), kind, range());
}

TEST(ca_symbol_attr, evaluate_undef_varsym)
{
    context::hlasm_context ctx;
    lib_prov_mock lib;
    evaluation_context eval_ctx { ctx, lib };

    auto res = create_var_sym_attr(context::data_attr_kind::D, ctx.ids()).evaluate(eval_ctx);

    ASSERT_EQ(eval_ctx.diags().size(), 1U);
    EXPECT_EQ(eval_ctx.diags().front().code, "E010");
}

TEST(ca_symbol_attr, evaluate_substituted_varsym_not_char)
{
    context::hlasm_context ctx;
    lib_prov_mock lib;
    evaluation_context eval_ctx { ctx, lib };

    auto name = ctx.ids().add("n");

    auto var = ctx.create_local_variable<int>(name, true);
    var->access_set_symbol<int>()->set_value(12);

    auto res = create_var_sym_attr(context::data_attr_kind::L, ctx.ids()).evaluate(eval_ctx);

    ASSERT_EQ(eval_ctx.diags().size(), 1U);
    EXPECT_EQ(eval_ctx.diags().front().code, "E066");
}

TEST(ca_symbol_attr, evaluate_substituted_varsym_char_not_sym)
{
    context::hlasm_context ctx;
    lib_prov_mock lib;
    evaluation_context eval_ctx { ctx, lib };

    auto name = ctx.ids().add("n");

    auto var = ctx.create_local_variable<context::C_t>(name, true);
    var->access_set_symbol<context::C_t>()->set_value("(abc");

    auto res = create_var_sym_attr(context::data_attr_kind::L, ctx.ids()).evaluate(eval_ctx);

    ASSERT_EQ(eval_ctx.diags().size(), 1U);
    EXPECT_EQ(eval_ctx.diags().front().code, "E065");
}
