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
#include "expressions/conditional_assembly/ca_expr_visitor.h"

namespace hlasm_plugin::parser_library::expressions {

ca_constant::ca_constant(context::A_t value, range expr_range)
    : ca_expression(context::SET_t_enum::A_TYPE, std::move(expr_range))
    , value(value)
{}

undef_sym_set ca_constant::get_undefined_attributed_symbols(const evaluation_context&) const { return undef_sym_set(); }

void ca_constant::resolve_expression_tree(context::SET_t_enum kind)
{
    if (kind == context::SET_t_enum::C_TYPE)
        add_diagnostic(diagnostic_op::error_CE004(expr_range));
}

void ca_constant::collect_diags() const
{
    // nothing to collect
}

bool ca_constant::is_character_expression(character_expression_purpose) const { return false; }

void ca_constant::apply(ca_expr_visitor& visitor) const { visitor.visit(*this); }

context::SET_t ca_constant::evaluate(const evaluation_context&) const { return value; }

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
                    case 'A': {
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
                    break;
                }
                break;
        }
    }
    add_diagnostic(diagnostic_op::error_CE015);
    return context::object_traits<context::A_t>::default_v();
}

context::A_t ca_constant::self_defining_term(const std::string& value, diagnostic_adder& add_diagnostic)
{
    auto q = value.find('\'');
    if (value.size() >= 3 && value.back() == '\'' && q != value.size() - 1)
        return self_defining_term(std::string_view(value.c_str(), q),
            std::string_view(value.c_str() + q + 1, value.size() - q - 2),
            add_diagnostic);
    else
        return self_defining_term("D", value, add_diagnostic);
}

std::optional<context::A_t> ca_constant::try_self_defining_term(const std::string& value)
{
    auto empty_add = diagnostic_adder();
    auto ret = self_defining_term(value, empty_add);
    if (empty_add.diagnostics_present)
        return std::nullopt;
    else
        return ret;
}

} // namespace hlasm_plugin::parser_library::expressions
