/*
 * Copyright (c) 2022 Broadcom.
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

#ifndef HLASMPLUGIN_UTILS_CONCAT_H
#define HLASMPLUGIN_UTILS_CONCAT_H

#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace hlasm_plugin::utils {

namespace detail {
struct concat_helper
{
    void operator()(std::string& s, std::string_view t) const { s.append(t); }
    template<typename T>
    std::enable_if_t<!std::is_convertible_v<T&&, std::string_view>> operator()(std::string& s, T&& t) const
    {
        s.append(std::to_string(std::forward<T>(t)));
    }

    constexpr static std::string_view span_sep = ", ";
    template<typename T>
    void operator()(std::string& s, typename std::span<T> span) const
    {
        bool first = true;
        for (const auto& e : span)
        {
            if (!first)
                s.append(span_sep);
            else
                first = false;

            operator()(s, e);
        }
    }

    size_t len(std::string_view t) const { return t.size(); }
    template<typename T>
    std::enable_if_t<!std::is_convertible_v<T&&, std::string_view>, size_t> len(const T&) const
    {
        return 8; // arbitrary estimate for the length of the stringified argument (typically small numbers)
    }
    template<typename T>
    size_t len(const typename std::span<T>& span) const
    {
        size_t result = 0;
        for (const auto& e : span)
            result += span_sep.size() + len(e);

        return result - (result ? span_sep.size() : 0);
    }
};

} // namespace detail

struct
{
    template<typename... Args>
    std::string operator()(Args&&... args) const
    {
        std::string result;

        detail::concat_helper h;

        result.reserve((... + h.len(std::as_const(args))));

        (h(result, std::forward<Args>(args)), ...);

        return result;
    }

} static constexpr concat;

} // namespace hlasm_plugin::utils

#endif
