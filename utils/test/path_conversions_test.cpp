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
    dissected_uri dis_uri;
    dis_uri.scheme = "file";
    dis_uri.path = "C:/User/file";

    EXPECT_EQ(reconstruct_uri(dis_uri), "file:C:/User/file");
}

TEST(path_conversions, reconstruct_uri_scheme_auth_path_01)
{
    dissected_uri::authority auth;
    auth.host = "";

    dissected_uri dis_uri;
    dis_uri.scheme = "file";
    dis_uri.auth = auth;
    dis_uri.path = "/C:/User/file";

    EXPECT_EQ(reconstruct_uri(dis_uri), "file:///C:/User/file");
}

TEST(path_conversions, reconstruct_uri_scheme_auth_path_02)
{
    dissected_uri::authority auth;
    auth.host = "";

    dissected_uri dis_uri;
    dis_uri.scheme = "file";
    dis_uri.auth = auth;
    dis_uri.path = "";

    EXPECT_EQ(reconstruct_uri(dis_uri), "file://");
}

TEST(path_conversions, reconstruct_uri_full)
{
    dissected_uri::authority auth;
    auth.host = "canteen.com";
    auth.user_info = "user:pass";
    auth.port = "1234";

    dissected_uri dis_uri;
    dis_uri.scheme = "aaa";
    dis_uri.auth = auth;
    dis_uri.path = "/table/1";
    dis_uri.query = "meal=dinner";
    dis_uri.fragment = "lasagne";

    EXPECT_EQ(reconstruct_uri(dis_uri), "aaa://user:pass@canteen.com:1234/table/1?meal=dinner#lasagne");
}
