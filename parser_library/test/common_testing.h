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

#ifndef HLASMPLUGIN_HLASMPARSERLIBRARY_COMMON_TESTING_H
#define HLASMPLUGIN_HLASMPARSERLIBRARY_COMMON_TESTING_H

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

#include "gmock/gmock.h"

#include "analyzer.h"
#include "ebcdic_encoding.h"
#include "lexing/input_source.h"
#include "lexing/token_stream.h"
#include "processing/instruction_sets/macro_processor.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::parser_library::processing;
using namespace hlasm_plugin::parser_library::expressions;

const size_t size_t_zero = static_cast<size_t>(0);

namespace antlr4 {
class ParserRuleContext;
} // namespace antlr4
std::pair<bool, antlr4::ParserRuleContext*> try_parse_sll(hlasm_plugin::parser_library::parsing::hlasmparser& h_parser);

template<typename T>
std::optional<T> get_var_value(hlasm_context& ctx, std::string name)
{
    auto var = ctx.get_var_sym(ctx.ids().find(name));
    if (!var)
        return std::nullopt;

    if (var->var_kind != context::variable_kind::SET_VAR_KIND)
        return std::nullopt;
    auto var_ = var->access_set_symbol_base();
    if (var_->type != object_traits<T>::type_enum || !var_->is_scalar)
        return std::nullopt;

    auto symbol = var_->template access_set_symbol<T>();
    if (!symbol)
        return std::nullopt;

    return symbol->get_value();
}

template<typename T>
std::optional<std::vector<T>> get_var_vector(hlasm_context& ctx, std::string name)
{
    auto var = ctx.get_var_sym(ctx.ids().find(name));
    if (!var)
        return std::nullopt;

    if (var->var_kind != context::variable_kind::SET_VAR_KIND)
        return std::nullopt;
    auto var_ = var->access_set_symbol_base();
    if (var_->type != object_traits<T>::type_enum || var_->is_scalar)
        return std::nullopt;

    auto symbol = var_->template access_set_symbol<T>();
    if (!symbol)
        return std::nullopt;

    auto keys = symbol->keys();

    std::vector<T> result;
    result.reserve(keys.size());
    for (size_t i = 0; i < keys.size(); ++i)
    {
        if (i != keys[i])
            return std::nullopt;
        result.push_back(symbol->get_value(i));
    }

    return result;
}

template<typename T>
std::optional<std::unordered_map<size_t, T>> get_var_vector_map(hlasm_context& ctx, std::string name)
{
    auto var = ctx.get_var_sym(ctx.ids().find(name));
    if (!var)
        return std::nullopt;

    if (var->var_kind != context::variable_kind::SET_VAR_KIND)
        return std::nullopt;
    auto var_ = var->access_set_symbol_base();
    if (var_->type != object_traits<T>::type_enum || var_->is_scalar)
        return std::nullopt;

    auto symbol = var_->template access_set_symbol<T>();
    if (!symbol)
        return std::nullopt;

    auto keys = symbol->keys();

    std::unordered_map<size_t, T> result;
    result.reserve(keys.size());
    for (size_t i = 0; i < keys.size(); ++i)
    {
        result.emplace(keys[i], symbol->get_value(keys[i]));
    }

    return result;
}

inline bool matches_message_codes(const std::vector<diagnostic_op>& d, std::initializer_list<std::string> m)
{
    std::vector<std::string> codes;
    std::transform(d.begin(), d.end(), std::back_inserter(codes), [](const auto& d) { return d.code; });

    return std::is_permutation(codes.begin(), codes.end(), m.begin(), m.end());
}

inline bool matches_message_codes(const std::vector<diagnostic_s>& d, std::initializer_list<std::string> m)
{
    std::vector<std::string> codes;
    std::transform(d.begin(), d.end(), std::back_inserter(codes), [](const auto& d) { return d.code; });

    return std::is_permutation(codes.begin(), codes.end(), m.begin(), m.end());
}

inline bool contains_message_codes(const std::vector<diagnostic_s>& d, std::initializer_list<std::string> m)
{
    if (d.size() < m.size())
        return false;

    std::vector<std::string> codes;
    std::transform(d.begin(), d.end(), std::back_inserter(codes), [](const auto& d) { return d.code; });

    std::vector to_find(m.begin(), m.end());

    std::sort(codes.begin(), codes.end());
    std::sort(to_find.begin(), to_find.end());

    return std::includes(codes.begin(), codes.end(), to_find.begin(), to_find.end());
}

inline const section* get_section(hlasm_context& ctx, std::string name)
{
    auto sect = ctx.ids().find(std::move(name));
    if (!sect)
        return nullptr;

    return ctx.ord_ctx.get_section(sect);
}

inline const symbol* get_symbol(hlasm_context& ctx, std::string name)
{
    auto symbol = ctx.ids().find(std::move(name));
    if (!symbol)
        return nullptr;

    return ctx.ord_ctx.get_symbol(symbol);
}

inline std::optional<symbol_value::abs_value_t> get_symbol_abs(hlasm_context& ctx, std::string name)
{
    auto symbol = ctx.ids().find(std::move(name));
    if (!symbol)
        return std::nullopt;

    auto s = ctx.ord_ctx.get_symbol(symbol);
    if (!s || s->kind() != symbol_value_kind::ABS)
        return std::nullopt;

    return s->value().get_abs();
}

inline std::optional<symbol_value::reloc_value_t> get_symbol_reloc(hlasm_context& ctx, std::string name)
{
    auto symbol = ctx.ids().find(std::move(name));
    if (!symbol)
        return std::nullopt;

    auto s = ctx.ord_ctx.get_symbol(symbol);
    if (!s || s->kind() != symbol_value_kind::RELOC)
        return std::nullopt;

    return s->value().get_reloc();
}

#endif
