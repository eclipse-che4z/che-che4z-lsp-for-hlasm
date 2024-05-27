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
#include <concepts>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "gmock/gmock.h"

#include "analyzer.h"
#include "context/common_types.h"
#include "diagnostic.h"
#include "ebcdic_encoding.h"
#include "utils/task.h"

namespace hlasm_plugin::utils {
class task;
}
namespace hlasm_plugin::parser_library {
class workspace_manager;
template<typename T>
class diagnostic_consumer_t;
struct diagnostic_op;
struct document_symbol_item;
bool is_similar(const std::vector<document_symbol_item>& l, const std::vector<document_symbol_item>& r);
bool is_similar(const document_symbol_item& l, const document_symbol_item& r);

namespace context {
struct address;
class section;
class symbol;
} // namespace context
namespace expressions {
class ca_expression;
struct data_definition;
} // namespace expressions
namespace workspaces {
class workspace;
} // namespace workspaces
} // namespace hlasm_plugin::parser_library

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::parser_library::processing;
using namespace hlasm_plugin::parser_library::expressions;

void parse_all_files(hlasm_plugin::parser_library::workspaces::workspace& ws);

void run_if_valid(hlasm_plugin::utils::task t);

void open_parse_and_recollect_diags(hlasm_plugin::parser_library::workspaces::workspace& ws,
    const std::vector<hlasm_plugin::utils::resource::resource_location>& files);
void close_parse_and_recollect_diags(hlasm_plugin::parser_library::workspaces::workspace& ws,
    const std::vector<hlasm_plugin::utils::resource::resource_location>& files);

template<typename T>
std::optional<T> get_var_value(hlasm_context& ctx, std::string name);

template<typename T>
std::optional<T> get_global_var_value(hlasm_context& ctx, std::string name);

template<typename T>
std::optional<std::vector<T>> get_var_vector(hlasm_context& ctx, std::string name);

template<typename T>
std::optional<std::unordered_map<size_t, T>> get_var_vector_map(hlasm_context& ctx, std::string name);

template<typename CMsg,
    typename Proj,
    std::predicate<std::decay_t<std::invoke_result_t<Proj, const typename std::decay_t<CMsg>::value_type>&>,
        std::decay_t<std::invoke_result_t<Proj, const typename std::decay_t<CMsg>::value_type&>>> BinPred =
        std::equal_to<>,
    typename C =
        std::initializer_list<std::decay_t<std::invoke_result_t<Proj, const typename std::decay_t<CMsg>::value_type&>>>>
inline bool matches_message_properties(CMsg&& d, const C& c, Proj p, BinPred b = BinPred())
{
    std::vector<std::decay_t<std::invoke_result_t<Proj, const typename std::decay_t<CMsg>::value_type&>>> properties;
    std::ranges::transform(d, std::back_inserter(properties), std::ref(p));

    return std::ranges::is_permutation(properties, c, std::ref(b));
}

template<typename CMsg,
    typename Proj,
    typename C =
        std::initializer_list<std::decay_t<std::invoke_result_t<Proj, const typename std::decay_t<CMsg>::value_type&>>>>
inline bool contains_message_properties(CMsg&& d, const C& c, Proj p)
{
    if (d.size() < c.size())
        return false;

    std::vector<std::decay_t<std::invoke_result_t<Proj, const typename std::decay_t<CMsg>::value_type&>>> properties;
    std::ranges::transform(d, std::back_inserter(properties), std::ref(p));

    std::vector to_find(c.begin(), c.end());

    std::ranges::sort(properties);
    std::ranges::sort(to_find);

    return std::ranges::includes(properties, to_find);
}

template<typename CMsg, typename C = std::initializer_list<std::string>>
inline bool matches_message_codes(CMsg&& d, const C& c)
{
    return matches_message_properties(d, c, &std::decay_t<CMsg>::value_type::code);
}

template<typename CMsg, typename C = std::initializer_list<std::string>>
inline bool contains_message_codes(CMsg&& d, const C& c)
{
    return contains_message_properties(d, c, &std::decay_t<CMsg>::value_type::code);
}

template<typename CMsg, typename C = std::initializer_list<std::pair<size_t, size_t>>>
inline bool matches_diagnosed_line_ranges(CMsg&& d, const C& c)
{
    return matches_message_properties(
        d, c, [](const auto& d) { return std::make_pair(d.diag_range.start.line, d.diag_range.end.line); });
}

template<typename CMsg, typename C = std::initializer_list<std::pair<size_t, size_t>>>
inline bool contains_diagnosed_line_ranges(CMsg&& d, const C& c)
{
    return contains_message_properties(
        d, c, [](const auto& d) { return std::make_pair(d.diag_range.start.line, d.diag_range.end.line); });
}

template<typename CMsg, typename C = std::initializer_list<std::string>>
inline bool matches_message_text(CMsg&& d, const C& c)
{
    return matches_message_properties(d, c, &std::decay_t<CMsg>::value_type::message);
}

template<typename CMsg, typename C = std::initializer_list<std::string>>
inline bool contains_message_text(CMsg&& d, const C& c)
{
    return contains_message_properties(d, c, &std::decay_t<CMsg>::value_type::message);
}

template<typename CMsg, typename C = std::initializer_list<std::string>>
inline bool matches_partial_message_text(CMsg&& d, const C& c)
{
    return matches_message_properties(
        d, c, &std::decay_t<CMsg>::value_type::message, [](std::string_view a, std::string_view b) -> bool {
            return a.find(b) != std::string_view::npos;
        });
}

bool matches_fade_messages(const std::vector<fade_message>& a, const std::vector<fade_message>& b);

bool contains_fade_messages(const std::vector<fade_message>& a, const std::vector<fade_message>& b);

const section* get_section(hlasm_context& ctx, std::string name);

const symbol* get_symbol(hlasm_context& ctx, std::string name);

std::optional<int32_t> get_symbol_abs(hlasm_context& ctx, std::string name);

std::optional<address> get_symbol_reloc(hlasm_context& ctx, std::string name);

std::optional<std::pair<int, std::string>> get_symbol_address(hlasm_context& ctx, std::string name);

size_t get_syntax_errors(analyzer& a);

std::unique_ptr<expressions::ca_expression> parse_ca_expression(analyzer& a);
expressions::data_definition parse_data_definition(analyzer& a, diagnostic_consumer_t<diagnostic_op>* diag = nullptr);

#endif
