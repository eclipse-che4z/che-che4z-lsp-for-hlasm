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

#include "ca_constant.h"

#include "ca_function.h"
#include "context/hlasm_context.h"
#include "expressions/conditional_assembly/ca_expr_visitor.h"
#include "expressions/evaluation_context.h"

namespace hlasm_plugin::parser_library::expressions {

ca_constant::ca_constant(context::A_t value, range expr_range)
    : ca_expression(context::SET_t_enum::A_TYPE, std::move(expr_range))
    , value(value)
{}

bool ca_constant::get_undefined_attributed_symbols(std::vector<context::id_index>&, const evaluation_context&) const
{
    return false;
}

void ca_constant::resolve_expression_tree(ca_expression_ctx expr_ctx, diagnostic_op_consumer& diags)
{
    if (expr_ctx.kind == context::SET_t_enum::C_TYPE || (expr_ctx.kind == context::SET_t_enum::B_TYPE && value < 0))
        diags.add_diagnostic(diagnostic_op::error_CE004(expr_range));
}

bool ca_constant::is_character_expression(character_expression_purpose) const { return false; }

void ca_constant::apply(ca_expr_visitor& visitor) const { visitor.visit(*this); }

context::SET_t ca_constant::evaluate(const evaluation_context&) const { return value; }

namespace {
context::A_t CA_selfdef(std::string_view value, diagnostic_adder& add_diagnostic)
{
    if (value.size() > 4)
    {
        add_diagnostic(diagnostic_op::error_CE007);
        return 0;
    }

    unsigned long long result = 0;
    for (unsigned char c : value)
        result = (result << 8) | c;

    return (context::A_t)result;
}
} // namespace

context::A_t ca_constant::self_defining_term(
    std::string_view type, std::string_view value, diagnostic_adder& add_diagnostic)
{
    if (value.empty())
    {
        /* noting to do */
    }
    else if (type.size() == 1)
    {
        switch (type.front())
        {
            case 'b':
            case 'B':
                return ca_function::B2A(value, add_diagnostic).access_a();
            case 'c':
            case 'C':
                return ca_function::C2A(value, add_diagnostic).access_a();
            case 'd':
            case 'D':
                return ca_function::D2A(value, add_diagnostic).access_a();
            case 'x':
            case 'X':
                return ca_function::X2A(value, add_diagnostic).access_a();
            default:
                break;
        }
    }
    else if (type.size() == 2)
    {
        switch (type.front())
        {
            case 'c':
            case 'C':
                switch (type.back())
                {
                    case 'a':
                    case 'A':
                        return CA_selfdef(value, add_diagnostic);

                    default:
                        break;
                }
                break;

            default:
                break;
        }
    }
    add_diagnostic(diagnostic_op::error_CE015);
    return context::object_traits<context::A_t>::default_v();
}

context::A_t ca_constant::self_defining_term(std::string_view value, diagnostic_adder& add_diagnostic)
{
    auto q = value.find('\'');
    if (value.size() >= 3 && value.back() == '\'' && q != value.size() - 1)
        return self_defining_term(std::string_view(value.data(), q),
            std::string_view(value.data() + q + 1, value.size() - q - 2),
            add_diagnostic);
    else
        return self_defining_term("D", value, add_diagnostic);
}

context::A_t ca_constant::self_defining_term_or_abs_symbol(
    std::string_view value, const evaluation_context& eval_ctx, range expr_range)
{
    auto add_diagnostic = diagnostic_adder(eval_ctx.diags, expr_range);
    auto q = value.find('\'');
    if (value.size() >= 3 && value.back() == '\'' && q != value.size() - 1)
        return self_defining_term(std::string_view(value.data(), q),
            std::string_view(value.data() + q + 1, value.size() - q - 2),
            add_diagnostic);
    else if (value.front() == '+' || value.front() == '-' || std::isdigit((unsigned char)value.front()))
        return self_defining_term("D", value, add_diagnostic);
    else
    {
        if (auto symbol = eval_ctx.hlasm_ctx.find_id(value); symbol.has_value())
        {
            if (auto s = eval_ctx.hlasm_ctx.ord_ctx.get_symbol(symbol.value());
                s && s->kind() == context::symbol_value_kind::ABS)
                return s->value().get_abs();
        }
        eval_ctx.diags.add_diagnostic(diagnostic_op::error_CE012(expr_range));
        return context::object_traits<context::A_t>::default_v();
    }
}

std::optional<context::A_t> ca_constant::try_self_defining_term(std::string_view value)
{
    auto empty_add = diagnostic_adder();
    auto ret = self_defining_term(value, empty_add);
    if (empty_add.diagnostics_present)
        return std::nullopt;
    else
        return ret;
}

} // namespace hlasm_plugin::parser_library::expressions
