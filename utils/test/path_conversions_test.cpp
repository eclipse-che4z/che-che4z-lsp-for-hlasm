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

#include "utils/path_conversions.h"
#include "utils/platform.h"

using namespace hlasm_plugin::utils::platform;
using namespace hlasm_plugin::utils::path;

TEST(path_conversions, uri_to_path)
{
    if (is_windows())
    {
        EXPECT_EQ(uri_to_path("file://czprfs50/Public"), "\\\\czprfs50\\Public");
        EXPECT_EQ(uri_to_path("file:///C%3A/Public"), "c:\\Public");
        EXPECT_EQ(uri_to_path("file:///c%3A/Public"), "c:\\Public");
        EXPECT_EQ(uri_to_path("file:///C%3a/Public"), "c:\\Public");
        EXPECT_EQ(uri_to_path("file:///c%3a/Public"), "c:\\Public");
        EXPECT_EQ(uri_to_path("file:///C:/Public"), "c:\\Public");
        EXPECT_EQ(uri_to_path("file:///c:/Public"), "c:\\Public");
    }
    else
    {
        EXPECT_EQ(uri_to_path("file:///home/user/somefile"), "/home/user/somefile");
        EXPECT_EQ(uri_to_path("file:///C%3A/Public"), "/C:/Public");
    }
}

TEST(path_conversions, path_to_uri)
{
    if (is_windows())
    {
        EXPECT_EQ(path_to_uri("\\\\czprfs50\\Public"), "file://czprfs50/Public");
        EXPECT_EQ(path_to_uri("c:\\Public"), "file:///c%3A/Public");
    }
    else
    {
        EXPECT_EQ(path_to_uri("/home/user/somefile"), "file:///home/user/somefile");
        EXPECT_EQ(path_to_uri("/C:/Public"), "file:///C%3A/Public");
    }
}

TEST(path_conversions, reconstruct_uri_scheme_path)
{
    dissected_uri_view dis_uri;
    dis_uri.scheme = "file";
    dis_uri.path = "C:/User/file";

    EXPECT_EQ(reconstruct_uri(dis_uri), "file:C:/User/file");
}

TEST(path_conversions, reconstruct_uri_scheme_auth_path_01)
{
    dissected_uri_view dis_uri;
    dis_uri.scheme = "file";
    dis_uri.auth = {
        .host = {},
    };
    dis_uri.path = "/C:/User/file";

    EXPECT_EQ(reconstruct_uri(dis_uri), "file:///C:/User/file");
}

TEST(path_conversions, reconstruct_uri_scheme_auth_path_02)
{
    dissected_uri_view dis_uri;
    dis_uri.scheme = "file";
    dis_uri.auth = {
        .host = "",
    };
    dis_uri.path = "";

    EXPECT_EQ(reconstruct_uri(dis_uri), "file://");
}

TEST(path_conversions, reconstruct_uri_full)
{
    dissected_uri_view dis_uri;
    dis_uri.scheme = "aaa";
    dis_uri.auth = {
        .user_info = "user:pass",
        .host = "canteen.com",
        .port = "1234",
    };
    dis_uri.path = "/table/1";
    dis_uri.query = "meal=dinner";
    dis_uri.fragment = "lasagne";

    EXPECT_EQ(reconstruct_uri(dis_uri), "aaa://user:pass@canteen.com:1234/table/1?meal=dinner#lasagne");
}

TEST(path_conversions, dissect_invalid_uri)
{
    EXPECT_FALSE(dissect_uri("x:///"));
    EXPECT_FALSE(dissect_uri("333:///"));
    EXPECT_FALSE(dissect_uri("a%3b://"));
    EXPECT_FALSE(dissect_uri("scheme://[]:{}@host"));
    EXPECT_FALSE(dissect_uri("scheme://host:port"));
    EXPECT_FALSE(dissect_uri("scheme://host:%3b"));
    EXPECT_FALSE(dissect_uri("scheme://[2000::1"));
    EXPECT_FALSE(dissect_uri("scheme://{}"));
    EXPECT_FALSE(dissect_uri("scheme://host/{}"));
    EXPECT_FALSE(dissect_uri("scheme://host?query={}"));
    EXPECT_FALSE(dissect_uri("scheme://host#fragment{}"));
}

TEST(path_conversions, dissect_uri_valid_host)
{
    EXPECT_TRUE(dissect_uri("scheme://hostname"));
    EXPECT_TRUE(dissect_uri("scheme://[2000::1]"));
    EXPECT_TRUE(dissect_uri("scheme://192.168.0.1"));
    EXPECT_TRUE(dissect_uri("scheme://hostname:30"));
    EXPECT_TRUE(dissect_uri("scheme://[2000::1]:30"));
    EXPECT_TRUE(dissect_uri("scheme://192.168.0.1:30"));
}
