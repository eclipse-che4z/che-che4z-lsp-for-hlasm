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
#include <functional>
#include <initializer_list>
#include <iterator>
#include <span>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "gmock/gmock.h"

#include "analyzer.h"
#include "ebcdic_encoding.h"
#include "processing/instruction_sets/macro_processor.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::parser_library::processing;
using namespace hlasm_plugin::parser_library::expressions;

const size_t size_t_zero = static_cast<size_t>(0);

template<typename T>
std::optional<T> get_var_value(hlasm_context& ctx, std::string name)
{
    auto id = ctx.ids().find(name);
    if (!id.has_value())
        return std::nullopt;

    auto var = ctx.get_var_sym(id.value());
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
    auto id = ctx.ids().find(name);
    if (!id.has_value())
        return std::nullopt;

    auto var = ctx.get_var_sym(id.value());
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
    auto id = ctx.ids().find(name);
    if (!id.has_value())
        return std::nullopt;

    auto var = ctx.get_var_sym(id.value());
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

template<typename Msg,
    typename Proj,
    typename C = std::initializer_list<std::decay_t<std::invoke_result_t<Proj, const Msg&>>>>
inline bool matches_message_properties(const std::vector<Msg>& d, const C& c, Proj p)
{
    std::vector<std::decay_t<std::invoke_result_t<Proj, const Msg&>>> properties;
    std::transform(
        d.begin(), d.end(), std::back_inserter(properties), [&p](const auto& d) { return std::invoke(p, d); });

    return std::is_permutation(properties.begin(), properties.end(), c.begin(), c.end());
}

template<typename Msg,
    typename Proj,
    typename C = std::initializer_list<std::decay_t<std::invoke_result_t<Proj, const Msg&>>>>
inline bool contains_message_properties(const std::vector<Msg>& d, const C& c, Proj p)
{
    if (d.size() < c.size())
        return false;

    std::vector<std::decay_t<std::invoke_result_t<Proj, const Msg&>>> properties;
    std::transform(
        d.begin(), d.end(), std::back_inserter(properties), [&p](const auto& d) { return std::invoke(p, d); });

    std::vector to_find(c.begin(), c.end());

    std::sort(properties.begin(), properties.end());
    std::sort(to_find.begin(), to_find.end());

    return std::includes(properties.begin(), properties.end(), to_find.begin(), to_find.end());
}

template<typename Msg, typename C = std::initializer_list<std::string>>
inline bool matches_message_codes(const std::vector<Msg>& d, const C& c)
{
    return matches_message_properties(d, c, &Msg::code);
}

template<typename Msg, typename C = std::initializer_list<std::string>>
inline bool contains_message_codes(const std::vector<Msg>& d, const C& c)
{
    return contains_message_properties(d, c, &Msg::code);
}

template<typename Msg, typename C = std::initializer_list<std::pair<size_t, size_t>>>
inline bool matches_diagnosed_line_ranges(const std::vector<Msg>& d, const C& c)
{
    return matches_message_properties(
        d, c, [](const auto& d) { return std::make_pair(d.diag_range.start.line, d.diag_range.end.line); });
}

template<typename Msg, typename C = std::initializer_list<std::pair<size_t, size_t>>>
inline bool contains_diagnosed_line_ranges(const std::vector<Msg>& d, const C& c)
{
    return contains_message_properties(
        d, c, [](const auto& d) { return std::make_pair(d.diag_range.start.line, d.diag_range.end.line); });
}

template<typename Msg, typename C = std::initializer_list<std::string>>
inline bool matches_message_text(const std::vector<Msg>& d, const C& c)
{
    return matches_message_properties(d, c, &Msg::message);
}

template<typename Msg, typename C = std::initializer_list<std::string>>
inline bool contains_message_text(const std::vector<Msg>& d, const C& c)
{
    return contains_message_properties(d, c, &Msg::message);
}

inline bool matches_fade_messages(const std::vector<fade_message_s>& a, const std::vector<fade_message_s>& b)
{
    return std::is_permutation(a.begin(), a.end(), b.begin(), b.end(), [](const auto& msg_a, const auto& msg_b) {
        return msg_a.code == msg_b.code && msg_a.r == msg_b.r && msg_a.uri == msg_b.uri;
    });
}

inline bool contains_fade_messages(const std::vector<fade_message_s>& a, const std::vector<fade_message_s>& b)
{
    return std::includes(a.begin(), a.end(), b.begin(), b.end(), [](const auto& msg_a, const auto& msg_b) {
        return msg_a.code == msg_b.code && msg_a.r == msg_b.r && msg_a.uri == msg_b.uri;
    });
}

inline const section* get_section(hlasm_context& ctx, std::string name)
{
    auto sect = ctx.ids().find(name);
    if (!sect.has_value())
        return nullptr;

    return ctx.ord_ctx.get_section(sect.value());
}

inline const symbol* get_symbol(hlasm_context& ctx, std::string name)
{
    auto symbol = ctx.ids().find(name);
    if (!symbol.has_value())
        return nullptr;

    return ctx.ord_ctx.get_symbol(symbol.value());
}

inline std::optional<symbol_value::abs_value_t> get_symbol_abs(hlasm_context& ctx, std::string name)
{
    auto symbol = ctx.ids().find(name);
    if (!symbol.has_value())
        return std::nullopt;

    auto s = ctx.ord_ctx.get_symbol(symbol.value());
    if (!s || s->kind() != symbol_value_kind::ABS)
        return std::nullopt;

    return s->value().get_abs();
}

inline std::optional<symbol_value::reloc_value_t> get_symbol_reloc(hlasm_context& ctx, std::string name)
{
    auto symbol = ctx.ids().find(name);
    if (!symbol.has_value())
        return std::nullopt;

    auto s = ctx.ord_ctx.get_symbol(symbol.value());
    if (!s || s->kind() != symbol_value_kind::RELOC)
        return std::nullopt;

    return s->value().get_reloc();
}

inline std::optional<std::pair<int, std::string>> get_symbol_address(hlasm_context& ctx, std::string name)
{
    auto symbol = ctx.ids().find(name);
    if (!symbol.has_value())
        return std::nullopt;

    auto s = ctx.ord_ctx.get_symbol(symbol.value());
    if (!s || s->kind() != symbol_value_kind::RELOC)
        return std::nullopt;

    const auto& val = s->value().get_reloc();

    if (val.bases().size() != 1 && val.bases().front().second != 1)
        return std::nullopt;

    return std::pair<int, std::string>(val.offset(), val.bases().front().first.owner->name.to_string());
}

#endif
