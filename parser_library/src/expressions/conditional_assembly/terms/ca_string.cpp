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

#include "ca_string.h"

#include "expressions/evaluation_context.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

ca_string::substring_t::substring_t()
    : start(nullptr)
    , count(nullptr)
    , substring_range()
{ }

ca_string::ca_string(
    semantics::concat_chain value, ca_expr_ptr duplication_factor, substring_t substring, range expr_range)
    : ca_expression(context::SET_t_enum::C_TYPE, std::move(expr_range))
    , value(std::move(value))
    , duplication_factor(std::move(duplication_factor))
    , substring(std::move(substring))
{ }

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

void ca_string::resolve_expression_tree(context::SET_t_enum kind)
{
    if (expr_kind != kind)
        add_diagnostic(diagnostic_op::error_CE004(expr_range));
    else
    {
        if (duplication_factor)
            duplication_factor->resolve_expression_tree(context::SET_t_enum::A_TYPE);
        if (substring.start)
            substring.start->resolve_expression_tree(context::SET_t_enum::A_TYPE);
        if (substring.count)
            substring.count->resolve_expression_tree(context::SET_t_enum::A_TYPE);
    }
}

void ca_string::collect_diags() const
{
    if (duplication_factor)
        collect_diags_from_child(*duplication_factor);
    if (substring.start)
        collect_diags_from_child(*substring.start);
    if (substring.count)
        collect_diags_from_child(*substring.count);
}

bool ca_string::is_character_expression() const { return duplication_factor == nullptr; }

context::SET_t ca_string::evaluate(evaluation_context& eval_ctx) const
{
    context::C_t str = ""; // evaluate concat chain

    if (str.size() > MAX_STR_SIZE)
    {
        eval_ctx.add_diagnostic(diagnostic_op::error_CE011(expr_range));
        return context::SET_t();
    }

    if (substring.start)
    {
        auto start = substring.start->evaluate(eval_ctx).access_a();
        auto count = substring.count ? substring.count->evaluate(eval_ctx).access_a() : (context::A_t)str.size();

        if (start < 0 || count < 0 || (start == 0 && count > 0))
        {
            eval_ctx.add_diagnostic(diagnostic_op::error_CE008(substring.substring_range));
            return context::SET_t();
        }
        if (start > (context::A_t)str.size())
        {
            eval_ctx.add_diagnostic(diagnostic_op::error_CE009(substring.start->expr_range));
            return context::SET_t();
        }

        str = str.substr(start - 1, count);
    }

    return duplicate(duplication_factor, std::move(str), expr_range, eval_ctx);
}

std::string ca_string::duplicate(
    const ca_expr_ptr& dupl_factor, std::string value, range expr_range, evaluation_context& eval_ctx)
{
    if (dupl_factor)
    {
        auto dupl = dupl_factor->evaluate(eval_ctx).access_a();

        if (dupl < 0)
        {
            eval_ctx.add_diagnostic(diagnostic_op::error_CE010(dupl_factor->expr_range));
            return "";
        }

        if (value.size() * dupl > MAX_STR_SIZE)
        {
            eval_ctx.add_diagnostic(diagnostic_op::error_CE011(expr_range));
            return "";
        }

        if (dupl == 0 || value.empty())
            value = "";
        else
        {
            value.reserve(value.size() * dupl);
            auto begin = value.begin();
            auto end = value.end();
            for (auto i = 1; i < dupl; ++i)
                value.append(begin, end);
        }
    }
    return value;
}

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin
