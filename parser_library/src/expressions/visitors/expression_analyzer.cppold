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

#include "expression_analyzer.h"

#include "processing/context_manager.h"
#include "semantics/range_provider.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::expressions;

antlrcpp::Any expression_analyzer::visitExpr(parsing::hlasmparser::ExprContext* ctx)
{
    return visit(ctx->expr_p_space_c());
}

antlrcpp::Any expression_analyzer::visitExpr_p(parsing::hlasmparser::Expr_pContext* ctx)
{
    if (ctx->t != nullptr)
        return visit(ctx->t);

    auto a = visit_ref(ctx->tmp);
    auto b = visit_ref(ctx->expr_s());

    a.merge(std::move(b));
    return a;
}

antlrcpp::Any expression_analyzer::visitExpr_s(parsing::hlasmparser::Expr_sContext* ctx)
{
    if (ctx->t != nullptr)
        return visit(ctx->t);

    auto a = visit_ref(ctx->tmp);
    auto b = visit_ref(ctx->term_c());

    a.merge(std::move(b));
    return a;
}

antlrcpp::Any expression_analyzer::visitExpr_p_space_c(parsing::hlasmparser::Expr_p_space_cContext* ctx)
{
    std::set<context::id_index> result;
    if (ctx->exs != nullptr)
        result = visit_ref(ctx->exs);
    result.merge(visit_ref(ctx->expr_p()));
    return result;
}

antlrcpp::Any expression_analyzer::visitTerm_c(parsing::hlasmparser::Term_cContext* ctx)
{
    if (ctx->t != nullptr)
        return visit(ctx->t);
    else
        return visit(ctx->term_c());
}

antlrcpp::Any expression_analyzer::visitTerm(parsing::hlasmparser::TermContext* ctx)
{
    if (ctx->var_symbolContext != nullptr)
    {
        std::set<context::id_index> result;

        for (auto expr : ctx->var_symbolContext->vs->subscript)
            result.merge(visit_ref(expr));

        if (ctx->var_symbolContext->vs->created)
            result.merge(get_undefined_symbol_references(ctx->var_symbolContext->vs->access_created()->created_name));

        return result;
    }
    else if (ctx->expr() != nullptr)
        return visit(ctx->expr());
    else if (ctx->ca_string() != nullptr)
        return visit(ctx->ca_string());
    else if (ctx->data_attribute() != nullptr)
        return visit(ctx->data_attribute());
    else if (ctx->id_sub() != nullptr)
        return visit(ctx->id_sub());

    return std::set<context::id_index> {};
}

antlrcpp::Any expression_analyzer::visitId_sub(parsing::hlasmparser::Id_subContext* ctx)
{
    return visit(ctx->subscriptContext);
}

antlrcpp::Any expression_analyzer::visitExpr_p_comma_c(parsing::hlasmparser::Expr_p_comma_cContext* ctx)
{
    std::set<context::id_index> result;
    if (ctx->exs != nullptr)
        result = visit_ref(ctx->exs);
    result.merge(visit_ref(ctx->expr_p()));
    return result;
}

antlrcpp::Any expression_analyzer::visitSubscript(parsing::hlasmparser::SubscriptContext* ctx)
{
    if (ctx->children.size() >= 3)
        return visit(ctx->expr_p_comma_cContext);
    else
        return std::set<context::id_index>();
}

antlrcpp::Any expression_analyzer::visitCa_string(parsing::hlasmparser::Ca_stringContext* ctx)
{
    std::set<context::id_index> result;
    if (ctx->tmp != nullptr)
    {
        result = visit_ref(ctx->tmp);
        result.merge(visit_ref(ctx->ca_string_b()));
    }
    else
        result = visit_ref(ctx->ca_string_b());
    return result;
}

antlrcpp::Any expression_analyzer::visitCa_string_b(parsing::hlasmparser::Ca_string_bContext* ctx)
{
    auto result = get_undefined_symbol_references(ctx->string_ch_v_c()->chain);

    if (ctx->substring() && ctx->substring()->e1 != nullptr)
        result.merge(visit_ref(ctx->substring()->e1));
    if (ctx->substring() && ctx->substring()->e2 != nullptr)
        result.merge(visit_ref(ctx->substring()->e2));

    return result;
}

antlrcpp::Any expression_analyzer::visitCa_dupl_factor(parsing::hlasmparser::Ca_dupl_factorContext* ctx)
{
    if (ctx->expr_p() != nullptr)
        return visit(ctx->expr_p());
    else
        return std::set<context::id_index> {};
}

antlrcpp::Any expression_analyzer::visitData_attribute(parsing::hlasmparser::Data_attributeContext* ctx)
{
    std::set<context::id_index> result;

    auto attr = ctx->attribute;

    if (!ctx->id() || attr == context::data_attr_kind::D || attr == context::data_attr_kind::O
        || attr == context::data_attr_kind::N || attr == context::data_attr_kind::K
        || eval_ctx_.hlasm_ctx.ord_ctx.get_symbol(ctx->id()->name))
        return std::set<context::id_index> {};

    result.insert(ctx->id()->name);

    return result;
}

expression_analyzer::expression_analyzer(evaluation_context eval_ctx)
    : eval_ctx_(eval_ctx)
{}

std::set<context::id_index> expression_analyzer::get_undefined_symbol_references(
    antlr4::ParserRuleContext* expr_context)
{
    return visit(expr_context).as<std::set<context::id_index>>();
}

std::set<context::id_index> expression_analyzer::get_undefined_symbol_references(const semantics::concat_chain& chain)
{
    std::set<context::id_index> result;

    for (auto& point : chain)
    {
        if (point == nullptr)
            continue;
        if (point->type == semantics::concat_type::VAR)
        {
            result = get_undefined_symbol_references(*point->access_var()->symbol);
        }
        if (point->type == semantics::concat_type::SUB)
        {
            for (auto& entry : point->access_sub()->list)
            {
                result.merge(get_undefined_symbol_references(entry));
            }
        }
    }

    return result;
}

std::set<context::id_index> expression_analyzer::get_undefined_symbol_references(
    const semantics::variable_symbol& symbol)
{
    std::set<context::id_index> result;

    for (auto expr : symbol.subscript)
        result.merge(visit_ref(expr));

    if (symbol.created)
        result.merge(get_undefined_symbol_references(symbol.access_created()->created_name));

    return result;
}

std::set<context::id_index> expression_analyzer::visit_ref(antlr4::ParserRuleContext* expr_context)
{
    return visit(expr_context).as<std::set<context::id_index>>();
}
