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

#ifndef HLASMPLUGIN_HLASMPARSERLIBRARY_LOGICAL_LINE_H
#define HLASMPLUGIN_HLASMPARSERLIBRARY_LOGICAL_LINE_H

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "utils/unicode_text.h"

namespace hlasm_plugin::parser_library::lexing {

// termination character of a line in a file
enum class logical_line_segment_eol : uint8_t
{
    none,
    lf,
    crlf,
    cr,
};

// HLASM logical line/statement representation
// segment 1: <code..............................><continuation><ignore...><logical_line_segment_eol>
// segment 2:              <code.................><continuation><ignore...><logical_line_segment_eol>
// segment 3:              <code.................><ignore.................><logical_line_segment_eol>

// a single line in a file that is a part of the logical (continued) HLASM line/statement.
template<typename It>
struct logical_line_segment
{
    It begin;
    It code;
    It continuation;
    It ignore;
    It end;

    bool continuation_error;
    bool so_si_continuation;

    logical_line_segment_eol eol;

    It skipped_begin() const noexcept { return begin; }
    It skipped_end() const noexcept { return code; }

    It code_begin() const noexcept { return code; }
    It code_end() const noexcept { return continuation; }

    It continuation_begin() const noexcept { return continuation; }
    It continuation_end() const noexcept { return ignore; }

    It ignore_begin() const noexcept { return ignore; }
    It ignore_end() const noexcept { return end; }

    It line_begin() const noexcept { return begin; }
    It line_end() const noexcept { return end; }
};

template<typename It>
size_t logical_distance(It b, It e)
{
    return std::ranges::distance(b, e);
}

template<utils::HasCounter It>
size_t logical_distance(It b, It e)
{
    return e.counter() - b.counter();
}

template<typename It>
struct logical_line;

template<typename It>
struct logical_line_const_iterator
{
    using segment_iterator = typename std::vector<logical_line_segment<It>>::const_iterator;
    using column_iterator = It;

    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = typename std::iterator_traits<It>::value_type;
    using pointer = typename std::iterator_traits<It>::pointer;
    using reference = typename std::iterator_traits<It>::reference;

    using iterator_concept = std::bidirectional_iterator_tag;

    logical_line_const_iterator() = default;
    logical_line_const_iterator(
        segment_iterator segment_it, column_iterator col_it, const logical_line<It>* ll) noexcept
        : m_segment_it(segment_it)
        , m_col_it(col_it)
        , m_logical_line(ll)
    {}

    reference operator*() const noexcept { return *m_col_it; }
    pointer operator->() const noexcept requires std::is_pointer_v<It> { return m_col_it; }
    pointer operator->() const noexcept(noexcept(m_col_it.operator->())) { return m_col_it.operator->(); }
    logical_line_const_iterator& operator++() noexcept
    {
        assert(m_logical_line);
        ++m_col_it;
        while (m_col_it == m_segment_it->continuation)
        {
            if (++m_segment_it == m_logical_line->segments.end())
            {
                m_col_it = column_iterator();
                break;
            }
            m_col_it = m_segment_it->code;
        }
        return *this;
    }
    logical_line_const_iterator operator++(int) noexcept
    {
        logical_line_const_iterator tmp = *this;
        ++(*this);
        return tmp;
    }
    logical_line_const_iterator& operator--() noexcept
    {
        assert(m_logical_line);
        while (m_segment_it == m_logical_line->segments.end() || m_col_it == m_segment_it->code)
        {
            --m_segment_it;
            m_col_it = m_segment_it->continuation;
        }
        --m_col_it;
        return *this;
    }
    logical_line_const_iterator operator--(int) noexcept
    {
        logical_line_const_iterator tmp = *this;
        --(*this);
        return tmp;
    }

    friend bool operator==(const logical_line_const_iterator& a, const logical_line_const_iterator& b) noexcept
    {
        assert(a.m_logical_line == b.m_logical_line);
        return a.m_segment_it == b.m_segment_it && a.m_col_it == b.m_col_it;
    }

    bool same_line(const logical_line_const_iterator& o) const noexcept
    {
        assert(m_logical_line == o.m_logical_line);
        return m_segment_it == o.m_segment_it;
    }

    std::pair<size_t, size_t> get_coordinates() const noexcept
    {
        assert(m_logical_line);

        if (m_segment_it == m_logical_line->segments.end())
            return { 0, 0 };

        return { logical_distance(m_segment_it->begin, m_col_it),
            logical_distance(m_logical_line->segments.begin(), m_segment_it) };
    }

    auto to_address() const noexcept { return std::to_address(m_col_it); }

    friend difference_type operator-(const logical_line_const_iterator& e,
        const logical_line_const_iterator& b) noexcept requires std::sized_sentinel_for<It, It>
    {
        assert(e.m_logical_line == b.m_logical_line);
        if (e.m_segment_it == b.m_segment_it)
            return std::ranges::distance(b.m_col_it, e.m_col_it);

        if (e.m_segment_it < b.m_segment_it)
            return -(b - e);

        /*
         * |      >XXXXXXXXXX
         * | XXXXXXXXXXXXXXXX
         * | XXXXXXXXXXXXXXXX
         * | XXXXXX<
         */

        size_t result = std::ranges::distance(b.m_col_it, b.m_segment_it->continuation);

        for (auto it = std::next(b.m_segment_it); it != e.m_segment_it; ++it)
            result += std::ranges::distance(it->code, it->continuation);

        if (e.m_segment_it != e.m_logical_line->segments.end())
            result += std::ranges::distance(e.m_segment_it->code, e.m_col_it);

        return result;
    }

private:
    segment_iterator m_segment_it = segment_iterator();
    column_iterator m_col_it = It();
    const logical_line<It>* m_logical_line = nullptr;
};


// represents a single (possibly continued) HLASM line/statement
template<typename It>
struct logical_line
{
    using const_iterator = logical_line_const_iterator<It>;

    std::vector<logical_line_segment<It>> segments;
    bool continuation_error;
    bool so_si_continuation;
    bool missing_next_line;

    void clear() noexcept
    {
        segments.clear();
        continuation_error = false;
        so_si_continuation = false;
        missing_next_line = false;
    }

    const_iterator begin() const noexcept
    {
        for (auto s = segments.begin(); s != segments.end(); ++s)
            if (s->code != s->continuation)
                return const_iterator(s, s->code, this);
        return end();
    }

    const_iterator end() const noexcept { return const_iterator(segments.end(), It(), this); }
};

// defines the layout of the hlasm source file and options to follow for line extraction
struct logical_line_extractor_args
{
    std::size_t begin; // 1-40
    std::size_t end; // 41-80
    std::size_t continuation; // begin+1-40, or 0 if off
    bool dbcs;
    bool eof_copy_rules;
};

constexpr const logical_line_extractor_args default_ictl = { 1, 71, 16, false, false };
constexpr const logical_line_extractor_args default_ictl_dbcs = { 1, 71, 16, true, false };
constexpr const logical_line_extractor_args default_ictl_copy = { 1, 71, 16, false, true };
constexpr const logical_line_extractor_args default_ictl_dbcs_copy = { 1, 71, 16, true, true };

// remove and return a single line from the input (terminated by LF, CRLF, CR, EOF)
std::pair<std::string_view, logical_line_segment_eol> extract_line(std::string_view& input);

template<typename It, typename Sentinel>
std::pair<std::pair<It, It>, logical_line_segment_eol> extract_line(It& input, const Sentinel& s)
{
    auto start = input;
    typename std::iterator_traits<It>::value_type c {};
    while (input != s)
    {
        c = *input;
        if (c == '\r' || c == '\n')
            break;
        ++input;
    }
    auto end = input;
    if (input == s)
        return std::make_pair(std::make_pair(start, end), logical_line_segment_eol::none);
    ++input;
    if (c == '\n')
        return std::make_pair(std::make_pair(start, end), logical_line_segment_eol::lf);
    if (input == s || *input != '\n')
        return std::make_pair(std::make_pair(start, end), logical_line_segment_eol::cr);
    ++input;

    return std::make_pair(std::make_pair(start, end), logical_line_segment_eol::crlf);
}

// appends a logical line segment to the logical line extracted from the input
// returns "need more" (appended line was continued), input must be non-empty
template<typename It, typename Sentinel>
bool append_to_logical_line(
    logical_line<std::remove_cvref_t<It>>& out, It&& input, const Sentinel& s, const logical_line_extractor_args& opts)
{
    auto [line_its, eol] = extract_line(input, s);

    auto& segment = out.segments.emplace_back();

    auto& it = line_its.first;
    const auto& end = line_its.second;

    segment.begin = it;
    utils::utf8_next(it, opts.begin - 1, end);
    segment.code = it;
    utils::utf8_next(it, opts.end + 1 - opts.begin, end);
    segment.continuation = it;
    utils::utf8_next(it, 1, end);
    segment.ignore = it;
    segment.end = end;
    segment.eol = eol;

    if (segment.continuation == segment.ignore)
        return false;

    if (*segment.continuation == ' ' || opts.end == 80 || opts.continuation == 0)
    {
        segment.ignore = segment.continuation;
        return false;
    }

    // line is continued

    if (opts.dbcs)
    {
        auto extended_cont = std::mismatch(std::make_reverse_iterator(segment.continuation),
            std::make_reverse_iterator(segment.code),
            std::make_reverse_iterator(segment.ignore))
                                 .first.base();

        utils::utf8_next(extended_cont, 0, segment.continuation);

        if (extended_cont != segment.continuation)
        {
            segment.continuation = extended_cont;
            if (const auto c = *segment.continuation; c == '<' || c == '>')
                out.so_si_continuation |= segment.so_si_continuation = true;
        }
    }

    return true;
}

template<typename Range>
std::pair<bool, decltype(std::begin(std::declval<Range&&>()))> append_to_logical_line(
    logical_line<decltype(std::begin(std::declval<Range&&>()))>& out,
    Range&& range,
    const logical_line_extractor_args& opts)
{
    std::pair<bool, decltype(std::begin(std::declval<Range&&>()))> result(false, std::begin(range));
    result.first = append_to_logical_line(out, result.second, std::end(range), opts);
    return result;
}

// logical line post-processing
template<typename It>
void finish_logical_line(logical_line<It>& out, const logical_line_extractor_args& opts)
{
    if (out.segments.empty())
        return;

    const size_t cont_size = opts.continuation - opts.begin;
    for (size_t i = 1; i < out.segments.size(); ++i)
    {
        auto& s = out.segments[i];

        auto blank_start = s.code;
        utils::utf8_next(s.code, cont_size, s.continuation);

        out.continuation_error |= s.continuation_error =
            std::any_of(blank_start, s.code, [](unsigned char c) { return c != ' '; });
    }
    auto& last = out.segments.back();
    if (!opts.eof_copy_rules)
    {
        out.missing_next_line = last.continuation != last.ignore;
    }
    else
        last.ignore = last.continuation;
}

// extract a logical line (extracting lines while continued and not EOF)
// returns true when a logical line was extracted
template<typename It, typename Sentinel>
bool extract_logical_line(
    logical_line<std::remove_cvref_t<It>>& out, It&& input, const Sentinel& s, const logical_line_extractor_args& opts)
{
    out.clear();

    if (input == s)
        return false;

    do
    {
        if (!append_to_logical_line(out, input, s, opts))
            break;
    } while (input != s);

    finish_logical_line(out, opts);

    return true;
}

template<typename Range>
std::pair<bool, decltype(std::begin(std::declval<Range&&>()))> extract_logical_line(
    logical_line<decltype(std::begin(std::declval<Range&&>()))>& out,
    Range&& range,
    const logical_line_extractor_args& opts)
{
    std::pair<bool, decltype(std::begin(std::declval<Range&&>()))> result(false, std::begin(range));
    result.first = extract_logical_line(out, result.second, std::end(range), opts);
    return result;
}

} // namespace hlasm_plugin::parser_library::lexing


namespace std {
template<std::bidirectional_iterator It>
struct pointer_traits<::hlasm_plugin::parser_library::lexing::logical_line_const_iterator<It>>
{
    using pointer = ::hlasm_plugin::parser_library::lexing::logical_line_const_iterator<It>;
    using element_type = typename pointer_traits<It>::element_type;
    using difference_type = typename pointer_traits<It>::difference_type;

    static element_type* to_address(::hlasm_plugin::parser_library::lexing::logical_line_const_iterator<It> p) noexcept
    {
        return p.to_address();
    }
};
} // namespace std

#endif // HLASMPLUGIN_HLASMPARSERLIBRARY_LOGICAL_LINE_H
