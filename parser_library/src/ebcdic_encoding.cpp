/*
 * Copyright (c) 2019 Broadcom.
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

#include "ebcdic_encoding.h"

namespace hlasm_plugin::parser_library {

std::pair<unsigned char, const char*> ebcdic_encoding::to_ebcdic_multibyte(const char* c, const char* const ce) noexcept
{
    const unsigned char first_byte = *(c + 0);
    if (c + 1 == ce)
    {
        return { EBCDIC_SUB, c + 1 };
    }
    const unsigned char second_byte = *(c + 1);

    if ((first_byte & 0xE0) == 0xC0) // 110xxxxx 10xxxxxx
    {
        const auto value = ((first_byte & 0x1F) << 6) | (second_byte & 0x3F);
        return { value < std::ssize(a2e) ? a2e[value] : EBCDIC_SUB, c + 2 };
    }

    if (c + 2 == ce)
    {
        return { EBCDIC_SUB, c + 2 };
    }
    const unsigned char third_byte = *(c + 2);

    if (first_byte == (0b11100000 | ebcdic_encoding::unicode_private >> 4)
        && (second_byte & 0b11111100) == (0x80 | (ebcdic_encoding::unicode_private & 0xF) << 2)
        && (third_byte & 0xC0) == 0x80) // our private plane
    {
        const unsigned char ebcdic_value = (second_byte & 3) << 6 | third_byte & 0x3f;
        return { ebcdic_value, c + 3 };
    }

    if ((first_byte & 0xF0) == 0xE0) // 1110xxxx 10xxxxxx 10xxxxxx
    {
        return { EBCDIC_SUB, c + 3 };
    }

    if (c + 3 == ce)
    {
        return { EBCDIC_SUB, c + 3 };
    }

    if ((first_byte & 0xF8) == 0xF0) // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    {
        return { EBCDIC_SUB, c + 4 };
    }

    return { EBCDIC_SUB, c + 1 };
}

std::string ebcdic_encoding::to_ascii(const std::string& s)
{
    std::string a;
    a.reserve(s.length());
    for (unsigned char c : s)
        a.append(to_ascii(c));
    return a;
}

std::string ebcdic_encoding::to_ebcdic(const std::string& s)
{
    std::string a;
    a.reserve(s.length());
    const auto end = std::to_address(s.end());
    for (const char* i = s.data(); i != end;)
    {
        const auto [ch, newi] = to_ebcdic(i, end);
        a.push_back(ch);
        i = newi;
    }
    return a;
}
} // namespace hlasm_plugin::parser_library
