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

#include "ca_var_sym.h"

#include "ca_constant.h"
#include "expressions/conditional_assembly/ca_expr_visitor.h"
#include "expressions/evaluation_context.h"
#include "semantics/concatenation.h"

namespace hlasm_plugin::parser_library::expressions {

ca_var_sym::ca_var_sym(semantics::vs_ptr symbol, range expr_range)
    : ca_expression(context::SET_t_enum::A_TYPE, std::move(expr_range))
    , symbol(std::move(symbol))
{}

bool ca_var_sym::get_undefined_attributed_symbols_vs(
    std::vector<context::id_index>& symbols, const semantics::vs_ptr& symbol, const evaluation_context& eval_ctx)
{
    bool result = false;

    for (auto&& expr : symbol->subscript)
        result |= expr->get_undefined_attributed_symbols(symbols, eval_ctx);

    if (symbol->created)
    {
        auto created = symbol->access_created();
        for (auto&& point : created->created_name)
            if (const auto* var = std::get_if<semantics::var_sym_conc>(&point.value))
                result |= get_undefined_attributed_symbols_vs(symbols, var->symbol, eval_ctx);
    }
    return result;
}

bool ca_var_sym::get_undefined_attributed_symbols(
    std::vector<context::id_index>& symbols, const evaluation_context& eval_ctx) const
{
    return get_undefined_attributed_symbols_vs(symbols, symbol, eval_ctx);
}

void ca_var_sym::resolve_expression_tree(ca_expression_ctx expr_ctx, diagnostic_op_consumer& diags)
{
    // this conversion request indicates that the variable was used without the mandatory quotes around it
    if (expr_ctx.kind == context::SET_t_enum::C_TYPE)
        diags.add_diagnostic(diagnostic_op::error_CE017_character_expression_expected(expr_range));
    expr_kind = expr_ctx.kind;
    symbol->resolve(expr_ctx.parent_expr_kind, diags);
}

bool ca_var_sym::is_character_expression(character_expression_purpose) const { return false; }

void ca_var_sym::apply(ca_expr_visitor& visitor) const { visitor.visit(*this); }

context::SET_t ca_var_sym::evaluate(const evaluation_context& eval_ctx) const
{
    return convert_return_types(symbol->evaluate(eval_ctx), expr_kind, eval_ctx);
}

context::SET_t ca_var_sym::convert_return_types(
    context::SET_t retval, context::SET_t_enum type, const evaluation_context& eval_ctx) const
{
    if (retval.type() == context::SET_t_enum::C_TYPE)
    {
        diagnostic_adder add_diags(eval_ctx.diags, expr_range);
        switch (type)
        {
            case context::SET_t_enum::A_TYPE:
                // empty string is convertible to 0, but it is not a self-def term
                if (const auto& val_c = retval.access_c(); val_c.size())
                    return ca_constant::self_defining_term_or_abs_symbol(val_c, eval_ctx, expr_range);
                else
                    return 0;

            case context::SET_t_enum::B_TYPE:
                // empty string is convertible to false, but it is not a self-def term
                if (const auto& val_c = retval.access_c(); val_c.size())
                    return !!ca_constant::self_defining_term_or_abs_symbol(val_c, eval_ctx, expr_range);
                else
                    return false;

            case context::SET_t_enum::C_TYPE:
                return retval;
            default:
                return context::SET_t(expr_kind);
        }
    }
    else if (retval.type() == context::SET_t_enum::B_TYPE && type == context::SET_t_enum::A_TYPE)
    {
        retval = context::SET_t(retval.access_b() ? 1 : 0);
    }
    else if (retval.type() == context::SET_t_enum::A_TYPE && type == context::SET_t_enum::B_TYPE)
    {
        retval = context::SET_t(!!retval.access_a());
    }
    return retval;
}

} // namespace hlasm_plugin::parser_library::expressions
