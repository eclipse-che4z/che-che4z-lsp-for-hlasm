/*
 * Copyright (c) 2021 Broadcom.
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

#ifndef HLASMPLUGIN_UTILS_SIMILAR_H
#define HLASMPLUGIN_UTILS_SIMILAR_H

#include <memory>
#include <type_traits>
#include <utility>

namespace hlasm_plugin::utils {

namespace detail {
template<typename T>
void is_similar(const T&, const T&); // dummy if ADL fails

class is_similar_t
{
    template<typename T>
    static auto use_standalone(const T& t)
        -> std::enable_if_t<std::is_same_v<bool, decltype(is_similar(t, t))>, std::true_type>;
    static std::false_type use_standalone(...);

    template<typename T>
    static auto use_member(const T& t)
        -> std::enable_if_t<std::is_same_v<bool, decltype(t.is_similar(t))>, std::true_type>;
    static std::false_type use_member(...);

    template<typename T>
    static auto use_equal(const T& t) -> std::enable_if_t<std::is_same_v<bool, decltype(t == t)>, std::true_type>;
    static std::false_type use_equal(...);

public:
    template<typename T>
    bool operator()(const T& l, const T& r) const
    {
        constexpr bool standalone = decltype(use_standalone(l))::value;
        constexpr bool member = decltype(use_member(l))::value;
        constexpr bool equal = decltype(use_equal(l))::value;
        static_assert(standalone || member || equal, "is_similar or equal to operator not available");

        if constexpr (standalone)
            return is_similar(l, r);
        else if constexpr (member)
            return l.is_similar(r);
        else if constexpr (equal)
            return l == r; // if things are the same then they are also similar
    }

    template<typename T>
    bool operator()(const std::shared_ptr<T>& l, const std::shared_ptr<T>& r) const
    {
        return l == r || (l && r && operator()(*l, *r));
    }

    template<typename T, class D1, class D2>
    bool operator()(const std::unique_ptr<T, D1>& l, const std::unique_ptr<T, D2>& r) const
    {
        return l == r || (l && r && operator()(*l, *r));
    }
};
} // namespace detail

constexpr detail::is_similar_t is_similar;

template<typename T>
class is_similar_to
{
    const T& value;

public:
    is_similar_to(const T& t)
        : value(t)
    {}
    is_similar_to(T&&) = delete;

    bool operator()(const T& r) const { return is_similar(value, r); }
};

} // namespace hlasm_plugin::utils

#endif
