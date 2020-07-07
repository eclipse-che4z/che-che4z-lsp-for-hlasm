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

#include "concatenation.h"

#include "concatenation_term.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

concatenation_point::concatenation_point(const concat_type type)
    : type(type)
{ }

char_str_conc* concatenation_point::access_str()
{
    return type == concat_type::STR ? static_cast<char_str_conc*>(this) : nullptr;
}

var_sym_conc* concatenation_point::access_var()
{
    return type == concat_type::VAR ? static_cast<var_sym_conc*>(this) : nullptr;
}

dot_conc* concatenation_point::access_dot()
{
    return type == concat_type::DOT ? static_cast<dot_conc*>(this) : nullptr;
}

equals_conc* concatenation_point::access_equ()
{
    return type == concat_type::EQU ? static_cast<equals_conc*>(this) : nullptr;
}

sublist_conc* concatenation_point::access_sub()
{
    return type == concat_type::SUB ? static_cast<sublist_conc*>(this) : nullptr;
}

std::string concatenation_point::evaluate(const concat_chain& chain, expressions::evaluation_context& eval_ctx)
{
    return evaluate(chain.begin(), chain.end(), eval_ctx);
}

std::string concatenation_point::evaluate(
    concat_chain::const_iterator begin, concat_chain::const_iterator end, expressions::evaluation_context& eval_ctx)
{
    std::string ret;
    bool was_var = false;
    for (auto it = begin; it != end; ++it)
    {
        auto&& point = *it;
        switch (point->type)
        {
            case concat_type::DOT:
                if (!was_var)
                    ret.append(point->evaluate(eval_ctx));
                was_var = false;
                break;
            case concat_type::EQU:
            case concat_type::STR:
            case concat_type::SUB:
                ret.append(point->evaluate(eval_ctx));
                was_var = false;
                break;
            case concat_type::VAR:
                ret.append(point->evaluate(eval_ctx));
                was_var = true;
                break;
            default:
                break;
        }
    }
    return ret;
}

void concatenation_point::clear_concat_chain(concat_chain& chain)
{
    size_t offset = 0;
    for (size_t i = 0; i < chain.size(); ++i)
    {
        // if not empty ptr and not empty string and not empty var
        if (chain[i] && !(chain[i]->type == concat_type::STR && chain[i]->access_str()->value.empty())
            && !(chain[i]->type == concat_type::VAR && !chain[i]->access_var()->symbol))
            chain[offset++] = std::move(chain[i]);
    }

    chain.resize(offset);
}

std::string concatenation_point::to_string(const concat_chain& chain) { return to_string(chain.begin(), chain.end()); }

std::string concatenation_point::to_string(concat_chain::const_iterator begin, concat_chain::const_iterator end)
{
    std::string ret;
    for (auto it = begin; it != end; ++it)
    {
        auto&& point = *it;
        switch (point->type)
        {
            case concat_type::DOT:
                ret.push_back('.');
                break;
            case concat_type::EQU:
                ret.push_back('=');
                break;
            case concat_type::STR:
                ret.append(point->access_str()->value);
                break;
            case concat_type::VAR:
                ret.push_back('&');
                if (point->access_var()->symbol->created)
                    ret.append(to_string(point->access_var()->symbol->access_created()->created_name));
                else
                    ret.append(*point->access_var()->symbol->access_basic()->name);
                break;
            case concat_type::SUB:
                ret.push_back('(');
                for (size_t i = 0; i < point->access_sub()->list.size(); ++i)
                {
                    ret += to_string(point->access_sub()->list[i]);
                    if (i != point->access_sub()->list.size() - 1)
                        ret.push_back(',');
                }
                ret.push_back(')');
                break;
            default:
                break;
        }
    }
    return ret;
}

var_sym_conc* concatenation_point::contains_var_sym(
    concat_chain::const_iterator begin, concat_chain::const_iterator end)
{
    for (auto it = begin; it != end; ++it)
    {
        auto&& point = *it;
        if (point->type == concat_type::VAR)
        {
            return point->access_var();
        }
        else if (point->type == concat_type::SUB)
        {
            for (const auto& entry : point->access_sub()->list)
            {
                auto tmp = contains_var_sym(entry.begin(), entry.end());
                if (tmp)
                    return tmp;
            }
        }
        else
            continue;
    }
    return nullptr;
}

} // namespace semantics
} // namespace parser_library
} // namespace hlasm_plugin
