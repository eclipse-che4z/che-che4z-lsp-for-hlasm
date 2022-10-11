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
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

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

constexpr const char substitute_character = 0x1a;

constexpr const size_t max_utf8_sequence_length = 4;

extern constinit const std::array<unsigned char, 128> utf8_valid_multibyte_prefix_table;

inline bool utf8_valid_multibyte_prefix(unsigned char first, unsigned char second)
{
    if (first < 0xc0)
        return false;
    unsigned bitid = (first - 0xC0) << 4 | second >> 4;
    return utf8_valid_multibyte_prefix_table[bitid / 8] & (0x80 >> bitid % 8);
}

void append_utf8_sanitized(std::string& result, std::string_view str);

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

// returns the length of the string in utf-16 symbols
size_t length_utf16(std::string_view text);
size_t length_utf16_no_validation(std::string_view text);

// return the length in unicode codepoints
size_t length_utf32(std::string_view text);
size_t length_utf32_no_validation(std::string_view text);

} // namespace hlasm_plugin::utils

#endif