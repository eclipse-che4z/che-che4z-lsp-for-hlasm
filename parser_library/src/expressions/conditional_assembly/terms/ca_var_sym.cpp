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
#include "processing/context_manager.h"
#include "semantics/concatenation_term.h"

namespace hlasm_plugin::parser_library::expressions {

ca_var_sym::ca_var_sym(semantics::vs_ptr symbol, range expr_range)
    : ca_expression(context::SET_t_enum::A_TYPE, std::move(expr_range))
    , symbol(std::move(symbol))
{}

undef_sym_set ca_var_sym::get_undefined_attributed_symbols_vs(
    const semantics::vs_ptr& symbol, const evaluation_context& eval_ctx)
{
    undef_sym_set tmp;
    for (auto&& expr : symbol->subscript)
        tmp.merge(expr->get_undefined_attributed_symbols(eval_ctx));

    if (symbol->created)
    {
        auto created = symbol->access_created();
        for (auto&& point : created->created_name)
            if (point->type == semantics::concat_type::VAR)
                tmp.merge(get_undefined_attributed_symbols_vs(point->access_var()->symbol, eval_ctx));
    }
    return tmp;
}

void ca_var_sym::resolve_expression_tree_vs(const semantics::vs_ptr&)
{
    // already resolved in parser
}

undef_sym_set ca_var_sym::get_undefined_attributed_symbols(const evaluation_context& eval_ctx) const
{
    return get_undefined_attributed_symbols_vs(symbol, eval_ctx);
}

void ca_var_sym::resolve_expression_tree(context::SET_t_enum kind)
{
    expr_kind = kind;
    resolve_expression_tree_vs(symbol);
}

void ca_var_sym::collect_diags() const
{
    for (auto&& expr : symbol->subscript)
        collect_diags_from_child(*expr);
}

bool ca_var_sym::is_character_expression() const { return false; }

void ca_var_sym::apply(ca_expr_visitor& visitor) const { visitor.visit(*this); }

context::SET_t ca_var_sym::evaluate(const evaluation_context& eval_ctx) const
{
    return convert_return_types(symbol->evaluate(eval_ctx), expr_kind, eval_ctx);
}

context::SET_t ca_var_sym::convert_return_types(
    context::SET_t retval, context::SET_t_enum type, const evaluation_context& eval_ctx) const
{
    if (retval.type == context::SET_t_enum::C_TYPE)
    {
        diagnostic_adder add_diags(&eval_ctx, expr_range);
        switch (type)
        {
            case context::SET_t_enum::A_TYPE:
                // empty string is convertible to 0, but it is not a self-def term
                if (const auto& val_c = retval.access_c(); val_c.size())
                    return ca_constant::self_defining_term(val_c, add_diags);
                else
                    return 0;

            case context::SET_t_enum::B_TYPE:
                // empty string is convertible to false, but it is not a self-def term
                if (const auto& val_c = retval.access_c(); val_c.size())
                    return !!ca_constant::self_defining_term(val_c, add_diags);
                else
                    return false;

            case context::SET_t_enum::C_TYPE:
                return retval;
            default:
                return context::SET_t(expr_kind);
        }
    }
    else if (retval.type == context::SET_t_enum::B_TYPE && type == context::SET_t_enum::A_TYPE)
    {
        retval.type = context::SET_t_enum::A_TYPE;
    }
    else if (retval.type == context::SET_t_enum::A_TYPE && type == context::SET_t_enum::B_TYPE)
    {
        retval.type = context::SET_t_enum::B_TYPE;
    }
    return retval;
}

} // namespace hlasm_plugin::parser_library::expressions
