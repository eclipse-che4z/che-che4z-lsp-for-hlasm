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

unsigned char hlasm_plugin::parser_library::ebcdic_encoding::to_pseudoascii(const char*& c)
{
    if ((unsigned char)*c < 0x80)
    {
        return *c;
    }
    else if (*(c + 1) == 0)
    {
        return SUB;
    }
    else
    {
        if ((*c & 0xE0) == 0xC0) // 110xxxxx 10xxxxxx
        {
            if ((*c & 0x1C) != 0)
            {
                ++c;
                return SUB;
            }
            else
            {
                unsigned char tmp = ((*c & 3) << 6) | (*(c + 1) & 0x3F);
                ++c;
                return tmp;
            }
        }
        else if ((*c & 0xF0) == 0xE0) // 1110xxxx 10xxxxxx 10xxxxxx
        {
            ++c;
            c += !!*c;
            return SUB;
        }
        else if ((*c & 0xF8) == 0xF0) // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        {
            ++c;
            c += !!*c;
            c += !!*c;
            return SUB;
        }
        else
        {
            return SUB;
        }
    }
}


unsigned char hlasm_plugin::parser_library::ebcdic_encoding::to_ebcdic(unsigned char c) { return a2e[c]; }

std::string hlasm_plugin::parser_library::ebcdic_encoding::to_ascii(unsigned char c)
{
    auto val = e2a[c];
    if (0x80 > val)
        return std::string({ static_cast<char>(val) });
    else
        return std::string({ static_cast<char>((val >> 6) | (3 << 6)), static_cast<char>((val & 63) | (1 << 7)) });
}

std::string hlasm_plugin::parser_library::ebcdic_encoding::to_ascii(const std::string& s)
{
    std::string a;
    a.reserve(s.length());
    for (unsigned char c : s)
        a.push_back(e2a[c]);
    return a;
}

std::string hlasm_plugin::parser_library::ebcdic_encoding::to_ebcdic(const std::string& s)
{
    std::string a;
    a.reserve(s.length());
    for (const char* i = s.c_str(); *i != 0; ++i)
        a.push_back(to_ebcdic(to_pseudoascii(i)));
    return a;
}
