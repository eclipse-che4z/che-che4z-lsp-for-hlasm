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

#include "ca_symbol.h"

#include "context/hlasm_context.h"
#include "expressions/conditional_assembly/ca_expr_visitor.h"
#include "expressions/evaluation_context.h"

namespace hlasm_plugin::parser_library::expressions {

ca_symbol::ca_symbol(context::id_index symbol, range expr_range)
    : ca_expression(context::SET_t_enum::A_TYPE, std::move(expr_range))
    , symbol(symbol)
{}

bool ca_symbol::get_undefined_attributed_symbols(std::vector<context::id_index>&, const evaluation_context&) const
{
    return false;
}

void ca_symbol::resolve_expression_tree(ca_expression_ctx expr_ctx, diagnostic_op_consumer& diags)
{
    if (expr_ctx.kind == context::SET_t_enum::C_TYPE)
        diags.add_diagnostic(diagnostic_op::error_CE004(expr_range));
}

bool ca_symbol::is_character_expression(character_expression_purpose) const { return false; }

void ca_symbol::apply(ca_expr_visitor& visitor) const { visitor.visit(*this); }

context::SET_t ca_symbol::evaluate(const evaluation_context& eval_ctx) const
{
    auto tmp_symbol = eval_ctx.hlasm_ctx.ord_ctx.get_symbol(symbol);

    if (tmp_symbol && tmp_symbol->kind() == context::symbol_value_kind::ABS)
        return tmp_symbol->value().get_abs();
    else
    {
        eval_ctx.diags.add_diagnostic(diagnostic_op::error_CE012(expr_range));
        return context::object_traits<context::A_t>::default_v();
    }
}

} // namespace hlasm_plugin::parser_library::expressions
