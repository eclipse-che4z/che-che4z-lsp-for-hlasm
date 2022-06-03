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
#include "workspaces/file_impl.h"

using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils::resource;

TEST(file, text_synchronization_rn)
{
    // the server shall support \r\n, \r and \n as line separators
    std::string test_rn = "this is first line \r\nsecond line blah\r\nthird line\r\n fourth line    \r\nfifthline\r\n";

    file_impl file_rn(resource_location("file_rn_uri"));
    file_rn.did_open(test_rn, 47);
    EXPECT_EQ(file_rn.get_text(), test_rn);

    file_rn.did_change({ { 1, 5 }, { 3, 9 } }, "");

    std::string expected1 = "this is first line \r\nseconine    \r\nfifthline\r\n";
    EXPECT_EQ(file_rn.get_text(), expected1);
    EXPECT_EQ(file_rn.get_version(), 48);

    file_rn.did_change({ { 1, 5 }, { 1, 5 } }, "THIS ARE NEW LINES\r\nANDSECOND");
    std::string expected2 = "this is first line \r\nseconTHIS ARE NEW LINES\r\nANDSECONDine    \r\nfifthline\r\n";
    EXPECT_EQ(file_rn.get_text(), expected2);
    EXPECT_EQ(file_rn.get_version(), 49);

    file_rn.did_change({ { 0, 1 }, { 1, 4 } }, "THIS ARE NEW LINES BUT NO NEWLINE");
    std::string expected3 =
        "tTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\r\nANDSECONDine    \r\nfifthline\r\n";
    EXPECT_EQ(file_rn.get_text(), expected3);
    EXPECT_EQ(file_rn.get_version(), 50);

    file_rn.did_change({ { 3, 0 }, { 3, 0 } }, "ADD TO THE END");
    std::string expected4 =
        "tTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\r\nANDSECONDine    \r\nfifthline\r\nADD TO THE END";
    EXPECT_EQ(file_rn.get_text(), expected4);
    EXPECT_EQ(file_rn.get_version(), 51);

    file_rn.did_change({ { 3, 14 }, { 3, 14 } }, "and again\r\n");
    std::string expected5 = "tTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\r\nANDSECONDine    "
                            "\r\nfifthline\r\nADD TO THE ENDand again\r\n";
    EXPECT_EQ(file_rn.get_text(), expected5);
    EXPECT_EQ(file_rn.get_version(), 52);

    file_rn.did_change({ { 0, 0 }, { 0, 0 } }, "\r\n");
    std::string expected6 = "\r\ntTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\r\nANDSECONDine    "
                            "\r\nfifthline\r\nADD TO THE ENDand again\r\n";
    EXPECT_EQ(file_rn.get_text(), expected6);

    file_rn.did_change({ { 1, 10 }, { 5, 0 } }, "big insert\r\ntest\r\n\r\n\r\ntest insertt");
    std::string expected7 = "\r\ntTHIS ARE big insert\r\ntest\r\n\r\n\r\ntest insertt";
    EXPECT_EQ(file_rn.get_text(), expected7);

    file_rn.did_change({ { 3, 0 }, { 3, 0 } }, "NEW LINE");
    std::string expected8 = "\r\ntTHIS ARE big insert\r\ntest\r\nNEW LINE\r\n\r\ntest insertt";
    EXPECT_EQ(file_rn.get_text(), expected8);

    file_rn.did_change({ { 0, 0 }, { 5, 12 } }, "");
    EXPECT_EQ(file_rn.get_text(), "");

    file_rn.did_change({ { 0, 0 }, { 0, 0 } }, "one");
    EXPECT_EQ(file_rn.get_text(), "one");


    std::string utf8test = "onexxxWASSPECIAL"; // one你WASSPECIAL
    utf8test[3] = (unsigned char)0xE4; //你 has 2 byte representation in utf-16 (one code unit)
    utf8test[4] = (unsigned char)0xBD;
    utf8test[5] = (unsigned char)0xA0;

    std::string expected = "one";
    expected.replace(0, 0, utf8test); // after: one你WASSPECIALone
    file_rn.did_change({ { 0, 0 }, { 0, 0 } }, utf8test);
    EXPECT_EQ(file_rn.get_text(), expected);

    expected.replace(10, 0, utf8test); // after: one你WASSone你WASSPECIALPECIALone
    file_rn.did_change({ { 0, 8 }, { 0, 8 } }, utf8test);
    EXPECT_EQ(file_rn.get_text(), expected);

    std::string four_byte = "xxxx"; // U+1700A
    four_byte[0] = static_cast<unsigned char>(0xF0);
    four_byte[1] = (unsigned char)0x97; // U+1700A has 4 byte representation in utf-16 (two code units)
    four_byte[2] = (unsigned char)0x80;
    four_byte[3] = (unsigned char)0x8A;

    expected.replace(10, 6, four_byte); // after: one你WASS𗀊WASSPECIALPECIALone
    file_rn.did_change({ { 0, 8 }, { 0, 12 } }, four_byte);
    EXPECT_EQ(file_rn.get_text(), expected);

    expected.replace(20, 0, "\r\n"); // after: one你WASS𗀊WASSPE\r\nCIALPECIALone
    file_rn.did_change({ { 0, 16 }, { 0, 16 } }, "\r\n");
    EXPECT_EQ(file_rn.get_text(), expected);

    expected.replace(22, 3, four_byte); // after: one你WASS𗀊WASSPE\r\n𗀊LPECIALone
    file_rn.did_change({ { 1, 0 }, { 1, 3 } }, four_byte);
    EXPECT_EQ(file_rn.get_text(), expected);

    std::string null_string("x");
    null_string[0] = '\0';

    expected[2] = '\0'; // after: on\0你WASS𗀊WASSPE\r\n𗀊LPECIALone
    file_rn.did_change({ { 0, 2 }, { 0, 3 } }, null_string);
    EXPECT_EQ(file_rn.get_text(), expected);
}

TEST(file, text_synchronization_r)
{
    std::string test_r = "this is first line \rsecond line blah\rthird line\r fourth line    \rfifthline\r";

    file_impl file_r(resource_location("file_r_uri"));
    file_r.did_open(test_r, 47);
    EXPECT_EQ(file_r.get_text(), test_r);

    file_r.did_change({ { 1, 5 }, { 3, 9 } }, "");

    std::string expected1 = "this is first line \rseconine    \rfifthline\r";
    EXPECT_EQ(file_r.get_text(), expected1);

    file_r.did_change({ { 1, 5 }, { 1, 5 } }, "THIS ARE NEW LINES\rANDSECOND");
    std::string expected2 = "this is first line \rseconTHIS ARE NEW LINES\rANDSECONDine    \rfifthline\r";
    EXPECT_EQ(file_r.get_text(), expected2);

    file_r.did_change({ { 0, 1 }, { 1, 4 } }, "THIS ARE NEW LINES BUT NO NEWLINE");
    std::string expected3 = "tTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\rANDSECONDine    \rfifthline\r";
    EXPECT_EQ(file_r.get_text(), expected3);

    file_r.did_change({ { 3, 0 }, { 3, 0 } }, "ADD TO THE END");
    std::string expected4 =
        "tTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\rANDSECONDine    \rfifthline\rADD TO THE END";
    EXPECT_EQ(file_r.get_text(), expected4);

    file_r.did_change({ { 3, 14 }, { 3, 14 } }, "and again\r");
    std::string expected5 =
        "tTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\rANDSECONDine    \rfifthline\rADD TO THE ENDand again\r";
    EXPECT_EQ(file_r.get_text(), expected5);

    file_r.did_change({ { 0, 0 }, { 0, 0 } }, "\r");
    std::string expected6 = "\rtTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\rANDSECONDine    \rfifthline\rADD "
                            "TO THE ENDand again\r";
    EXPECT_EQ(file_r.get_text(), expected6);

    file_r.did_change({ { 1, 10 }, { 5, 0 } }, "big insert\rtest\r\r\rtest insertt");
    std::string expected7 = "\rtTHIS ARE big insert\rtest\r\r\rtest insertt";
    EXPECT_EQ(file_r.get_text(), expected7);

    file_r.did_change({ { 3, 0 }, { 3, 0 } }, "NEW LINE");
    std::string expected8 = "\rtTHIS ARE big insert\rtest\rNEW LINE\r\rtest insertt";
    EXPECT_EQ(file_r.get_text(), expected8);

    file_r.did_change({ { 0, 0 }, { 5, 12 } }, "");
    EXPECT_EQ(file_r.get_text(), "");

    file_r.did_change({ { 0, 0 }, { 0, 0 } }, "one");
    EXPECT_EQ(file_r.get_text(), "one");
}

TEST(file, text_synchronization_n)
{
    std::string test_n = "this is first line \nsecond line blah\nthird line\n fourth line    \nfifthline\n";

    file_impl file_n(resource_location("file_n_uri"));
    file_n.did_open(test_n, 47);
    EXPECT_EQ(file_n.get_text(), test_n);

    file_n.did_change({ { 1, 5 }, { 3, 9 } }, "");

    std::string expected1 = "this is first line \nseconine    \nfifthline\n";
    EXPECT_EQ(file_n.get_text(), expected1);

    file_n.did_change({ { 1, 5 }, { 1, 5 } }, "THIS ARE NEW LINES\nANDSECOND");
    std::string expected2 = "this is first line \nseconTHIS ARE NEW LINES\nANDSECONDine    \nfifthline\n";
    EXPECT_EQ(file_n.get_text(), expected2);

    file_n.did_change({ { 0, 1 }, { 1, 4 } }, "THIS ARE NEW LINES BUT NO NEWLINE");
    std::string expected3 = "tTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\nANDSECONDine    \nfifthline\n";
    EXPECT_EQ(file_n.get_text(), expected3);

    file_n.did_change({ { 3, 0 }, { 3, 0 } }, "ADD TO THE END");
    std::string expected4 =
        "tTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\nANDSECONDine    \nfifthline\nADD TO THE END";
    EXPECT_EQ(file_n.get_text(), expected4);

    file_n.did_change({ { 3, 14 }, { 3, 14 } }, "and again\n");
    std::string expected5 =
        "tTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\nANDSECONDine    \nfifthline\nADD TO THE ENDand again\n";
    EXPECT_EQ(file_n.get_text(), expected5);

    file_n.did_change({ { 0, 0 }, { 0, 0 } }, "\n");
    std::string expected6 = "\ntTHIS ARE NEW LINES BUT NO NEWLINEnTHIS ARE NEW LINES\nANDSECONDine    \nfifthline\nADD "
                            "TO THE ENDand again\n";
    EXPECT_EQ(file_n.get_text(), expected6);

    file_n.did_change({ { 1, 10 }, { 5, 0 } }, "big insert\ntest\n\n\ntest insertt");
    std::string expected7 = "\ntTHIS ARE big insert\ntest\n\n\ntest insertt";
    EXPECT_EQ(file_n.get_text(), expected7);

    file_n.did_change({ { 3, 0 }, { 3, 0 } }, "NEW LINE");
    std::string expected8 = "\ntTHIS ARE big insert\ntest\nNEW LINE\n\ntest insertt";
    EXPECT_EQ(file_n.get_text(), expected8);

    file_n.did_change({ { 0, 0 }, { 5, 12 } }, "");
    EXPECT_EQ(file_n.get_text(), "");

    file_n.did_change({ { 0, 0 }, { 0, 0 } }, "one");
    EXPECT_EQ(file_n.get_text(), "one");
}