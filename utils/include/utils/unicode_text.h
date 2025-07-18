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

#ifndef HLASMPLUGIN_UTILS_UNICODE_TEXT_H
#define HLASMPLUGIN_UTILS_UNICODE_TEXT_H

#include <array>
#include <cassert>
#include <concepts>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace hlasm_plugin::utils {

class utf8_error : public std::runtime_error
{
public:
    utf8_error()
        : std::runtime_error("Invalid UTF-8 sequence encountered.")
    {}
};

// Length of Unicode character in 8/16-bit chunks
struct char_size
{
    uint8_t utf8 : 4;
    uint8_t utf16 : 4;
};

// Map first byte of UTF-8 encoded Unicode character to char_size
extern constinit const std::array<char_size, 256> utf8_prefix_sizes;

// Substitute character used by VSCode
constexpr char32_t substitute_character = 0xFFFD;

constexpr size_t max_utf8_sequence_length = 4;

extern constinit const std::array<unsigned char, 128> utf8_valid_multibyte_prefix_table;

inline bool utf8_valid_multibyte_prefix(unsigned char first, unsigned char second)
{
    if (first < 0xc0)
        return false;
    unsigned bitid = (first - 0xC0) << 4 | second >> 4;
    return utf8_valid_multibyte_prefix_table[bitid / 8] & (0x80 >> bitid % 8);
}

enum class character_replaced : bool
{
    no,
    yes,
};

character_replaced append_utf8_sanitized(std::string& result, std::string_view str);

bool utf8_one_byte_begin(char ch);

std::string replace_non_utf8_chars(std::string_view text);

// skip <count> UTF-8 characters
// returns the remaining string and size of the skipped length in utf-16 encoding
std::pair<std::string_view, size_t> skip_chars(std::string_view s, size_t count);

struct utf8_substr_result
{
    std::string_view str;
    size_t char_count;
    size_t utf16_len;
    bool offset_valid;

    bool operator==(const utf8_substr_result&) const noexcept = default;
};

// utf-8 substr in unicode characters with optional validation
template<bool validate = false>
utf8_substr_result utf8_substr(std::string_view s, size_t offset_chars = 0, size_t length_chars = (size_t)-1);
template<bool validate = false>
void utf8_resize(std::string& s, size_t count, char pad);

// returns the length of the string in utf-16 symbols
size_t length_utf16(std::string_view text);
size_t length_utf16_no_validation(std::string_view text);

// return the length in unicode codepoints
size_t length_utf32(std::string_view text);
size_t length_utf32_no_validation(std::string_view text);

template<size_t>
struct counter_index_t
{};

template<size_t n>
constexpr counter_index_t<n> counter_index = {};

template<typename T>
concept HasCounter = requires(const T& c) {
    { c.counter() } noexcept -> std::same_as<size_t>;
};
template<typename T, size_t n>
concept HasCounters = requires(const T& c) {
    { c.counter(counter_index<n>) } noexcept -> std::same_as<size_t>;
};

template<typename T>
concept ByteCounter = std::semiregular<T> && HasCounter<T> && requires(T t, unsigned char c) {
    { t.add(c) } noexcept;
    { t.remove(c) } noexcept;
};

class utf8_dummy_counter
{
public:
    utf8_dummy_counter() = default;
    explicit utf8_dummy_counter(size_t) {};

    void add(unsigned char) noexcept {}
    void remove(unsigned char) noexcept {}
    size_t counter() const noexcept { return 0; }
};

class utf8_byte_counter
{
    size_t m_value = 0;

public:
    utf8_byte_counter() = default;
    explicit utf8_byte_counter(size_t value)
        : m_value(value) {};

    void add(unsigned char) noexcept { ++m_value; }
    void remove(unsigned char) noexcept { --m_value; }
    size_t counter() const noexcept { return m_value; }
};

class utf8_utf16_counter
{
    size_t m_value = 0;

    static constexpr auto utf16_lengths = []() {
        unsigned long long v = 0;

        for (size_t i = 0; i < 0x100; i += 8)
        {
            unsigned long long bits = 0;
            if (i <= 0b0111'1111)
                bits = 1;
            else if (0b1100'0000 <= i && i <= 0b1101'1111)
                bits = 1;
            else if (0b1110'0000 <= i && i <= 0b1110'1111)
                bits = 1;
            else if (0b1111'0000 <= i && i <= 0b1111'0111)
                bits = 2;
            v |= bits << (i >> 3 << 1);
        }

        return v;
    }();

public:
    utf8_utf16_counter() = default;
    explicit utf8_utf16_counter(size_t value)
        : m_value(value) {};

    void add(unsigned char c) noexcept { m_value += utf16_lengths >> (c >> 3 << 1) & 0b11; }
    void remove(unsigned char c) noexcept { m_value -= utf16_lengths >> (c >> 3 << 1) & 0b11; }
    size_t counter() const noexcept { return m_value; }
};

class utf8_utf32_counter
{
    size_t m_value = 0;

public:
    utf8_utf32_counter() = default;
    explicit utf8_utf32_counter(size_t value)
        : m_value(value) {};

    void add(unsigned char c) noexcept { m_value += (c & 0xc0) != 0x80; }
    void remove(unsigned char c) noexcept { m_value -= (c & 0xc0) != 0x80; }
    size_t counter() const noexcept { return m_value; }
};

template<ByteCounter... Counters>
class utf8_multicounter : Counters...
{
    template<typename>
    using replace_size_t = size_t;

public:
    utf8_multicounter() = default;
    explicit utf8_multicounter(replace_size_t<Counters>... counters)
        : Counters(counters)...
    {}
    void add(unsigned char c) noexcept { (Counters::add(c), ...); }
    void remove(unsigned char c) noexcept { (Counters::remove(c), ...); }
    template<size_t n>
    size_t counter(counter_index_t<n> = {}) const noexcept
    {
        return [this]<size_t... idx>(std::index_sequence<idx...>) {
            return ((idx == n ? static_cast<const Counters*>(this)->counter() : 0) + ...);
        }(std::index_sequence_for<Counters...>());
    }
    size_t counter() const noexcept { return counter(counter_index<0>); }
};

template<std::bidirectional_iterator BidirIt, ByteCounter Counter = utf8_dummy_counter>
class utf8_iterator : Counter
{
    BidirIt m_base;

public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = typename std::iterator_traits<BidirIt>::difference_type;
    using value_type = typename std::iterator_traits<BidirIt>::value_type;
    using pointer = typename std::iterator_traits<BidirIt>::pointer;
    using reference = typename std::iterator_traits<BidirIt>::reference;

    using iterator_concept = std::bidirectional_iterator_tag;

    utf8_iterator() requires(std::is_default_constructible_v<BidirIt>)
        : m_base(BidirIt())
    {}

    explicit utf8_iterator(BidirIt it)
        : m_base(std::move(it))
    {}

    explicit utf8_iterator(BidirIt it, size_t counter) requires std::is_constructible_v<Counter, size_t>
        : Counter(counter)
        , m_base(std::move(it))
    {}

    explicit utf8_iterator(BidirIt it, Counter counter)
        : Counter(std::move(counter))
        , m_base(std::move(it))
    {}

    auto base() const noexcept { return m_base; }

    size_t counter() const noexcept requires HasCounter<Counter> { return Counter::counter(); }

    template<size_t n>
    size_t counter(counter_index_t<n> idx = {}) const noexcept requires HasCounters<Counter, n>
    {
        return Counter::counter(idx);
    }

    bool operator==(const utf8_iterator& o) const noexcept { return m_base == o.m_base; }
    bool operator==(const BidirIt& o) const noexcept { return m_base == o; }

    utf8_iterator& operator++()
    {
        Counter::add(*m_base);
        ++m_base;
        return *this;
    }
    utf8_iterator operator++(int)
    {
        auto ret = *this;
        operator++();
        return ret;
    }

    utf8_iterator& operator--()
    {
        --m_base;
        Counter::remove(*m_base);
        return *this;
    }
    utf8_iterator operator--(int)
    {
        auto ret = *this;
        operator--();
        return ret;
    }

    auto& operator*() const noexcept(noexcept(*m_base)) { return *m_base; }
    auto operator->() const noexcept requires std::is_pointer_v<BidirIt> { return m_base; }
    auto operator->() const noexcept(noexcept(m_base.operator->())) { return m_base.operator->(); }

    friend difference_type operator-(const utf8_iterator& l, const utf8_iterator& r) noexcept
        requires std::sized_sentinel_for<BidirIt, BidirIt>
    {
        return l.m_base - r.m_base;
    }
    friend difference_type operator-(const BidirIt& l, const utf8_iterator& r) noexcept
        requires std::sized_sentinel_for<BidirIt, BidirIt>
    {
        return l - r.m_base;
    }
    friend difference_type operator-(const utf8_iterator& l, const BidirIt& r) noexcept
        requires std::sized_sentinel_for<BidirIt, BidirIt>
    {
        return l.m_base - r;
    }

    auto to_address() const noexcept { return std::to_address(m_base); }
};


// skips n full utf-8 characters
// for n == 0 skips partial sequence
template<typename It, typename Sentinel>
void utf8_next(It& it, size_t n, const Sentinel& end)
{
    while (it != end)
    {
        if (unsigned char c = *it; (c & 0xc0) == 0x80)
        {
            ++it;
            continue;
        }
        if (n == 0)
            break;
        ++it;
        --n;
    }
}

// returns back by n utf-8 characters
// n positive
template<typename It, typename Sentinel>
void utf8_prev(It& it, size_t n, const Sentinel& begin)
{
    assert(n);
    while (it != begin)
    {
        --it;
        if (unsigned char c = *it; (c & 0xc0) == 0x80)
            continue;
        --n;
        if (n == 0)
            break;
    }
}

} // namespace hlasm_plugin::utils

namespace std {
template<std::bidirectional_iterator BidirIt, ::hlasm_plugin::utils::ByteCounter Counter>
struct pointer_traits<::hlasm_plugin::utils::utf8_iterator<BidirIt, Counter>>
{
    using pointer = ::hlasm_plugin::utils::utf8_iterator<BidirIt, Counter>;
    using element_type = typename pointer_traits<BidirIt>::element_type;
    using difference_type = typename pointer_traits<BidirIt>::difference_type;

    static element_type* to_address(::hlasm_plugin::utils::utf8_iterator<BidirIt, Counter> p) noexcept
    {
        return p.to_address();
    }
};
} // namespace std

#endif
