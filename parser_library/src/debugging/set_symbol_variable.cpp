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

#include <optional>

#include "context/variables/set_symbol.h"
#include "debug_types.h"
#include "variable.h"

namespace hlasm_plugin::parser_library::debugging {
namespace {
std::string get_string_array_value(const context::set_symbol_base& set_sym);
std::string get_string_value(const context::set_symbol_base& set_sym, const std::optional<int>& index);

std::string get_string_array_value(const context::set_symbol_base& set_sym)
{
    std::string array_value;
    array_value.append("(");

    auto keys = set_sym.keys();

    if (keys.empty())
    {
        array_value.append(")");
        return array_value;
    }

    for (const auto& key : keys)
    {
        array_value.append(get_string_value(set_sym, key));
        array_value.append(",");
    }
    array_value.back() = ')';

    return array_value;
}

template<typename T>
inline T get_value(const context::set_symbol_base& set_sym, const std::optional<int>& index)
{
    if (!set_sym.is_scalar && index)
        return set_sym.access_set_symbol<T>()->get_value(*index);
    else
        return set_sym.access_set_symbol<T>()->get_value();
}

std::string get_string_value(const context::set_symbol_base& set_sym, const std::optional<int>& index)
{
    using enum context::SET_t_enum;
    if (!index && !set_sym.is_scalar)
        return get_string_array_value(set_sym);
    else if (set_sym.type == A_TYPE)
        return std::to_string(get_value<context::A_t>(set_sym, index));
    else if (set_sym.type == B_TYPE)
        return get_value<context::B_t>(set_sym, index) ? "TRUE" : "FALSE";
    else
        return get_value<context::C_t>(set_sym, index);
}

set_type to_set_type(context::SET_t_enum e)
{
    switch (e)
    {
        case context::SET_t_enum::A_TYPE:
            return set_type::A_TYPE;
        case context::SET_t_enum::B_TYPE:
            return set_type::B_TYPE;
        case context::SET_t_enum::C_TYPE:
            return set_type::C_TYPE;
        case context::SET_t_enum::UNDEF_TYPE:
            return set_type::UNDEF_TYPE;
    }
}
} // namespace

variable generate_set_symbol_variable(const context::set_symbol_base& set_sym, int32_t index)
{
    return variable {
        .name = std::to_string(index),
        .value = get_string_value(set_sym, index),
        .type = to_set_type(set_sym.type),
    };
}

variable generate_set_symbol_variable(const context::set_symbol_base& set_sym)
{
    variable result {
        .name = "&" + set_sym.id.to_string(),
        .value = get_string_value(set_sym, std::nullopt),
        .type = to_set_type(set_sym.type),
    };

    if (!set_sym.is_scalar)
    {
        result.values = [&set_sym]() {
            std::vector<variable> vars;

            for (const auto& key : set_sym.keys())
                vars.emplace_back(generate_set_symbol_variable(set_sym, key));

            return vars;
        };
    }

    return result;
}

} // namespace hlasm_plugin::parser_library::debugging
