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

#include "variable_symbol.h"

#include "concatenation.h"
#include "context/hlasm_context.h"
#include "expressions/evaluation_context.h"

namespace hlasm_plugin::parser_library::semantics {

context::id_index variable_symbol::evaluate_name(const expressions::evaluation_context& eval_ctx) const
{
    if (const auto* name = named())
        return *name;

    auto str_name = concatenation_point::evaluate(*created(), eval_ctx);

    auto [valid, id] = eval_ctx.hlasm_ctx.try_get_symbol_name(str_name);
    if (!valid)
        eval_ctx.diags.add_diagnostic(diagnostic_op::error_E065(symbol_range));

    return id;
}

void variable_symbol::resolve(context::SET_t_enum parent_expr_kind, diagnostic_op_consumer& diag)
{
    if (const auto* created_name = created())
    {
        for (const auto& c : *created_name)
            c.resolve(diag);
    }

    const expressions::ca_expression_ctx expr_ctx = {
        context::SET_t_enum::A_TYPE,
        parent_expr_kind == context::SET_t_enum::B_TYPE ? parent_expr_kind : context::SET_t_enum::A_TYPE,
        true,
    };

    for (const auto& v : subscript)
        v->resolve_expression_tree(expr_ctx, diag);
}

vs_eval variable_symbol::evaluate_symbol(const expressions::evaluation_context& eval_ctx) const
{
    return vs_eval(evaluate_name(eval_ctx), evaluate_subscript(eval_ctx));
}

std::vector<context::A_t> variable_symbol::evaluate_subscript(const expressions::evaluation_context& eval_ctx) const
{
    std::vector<context::A_t> eval_subscript;
    for (const auto& expr : subscript)
    {
        auto val = expr->evaluate<context::A_t>(eval_ctx);
        eval_subscript.push_back(val);
    }

    return eval_subscript;
}

context::SET_t variable_symbol::evaluate(const expressions::evaluation_context& eval_ctx) const
{
    auto [name, evaluated_subscript] = evaluate_symbol(eval_ctx);

    auto val = get_var_sym_value(eval_ctx, name, evaluated_subscript, symbol_range);

    return val;
}

} // namespace hlasm_plugin::parser_library::semantics
