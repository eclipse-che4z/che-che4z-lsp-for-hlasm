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

#include "utils/resource_location.h"
#include "workspaces/file.h"

using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils::resource;

auto generate_text_pair(std::string s)
{
    auto lines = create_line_indices(s);
    return std::make_pair(std::move(s), std::move(lines));
}

TEST(apply_text_diff, text_synchronization_rn)
{
    // the server shall support \r\n, \r and \n as line separators
    auto [text_rn, text_rn_lines] = generate_text_pair(
        "this is first line \r\nsecond line blah\r\nthird line\r\n fourth line    \r\nfifthline\r\n");

    apply_text_diff(text_rn, text_rn_lines, { { 1, 5 }, { 3, 9 } }, "");

    std::string expected1 = "this is first line \r\nseconine    \r\nfifthline\r\n";
    EXPECT_EQ(text_rn, expected1);

    apply_text_diff(text_rn, text_rn_lines, { { 1, 5 }, { 1, 5 } }, "THIS ARE NEW LINES\r\nANDSECOND");
    std::string expected2 = "this is first line \r\nseconTHIS ARE NEW LINES\r\nANDSECONDine    \r\nfifthline\r\n";
    EXPECT_EQ(text_rn, expected2);

    apply_text_diff(text_rn, text_rn_lines, { { 0, 1 }, { 1, 4 } }, "THIS ARE NEW LINES BUT NO NEWLINE");
    std::string expected3 =
        "tTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\r\nANDSECONDine    \r\nfifthline\r\n";
    EXPECT_EQ(text_rn, expected3);

    apply_text_diff(text_rn, text_rn_lines, { { 3, 0 }, { 3, 0 } }, "ADD TO THE END");
    std::string expected4 =
        "tTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\r\nANDSECONDine    \r\nfifthline\r\nADD TO THE END";
    EXPECT_EQ(text_rn, expected4);

    apply_text_diff(text_rn, text_rn_lines, { { 3, 14 }, { 3, 14 } }, "and again\r\n");
    std::string expected5 = "tTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\r\nANDSECONDine    "
                            "\r\nfifthline\r\nADD TO THE ENDand again\r\n";
    EXPECT_EQ(text_rn, expected5);

    apply_text_diff(text_rn, text_rn_lines, { { 0, 0 }, { 0, 0 } }, "\r\n");
    std::string expected6 = "\r\ntTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\r\nANDSECONDine    "
                            "\r\nfifthline\r\nADD TO THE ENDand again\r\n";
    EXPECT_EQ(text_rn, expected6);

    apply_text_diff(text_rn, text_rn_lines, { { 1, 10 }, { 5, 0 } }, "big insert\r\ntest\r\n\r\n\r\ntest insertt");
    std::string expected7 = "\r\ntTHIS ARE big insert\r\ntest\r\n\r\n\r\ntest insertt";
    EXPECT_EQ(text_rn, expected7);

    apply_text_diff(text_rn, text_rn_lines, { { 3, 0 }, { 3, 0 } }, "NEW LINE");
    std::string expected8 = "\r\ntTHIS ARE big insert\r\ntest\r\nNEW LINE\r\n\r\ntest insertt";
    EXPECT_EQ(text_rn, expected8);

    apply_text_diff(text_rn, text_rn_lines, { { 0, 0 }, { 5, 12 } }, "");
    EXPECT_EQ(text_rn, "");

    apply_text_diff(text_rn, text_rn_lines, { { 0, 0 }, { 0, 0 } }, "one");
    EXPECT_EQ(text_rn, "one");


    std::string utf8test = "onexxxWASSPECIAL"; // one你WASSPECIAL
    utf8test[3] = (unsigned char)0xE4; //你 has 2 byte representation in utf-16 (one code unit)
    utf8test[4] = (unsigned char)0xBD;
    utf8test[5] = (unsigned char)0xA0;

    std::string expected = "one";
    expected.replace(0, 0, utf8test); // after: one你WASSPECIALone
    apply_text_diff(text_rn, text_rn_lines, { { 0, 0 }, { 0, 0 } }, utf8test);
    EXPECT_EQ(text_rn, expected);

    expected.replace(10, 0, utf8test); // after: one你WASSone你WASSPECIALPECIALone
    apply_text_diff(text_rn, text_rn_lines, { { 0, 8 }, { 0, 8 } }, utf8test);
    EXPECT_EQ(text_rn, expected);

    std::string four_byte = "xxxx"; // U+1700A
    four_byte[0] = static_cast<unsigned char>(0xF0);
    four_byte[1] = (unsigned char)0x97; // U+1700A has 4 byte representation in utf-16 (two code units)
    four_byte[2] = (unsigned char)0x80;
    four_byte[3] = (unsigned char)0x8A;

    expected.replace(10, 6, four_byte); // after: one你WASS𗀊WASSPECIALPECIALone
    apply_text_diff(text_rn, text_rn_lines, { { 0, 8 }, { 0, 12 } }, four_byte);
    EXPECT_EQ(text_rn, expected);

    expected.replace(20, 0, "\r\n"); // after: one你WASS𗀊WASSPE\r\nCIALPECIALone
    apply_text_diff(text_rn, text_rn_lines, { { 0, 16 }, { 0, 16 } }, "\r\n");
    EXPECT_EQ(text_rn, expected);

    expected.replace(22, 3, four_byte); // after: one你WASS𗀊WASSPE\r\n𗀊LPECIALone
    apply_text_diff(text_rn, text_rn_lines, { { 1, 0 }, { 1, 3 } }, four_byte);
    EXPECT_EQ(text_rn, expected);

    std::string null_string("x");
    null_string[0] = '\0';

    expected[2] = '\0'; // after: on\0你WASS𗀊WASSPE\r\n𗀊LPECIALone
    apply_text_diff(text_rn, text_rn_lines, { { 0, 2 }, { 0, 3 } }, null_string);
    EXPECT_EQ(text_rn, expected);
}

TEST(apply_text_diff, text_synchronization_r)
{
    auto [text_r, text_r_lines] =
        generate_text_pair("this is first line \rsecond line blah\rthird line\r fourth line    \rfifthline\r");

    apply_text_diff(text_r, text_r_lines, { { 1, 5 }, { 3, 9 } }, "");

    std::string expected1 = "this is first line \rseconine    \rfifthline\r";
    EXPECT_EQ(text_r, expected1);

    apply_text_diff(text_r, text_r_lines, { { 1, 5 }, { 1, 5 } }, "THIS ARE NEW LINES\rANDSECOND");
    std::string expected2 = "this is first line \rseconTHIS ARE NEW LINES\rANDSECONDine    \rfifthline\r";
    EXPECT_EQ(text_r, expected2);

    apply_text_diff(text_r, text_r_lines, { { 0, 1 }, { 1, 4 } }, "THIS ARE NEW LINES BUT NO NEWLINE");
    std::string expected3 = "tTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\rANDSECONDine    \rfifthline\r";
    EXPECT_EQ(text_r, expected3);

    apply_text_diff(text_r, text_r_lines, { { 3, 0 }, { 3, 0 } }, "ADD TO THE END");
    std::string expected4 =
        "tTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\rANDSECONDine    \rfifthline\rADD TO THE END";
    EXPECT_EQ(text_r, expected4);

    apply_text_diff(text_r, text_r_lines, { { 3, 14 }, { 3, 14 } }, "and again\r");
    std::string expected5 =
        "tTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\rANDSECONDine    \rfifthline\rADD TO THE ENDand again\r";
    EXPECT_EQ(text_r, expected5);

    apply_text_diff(text_r, text_r_lines, { { 0, 0 }, { 0, 0 } }, "\r");
    std::string expected6 = "\rtTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\rANDSECONDine    \rfifthline\rADD "
                            "TO THE ENDand again\r";
    EXPECT_EQ(text_r, expected6);

    apply_text_diff(text_r, text_r_lines, { { 1, 10 }, { 5, 0 } }, "big insert\rtest\r\r\rtest insertt");
    std::string expected7 = "\rtTHIS ARE big insert\rtest\r\r\rtest insertt";
    EXPECT_EQ(text_r, expected7);

    apply_text_diff(text_r, text_r_lines, { { 3, 0 }, { 3, 0 } }, "NEW LINE");
    std::string expected8 = "\rtTHIS ARE big insert\rtest\rNEW LINE\r\rtest insertt";
    EXPECT_EQ(text_r, expected8);

    apply_text_diff(text_r, text_r_lines, { { 0, 0 }, { 5, 12 } }, "");
    EXPECT_EQ(text_r, "");

    apply_text_diff(text_r, text_r_lines, { { 0, 0 }, { 0, 0 } }, "one");
    EXPECT_EQ(text_r, "one");
}

TEST(apply_text_diff, text_synchronization_n)
{
    auto [text_n, text_n_lines] =
        generate_text_pair("this is first line \nsecond line blah\nthird line\n fourth line    \nfifthline\n");

    apply_text_diff(text_n, text_n_lines, { { 1, 5 }, { 3, 9 } }, "");

    std::string expected1 = "this is first line \nseconine    \nfifthline\n";
    EXPECT_EQ(text_n, expected1);

    apply_text_diff(text_n, text_n_lines, { { 1, 5 }, { 1, 5 } }, "THIS ARE NEW LINES\nANDSECOND");
    std::string expected2 = "this is first line \nseconTHIS ARE NEW LINES\nANDSECONDine    \nfifthline\n";
    EXPECT_EQ(text_n, expected2);

    apply_text_diff(text_n, text_n_lines, { { 0, 1 }, { 1, 4 } }, "THIS ARE NEW LINES BUT NO NEWLINE");
    std::string expected3 = "tTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\nANDSECONDine    \nfifthline\n";
    EXPECT_EQ(text_n, expected3);

    apply_text_diff(text_n, text_n_lines, { { 3, 0 }, { 3, 0 } }, "ADD TO THE END");
    std::string expected4 =
        "tTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\nANDSECONDine    \nfifthline\nADD TO THE END";
    EXPECT_EQ(text_n, expected4);

    apply_text_diff(text_n, text_n_lines, { { 3, 14 }, { 3, 14 } }, "and again\n");
    std::string expected5 =
        "tTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\nANDSECONDine    \nfifthline\nADD TO THE ENDand again\n";
    EXPECT_EQ(text_n, expected5);

    apply_text_diff(text_n, text_n_lines, { { 0, 0 }, { 0, 0 } }, "\n");
    std::string expected6 = "\ntTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\nANDSECONDine    \nfifthline\nADD "
                            "TO THE ENDand again\n";
    EXPECT_EQ(text_n, expected6);

    apply_text_diff(text_n, text_n_lines, { { 1, 10 }, { 5, 0 } }, "big insert\ntest\n\n\ntest insertt");
    std::string expected7 = "\ntTHIS ARE big insert\ntest\n\n\ntest insertt";
    EXPECT_EQ(text_n, expected7);

    apply_text_diff(text_n, text_n_lines, { { 3, 0 }, { 3, 0 } }, "NEW LINE");
    std::string expected8 = "\ntTHIS ARE big insert\ntest\nNEW LINE\n\ntest insertt";
    EXPECT_EQ(text_n, expected8);

    apply_text_diff(text_n, text_n_lines, { { 0, 0 }, { 5, 12 } }, "");
    EXPECT_EQ(text_n, "");

    apply_text_diff(text_n, text_n_lines, { { 0, 0 }, { 0, 0 } }, "one");
    EXPECT_EQ(text_n, "one");
}
