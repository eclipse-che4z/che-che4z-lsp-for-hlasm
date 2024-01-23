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

#ifndef HLASMPLUGIN_PARSER_HLASMEBCDIC_H
#define HLASMPLUGIN_PARSER_HLASMEBCDIC_H

#include <string>
#include <utility>

namespace hlasm_plugin::parser_library {

// Class that provides support for EBCDIC encoding.
class ebcdic_encoding
{
    static std::pair<unsigned char, const char*> to_ebcdic_multibyte(const char* c, const char* ce) noexcept;

    // clang-format off
    // IBM037 - CR,LF need to be handled separately via private plane
    static constexpr unsigned char a2e[256] = {
    // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
       0,  1,  2,  3, 55, 45, 46, 47, 22,  5, 37, 11, 12, 13, 14, 15, // 0x
      16, 17, 18, 19, 60, 61, 50, 38, 24, 25, 63, 39, 28, 29, 30, 31, // 1x
      64, 90,127,123, 91,108, 80,125, 77, 93, 92, 78,107, 96, 75, 97, // 2x
     240,241,242,243,244,245,246,247,248,249,122, 94, 76,126,110,111, // 3x
     124,193,194,195,196,197,198,199,200,201,209,210,211,212,213,214, // 4x
     215,216,217,226,227,228,229,230,231,232,233,186,224,187,176,109, // 5x
     121,129,130,131,132,133,134,135,136,137,145,146,147,148,149,150, // 6x
     151,152,153,162,163,164,165,166,167,168,169,192, 79,208,161,  7, // 7x
      32, 33, 34, 35, 36, 21,  6, 23, 40, 41, 42, 43, 44,  9, 10, 27, // 8x
      48, 49, 26, 51, 52, 53, 54,  8, 56, 57, 58, 59,  4, 20, 62,255, // 9x
      65,170, 74,177,159,178,106,181,189,180,154,138, 95,202,175,188, // ax
     144,143,234,250,190,160,182,179,157,218,155,139,183,184,185,171, // bx
     100,101, 98,102, 99,103,158,104,116,113,114,115,120,117,118,119, // cx
     172,105,237,238,235,239,236,191,128,253,254,251,252,173,174, 89, // dx
      68, 69, 66, 70, 67, 71,156, 72, 84, 81, 82, 83, 88, 85, 86, 87, // ex
     140, 73,205,206,203,207,204,225,112,221,222,219,220,141,142,223, // fx
    };
    static constexpr unsigned char e2a[256] = {
    // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
       0,  1,  2,  3,156,  9,134,127,151,141,142, 11, 12, 13, 14, 15, // 0x
      16, 17, 18, 19,157,133,  8,135, 24, 25,146,143, 28, 29, 30, 31, // 1x
     128,129,130,131,132, 10, 23, 27,136,137,138,139,140,  5,  6,  7, // 2x
     144,145, 22,147,148,149,150,  4,152,153,154,155, 20, 21,158, 26, // 3x
      32,160,226,228,224,225,227,229,231,241,162, 46, 60, 40, 43,124, // 4x
      38,233,234,235,232,237,238,239,236,223, 33, 36, 42, 41, 59,172, // 5x
      45, 47,194,196,192,193,195,197,199,209,166, 44, 37, 95, 62, 63, // 6x
     248,201,202,203,200,205,206,207,204, 96, 58, 35, 64, 39, 61, 34, // 7x
     216, 97, 98, 99,100,101,102,103,104,105,171,187,240,253,254,177, // 8x
     176,106,107,108,109,110,111,112,113,114,170,186,230,184,198,164, // 9x
     181,126,115,116,117,118,119,120,121,122,161,191,208,221,222,174, // ax
      94,163,165,183,169,167,182,188,189,190, 91, 93,175,168,180,215, // bx
     123, 65, 66, 67, 68, 69, 70, 71, 72, 73,173,244,246,242,243,245, // cx
     125, 74, 75, 76, 77, 78, 79, 80, 81, 82,185,251,252,249,250,255, // dx
      92,247, 83, 84, 85, 86, 87, 88, 89, 90,178,212,214,210,211,213, // ex
      48, 49, 50, 51, 52, 53, 54, 55, 56, 57,179,219,220,217,218,159, // fx
    };
    // clang-format on

public:
    static constexpr unsigned char SUB = 26;
    static constexpr unsigned char EBCDIC_SUB = a2e[SUB];

    // Converts an ASCII character to EBCDIC character.
    static constexpr unsigned char to_ebcdic(unsigned char c) noexcept { return a2e[c]; }
    // Converts an UTF-8 character to EBCDIC character.
    static constexpr std::pair<unsigned char, const char*> to_ebcdic(const char* c, const char* const ce) noexcept
    {
        if (const unsigned char first = *c; first < 0x80) [[likely]] // 0xxxxxxx
        {
            return { a2e[first], c + 1 };
        }
        else
            return to_ebcdic_multibyte(c, ce);
    }

    // Converts UTF-8 string to EBCDIC string.
    static std::string to_ebcdic(const std::string& s);
    // Converts EBCDIC character to UTF-8 string.
    static std::string to_ascii(unsigned char c)
    {
        if (c == 0x0D || c == 0x25) // CR LF
            return std::string {
                static_cast<char>(0b11100000 | ebcdic_encoding::unicode_private >> 4),
                static_cast<char>(0x80 | (ebcdic_encoding::unicode_private & 0xf) << 2 | c >> 6),
                static_cast<char>(0x80 | c & 0x3f),
            };
        auto val = e2a[c];
        if (val < 0x80)
            return std::string({ static_cast<char>(val) });
        else
            return std::string({ static_cast<char>(0xC0 | (val >> 6)), static_cast<char>(0x80 | (val & 63)) });
    }
    // Converts EBCDIC string to UTF-8 string.
    static std::string to_ascii(const std::string& s);

    static constexpr unsigned char unicode_private = 0xe0;

    friend consteval unsigned char operator""_ebcdic(char c) noexcept;
};

consteval unsigned char operator""_ebcdic(char c) noexcept
{
    return ebcdic_encoding::a2e[static_cast<unsigned char>(c)];
}

} // namespace hlasm_plugin::parser_library

#endif
