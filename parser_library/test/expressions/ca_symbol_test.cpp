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

#include "diagnostic_adder.h"
#include "expr_mocks.h"
#include "expressions/conditional_assembly/terms/ca_symbol.h"

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library;

TEST(ca_symbol, undefined_attributes)
{
    dep_sol_mock m;
    std::string name = "n";

    ca_symbol sym(&name, range());

    auto res = sym.get_undefined_attributed_symbols(m);

    ASSERT_EQ(res.size(), 0U);
}

TEST(ca_symbol, resolve_expr_tree)
{
    diagnostic_adder add_diags;

    ca_symbol sym(nullptr, range());

    sym.resolve_expression_tree(context::SET_t_enum::C_TYPE);

    ASSERT_NE(sym.diags().size(), 0U);
}

TEST(ca_symbol, is_char) { ASSERT_FALSE(ca_symbol(nullptr, range()).is_character_expression()); }
