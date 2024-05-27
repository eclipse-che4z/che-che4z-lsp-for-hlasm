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

#include <span>

#include "ca_constant.h"
#include "ca_var_sym.h"
#include "context/hlasm_context.h"
#include "context/literal_pool.h"
#include "context/ordinary_assembly/ordinary_assembly_dependency_solver.h"
#include "context/variables/set_symbol.h"
#include "diagnostic_consumer.h"
#include "ebcdic_encoding.h"
#include "expressions/conditional_assembly/ca_expr_visitor.h"
#include "expressions/evaluation_context.h"
#include "lexing/lexer.h"
#include "parsing/parser_impl.h"
#include "processing/op_code.h"
#include "semantics/range_provider.h"
#include "semantics/statement_fields.h"
#include "utils/similar.h"

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

ca_symbol_attribute::ca_symbol_attribute(
    context::id_index symbol, context::data_attr_kind attribute, range expr_range, range symbol_rng)
    : ca_expression(get_attribute_type(attribute), std::move(expr_range))
    , attribute(attribute)
    , symbol(symbol)
    , symbol_range(symbol_rng)
{}

ca_symbol_attribute::ca_symbol_attribute(
    semantics::vs_ptr symbol, context::data_attr_kind attribute, range expr_range, range symbol_rng)
    : ca_expression(get_attribute_type(attribute), std::move(expr_range))
    , attribute(attribute)
    , symbol(std::move(symbol))
    , symbol_range(symbol_rng)
{
    is_t_attr_var = attribute == context::data_attr_kind::T;
}

ca_symbol_attribute::ca_symbol_attribute(
    semantics::literal_si lit, context::data_attr_kind attribute, range expr_range, range symbol_rng)
    : ca_expression(get_attribute_type(attribute), std::move(expr_range))
    , attribute(attribute)
    , symbol(std::move(lit))
    , symbol_range(symbol_rng)
{}

bool ca_symbol_attribute::get_undefined_attributed_symbols(
    std::vector<context::id_index>& symbols, const evaluation_context& eval_ctx) const
{
    if (std::holds_alternative<context::id_index>(symbol))
    {
        if (context::symbol_attributes::is_ordinary_attribute(attribute)
            && !eval_ctx.hlasm_ctx.ord_ctx.get_symbol(std::get<context::id_index>(symbol))
            && !eval_ctx.hlasm_ctx.ord_ctx.get_symbol_reference(std::get<context::id_index>(symbol)))
        {
            symbols.emplace_back(std::get<context::id_index>(symbol));
            return true;
        }
    }
    else if (std::holds_alternative<semantics::vs_ptr>(symbol))
    {
        const auto& vs = std::get<semantics::vs_ptr>(symbol);

        bool added = ca_var_sym::get_undefined_attributed_symbols_vs(symbols, vs, eval_ctx);

        if (!added && context::symbol_attributes::is_ordinary_attribute(attribute))
        {
            context::SET_t substituted_name = vs->evaluate(eval_ctx);

            if (substituted_name.type() != context::SET_t_enum::C_TYPE)
                return false;

            auto [valid, ord_name] =
                eval_ctx.hlasm_ctx.try_get_symbol_name(try_extract_leading_symbol(substituted_name.access_c()));

            if (!valid)
                return false;

            if (context::symbol_attributes::is_ordinary_attribute(attribute)
                && !eval_ctx.hlasm_ctx.ord_ctx.get_symbol(ord_name)
                && !eval_ctx.hlasm_ctx.ord_ctx.get_symbol_reference(ord_name))
            {
                symbols.emplace_back(ord_name);
                return true;
            }
        }
        return added;
    }
    else if (std::holds_alternative<semantics::literal_si>(symbol))
    {
        // everything needs to be defined
    }
    else
    {
        assert(false);
    }
    return false;
}

void ca_symbol_attribute::resolve_expression_tree(ca_expression_ctx expr_ctx, diagnostic_op_consumer& diags)
{
    if (expr_ctx.kind == context::SET_t_enum::C_TYPE && expr_ctx.kind != expr_kind)
        diags.add_diagnostic(diagnostic_op::error_CE004(expr_range));
    else if (std::holds_alternative<semantics::vs_ptr>(symbol))
        std::get<semantics::vs_ptr>(symbol)->resolve(expr_ctx.parent_expr_kind, diags);
}

bool ca_symbol_attribute::is_character_expression(character_expression_purpose) const
{
    return get_attribute_type(attribute) == context::SET_t_enum::C_TYPE;
}

void ca_symbol_attribute::apply(ca_expr_visitor& visitor) const { visitor.visit(*this); }

context::SET_t ca_symbol_attribute::evaluate(const evaluation_context& eval_ctx) const
{
    if (std::holds_alternative<context::id_index>(symbol))
    {
        return evaluate_ordsym(std::get<context::id_index>(symbol), eval_ctx);
    }

    if (std::holds_alternative<semantics::vs_ptr>(symbol))
    {
        return evaluate_varsym(std::get<semantics::vs_ptr>(symbol), eval_ctx);
    }

    if (std::holds_alternative<semantics::literal_si>(symbol))
    {
        return evaluate_literal(std::get<semantics::literal_si>(symbol), eval_ctx);
    }

    return context::SET_t(expr_kind);
}
std::string_view ca_symbol_attribute::try_extract_leading_symbol(std::string_view expr)
{
    // remove parentheses
    while (!expr.empty() && expr.front() == '(' && expr.back() == ')')
    {
        expr.remove_prefix(1);
        expr.remove_suffix(1);
    }

    // remove leading using prefixes
    for (auto p = expr.find_first_of('.'); p != std::string_view::npos && !std::isdigit((unsigned char)expr.front())
         && std::all_of(expr.begin(), expr.begin() + p, lexing::lexer::ord_char);
         p = expr.find_first_of('.'))
        expr.remove_prefix(p + 1);

    // try to isolate one ordinary symbol
    if (!expr.empty() && !std::isdigit((unsigned char)expr.front()) && lexing::lexer::ord_char(expr.front()))
    {
        if (auto d = expr.find_first_of("+-*/()"); d != std::string_view::npos)
            expr = expr.substr(0, d);
    }
    return expr;
}

context::SET_t ca_symbol_attribute::retrieve_value(
    const context::symbol* ord_symbol, const evaluation_context& eval_ctx) const
{
    if (attribute == context::data_attr_kind::T)
        return eval_ctx.hlasm_ctx.get_attribute_value_ord(attribute, ord_symbol);

    if (!ord_symbol)
    {
        eval_ctx.diags.add_diagnostic(diagnostic_op::warning_W013(expr_range));
        return context::symbol_attributes::default_value(attribute);
    }

    if ((attribute == context::data_attr_kind::S || attribute == context::data_attr_kind::I)
        && !ord_symbol->attributes().can_have_SI_attr())
    {
        eval_ctx.diags.add_diagnostic(diagnostic_op::error_E066(expr_range));
        return context::symbol_attributes::default_value(attribute);
    }

    if (!ord_symbol->attributes().is_defined(attribute))
    {
        eval_ctx.diags.add_diagnostic(diagnostic_op::warning_W013(expr_range));
        return context::symbol_attributes::default_value(attribute);
    }

    return eval_ctx.hlasm_ctx.get_attribute_value_ord(attribute, ord_symbol);
}

bool iequals(std::string_view l, std::string_view r)
{
    return std::ranges::equal(
        l, r, [](unsigned char lc, unsigned char rc) { return std::toupper(lc) == std::toupper(rc); });
}

context::C_t get_current_macro_name_field(const context::code_scope& scope)
{
    if (!scope.is_in_macro())
        return {};
    return scope.this_macro->named_params.at(context::id_storage::well_known::SYSLIST)
        ->get_data(std::array<context::A_t, 1> { 0 })
        ->get_value();
}

context::SET_t ca_symbol_attribute::evaluate_ordsym(context::id_index name, const evaluation_context& eval_ctx) const
{
    if (context::symbol_attributes::is_ordinary_attribute(attribute))
    {
        const context::symbol* ord_symbol = eval_ctx.hlasm_ctx.ord_ctx.get_symbol(name);

        if (!ord_symbol)
            ord_symbol = eval_ctx.hlasm_ctx.ord_ctx.get_symbol_reference(name);

        return retrieve_value(ord_symbol, eval_ctx);
    }
    else if (attribute == context::data_attr_kind::D)
    {
        return eval_ctx.hlasm_ctx.get_attribute_value_ord(attribute, name);
    }
    else if (attribute == context::data_attr_kind::O)
    {
        auto tmp = eval_ctx.hlasm_ctx.get_attribute_value_ord(attribute, name);
        if (tmp.access_c() == "U" && eval_ctx.lib_info.has_library(name.to_string_view()))
            return std::string("S");
        return tmp;
    }
    else
    {
        eval_ctx.diags.add_diagnostic(diagnostic_op::error_E066(expr_range));
        return context::symbol_attributes::default_ca_value(attribute);
    }
}

context::SET_t ca_symbol_attribute::evaluate_literal(
    const semantics::literal_si& lit, const evaluation_context& eval_ctx) const
{
    auto& literals = eval_ctx.hlasm_ctx.ord_ctx.literals();

    bool defined =
        literals.defined_for_ca_expr(std::shared_ptr<const expressions::data_definition>(lit, &lit->get_dd()));
    if (attribute == context::data_attr_kind::D)
        return defined;

    const auto& scope = eval_ctx.active_scope();

    if (!scope.is_in_macro())
        literals.mentioned_in_ca_expr(std::shared_ptr<const expressions::data_definition>(lit, &lit->get_dd()));

    if (attribute == context::data_attr_kind::O)
        return "U";
    else if (attribute == context::data_attr_kind::T)
    {
        if (!defined)
        {
            auto name_field = get_current_macro_name_field(scope);
            if (name_field == lit->get_text())
                return "M";

            // all literals have the form =XY that can be trivially compared
            if (iequals(std::string_view(name_field).substr(0, 3), std::string_view(lit->get_text()).substr(0, 3))
                && utils::is_similar(lit,
                    reparse_substituted_literal(
                        name_field, lit->get_range(), { eval_ctx.hlasm_ctx, eval_ctx.lib_info, drop_diagnostic_op })))
                return "M";
        }
        return std::string { lit->get_dd().get_type_attribute() };
    }
    else
    {
        context::ordinary_assembly_dependency_solver solver(
            eval_ctx.hlasm_ctx.ord_ctx, context::address(), eval_ctx.lib_info);
        context::symbol_attributes attrs = lit->get_dd().get_symbol_attributes(solver, eval_ctx.diags);
        if ((attribute == context::data_attr_kind::S || attribute == context::data_attr_kind::I)
            && !attrs.can_have_SI_attr())
        {
            eval_ctx.diags.add_diagnostic(diagnostic_op::warning_W011(symbol_range));
            return 0;
        }
        return attrs.get_attribute_value(attribute);
    }
}

std::optional<context::C_t> read_string_var_sym(
    const context::variable_symbol& vs, std::span<const context::A_t> indices)
{
    context::C_t var_value;
    if (auto set_sym = vs.access_set_symbol_base())
    {
        if (set_sym->type != context::SET_t_enum::C_TYPE)
            return std::nullopt;

        auto setc_sym = set_sym->access_set_symbol<context::C_t>();
        if (indices.empty())
            var_value = setc_sym->get_value();
        else
            var_value = setc_sym->get_value(indices.front());
    }
    else if (auto mac_par = vs.access_macro_param_base())
    {
        auto data = mac_par->get_data(indices);

        while (dynamic_cast<const context::macro_param_data_composite*>(data))
            data = data->get_ith(1);

        var_value = data->get_value();
    }
    else
        assert(false);

    return var_value;
}

context::SET_t ca_symbol_attribute::evaluate_varsym(
    const semantics::vs_ptr& vs, const evaluation_context& eval_ctx) const
{
    auto [var_name, expr_subscript] = vs->evaluate_symbol(eval_ctx);

    // get symbol
    auto var_symbol = get_var_sym(eval_ctx, var_name);

    if (!var_symbol)
    {
        eval_ctx.diags.add_diagnostic(
            diagnostic_op::error_E010("variable", var_name.to_string_view(), vs->symbol_range));
        return context::symbol_attributes::default_ca_value(attribute);
    }

    switch (attribute)
    {
        // requires_ordinary_symbol
        case context::data_attr_kind::D:
        case context::data_attr_kind::L:
        case context::data_attr_kind::O:
        case context::data_attr_kind::S:
        case context::data_attr_kind::I:
            return evaluate_substituted(var_name, expr_subscript, vs->symbol_range, eval_ctx);

        case context::data_attr_kind::T: {
            if (!test_symbol_for_read(
                    var_symbol, expr_subscript, vs->symbol_range, eval_ctx.diags, var_name.to_string_view()))
                return "U";

            auto var_value_o = read_string_var_sym(*var_symbol, expr_subscript);
            if (!var_value_o.has_value())
                return "N";

            std::string_view var_value = var_value_o.value();

            if (var_value.empty())
                return "O";

            var_value = expressions::ca_symbol_attribute::try_extract_leading_symbol(var_value);

            if (auto res = expressions::ca_constant::try_self_defining_term(var_value))
                return "N";

            auto symbol_name = eval_ctx.hlasm_ctx.ids().add(var_value);

            if (auto tmp_symbol = eval_ctx.hlasm_ctx.ord_ctx.get_symbol(symbol_name))
                return ebcdic_encoding::to_ascii((unsigned char)tmp_symbol->attributes().type());

            return evaluate_substituted(
                var_name, expr_subscript, vs->symbol_range, eval_ctx); // is type U, must substitute var sym
        }
        case context::data_attr_kind::K:
            if (!test_symbol_for_read(
                    var_symbol, expr_subscript, vs->symbol_range, eval_ctx.diags, var_name.to_string_view()))
                return context::symbol_attributes::default_ca_value(attribute);

            return var_symbol ? var_symbol->count(expr_subscript) : 0;

        case context::data_attr_kind::N:
            return var_symbol ? var_symbol->number(expr_subscript) : 0;

        default:
            return context::SET_t();
    }
}

context::SET_t ca_symbol_attribute::evaluate_substituted(context::id_index var_name,
    std::span<const context::A_t> expr_subscript,
    range var_range,
    const evaluation_context& eval_ctx) const
{
    context::SET_t substituted_name = get_var_sym_value(eval_ctx, var_name, expr_subscript, var_range);

    if (substituted_name.type() != context::SET_t_enum::C_TYPE)
    {
        if (attribute != context::data_attr_kind::O && attribute != context::data_attr_kind::T)
            eval_ctx.diags.add_diagnostic(diagnostic_op::error_E066(expr_range));
        return context::symbol_attributes::default_ca_value(attribute);
    }

    const auto& text = substituted_name.access_c();

    if (!text.empty() && text.starts_with('='))
    {
        if (auto lit = reparse_substituted_literal(text, var_range, eval_ctx))
            return evaluate_literal(lit, eval_ctx);
        else if (iequals(text, get_current_macro_name_field(eval_ctx.active_scope())))
            return "M";
        else
            return context::symbol_attributes::default_ca_value(attribute);
    }

    auto [valid, ord_name] = eval_ctx.hlasm_ctx.try_get_symbol_name(try_extract_leading_symbol(text));

    if (!valid)
    {
        if (attribute != context::data_attr_kind::O && attribute != context::data_attr_kind::T)
            eval_ctx.diags.add_diagnostic(diagnostic_op::error_E065(expr_range));
        return context::symbol_attributes::default_ca_value(attribute);
    }
    else
        return evaluate_ordsym(ord_name, eval_ctx);
}

semantics::literal_si ca_symbol_attribute::reparse_substituted_literal(
    const std::string& text, range var_range, const evaluation_context& eval_ctx) const
{
    // error production is suppressed when evaluating D', T' and O' attributes
    using attr_kind = context::data_attr_kind;
    const bool suppress = attribute == attr_kind::T || attribute == attr_kind::D || attribute == attr_kind::O;
    bool error = false;
    diagnostic_consumer_transform add_diag_subst([&text, &eval_ctx, suppress, &error](diagnostic_op diag) {
        error = true;
        if (suppress)
            return;
        diag.message = diagnostic_decorate_message(text, diag.message);
        eval_ctx.diags.add_diagnostic(std::move(diag));
    });
    auto h = parsing::parser_holder::create(nullptr, &eval_ctx.hlasm_ctx, &add_diag_subst, false);

    h->prepare_parser(text,
        &eval_ctx.hlasm_ctx,
        &add_diag_subst,
        semantics::range_provider(var_range, semantics::adjusting_state::SUBSTITUTION),
        var_range,
        0,
        processing::processing_status(processing::processing_format(processing::processing_kind::ORDINARY,
                                          processing::processing_form::CA,
                                          processing::operand_occurrence::ABSENT),
            processing::op_code()),
        true);

    auto literal_value = h->literal_reparse();

    if (!error)
        return literal_value;

    return {};
}

} // namespace hlasm_plugin::parser_library::expressions
