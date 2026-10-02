/*
 * Copyright (c) 2025 Broadcom.
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

#ifndef HLASMPARSER_PARSERLIBRARY_EXTERNAL_FUNCTIONS_H
#define HLASMPARSER_PARSERLIBRARY_EXTERNAL_FUNCTIONS_H

#include <concepts>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace hlasm_plugin::parser_library {
enum class external_function_type
{
    arithmetic,
    character,
};

class external_function_args
{
    struct arithmetic_t
    {
        std::span<const int32_t> args;
        std::int32_t result = 0;
    };
    struct character_t
    {
        std::span<const std::string_view> args;
        std::string result;
    };

    std::variant<arithmetic_t, character_t> m_data;
    std::optional<std::pair<uint8_t, std::string>> m_message;

public:
    explicit constexpr external_function_args(std::span<const int32_t> args) noexcept
        : m_data(std::in_place_type<arithmetic_t>, args)
    {}

    explicit constexpr external_function_args(std::span<const std::string_view> args) noexcept
        : m_data(std::in_place_type<character_t>, args)
    {}

    auto type() const noexcept { return static_cast<external_function_type>(m_data.index()); }

    auto* arithmetic() noexcept { return std::get_if<arithmetic_t>(&m_data); }
    auto* character() noexcept { return std::get_if<character_t>(&m_data); }

    auto& message() noexcept { return m_message; }
};

class external_function
{
    std::function<void(external_function_args&)> m_func;

public:
#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 230000
    template<typename T>
#else
    template<std::convertible_to<std::function<void(external_function_args&)>> T>
#endif
    explicit(false) external_function(T&& func) noexcept
        requires(!std::same_as<external_function, std::remove_cvref_t<T>>)
        : m_func(std::forward<T>(func))
    {}

    void operator()(external_function_args& arg) const { m_func(arg); }
};

using external_functions_list = std::vector<std::pair<std::string, external_function>>;

} // namespace hlasm_plugin::parser_library

#endif
