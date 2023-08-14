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

#include <optional>

#include "expressions/conditional_assembly/ca_expr_visitor.h"
#include "expressions/conditional_assembly/ca_operator_binary.h"
#include "expressions/evaluation_context.h"
#include "semantics/variable_symbol.h"
#include "utils/unicode_text.h"

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

bool ca_string::get_undefined_attributed_symbols(undef_sym_set& symbols, const evaluation_context& eval_ctx) const
{
    bool result = false;
    if (duplication_factor)
        result |= duplication_factor->get_undefined_attributed_symbols(symbols, eval_ctx);
    if (substring.start)
        result |= substring.start->get_undefined_attributed_symbols(symbols, eval_ctx);
    if (substring.count)
        result |= substring.count->get_undefined_attributed_symbols(symbols, eval_ctx);
    return result;
}

void ca_string::resolve_expression_tree(ca_expression_ctx expr_ctx, diagnostic_op_consumer& diags)
{
    if (expr_kind != expr_ctx.kind)
        diags.add_diagnostic(diagnostic_op::error_CE004(expr_range));

    expr_ctx.kind = context::SET_t_enum::A_TYPE;
    expr_ctx.parent_expr_kind = expr_ctx.parent_expr_kind == context::SET_t_enum::B_TYPE ? expr_ctx.parent_expr_kind
                                                                                         : context::SET_t_enum::A_TYPE;

    if (duplication_factor)
        duplication_factor->resolve_expression_tree({ expr_ctx.kind, expr_ctx.parent_expr_kind, false }, diags);
    if (substring.start)
        substring.start->resolve_expression_tree(expr_ctx, diags);
    if (substring.count)
        substring.count->resolve_expression_tree(expr_ctx, diags);

    for (const auto& concat_point : value)
    {
        concat_point.resolve(diags);
    }
}

bool ca_string::is_character_expression(character_expression_purpose) const { return true; }

void ca_string::apply(ca_expr_visitor& visitor) const { visitor.visit(*this); }

namespace {
bool string_too_long(std::string_view s, uint64_t dupl)
{
    return s.size() * dupl > utils::max_utf8_sequence_length * ca_string::MAX_STR_SIZE
        || s.size() * dupl > ca_string::MAX_STR_SIZE
        && utils::length_utf32_no_validation(s) * dupl > ca_string::MAX_STR_SIZE;
}
} // namespace

context::SET_t ca_string::evaluate(const evaluation_context& eval_ctx) const
{
    context::C_t str = semantics::concatenation_point::evaluate(value, eval_ctx);

    if (string_too_long(str, 1))
    {
        eval_ctx.diags.add_diagnostic(diagnostic_op::error_CE011(expr_range));
        return context::object_traits<context::C_t>::default_v();
    }

    if (substring.start)
    {
        auto start = substring.start->evaluate(eval_ctx).access_a();
        std::optional<context::A_t> count;
        if (substring.count)
            count = substring.count->evaluate(eval_ctx).access_a();

        if (count == 0)
        {
            // when zero-length substring is requested, validation of the first parameter seems suppressed
            str = "";
        }
        else if (start <= 0 || count && count < 0)
        {
            eval_ctx.diags.add_diagnostic(diagnostic_op::error_CE008(substring.substring_range));
            return context::object_traits<context::C_t>::default_v();
        }
        else
        {
            auto substr = utils::utf8_substr<false>(str, start - 1, (size_t)count.value_or(-1));
            if (!substr.offset_valid || count && substr.str.empty())
            {
                eval_ctx.diags.add_diagnostic(diagnostic_op::error_CE009(substring.start->expr_range));
                return context::object_traits<context::C_t>::default_v();
            }
            /* TODO uncomment when assembler options will be implemented
            if (substr.char_count != count)
                eval_ctx.diags.add_diagnostic(diagnostic_op::error_CW001(substring.count->expr_range));
            */
            str = std::string(substr.str);
        }
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

        if (string_too_long(value, (uint64_t)dupl))
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
