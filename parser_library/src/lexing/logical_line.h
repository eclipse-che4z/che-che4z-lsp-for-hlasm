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

#include <cassert>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace hlasm_plugin::parser_library::lexing {

class utf8_error : public std::runtime_error
{
public:
    utf8_error()
        : std::runtime_error("Invalid UTF-8 sequence encountered.")
    {}
};

// termination character of a line in a file
enum class logical_line_segment_eol : uint8_t
{
    none,
    lf,
    crlf,
    cr,
};

// HLASM logical line/statment representation
// segment 1: <code..............................><continuation><ignore...><logical_line_segment_eol>
// segment 2:              <code.................><continuation><ignore...><logical_line_segment_eol>
// segment 3:              <code.................><ignore.................><logical_line_segment_eol>

// a single line in a file that is a part of the logical (continued) HLASM line/statement.
struct logical_line_segment
{
    std::string_view code;
    std::string_view continuation;
    std::string_view ignore;

    std::string_view line;

    size_t code_offset;
    size_t code_offset_utf16;

    bool continuation_error;
    bool so_si_continuation;

    logical_line_segment_eol eol;
};

// represents a single (possibly continued) HLASM line/statement
struct logical_line
{
    std::vector<logical_line_segment> segments;
    bool continuation_error;
    bool so_si_continuation;
    bool missing_next_line;

    void clear()
    {
        segments.clear();
        continuation_error = false;
        so_si_continuation = false;
        missing_next_line = false;
    }

    struct const_iterator
    {
        using segment_iterator = std::vector<logical_line_segment>::const_iterator;
        using column_iterator = std::string_view::const_iterator;

        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = char;
        using pointer = const char*;
        using reference = const char&;

        const_iterator() = default;
        const_iterator(segment_iterator segment, column_iterator col, const logical_line* ll)
            : m_segment(segment)
            , m_col(col)
            , m_logical_line(ll)
        {}

        reference operator*() const noexcept { return *m_col; }
        pointer operator->() const noexcept { return &*m_col; }
        const_iterator& operator++() noexcept
        {
            assert(m_logical_line);
            ++m_col;
            while (m_col == m_segment->code.end())
            {
                if (++m_segment == m_logical_line->segments.end())
                {
                    m_col = column_iterator();
                    break;
                }
                m_col = m_segment->code.begin();
            }
            return *this;
        }
        const_iterator operator++(int) noexcept
        {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        const_iterator& operator--() noexcept
        {
            assert(m_logical_line);
            while (m_segment == m_logical_line->segments.end() || m_col == m_segment->code.begin())
            {
                --m_segment;
                m_col = m_segment->code.end();
            }
            --m_col;
            return *this;
        }
        const_iterator operator--(int) noexcept
        {
            const_iterator tmp = *this;
            --(*this);
            return tmp;
        }
        friend bool operator==(const const_iterator& a, const const_iterator& b) noexcept
        {
            assert(a.m_logical_line == b.m_logical_line);
            return a.m_segment == b.m_segment && a.m_col == b.m_col;
        }
        friend bool operator!=(const const_iterator& a, const const_iterator& b) noexcept { return !(a == b); }

        bool same_line(const const_iterator& o) const noexcept
        {
            assert(m_logical_line == o.m_logical_line);
            return m_segment == o.m_segment;
        }

    private:
        segment_iterator m_segment = segment_iterator();
        column_iterator m_col = std::string_view::const_iterator();
        const logical_line* m_logical_line = nullptr;
    };

    const_iterator begin() const noexcept
    {
        for (auto s = segments.begin(); s != segments.end(); ++s)
            if (!s->code.empty())
                return const_iterator(s, s->code.begin(), this);
        return end();
    }
    const_iterator end() const noexcept
    {
        return const_iterator(segments.end(), std::string_view::const_iterator(), this);
    }
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

// extract a logical line (extacting lines while continued and not EOF)
// returns true when a logical line was extracted
bool extract_logical_line(logical_line& out, std::string_view& input, const logical_line_extractor_args& opts);

// appends a logical line segment to the logical line extracted from the input
// returns "need more" (appended line was continued), input must be non-empty
bool append_to_logical_line(logical_line& out, std::string_view& input, const logical_line_extractor_args& opts);

// logical line post-processing
void finish_logical_line(logical_line& out, const logical_line_extractor_args& opts);

// skip <count> UTF-8 characters
// returns the remaining string and size of the skipped length in utf-16 encoding
std::pair<std::string_view, size_t> skip_chars(std::string_view s, size_t count);

struct utf8_substr_result
{
    std::string_view str;
    size_t char_count;
    size_t utf16_len;
};

inline bool operator==(const utf8_substr_result& l, const utf8_substr_result& r)
{
    return l.str == r.str && l.char_count == r.char_count && l.utf16_len == r.utf16_len;
}

inline bool operator!=(const utf8_substr_result& l, const utf8_substr_result& r) { return !(l == r); }

// utf-8 substr in unicode characters with optional validation
template<bool validate = false>
utf8_substr_result utf8_substr(std::string_view s, size_t offset_chars = 0, size_t length_chars = (size_t)-1);

// returns the length of the string in utf-16 symbols
size_t length_utf16(std::string_view text);

} // namespace hlasm_plugin::parser_library::lexing

#endif // HLASMPLUGIN_HLASMPARSERLIBRARY_LOGICAL_LINE_H