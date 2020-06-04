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

#include "ca_symbol_attribute.h"

#include "ca_var_sym.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {


ca_symbol_attribute::ca_symbol_attribute(context::id_index symbol, context::data_attr_kind attribute, range expr_range)
    : ca_expression(attribute == context::data_attr_kind::T ? context::SET_t_enum::C_TYPE : context::SET_t_enum::A_TYPE,
        std::move(expr_range))
    , attribute(attribute)
    , symbol(symbol)

{ }

ca_symbol_attribute::ca_symbol_attribute(semantics::vs_ptr symbol, context::data_attr_kind attribute, range expr_range)
    : ca_expression(attribute == context::data_attr_kind::T ? context::SET_t_enum::C_TYPE : context::SET_t_enum::A_TYPE,
        std::move(expr_range))
    , attribute(attribute)
    , symbol(std::move(symbol))
{ }

undef_sym_set ca_symbol_attribute::get_undefined_attributed_symbols(const context::dependency_solver& solver) const
{
    if (!context::symbol_attributes::ordinary_allowed(attribute))
        return undef_sym_set();

    if (std::holds_alternative<context::id_index>(symbol))
        return { std::get<context::id_index>(symbol) };
    else if (std::holds_alternative<semantics::vs_ptr>(symbol))
        return ca_var_sym::get_undefined_attributed_symbols_vs(std::get<semantics::vs_ptr>(symbol), solver);
    else
    {
        assert(false);
        return undef_sym_set();
    }
}

void ca_symbol_attribute::resolve_expression_tree(context::SET_t_enum kind)
{
    if (kind == context::SET_t_enum::C_TYPE && kind != expr_kind)
        add_diagnostic(diagnostic_op::error_CE004(expr_range));
    else if (std::holds_alternative<semantics::vs_ptr>(symbol))
    {
        // auto&& sym = std::get<semantics::vs_ptr>(symbol);
        // for (auto&& expr : sym->subscript)
        //    expr->resolve_expression_tree(context::SET_t_enum::SETA_type);
    }
}

void ca_symbol_attribute::collect_diags() const
{
    if (std::holds_alternative<semantics::vs_ptr>(symbol))
    {
        // auto&& sym = std::get<semantics::vs_ptr>(symbol);
        // for (auto&& expr : sym->subscript)
        //    collect_diags_from_child(*expr);
    }
}

bool ca_symbol_attribute::is_character_expression() const { return false; }

context::SET_t ca_symbol_attribute::evaluate(evaluation_context& eval_ctx) const { return context::SET_t(); }

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin
