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

#include "utils/platform.h"
#include "utils/resource_location.h"

using namespace hlasm_plugin::utils::platform;
using namespace hlasm_plugin::utils::resource;

TEST(resource_location, empty_uri)
{
    resource_location res("");
    EXPECT_EQ(res.get_uri(), "");
    EXPECT_EQ(res.get_path(), "");
}

TEST(resource_location, invalid_uri)
{
    resource_location res("src/temp");
    EXPECT_EQ(res.get_uri(), "src/temp");
    EXPECT_EQ(res.get_path(), "src/temp");
}

TEST(resource_location, non_supported_uri)
{
    resource_location res("aaa://src/temp");
    EXPECT_EQ(res.get_uri(), "aaa://src/temp");
    EXPECT_EQ(res.get_path(), "");
}

TEST(resource_location, file_uri)
{
    if (is_windows())
    {
        resource_location res("file:///c%3A/Public");
        EXPECT_EQ(res.get_uri(), "file:///c%3A/Public");
        EXPECT_EQ(res.get_path(), "c:\\Public");
    }
    else
    {
        resource_location res("file:///home/user/somefile");
        EXPECT_EQ(res.get_uri(), "file:///home/user/somefile");
        EXPECT_EQ(res.get_path(), "/home/user/somefile");
    }
}

TEST(resource_location, non_file_uri)
{
    resource_location res("hlasm://0/AINSERT:1");
    EXPECT_EQ(res.get_uri(), "hlasm://0/AINSERT:1");
    EXPECT_EQ(res.get_path(), "");
}

TEST(resource_location, to_presentable_no_authority)
{
    std::string expected = "untitled:Untitled-1";

    resource_location res("untitled:Untitled-1");
    EXPECT_EQ(res.to_presentable(), expected);
}

TEST(resource_location, to_presentable_file_scheme_localhost)
{
    if (is_windows())
    {
        std::string expected = "c:\\Public\\folder With spaces\\filE nAme";

        resource_location res("file:///c%3A/Public/folder%20With%20spaces/filE%20nAme");
        EXPECT_EQ(res.to_presentable(), expected);
    }
    else
    {
        std::string expected = "/home/user/folder With spaces/filE nAme";

        resource_location res("file:///home/user/folder%20With%20spaces/filE%20nAme");
        EXPECT_EQ(res.to_presentable(), expected);
    }
}

TEST(resource_location, to_presentable_file_scheme_network)
{
    if (is_windows())
    {
        std::string expected = "\\\\server\\Public\\folder With spaces\\filE nAme";

        resource_location res("file://server/Public/folder%20With%20spaces/filE%20nAme");
        EXPECT_EQ(res.to_presentable(), expected);
    }
    else
    {
        std::string expected = "file://server/Public/folder%20With%20spaces/filE%20nAme";

        resource_location res("file://server/Public/folder%20With%20spaces/filE%20nAme");
        EXPECT_EQ(res.to_presentable(), expected);
    }
}

TEST(resource_location, to_presentable_file_scheme_network_with_port)
{
    if (is_windows())
    {
        std::string expected = "\\\\server:50\\Public\\folder With spaces\\filE nAme";

        resource_location res("file://server:50/Public/folder%20With%20spaces/filE%20nAme");
        EXPECT_EQ(res.to_presentable(), expected);
    }
    else
    {
        std::string expected = "file://server:50/Public/folder%20With%20spaces/filE%20nAme";

        resource_location res("file://server:50/Public/folder%20With%20spaces/filE%20nAme");
        EXPECT_EQ(res.to_presentable(), expected);
    }
}

TEST(resource_location, to_presentable_other_schemes)
{
    std::string expected = "aaa://server/Public/folder%20With%20spaces/filE%20nAme";

    resource_location res("aaa://server/Public/folder%20With%20spaces/filE%20nAme");
    EXPECT_EQ(res.to_presentable(), expected);
}

TEST(resource_location, to_presentable_other_schemes_with_port)
{
    std::string expected = "aaa://server:80/Public/folder%20With%20spaces/filE%20nAme";

    resource_location res("aaa://server:80/Public/folder%20With%20spaces/filE%20nAme");
    EXPECT_EQ(res.to_presentable(), expected);
}

TEST(resource_location, to_presentable_other_schemes_full_debug)
{
    std::string expected = R"(Scheme: aaa
User info: user::pass
Hostname: 127.0.0.1
Port: 1234
Path: /path/to/resource
Query: fileset=sources
Fragment: pgm
Raw URI: aaa://user::pass@127.0.0.1:1234/path/to/resource?fileset=sources#pgm)";

    resource_location res("aaa://user::pass@127.0.0.1:1234/path/to/resource?fileset=sources#pgm");
    EXPECT_EQ(res.to_presentable(true), expected);
}

TEST(resource_location, join)
{
    resource_location res("aaa://src/temp");
    std::string rel_loc = ".hlasmplugin";

    EXPECT_EQ(resource_location::join(res, rel_loc), resource_location("aaa://src/temp/.hlasmplugin"));
}