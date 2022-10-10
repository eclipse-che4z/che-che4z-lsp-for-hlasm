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
#include "expressions/conditional_assembly/terms/ca_constant.h"
#include "expressions/conditional_assembly/terms/ca_var_sym.h"
#include "expressions/evaluation_context.h"
#include "library_info_transitional.h"
#include "semantics/concatenation.h"

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library;

TEST(ca_var_sym_basic, undefined_attributes)
{
    diagnostic_op_consumer_container diags;
    context::hlasm_context ctx;
    evaluation_context eval_ctx { ctx, library_info_transitional::empty, diags };

    std::string name = "n";
    std::vector<ca_expr_ptr> subscript;

    subscript.push_back(std::make_unique<ca_constant>(1, range()));

    auto vs = std::make_unique<basic_variable_symbol>(&name, std::move(subscript), range());

    ca_var_sym var(std::move(vs), range());

    auto res = var.get_undefined_attributed_symbols(eval_ctx);

    ASSERT_EQ(res.size(), 0U);
}

TEST(ca_var_sym_created, undefined_attributes)
{
    context::hlasm_context ctx;
    diagnostic_op_consumer_container diags;
    evaluation_context eval_ctx { ctx, library_info_transitional::empty, diags };

    std::string name = "n";
    concat_chain created_name;
    std::vector<ca_expr_ptr> subscript;

    created_name.emplace_back(char_str_conc("n", range()));

    subscript.push_back(std::make_unique<ca_constant>(1, range()));

    auto vs = std::make_unique<created_variable_symbol>(std::move(created_name), std::move(subscript), range());

    ca_var_sym var(std::move(vs), range());

    auto res = var.get_undefined_attributed_symbols(eval_ctx);

    ASSERT_EQ(res.size(), 0U);
}

TEST(ca_var_sym, invalid_definitions)
{
    std::string input = R"(
    GBLC (([
    GBLC (([
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002", "S0002" }));
}
