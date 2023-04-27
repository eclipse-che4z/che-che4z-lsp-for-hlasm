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

#include "gtest/gtest.h"
#include <utils/encoding.h>

using namespace hlasm_plugin::utils::encoding;

TEST(encoding, percent_encode)
{
    EXPECT_EQ(percent_encode("\\abc"), "/abc");
    EXPECT_EQ(percent_encode("%"), "%25");
    EXPECT_EQ(percent_encode("%25"), "%2525");
    EXPECT_EQ(percent_encode("%st"), "%25st");
    EXPECT_EQ(percent_encode("%F0"), "%25F0");
    EXPECT_EQ(percent_encode("%F0123456789"), "%25F0123456789");
    EXPECT_EQ(percent_encode("%F0%82%83%AF"), "%25F0%2582%2583%25AF");
    EXPECT_EQ(percent_encode("%F0%92%83%AF"), "%25F0%2592%2583%25AF");
    EXPECT_EQ(percent_encode("%F0%92%83%D0"), "%25F0%2592%2583%25D0");
}

TEST(encoding, percent_encode_and_ignore_utf8)
{
    EXPECT_EQ(percent_encode_and_ignore_utf8("\\abc"), "/abc");
    EXPECT_EQ(percent_encode_and_ignore_utf8("%"), "%25");
    EXPECT_EQ(percent_encode_and_ignore_utf8("%25"), "%25");
    EXPECT_EQ(percent_encode_and_ignore_utf8("%st"), "%25st");
    EXPECT_EQ(percent_encode_and_ignore_utf8("%F0"), "%25F0");
    EXPECT_EQ(percent_encode_and_ignore_utf8("%F0123456789"), "%25F0123456789");
    EXPECT_EQ(percent_encode_and_ignore_utf8("%F0%82%83%AF"), "%25F0%2582%2583%25AF");
    EXPECT_EQ(percent_encode_and_ignore_utf8("%F0%92%83%AF"), "%F0%92%83%AF");
    EXPECT_EQ(percent_encode_and_ignore_utf8("%F0%92%83%D0"), "%25F0%2592%2583%25D0");
}

TEST(encoding, uri_friendly_base16_encode)
{
    EXPECT_EQ(uri_friendly_base16_encode(""), "");
    EXPECT_EQ(uri_friendly_base16_encode("A"), "eb");
    EXPECT_EQ(uri_friendly_base16_encode("AB"), "ebec");
}

TEST(encoding, uri_friendly_base16_decode)
{
    EXPECT_EQ(uri_friendly_base16_decode(""), "");
    EXPECT_EQ(uri_friendly_base16_decode("eb"), "A");
    EXPECT_EQ(uri_friendly_base16_decode("ebec"), "AB");
    EXPECT_EQ(uri_friendly_base16_decode("EB"), "A");
    EXPECT_EQ(uri_friendly_base16_decode("EBEC"), "AB");
    EXPECT_EQ(uri_friendly_base16_decode("EbeC"), "AB");
    EXPECT_EQ(uri_friendly_base16_decode("e"), "");
    EXPECT_EQ(uri_friendly_base16_decode("aX"), "");
    EXPECT_EQ(uri_friendly_base16_decode("Xa"), "");
    EXPECT_EQ(uri_friendly_base16_decode("ax"), "");
    EXPECT_EQ(uri_friendly_base16_decode("xa"), "");
}

TEST(encoding, uri_friendly_base16_roundtrip)
{
    std::string s;
    for (int i = 0; i < 256; ++i)
        s.push_back(i);

    EXPECT_EQ(uri_friendly_base16_decode(uri_friendly_base16_encode(s)), s);
}
