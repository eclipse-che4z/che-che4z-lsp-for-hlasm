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
#include "expressions/conditional_assembly/terms/ca_var_sym.h"
#include "expressions/evaluation_context.h"
#include "semantics/concatenation_term.h"

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library;

TEST(ca_var_sym_basic, undefined_attributes)
{
    context::hlasm_context ctx;
    lib_prov_mock lib;
    evaluation_context eval_ctx { ctx, lib };

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
    lib_prov_mock lib;
    evaluation_context eval_ctx { ctx, lib };

    std::string name = "n";
    concat_chain created_name;
    std::vector<ca_expr_ptr> subscript;

    created_name.push_back(std::make_unique<char_str_conc>("n"));

    subscript.push_back(std::make_unique<ca_constant>(1, range()));

    auto vs = std::make_unique<created_variable_symbol>(std::move(created_name), std::move(subscript), range());

    ca_var_sym var(std::move(vs), range());

    auto res = var.get_undefined_attributed_symbols(eval_ctx);

    ASSERT_EQ(res.size(), 0U);
}
