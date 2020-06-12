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

#include "processing/context_manager.h"
#include "semantics/concatenation_term.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

ca_var_sym::ca_var_sym(semantics::vs_ptr symbol, range expr_range)
    : ca_expression(context::SET_t_enum::A_TYPE, std::move(expr_range))
    , symbol(std::move(symbol))
{ }

undef_sym_set ca_var_sym::get_undefined_attributed_symbols_vs(
    const semantics::vs_ptr& symbol, const context::dependency_solver& solver)
{
    undef_sym_set tmp;
    for (auto&& expr : symbol->subscript)
        tmp.merge(expr->get_undefined_attributed_symbols(solver));

    if (symbol->created)
    {
        auto created = symbol->access_created();
        for (auto&& point : created->created_name)
            if (point->type == semantics::concat_type::VAR)
                tmp.merge(get_undefined_attributed_symbols_vs(point->access_var()->symbol, solver));
    }
    return tmp;
}

void ca_var_sym::resolve_expression_tree_vs(const semantics::vs_ptr& symbol) { }

undef_sym_set ca_var_sym::get_undefined_attributed_symbols(const context::dependency_solver& solver) const
{
    return get_undefined_attributed_symbols_vs(symbol, solver);
}

void ca_var_sym::resolve_expression_tree(context::SET_t_enum) { resolve_expression_tree_vs(symbol); }

void ca_var_sym::collect_diags() const
{
    for (auto&& expr : symbol->subscript)
        collect_diags_from_child(*expr);
}

bool ca_var_sym::is_character_expression() const { return false; }

context::SET_t ca_var_sym::evaluate(evaluation_context& eval_ctx) const { return symbol->evaluate(eval_ctx); }

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin
