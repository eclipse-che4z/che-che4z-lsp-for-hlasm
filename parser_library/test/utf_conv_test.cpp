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

#include "gtest/gtest.h"

#include "common_testing.h"
#include "ebcdic_encoding.h"
#include "workspaces/file_impl.h"

TEST(input_source, utf8conv)
{
    std::string u8;

    u8.insert(u8.end(), (unsigned char)0xf0);
    u8.insert(u8.end(), (unsigned char)0x90);
    u8.insert(u8.end(), (unsigned char)0x80);
    u8.insert(u8.end(), (unsigned char)0x80);

    hlasm_plugin::parser_library::lexing::input_source input1(u8);

    EXPECT_EQ(u8, input1.getText({ (ssize_t)0, (ssize_t)0 }));

    u8.insert(u8.end(), (unsigned char)0xEA);
    u8.insert(u8.end(), (unsigned char)0x84);
    u8.insert(u8.end(), (unsigned char)0xA3);

    hlasm_plugin::parser_library::lexing::input_source input2(u8);

    EXPECT_EQ(u8, input2.getText({ (ssize_t)0, (ssize_t)1 }));

    u8.insert(u8.end(), (unsigned char)0xC5);
    u8.insert(u8.end(), (unsigned char)0x80);

    hlasm_plugin::parser_library::lexing::input_source input3(u8);

    EXPECT_EQ(u8, input3.getText({ (ssize_t)0, (ssize_t)2 }));

    u8.insert(u8.end(), (unsigned char)0x41);

    hlasm_plugin::parser_library::lexing::input_source input4(u8);

    EXPECT_EQ(u8, input4.getText({ (ssize_t)0, (ssize_t)3 }));
}

TEST(ebcdic_encoding, unicode)
{
    std::string u8;

    u8.insert(u8.end(), (unsigned char)0xf0);
    u8.insert(u8.end(), (unsigned char)0x90);
    u8.insert(u8.end(), (unsigned char)0x80);
    u8.insert(u8.end(), (unsigned char)0x80);

    u8.insert(u8.end(), (unsigned char)0xEA);
    u8.insert(u8.end(), (unsigned char)0x84);
    u8.insert(u8.end(), (unsigned char)0xA3);

    u8.insert(u8.end(), (unsigned char)0xC5);
    u8.insert(u8.end(), (unsigned char)0x80);

    u8.insert(u8.end(), (unsigned char)0x41);

    //�
    u8.insert(u8.end(), (unsigned char)0xC3);
    u8.insert(u8.end(), (unsigned char)0xA4);

    auto begin = u8.c_str();

    EXPECT_EQ(hlasm_plugin::parser_library::ebcdic_encoding::to_pseudoascii(begin),
        hlasm_plugin::parser_library::ebcdic_encoding::SUB);

    EXPECT_EQ(begin, u8.c_str() + 3);


    EXPECT_EQ(hlasm_plugin::parser_library::ebcdic_encoding::to_pseudoascii(++begin),
        hlasm_plugin::parser_library::ebcdic_encoding::SUB);

    EXPECT_EQ(begin, u8.c_str() + 4 + 2);


    EXPECT_EQ(hlasm_plugin::parser_library::ebcdic_encoding::to_pseudoascii(++begin),
        hlasm_plugin::parser_library::ebcdic_encoding::SUB);

    EXPECT_EQ(begin, u8.c_str() + 4 + 3 + 1);


    EXPECT_EQ(hlasm_plugin::parser_library::ebcdic_encoding::to_pseudoascii(++begin), 0x41);

    EXPECT_EQ(begin, u8.c_str() + 4 + 3 + 2);


    EXPECT_EQ(hlasm_plugin::parser_library::ebcdic_encoding::to_pseudoascii(++begin), (unsigned char)0xE4);

    EXPECT_EQ(begin, u8.c_str() + 4 + 3 + 2 + 1 + 1);
}

TEST(replace_non_utf8_chars, no_change)
{
    std::string u8;

    u8.push_back((unsigned char)0xf0);
    u8.push_back((unsigned char)0x90);
    u8.push_back((unsigned char)0x80);
    u8.push_back((unsigned char)0x80);

    u8.push_back((unsigned char)0xEA);
    u8.push_back((unsigned char)0x84);
    u8.push_back((unsigned char)0xA3);

    u8.push_back((unsigned char)0xC5);
    u8.push_back((unsigned char)0x80);

    u8.push_back((unsigned char)0x41);

    std::string res = file_impl::replace_non_utf8_chars(u8);

    EXPECT_EQ(u8, res);
}

TEST(replace_non_utf8_chars, last_char)
{
    std::string common_str = "this is some common string";
    std::string u8 = common_str;
    u8.push_back((uint8_t)0xAF);

    std::string res = file_impl::replace_non_utf8_chars(u8);

    EXPECT_NE(u8, res);
    EXPECT_EQ(res.size(), u8.size() + 2);
    EXPECT_EQ(res[res.size() - 3], '\xEF');
    EXPECT_EQ(res[res.size() - 2], '\xBF');
    EXPECT_EQ(res[res.size() - 1], '\xBD');
    EXPECT_EQ(res.substr(0, common_str.size()), common_str);
}

TEST(replace_non_utf8_chars, middle_char)
{
    std::string begin = "begin";
    std::string end = "end";
    std::string u8 = begin + '\xF0' + end;

    std::string res = file_impl::replace_non_utf8_chars(u8);

    EXPECT_NE(u8, res);
    EXPECT_EQ(res.size(), u8.size() + 2);
    EXPECT_EQ(res[begin.size()], '\xEF');
    EXPECT_EQ(res[begin.size() + 1], '\xBF');
    EXPECT_EQ(res[begin.size() + 2], '\xBD');
    EXPECT_EQ(res.substr(0, begin.size()), begin);
    EXPECT_EQ(res.substr(begin.size() + 3), end);
}