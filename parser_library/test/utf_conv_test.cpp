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

#include <memory>
#include <string>

#include "gtest/gtest.h"

#include "analyzer.h"
#include "common_testing.h"
#include "ebcdic_encoding.h"

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

    // ä
    u8.insert(u8.end(), (unsigned char)0xC3);
    u8.insert(u8.end(), (unsigned char)0xA4);

    auto begin = u8.c_str();
    const auto end = std::to_address(u8.end());

    EXPECT_EQ(ebcdic_encoding::to_ebcdic(begin, end), std::pair(ebcdic_encoding::EBCDIC_SUB, begin + 4));
    begin += 4;

    EXPECT_EQ(ebcdic_encoding::to_ebcdic(begin, end), std::pair(ebcdic_encoding::EBCDIC_SUB, begin + 3));
    begin += 3;

    EXPECT_EQ(ebcdic_encoding::to_ebcdic(begin, end), std::pair(ebcdic_encoding::EBCDIC_SUB, begin + 2));
    begin += 2;

    EXPECT_EQ(ebcdic_encoding::to_ebcdic(begin, end), std::pair((unsigned char)0xC1, begin + 1));
    begin += 1;

    EXPECT_EQ(ebcdic_encoding::to_ebcdic(begin, end), std::pair((unsigned char)0x43, begin + 2));
    begin += 2;

    EXPECT_EQ(begin, std::to_address(u8.end()));
}

TEST(encoding, server_substitution)
{
    std::string input = "&VAR SETC 'PARAM\xA2'"; // 'PARAM¢' in ISO/IEC 8859-1

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W017" }));
}

TEST(encoding, client_substitution_vscode)
{
    std::string input = "&VAR SETC 'PARAM\xEF\xBF\xBD'"; // 'PARAMï¿½' - Replacement characters from VSCode

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W018" }));
}

TEST(encoding, client_server_substitution_multiple_vscode)
{
    std::string input = "&VAR1 SETC 'PARAM\xEF\xBF\xBD\xA2'\n&VAR2 SETC 'PARAM\xA2\xEF\xBF\xBD'";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W017", "W018" }));
}
