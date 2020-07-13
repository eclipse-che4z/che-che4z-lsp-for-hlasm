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
#include "context/ordinary_assembly/dependable.h"
#include "expressions/evaluation_context.h"
#include "processing/context_manager.h"

namespace hlasm_plugin::parser_library::expressions {

context::SET_t_enum get_attribute_type(context::data_attr_kind attr)
{
    switch (attr)
    {
        case context::data_attr_kind::T:
        case context::data_attr_kind::O:
            return context::SET_t_enum::C_TYPE;
        case context::data_attr_kind::L:
        case context::data_attr_kind::S:
        case context::data_attr_kind::I:
        case context::data_attr_kind::K:
        case context::data_attr_kind::N:
        case context::data_attr_kind::D:
            return context::SET_t_enum::A_TYPE;
        default:
            return context::SET_t_enum::UNDEF_TYPE;
    }
}

ca_symbol_attribute::ca_symbol_attribute(context::id_index symbol, context::data_attr_kind attribute, range expr_range)
    : ca_expression(get_attribute_type(attribute), std::move(expr_range))
    , attribute(attribute)
    , symbol(symbol)

{}

ca_symbol_attribute::ca_symbol_attribute(semantics::vs_ptr symbol, context::data_attr_kind attribute, range expr_range)
    : ca_expression(get_attribute_type(attribute), std::move(expr_range))
    , attribute(attribute)
    , symbol(std::move(symbol))
{}

undef_sym_set ca_symbol_attribute::get_undefined_attributed_symbols(const context::dependency_solver& solver) const
{
    if (!context::symbol_attributes::is_ordinary_attribute(attribute))
        return undef_sym_set();

    if (std::holds_alternative<context::id_index>(symbol))
    {
        if (!solver.get_symbol(std::get<context::id_index>(symbol)))
            return { std::get<context::id_index>(symbol) };
        return undef_sym_set();
    }
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
        ca_var_sym::resolve_expression_tree_vs(std::get<semantics::vs_ptr>(symbol));
}

void ca_symbol_attribute::collect_diags() const
{
    if (std::holds_alternative<semantics::vs_ptr>(symbol))
    {
        auto&& sym = std::get<semantics::vs_ptr>(symbol);
        for (auto&& expr : sym->subscript)
            collect_diags_from_child(*expr);
    }
}

bool ca_symbol_attribute::is_character_expression() const
{
    return get_attribute_type(attribute) == context::SET_t_enum::C_TYPE;
}

context::SET_t ca_symbol_attribute::evaluate(evaluation_context& eval_ctx) const
{
    if (std::holds_alternative<context::id_index>(symbol))
    {
        return evaluate_ordsym(std::get<context::id_index>(symbol), eval_ctx);
    }

    if (std::holds_alternative<semantics::vs_ptr>(symbol))
    {
        return evaluate_varsym(std::get<semantics::vs_ptr>(symbol), eval_ctx);
    }

    return context::SET_t(expr_kind);
}

context::SET_t ca_symbol_attribute::get_ordsym_attr_value(context::id_index name, evaluation_context& eval_ctx) const
{
    auto ord_symbol = eval_ctx.hlasm_ctx.ord_ctx.get_symbol(name);

    if (!ord_symbol)
    {
        auto found = eval_ctx.attr_provider.lookup_forward_attribute_references({ name });

        if (auto it = found.find(name); it != found.end())
            return retrieve_value(&it->second, eval_ctx);
    }
    return retrieve_value(ord_symbol, eval_ctx);
}

context::SET_t ca_symbol_attribute::retrieve_value(const context::symbol* ord_symbol, evaluation_context& eval_ctx) const
{
    if (attribute == context::data_attr_kind::T)
        return eval_ctx.hlasm_ctx.get_attribute_value_ca(attribute, ord_symbol);

    if (!ord_symbol)
    {
        eval_ctx.add_diagnostic(diagnostic_op::warning_W013(expr_range));
        return context::symbol_attributes::default_value(attribute);
    }

    if ((attribute == context::data_attr_kind::S || attribute == context::data_attr_kind::I)
        && !ord_symbol->attributes().can_have_SI_attr())
    {
        eval_ctx.add_diagnostic(diagnostic_op::error_E066(expr_range));
        return context::symbol_attributes::default_value(attribute);
    }

    if (!ord_symbol->attributes().is_defined(attribute))
    {
        eval_ctx.add_diagnostic(diagnostic_op::warning_W013(expr_range));
        return context::symbol_attributes::default_value(attribute);
    }

    return eval_ctx.hlasm_ctx.get_attribute_value_ca(attribute, ord_symbol);
}

context::SET_t ca_symbol_attribute::evaluate_ordsym(context::id_index name, evaluation_context& eval_ctx) const
{
    if (context::symbol_attributes::is_ordinary_attribute(attribute))
    {
        return get_ordsym_attr_value(name, eval_ctx);
    }
    else if (attribute == context::data_attr_kind::D)
    {
        return eval_ctx.hlasm_ctx.get_attribute_value_ca(attribute, name);
    }
    else if (attribute == context::data_attr_kind::O)
    {
        auto tmp = eval_ctx.hlasm_ctx.get_attribute_value_ca(attribute, name);
        if (tmp.access_c() == "U" && eval_ctx.lib_provider.has_library(*name, eval_ctx.hlasm_ctx))
            return std::string("S");
        return tmp;
    }
    else
    {
        eval_ctx.add_diagnostic(diagnostic_op::error_E066(expr_range));
        return context::symbol_attributes::default_ca_value(attribute);
    }
}

std::vector<size_t> transform(const std::vector<context::A_t>& v)
{
    std::vector<size_t> ret;
    for (auto val : v)
        ret.push_back((size_t)val);
    return ret;
}

context::SET_t ca_symbol_attribute::evaluate_varsym(const semantics::vs_ptr& vs, evaluation_context& eval_ctx) const
{
    processing::context_manager mngr(&eval_ctx);

    auto [var_name, expr_subscript] = vs->evaluate_symbol(eval_ctx);

    // get symbol
    auto var_symbol = eval_ctx.hlasm_ctx.get_var_sym(var_name);

    if (!var_symbol)
    {
        eval_ctx.add_diagnostic(diagnostic_op::error_E010("variable", vs->symbol_range));
        return context::symbol_attributes::default_ca_value(attribute);
    }

    // must substitute var sym
    if (context::symbol_attributes::requires_ordinary_symbol(attribute))
    {
        return evaluate_substituted(var_name, std::move(expr_subscript), vs->symbol_range, eval_ctx);
    }
    else if (attribute == context::data_attr_kind::T)
    {
        if (!mngr.test_symbol_for_read(var_symbol, expr_subscript, vs->symbol_range))
            return std::string("U");

        context::SET_t value =
            eval_ctx.hlasm_ctx.get_attribute_value_ca(attribute, var_symbol, transform(expr_subscript)).access_c();

        if (value.access_c() != "U")
            return value;
        return evaluate_substituted(
            var_name, std::move(expr_subscript), vs->symbol_range, eval_ctx); // is type U, must substitute var sym
    }
    else
    {
        if (attribute == context::data_attr_kind::K
            && !mngr.test_symbol_for_read(var_symbol, expr_subscript, vs->symbol_range))
            return context::symbol_attributes::default_ca_value(attribute);
        return eval_ctx.hlasm_ctx.get_attribute_value_ca(attribute, var_symbol, transform(expr_subscript));
    }
}

context::SET_t ca_symbol_attribute::evaluate_substituted(context::id_index var_name,
    std::vector<context::A_t> expr_subscript,
    range var_range,
    evaluation_context& eval_ctx) const
{
    processing::context_manager mngr(&eval_ctx);

    context::SET_t substituted_name = mngr.get_var_sym_value(var_name, expr_subscript, var_range);

    if (substituted_name.type != context::SET_t_enum::C_TYPE)
    {
        if (attribute != context::data_attr_kind::O && attribute != context::data_attr_kind::T)
            eval_ctx.add_diagnostic(diagnostic_op::error_E066(expr_range));
        return context::symbol_attributes::default_ca_value(attribute);
    }

    auto [valid, ord_name] = mngr.try_get_symbol_name(substituted_name.access_c());

    if (!valid)
    {
        if (attribute != context::data_attr_kind::O && attribute != context::data_attr_kind::T)
            eval_ctx.add_diagnostic(diagnostic_op::error_E065(expr_range));
        return context::symbol_attributes::default_ca_value(attribute);
    }
    else
        return evaluate_ordsym(ord_name, eval_ctx);
}

} // namespace hlasm_plugin
