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

#include <array>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace hlasm_plugin::parser_library::lexing {

// Length of Unicode character in 8/16-bit chunks
struct char_size
{
    uint8_t utf8 : 4;
    uint8_t utf16 : 4;
};

// Map first byte of UTF-8 encoded Unicode character to char_size
constexpr const auto utf8_prefix_sizes = []() {
    std::array<char_size, 256> sizes = {};
    static_assert(std::numeric_limits<unsigned char>::max() < sizes.size());
    for (int i = 0b0000'0000; i <= 0b0111'1111; ++i)
        sizes[i] = { 1, 1 };
    for (int i = 0b1100'0000; i <= 0b1101'1111; ++i)
        sizes[i] = { 2, 1 };
    for (int i = 0b1110'0000; i <= 0b1110'1111; ++i)
        sizes[i] = { 3, 1 };
    for (int i = 0b1111'0000; i <= 0b1111'0111; ++i)
        sizes[i] = { 4, 2 };
    return sizes;
}();

constexpr const char substitute_character = 0x1a;

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