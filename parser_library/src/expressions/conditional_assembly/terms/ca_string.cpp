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
#include "expressions/conditional_assembly/ca_operator_binary.h"
#include "expressions/evaluation_context.h"
#include "semantics/variable_symbol.h"
#include "utils/unicode_text.h"

namespace hlasm_plugin::parser_library::expressions {

ca_string::ca_string(
    semantics::concat_chain value, ca_expr_ptr duplication_factor, substring_t substring, range expr_range)
    : ca_expression(context::SET_t_enum::C_TYPE, std::move(expr_range))
    , value(std::move(value))
    , duplication_factor(std::move(duplication_factor))
    , substring(std::move(substring))
{
    is_ca_string = true;
}

bool ca_string::get_undefined_attributed_symbols(
    std::vector<context::id_index>& symbols, const evaluation_context& eval_ctx) const
{
    bool result = false;
    if (duplication_factor)
        result |= duplication_factor->get_undefined_attributed_symbols(symbols, eval_ctx);
    if (substring.start)
        result |= substring.start->get_undefined_attributed_symbols(symbols, eval_ctx);
    if (substring.count)
        result |= substring.count->get_undefined_attributed_symbols(symbols, eval_ctx);
    result |= semantics::concatenation_point::get_undefined_attributed_symbols(symbols, value, eval_ctx);
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

context::A_t ca_string::compute_duplication_factor(const ca_expr_ptr& dupl_factor, const evaluation_context& eval_ctx)
{
    if (!dupl_factor)
        return 1;
    auto dupl = dupl_factor->evaluate(eval_ctx).access_a();

    if (dupl < 0)
    {
        eval_ctx.diags.add_diagnostic(diagnostic_op::error_CE010(dupl_factor->expr_range));
        return -1;
    }
    return dupl;
}

context::SET_t ca_string::evaluate(const evaluation_context& eval_ctx) const
{
    context::A_t dupl = compute_duplication_factor(duplication_factor, eval_ctx);

    context::A_t start = -1;
    context::A_t count = -1;
    if (substring.start)
    {
        start = substring.start->evaluate(eval_ctx).access_a();
        if (substring.count)
            count = substring.count->evaluate(eval_ctx).access_a();

        if (count == 0)
        {
            // when zero-length substring is requested, validation of the first parameter seems suppressed
            return context::object_traits<context::C_t>::default_v();
        }
        else if (start <= 0 || substring.count && count < 0)
        {
            eval_ctx.diags.add_diagnostic(diagnostic_op::error_CE008(substring.substring_range));
            return context::object_traits<context::C_t>::default_v();
        }
    }

    context::C_t str = semantics::concatenation_point::evaluate(value, eval_ctx);

    if (start > 0)
    {
        auto substr = utils::utf8_substr<false>(str, start - 1, (size_t)count);
        if (!substr.offset_valid || substring.count && substr.str.empty())
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

    return duplicate(dupl, std::move(str), expr_range, eval_ctx);
}

namespace {
std::optional<size_t> get_utf32_length_if_too_long(std::string_view s, uint64_t dupl)
{
    if (s.size() > ca_string::MAX_STR_SIZE || s.size() * dupl > ca_string::MAX_STR_SIZE)
    {
        const auto len = utils::length_utf32_no_validation(s);
        if (len > ca_string::MAX_STR_SIZE || len * dupl > ca_string::MAX_STR_SIZE)
            return len;
    }
    return std::nullopt;
}

void repeat_string(std::string& s, uint64_t n)
{
    s.reserve(s.size() * n);
    auto begin = s.begin();
    auto end = s.end();
    for (uint64_t i = 1; i < n; ++i)
        s.append(begin, end);
}
} // namespace

std::string ca_string::duplicate(
    context::A_t dupl, std::string value, range expr_range, const evaluation_context& eval_ctx)
{
    if (dupl <= 0 || value.empty())
        return {};

    const auto len = get_utf32_length_if_too_long(value, dupl);
    if (!len)
    {
        repeat_string(value, dupl);
        return value;
    }

    eval_ctx.diags.add_diagnostic(diagnostic_op::error_CE011(expr_range));

    if (len > MAX_STR_SIZE)
    {
        value = utils::utf8_substr(value, 0, MAX_STR_SIZE).str;
        return value;
    }

    const auto repeat = MAX_STR_SIZE / *len;
    const auto remainder = MAX_STR_SIZE % *len;

    repeat_string(value, repeat);
    value.append(utils::utf8_substr(value, 0, remainder).str);

    return value;
}

} // namespace hlasm_plugin::parser_library::expressions
