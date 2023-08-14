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

#include "context/hlasm_context.h"
#include "diagnostic_adder.h"
#include "expressions/conditional_assembly/terms/ca_symbol.h"
#include "expressions/evaluation_context.h"
#include "library_info_transitional.h"

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library;

TEST(ca_symbol, undefined_attributes)
{
    context::hlasm_context ctx;
    diagnostic_op_consumer_container diags;
    evaluation_context eval_ctx { ctx, library_info_transitional::empty, diags };

    ca_symbol sym(context::id_index("N"), range());

    std::set<context::id_index> references;
    EXPECT_FALSE(sym.get_undefined_attributed_symbols(references, eval_ctx));

    EXPECT_EQ(references.size(), 0U);
}

TEST(ca_symbol, resolve_expr_tree)
{
    diagnostic_adder add_diags;

    diagnostic_op_consumer_container diags;

    ca_symbol(context::id_index(), range())
        .resolve_expression_tree({ context::SET_t_enum::C_TYPE, context::SET_t_enum::C_TYPE, true }, diags);

    EXPECT_FALSE(diags.diags.empty());
}

TEST(ca_symbol, is_char)
{
    EXPECT_FALSE(
        ca_symbol(context::id_index(), range()).is_character_expression(character_expression_purpose::assignment));
    EXPECT_FALSE(ca_symbol(context::id_index(), range())
                     .is_character_expression(character_expression_purpose::left_side_of_comparison));
}
