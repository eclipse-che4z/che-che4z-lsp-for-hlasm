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

#ifndef HLASMPLUGIN_UTILS_UTF8TEXT_H
#define HLASMPLUGIN_UTILS_UTF8TEXT_H

#include <array>
#include <limits>
#include <string>
#include <string_view>

namespace hlasm_plugin::utils {

// Length of Unicode character in 8/16-bit chunks
struct char_size
{
    uint8_t utf8 : 4;
    uint8_t utf16 : 4;
};

// Map first byte of UTF-8 encoded Unicode character to char_size
extern constinit const std::array<char_size, 256> utf8_prefix_sizes;

constexpr const char substitute_character = 0x1a;

extern constinit const std::array<unsigned char, 128> utf8_valid_multibyte_prefix_table;

inline bool utf8_valid_multibyte_prefix(unsigned char first, unsigned char second)
{
    if (first < 0xc0)
        return false;
    unsigned bitid = (first - 0xC0) << 4 | second >> 4;
    return utf8_valid_multibyte_prefix_table[bitid / 8] & (0x80 >> bitid % 8);
}

void append_utf8_sanitized(std::string& result, std::string_view str);
} // namespace hlasm_plugin::utils

#endif