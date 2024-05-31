/*
 * Copyright (c) 2023 Broadcom.
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

#ifndef HLASMPLUGIN_UTILS_TEXT_MATCHERS_H
#define HLASMPLUGIN_UTILS_TEXT_MATCHERS_H

#include <array>
#include <cctype>
#include <concepts>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

namespace hlasm_plugin::utils::text_matchers {

template<typename It>
[[maybe_unused]] std::true_type same_line_detector(const It& t, decltype(t.same_line(t)) = false);
[[maybe_unused]] std::false_type same_line_detector(...);

template<typename It>
constexpr bool same_line(const It& l, const It& r)
{
    if constexpr (decltype(same_line_detector(l))::value)
        return l.same_line(r);
    else
        return true;
}

template<bool case_sensitive, bool single_line>
class basic_string_matcher
{
    std::string_view to_match;

public:
    // case_sensitive = false => to_match assumed in capitals
    explicit constexpr basic_string_matcher(std::string_view to_match)
        : to_match(to_match)
    {}

    template<typename It>
    bool operator()(It& b, const It& e) const noexcept
    {
        It work = b;

        for (auto c : to_match)
        {
            if (work == e)
                return false;
            if constexpr (single_line)
            {
                if (!same_line(work, b))
                    return false;
            }
            auto in_c = *work++;
            if constexpr (not case_sensitive)
                in_c = std::toupper((unsigned char)in_c);

            if (in_c != c)
                return false;
        }
        b = work;
        return true;
    }
};

template<bool negate>
class char_matcher_impl
{
    std::string_view to_match;

public:
    explicit constexpr char_matcher_impl(std::string_view to_match)
        : to_match(to_match)
    {}

    template<typename It>
    constexpr bool operator()(It& b, const It& e) const noexcept
    {
        if (b == e)
            return false;
        bool result = (to_match.find(*b) != std::string_view::npos) != negate;
        if (result)
            ++b;
        return result;
    }
};

using char_matcher = char_matcher_impl<false>;
using not_char_matcher = char_matcher_impl<true>;

class byte_matcher
{
    const std::array<bool, 1 + (unsigned char)-1>& table;

public:
    explicit constexpr byte_matcher(const std::array<bool, 1 + (unsigned char)-1>& table)
        : table(table)
    {}

    template<typename It>
    constexpr bool operator()(It& b, const It& e) const noexcept
    {
        if (b == e)
            return false;
        auto result = table[(unsigned char)*b];
        if (result)
            ++b;
        return result;
    }
};

class any_matcher
{
public:
    template<typename It>
    constexpr bool operator()(It& b, const It& e) const noexcept
    {
        if (b == e)
            return false;
        ++b;
        return true;
    }
};

class skip_to_end
{
public:
    template<typename It>
    constexpr bool operator()(It& b, const It& e) const noexcept
    {
        b = e;
        return true;
    }
};

template<typename Matcher>
constexpr auto star(Matcher&& matcher)
{
    return [matcher = std::forward<Matcher>(matcher)]<typename It>(It& b, const It& e) noexcept {
        while (matcher(b, e))
            ;
        return true;
    };
}

template<typename Matcher>
constexpr auto plus(Matcher&& matcher)
{
    return [matcher = std::forward<Matcher>(matcher)]<typename It>(It& b, const It& e) noexcept {
        bool matched = false;
        while (matcher(b, e))
            matched = true;

        return matched;
    };
}

template<size_t min, size_t max = min, typename Matcher>
constexpr auto times(Matcher&& matcher) requires(min <= max)
{
    return [matcher = std::forward<Matcher>(matcher)]<typename It>(It& b, const It& e) noexcept {
        auto work = b;
        size_t count = 0;
        while (count < max && matcher(work, e))
            ++count;
        if (count < min)
            return false;
        b = work;
        return true;
    };
}

template<bool allow_empty_match, bool single_line>
class space_matcher
{
public:
    template<typename It>
    constexpr bool operator()(It& b, const It& e) const noexcept
    {
        auto work = b;
        while (work != e)
        {
            if constexpr (single_line)
            {
                if (!same_line(work, b))
                    break;
            }
            if (*work != ' ')
                break;
            ++work;
        }
        if constexpr (not allow_empty_match)
        {
            if (work == b)
                return false;
        }
        b = work;
        return true;
    }
};

template<typename DefaultStringMatcher>
struct matcher_convertor
{
    template<typename T>
    constexpr T&& operator()(T&& t) const noexcept
    {
        return std::forward<T>(t);
    }
    template<std::convertible_to<std::string_view> T>
    constexpr DefaultStringMatcher operator()(T&& t) const noexcept(std::is_nothrow_convertible_v<T, std::string_view>)
    {
        return DefaultStringMatcher(std::forward<T>(t));
    }
};
template<typename DefaultStringMatcher>
constexpr const matcher_convertor<DefaultStringMatcher> convert_matcher;

template<typename DefaultStringMatcher = void, typename... Matchers>
constexpr auto seq(Matchers&&... matchers)
{
    return [... matchers = convert_matcher<DefaultStringMatcher>(std::forward<Matchers>(matchers))]<typename It>(
               It& b, const It& e) noexcept {
        auto work = b;
        return ((matchers(work, e) && ...)) && (b = work, true);
    };
}

template<typename DefaultStringMatcher = void, typename... Matchers>
constexpr auto alt(Matchers&&... matchers)
{
    return [... matchers = convert_matcher<DefaultStringMatcher>(std::forward<Matchers>(matchers))]<typename It>(
               It& b, const It& e) noexcept { return ((matchers(b, e) || ...)); };
}

template<typename Matcher>
constexpr auto opt(Matcher&& matcher)
{
    return [matcher = std::forward<Matcher>(matcher)]<typename It>(It& b, const It& e) noexcept {
        matcher(b, e);
        return true;
    };
}

class start_of_next_line
{
public:
    template<typename It>
    constexpr bool operator()(It& b, const It&) const noexcept
    {
        return !same_line(std::prev(b), b);
    }
};

template<typename It>
class start_of_line
{
    It start;

public:
    explicit start_of_line(It start)
        : start(std::move(start))
    {}
    constexpr bool operator()(It& b, const It& e) const noexcept { return b == start || start_of_next_line()(b, e); }
};

class end
{
public:
    template<typename It>
    constexpr bool operator()(It& b, const It& e) const noexcept
    {
        return b == e;
    }
};

template<typename It, typename Matcher>
constexpr auto capture(std::optional<std::pair<It, It>>& capture, Matcher&& matcher)
{
    return [&capture, matcher = std::forward<Matcher>(matcher)](It& b, const It& e) noexcept {
        auto work = b;
        if (matcher(b, e))
        {
            capture.emplace(work, b);
            return true;
        }
        else
        {
            capture.reset();
            return false;
        }
    };
}

template<typename It, typename Matcher>
constexpr auto capture(std::pair<It, It>& capture, Matcher&& matcher)
{
    return [&capture, matcher = std::forward<Matcher>(matcher)](It& b, const It& e) noexcept {
        auto work = b;
        if (matcher(b, e))
        {
            capture = std::pair<It, It>(work, b);
            return true;
        }
        return false;
    };
}

} // namespace hlasm_plugin::utils::text_matchers

#endif
