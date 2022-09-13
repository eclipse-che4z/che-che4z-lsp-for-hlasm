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

#include "concatenation_term.h"

#include <cassert>

namespace hlasm_plugin::parser_library::semantics {

char_str_conc::char_str_conc(std::string value, const range& conc_range)
    : concatenation_point(concat_type::STR)
    , value(std::move(value))
    , conc_range(conc_range)
{}

std::string char_str_conc::evaluate(const expressions::evaluation_context&) const { return value; }

void char_str_conc::resolve(diagnostic_op_consumer&) {}

var_sym_conc::var_sym_conc(vs_ptr symbol)
    : concatenation_point(concat_type::VAR)
    , symbol(std::move(symbol))
{}

std::string var_sym_conc::evaluate(const expressions::evaluation_context& eval_ctx) const
{
    auto value = symbol->evaluate(eval_ctx);

    return evaluate(std::move(value));
}

void var_sym_conc::resolve(diagnostic_op_consumer& diag) { symbol->resolve(context::SET_t_enum::A_TYPE, diag); }

std::string var_sym_conc::evaluate(context::SET_t varsym_value)
{
    switch (varsym_value.type)
    {
        case context::SET_t_enum::A_TYPE:
            return std::to_string(std::abs(varsym_value.access_a()));
        case context::SET_t_enum::B_TYPE:
            return varsym_value.access_b() ? "1" : "0";
        case context::SET_t_enum::C_TYPE:
            return std::move(varsym_value.access_c());
        default:
            return "";
    }
}

dot_conc::dot_conc()
    : concatenation_point(concat_type::DOT)
{}

std::string dot_conc::evaluate(const expressions::evaluation_context&) const { return "."; }

void dot_conc::resolve(diagnostic_op_consumer&) {}

equals_conc::equals_conc()
    : concatenation_point(concat_type::EQU)
{}

std::string equals_conc::evaluate(const expressions::evaluation_context&) const { return "="; }

void equals_conc::resolve(diagnostic_op_consumer&) {}

sublist_conc::sublist_conc(std::vector<concat_chain> list)
    : concatenation_point(concat_type::SUB)
    , list(std::move(list))
{}

std::string sublist_conc::evaluate(const expressions::evaluation_context& eval_ctx) const
{
    std::string ret;
    ret.push_back('(');
    for (size_t i = 0; i < list.size(); ++i)
    {
        ret.append(concatenation_point::evaluate(list[i], eval_ctx));
        if (i + 1 != list.size())
            ret.push_back(',');
    }
    ret.push_back(')');
    return ret;
}

void sublist_conc::resolve(diagnostic_op_consumer& diag)
{
    for (const auto& l : list)
        for (const auto& e : l)
            e->resolve(diag);
}

} // namespace hlasm_plugin::parser_library::semantics
