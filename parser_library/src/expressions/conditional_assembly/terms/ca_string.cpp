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

#include "expressions/conditional_assembly/ca_expr_visitor.h"
#include "expressions/evaluation_context.h"

namespace hlasm_plugin::parser_library::expressions {

ca_string::substring_t::substring_t()
    : start(nullptr)
    , count(nullptr)
    , substring_range()
{}

ca_string::ca_string(
    semantics::concat_chain value, ca_expr_ptr duplication_factor, substring_t substring, range expr_range)
    : ca_expression(context::SET_t_enum::C_TYPE, std::move(expr_range))
    , value(std::move(value))
    , duplication_factor(std::move(duplication_factor))
    , substring(std::move(substring))
{}

undef_sym_set ca_string::get_undefined_attributed_symbols(const evaluation_context& eval_ctx) const
{
    undef_sym_set tmp;
    if (duplication_factor)
        tmp = duplication_factor->get_undefined_attributed_symbols(eval_ctx);
    if (substring.start)
        tmp.merge(substring.start->get_undefined_attributed_symbols(eval_ctx));
    if (substring.count)
        tmp.merge(substring.count->get_undefined_attributed_symbols(eval_ctx));
    return tmp;
}

void ca_string::resolve_expression_tree(context::SET_t_enum kind, diagnostic_op_consumer& diags)
{
    if (expr_kind != kind)
        diags.add_diagnostic(diagnostic_op::error_CE004(expr_range));
}

bool ca_string::is_character_expression(character_expression_purpose) const { return true; }

void ca_string::apply(ca_expr_visitor& visitor) const { visitor.visit(*this); }

context::SET_t ca_string::evaluate(const evaluation_context& eval_ctx) const
{
    context::C_t str = semantics::concatenation_point::evaluate(value, eval_ctx);

    if (str.size() > MAX_STR_SIZE)
    {
        eval_ctx.diags.add_diagnostic(diagnostic_op::error_CE011(expr_range));
        return context::object_traits<context::C_t>::default_v();
    }

    if (substring.start)
    {
        auto start = substring.start->evaluate(eval_ctx).access_a();
        auto count =
            substring.count ? substring.count->evaluate(eval_ctx).access_a() : (context::A_t)str.size() - start + 1;

        if (count == 0)
        {
            // when zero-length substring is requested, validation of the first parameter seems supressed
            str = "";
        }
        else if (start < 0 || count < 0 || (start == 0 && count > 0))
        {
            eval_ctx.diags.add_diagnostic(diagnostic_op::error_CE008(substring.substring_range));
            return context::object_traits<context::C_t>::default_v();
        }
        else if (start > (context::A_t)str.size())
        {
            eval_ctx.diags.add_diagnostic(diagnostic_op::error_CE009(substring.start->expr_range));
            return context::object_traits<context::C_t>::default_v();
        }
        /* TODO uncomment when assembler options will be implemented
        if (start + count - 1 > (int)str.size())
            eval_ctx.diags.add_diagnostic(diagnostic_op::error_CW001(substring.count->expr_range));
        */
        else
            str = str.substr(start - 1, count);
    }

    return duplicate(duplication_factor, std::move(str), expr_range, eval_ctx);
}

std::string ca_string::duplicate(
    const ca_expr_ptr& dupl_factor, std::string value, range expr_range, const evaluation_context& eval_ctx)
{
    if (dupl_factor)
    {
        auto dupl = dupl_factor->evaluate(eval_ctx).access_a();

        if (dupl < 0)
        {
            eval_ctx.diags.add_diagnostic(diagnostic_op::error_CE010(dupl_factor->expr_range));
            return "";
        }

        if (value.size() * dupl > MAX_STR_SIZE)
        {
            eval_ctx.diags.add_diagnostic(diagnostic_op::error_CE011(expr_range));
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

} // namespace hlasm_plugin::parser_library::expressions
