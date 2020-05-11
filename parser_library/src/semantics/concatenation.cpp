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

using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library;

concatenation_point::concatenation_point(const concat_type type)
    : type(type)
{}

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

void concatenation_point::clear_concat_chain(concat_chain& chain)
{
    size_t offset = 0;
    for (size_t i = 0; i < chain.size(); ++i)
    {
        if (chain[i]
            && !(chain[i]->type == concat_type::STR
                && chain[i]->access_str()->value.empty())) // if not empty ptr and not empty string
            chain[offset++] = std::move(chain[i]);
    }

    chain.resize(offset);
}

std::string concatenation_point::to_string(const concat_chain& chain)
{
    std::string ret;
    for (auto& point : chain)
    {
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

var_sym_conc* concatenation_point::contains_var_sym(const concat_chain& chain)
{
    for (const auto& point : chain)
    {
        if (point->type == concat_type::VAR)
        {
            return point->access_var();
        }
        else if (point->type == concat_type::SUB)
        {
            for (const auto& entry : point->access_sub()->list)
            {
                auto tmp = contains_var_sym(entry);
                if (tmp)
                    return tmp;
            }
        }
        else
            continue;
    }
    return nullptr;
}

concat_chain concatenation_point::clone(const concat_chain& chain)
{
    concat_chain res;
    res.reserve(chain.size());

    for (auto& point : chain)
    {
        switch (point->type)
        {
            case concat_type::DOT:
                res.push_back(std::make_unique<dot_conc>());
                break;
            case concat_type::EQU:
                res.push_back(std::make_unique<equals_conc>());
                break;
            case concat_type::STR:
                res.push_back(std::make_unique<char_str_conc>(point->access_str()->value));
                break;
            case concat_type::SUB: {
                std::vector<concat_chain> tmp;
                for (auto& ch : point->access_sub()->list)
                    tmp.emplace_back(clone(ch));
                res.push_back(std::make_unique<sublist_conc>(std::move(tmp)));
            }
            break;
            case concat_type::VAR:
                if (!point->access_var()->symbol->created)
                {
                    auto tmp = point->access_var()->symbol->access_basic();
                    auto symbol = std::make_unique<basic_variable_symbol>(tmp->name, tmp->subscript, tmp->symbol_range);
                    res.push_back(std::make_unique<var_sym_conc>(std::move(symbol)));
                }
                else
                {
                    auto tmp = point->access_var()->symbol->access_created();
                    auto symbol = std::make_unique<created_variable_symbol>(
                        clone(tmp->created_name), tmp->subscript, tmp->symbol_range);
                    res.push_back(std::make_unique<var_sym_conc>(std::move(symbol)));
                }
                break;
            default:
                break;
        }
    }

    return res;
}
