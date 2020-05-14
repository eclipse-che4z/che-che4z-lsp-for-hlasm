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

#include "ca_expr_term.h"

#include "semantics/concatenation_term.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

ca_expr_list::ca_expr_list(std::vector<ca_expr_ptr> expr_list)
    : expr_list(std::move(expr_list))
{}

undef_sym_set ca_expr_list::get_undefined_attributed_symbols(const context::dependency_solver& solver) const
{
    undef_sym_set tmp;
    for (auto&& expr : expr_list)
        tmp.merge(expr->get_undefined_attributed_symbols(solver));
    return tmp;
}

void ca_expr_list::resolve_expression_tree(context::SET_t_enum ) {}

ca_string::substring_t::substring_t()
    : start(nullptr)
    , count(nullptr)
    , to_end(false)
{}

ca_string::ca_string(semantics::concat_chain value, ca_expr_ptr duplication_factor, substring_t substring)
    : value(std::move(value))
    , duplication_factor(std::move(duplication_factor))
    , substring(std::move(substring))
{}

undef_sym_set ca_string::get_undefined_attributed_symbols(const context::dependency_solver& solver) const
{
    undef_sym_set tmp;
    if (duplication_factor)
        tmp = duplication_factor->get_undefined_attributed_symbols(solver);
    if (substring.start)
        tmp.merge(substring.start->get_undefined_attributed_symbols(solver));
    if (substring.count)
        tmp.merge(substring.count->get_undefined_attributed_symbols(solver));
    return tmp;
}

void ca_string::resolve_expression_tree(context::SET_t_enum ) {}

ca_var_sym::ca_var_sym(semantics::vs_ptr symbol)
    : symbol(std::move(symbol))
{}

undef_sym_set get_undefined_attributed_symbols_vs(
    const semantics::vs_ptr& symbol, const context::dependency_solver& solver)
{
    undef_sym_set tmp;
    // for (auto&& expr : symbol->subscript)
    //    tmp.merge(expr->get_undefined_attributed_symbols_vs(solver));

    if (symbol->created)
    {
        auto created = symbol->access_created();
        for (auto&& point : created->created_name)
            if (point->type == semantics::concat_type::VAR)
                tmp.merge(get_undefined_attributed_symbols_vs(point->access_var()->symbol, solver));
    }
    return tmp;
}

undef_sym_set ca_var_sym::get_undefined_attributed_symbols(const context::dependency_solver& solver) const
{
    return get_undefined_attributed_symbols_vs(symbol, solver);
}

void ca_var_sym::resolve_expression_tree(context::SET_t_enum ) {}

ca_constant::ca_constant(context::A_t value)
    : value(value)
{}

undef_sym_set ca_constant::get_undefined_attributed_symbols(const context::dependency_solver&) const
{
    return undef_sym_set();
}

void ca_constant::resolve_expression_tree(context::SET_t_enum ) {}

ca_symbol::ca_symbol(context::id_index symbol)
    : symbol(symbol)
{}

undef_sym_set ca_symbol::get_undefined_attributed_symbols(const context::dependency_solver&) const
{
    return undef_sym_set();
}

void ca_symbol::resolve_expression_tree(context::SET_t_enum ) {}

ca_symbol_attribute::ca_symbol_attribute(context::id_index symbol, context::data_attr_kind attribute)
    : attribute(attribute)
    , symbol(symbol)

{}

ca_symbol_attribute::ca_symbol_attribute(semantics::vs_ptr symbol, context::data_attr_kind attribute)
    : attribute(attribute)
    , symbol(std::move(symbol))
{}

undef_sym_set ca_symbol_attribute::get_undefined_attributed_symbols(const context::dependency_solver& solver) const
{
    if (!context::symbol_attributes::ordinary_allowed(attribute))
        return undef_sym_set();

    if (std::holds_alternative<context::id_index>(symbol))
        return { std::get<context::id_index>(symbol) };
    else if (std::holds_alternative<semantics::vs_ptr>(symbol))
        return get_undefined_attributed_symbols_vs(std::get<semantics::vs_ptr>(symbol), solver);
    else
    {
        assert(false);
        return undef_sym_set();
    }
}

void ca_symbol_attribute::resolve_expression_tree(context::SET_t_enum ) {}

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin
