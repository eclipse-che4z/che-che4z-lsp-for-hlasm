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

#include <vector>

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

TEST(resource_location, file_uri_empty_auth)
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

TEST(resource_location, file_uri_non_empty_auth)
{
    if (is_windows())
    {
        resource_location res("file://share/c%3A/Public");
        EXPECT_EQ(res.get_uri(), "file://share/c%3A/Public");
        EXPECT_EQ(res.get_path(), "\\\\share\\c:\\Public");
    }
    else
    {
        resource_location res("file://share/home/user/somefile");
        EXPECT_EQ(res.get_uri(), "file://share/home/user/somefile");
        EXPECT_EQ(res.get_path(), "");
    }
}

TEST(resource_location, file_uri_non_standard_0)
{
    if (is_windows())
    {
        resource_location res("file:C%3A/Public");
        EXPECT_EQ(res.get_uri(), "file:C%3A/Public");
        EXPECT_EQ(res.get_path(), "c:\\Public");
    }
    else
    {
        resource_location res("file:home/user/somefile");
        EXPECT_EQ(res.get_uri(), "file:home/user/somefile");
        EXPECT_EQ(res.get_path(), "home/user/somefile");
    }
}

TEST(resource_location, file_uri_non_standard_1)
{
    if (is_windows())
    {
        resource_location res("file:/c%3A/Public");
        EXPECT_EQ(res.get_uri(), "file:/c%3A/Public");
        EXPECT_EQ(res.get_path(), "c:\\Public");
    }
    else
    {
        resource_location res("file:/home/user/somefile");
        EXPECT_EQ(res.get_uri(), "file:/home/user/somefile");
        EXPECT_EQ(res.get_path(), "/home/user/somefile");
    }
}


TEST(resource_location, file_uri_non_standard_2)
{
    if (is_windows())
    {
        resource_location res("file://c%3A/Public");
        EXPECT_EQ(res.get_uri(), "file://c%3A/Public");
        EXPECT_EQ(res.get_path(), "c:\\Public");
    }
    else
    {
        resource_location res("file://home/user/somefile");
        EXPECT_EQ(res.get_uri(), "file://home/user/somefile");
        EXPECT_EQ(res.get_path(), "");
    }
}

TEST(resource_location, non_file_uri)
{
    resource_location res("hlasm://0/AINSERT_1");
    EXPECT_EQ(res.get_uri(), "hlasm://0/AINSERT_1");
    EXPECT_EQ(res.get_path(), "");
}

TEST(resource_location, is_local)
{
    if (is_windows())
    {
        EXPECT_TRUE(resource_location::is_local("file:///C:"));
        EXPECT_TRUE(resource_location::is_local("file:///d:"));
        EXPECT_TRUE(resource_location::is_local("file:///Z:/"));
        EXPECT_TRUE(resource_location::is_local("file:///C:/"));
        EXPECT_TRUE(resource_location::is_local("file:///C:/file"));
        EXPECT_TRUE(resource_location::is_local("file:///C:/"));

        EXPECT_TRUE(resource_location::is_local("file:///C%3A"));
        EXPECT_TRUE(resource_location::is_local("file:///d%3A"));
        EXPECT_TRUE(resource_location::is_local("file:///Z%3A/"));
        EXPECT_TRUE(resource_location::is_local("file:///C%3A/"));
        EXPECT_TRUE(resource_location::is_local("file:///C%3A/file"));
        EXPECT_TRUE(resource_location::is_local("file:///C%3A/"));

        EXPECT_TRUE(resource_location::is_local("file:///C%3a"));
        EXPECT_TRUE(resource_location::is_local("file:///d%3a"));
        EXPECT_TRUE(resource_location::is_local("file:///Z%3a/"));
        EXPECT_TRUE(resource_location::is_local("file:///C%3a/"));
        EXPECT_TRUE(resource_location::is_local("file:///C%3a/file"));
        EXPECT_TRUE(resource_location::is_local("file:///C%3a/"));

        EXPECT_TRUE(resource_location::is_local("file:C:"));
        EXPECT_TRUE(resource_location::is_local("file:\\C:"));
        EXPECT_TRUE(resource_location::is_local("file:\\\\C:"));
        EXPECT_TRUE(resource_location::is_local("file:\\\\\\C:"));
        EXPECT_TRUE(resource_location::is_local("file:C:/"));
        EXPECT_TRUE(resource_location::is_local("file:/C:/"));
        EXPECT_TRUE(resource_location::is_local("file://C:/"));

        EXPECT_FALSE(resource_location::is_local("file://host/C:"));

        EXPECT_FALSE(resource_location::is_local("aaa:C:/"));
        EXPECT_FALSE(resource_location::is_local("aaa:/C:/"));
        EXPECT_FALSE(resource_location::is_local("aaa://C:/"));
        EXPECT_FALSE(resource_location::is_local("aaa:///C:/"));
    }
    else
    {
        EXPECT_TRUE(resource_location::is_local("file:/local"));
        EXPECT_TRUE(resource_location::is_local("file:/local/"));
        EXPECT_TRUE(resource_location::is_local("file:/local/file"));

        EXPECT_TRUE(resource_location::is_local("file:///local"));
        EXPECT_TRUE(resource_location::is_local("file:///local/"));
        EXPECT_TRUE(resource_location::is_local("file:///local/file"));

        EXPECT_TRUE(resource_location::is_local("file:\\C:"));
        EXPECT_TRUE(resource_location::is_local("file:\\\\\\C:"));
        EXPECT_TRUE(resource_location::is_local("file:/C:/"));
        EXPECT_TRUE(resource_location::is_local("file:///C:"));

        EXPECT_FALSE(resource_location::is_local("file:C:"));
        EXPECT_FALSE(resource_location::is_local("file:\\\\C:"));
        EXPECT_FALSE(resource_location::is_local("file:C:/"));
        EXPECT_FALSE(resource_location::is_local("file://C:/"));

        EXPECT_FALSE(resource_location::is_local("aaa:/local"));
        EXPECT_FALSE(resource_location::is_local("aaa:/local/"));
        EXPECT_FALSE(resource_location::is_local("aaa:/local/file"));
    }

    EXPECT_FALSE(resource_location::is_local(""));

    EXPECT_FALSE(resource_location::is_local("file:relative"));
    EXPECT_FALSE(resource_location::is_local("file://host"));
    EXPECT_FALSE(resource_location::is_local("file://host/"));
    EXPECT_FALSE(resource_location::is_local("file://host/user/somefile"));

    EXPECT_FALSE(resource_location::is_local("aaa:relative"));
    EXPECT_FALSE(resource_location::is_local("aaa://host"));
    EXPECT_FALSE(resource_location::is_local("aaa://host/"));
    EXPECT_FALSE(resource_location::is_local("aaa://host/user/somefile"));
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

TEST(resource_location, lexically_relative_subset_of_base)
{
    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/"))
                  .get_uri(),
        "file_a");

    EXPECT_EQ(
        resource_location("file:///C:/dir/subdir/").lexically_relative(resource_location("file:///C:/dir/")).get_uri(),
        "subdir/");

    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/"))
                  .get_uri(),
        "subdir/file_a");
}

TEST(resource_location, lexically_relative)
{
    EXPECT_EQ(resource_location("file:///C:/dir/file_b")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/file_a"))
                  .get_uri(),
        "../../file_b");

    EXPECT_EQ(resource_location("file:///C:/dir/subdir/")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/file_a"))
                  .get_uri(),
        "../");

    EXPECT_EQ(
        resource_location("file:///C:/").lexically_relative(resource_location("file:///C:/dir/subdir/")).get_uri(),
        "../../");

    EXPECT_EQ(resource_location("file:///C:/")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/file_a"))
                  .get_uri(),
        "../../../");

    EXPECT_EQ(resource_location("file:///C:/dir/file_b")
                  .lexically_relative(resource_location("file:///C:/dir/file_a"))
                  .get_uri(),
        "../file_b");

    EXPECT_EQ(resource_location("file:///C:/dir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/file_a"))
                  .get_uri(),
        "../../file_a");

    EXPECT_EQ(resource_location("file:///C:/dir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/"))
                  .get_uri(),
        "../file_a");

    EXPECT_EQ(resource_location("file:///C:/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/file_a"))
                  .get_uri(),
        "../../../file_a");
}

TEST(resource_location, lexically_relative_single_dots)
{
    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/."))
                  .get_uri(),
        "file_a");

    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/./"))
                  .get_uri(),
        "file_a");

    EXPECT_EQ(resource_location("file:///C:/dir/./subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/./"))
                  .get_uri(),
        ".././subdir/file_a");

    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/./subdir"))
                  .get_uri(),
        "../subdir/file_a");

    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/./subdir/"))
                  .get_uri(),
        "../subdir/file_a");
}

TEST(resource_location, lexically_relative_double_dots)
{
    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/.."))
                  .get_uri(),
        "");

    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/../"))
                  .get_uri(),
        "");

    EXPECT_EQ(resource_location("file:///C:/dir/../subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/../"))
                  .get_uri(),
        "../subdir/file_a");

    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/../file_b"))
                  .get_uri(),
        "subdir/file_a");

    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/../subdir/"))
                  .get_uri(),
        "subdir/file_a");
}

TEST(resource_location, lexically_relative_multiple_slashes)
{
    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir////"))
                  .get_uri(),
        "file_a");

    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:///dir/subdir////"))
                  .get_uri(),
        "file_a");

    EXPECT_EQ(resource_location("file:///C:/dir/subdir///file_a")
                  .lexically_relative(resource_location("file:///C:/dir/"))
                  .get_uri(),
        "subdir/file_a");
}

TEST(resource_location, lexically_relative_win_drive_letter)
{
    EXPECT_EQ(resource_location("file:///C:/dir/subdir/")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/"))
                  .get_uri(),
        ".");

    EXPECT_EQ(resource_location("file:///c:/dir/subdir/")
                  .lexically_relative(resource_location("file:///c:/dir/subdir/"))
                  .get_uri(),
        ".");

    EXPECT_EQ(resource_location("file:///C%3A/dir/subdir/")
                  .lexically_relative(resource_location("file:///C%3A/dir/subdir/"))
                  .get_uri(),
        ".");

    EXPECT_EQ(resource_location("file:///c%3A/dir/subdir/")
                  .lexically_relative(resource_location("file:///c%3A/dir/subdir/"))
                  .get_uri(),
        ".");

    EXPECT_EQ(resource_location("file:///C:/dir/file_a")
                  .lexically_relative(resource_location("file:///D:/dir/file_a"))
                  .get_uri(),
        "../../../C:/dir/file_a");

    EXPECT_EQ(resource_location("file:///D:/dir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/file_a"))
                  .get_uri(),
        "../../../D:/dir/file_a");
}

TEST(resource_location, lexically_relative_different_schemes)
{
    EXPECT_EQ(resource_location("file:///home/").lexically_relative(resource_location("aaa:dir/file_a")).get_uri(), "");

    EXPECT_EQ(
        resource_location("aaa:dir/file_a").lexically_relative(resource_location("file:///C:/dir/file_a")).get_uri(),
        "");
}

TEST(resource_location, lexically_normal)
{
    EXPECT_EQ(resource_location("file:///").lexically_normal().get_uri(), "file:///");
    EXPECT_EQ(resource_location("file:///././file").lexically_normal().get_uri(), "file:///file");
    if (is_windows())
    {
        EXPECT_EQ(resource_location("file:///C:/./file").lexically_normal().get_uri(), "file:///c%3A/file");
        EXPECT_EQ(resource_location("file:///C:/./dir/").lexically_normal().get_uri(), "file:///c%3A/dir/");
        EXPECT_EQ(resource_location("file:///C:/./dir/.").lexically_normal().get_uri(), "file:///c%3A/dir/");
        EXPECT_EQ(resource_location("file:///C:/./dir../.").lexically_normal().get_uri(), "file:///c%3A/dir../");
        EXPECT_EQ(resource_location("file:///C:/./dir./.").lexically_normal().get_uri(), "file:///c%3A/dir./");
        EXPECT_EQ(resource_location("file:///C:/./dir/..").lexically_normal().get_uri(), "file:///c%3A/");
        EXPECT_EQ(resource_location("file:///C:/.///dir/../").lexically_normal().get_uri(), "file:///c%3A/");
    }
    else
    {
        EXPECT_EQ(resource_location("file:///C:/./file").lexically_normal().get_uri(), "file:///C%3A/file");
        EXPECT_EQ(resource_location("file:///C:/./dir/").lexically_normal().get_uri(), "file:///C%3A/dir/");
        EXPECT_EQ(resource_location("file:///C:/./dir/.").lexically_normal().get_uri(), "file:///C%3A/dir/");
        EXPECT_EQ(resource_location("file:///C:/./dir../.").lexically_normal().get_uri(), "file:///C%3A/dir../");
        EXPECT_EQ(resource_location("file:///C:/./dir./.").lexically_normal().get_uri(), "file:///C%3A/dir./");
        EXPECT_EQ(resource_location("file:///C:/./dir/..").lexically_normal().get_uri(), "file:///C%3A/");
        EXPECT_EQ(resource_location("file:///C:/.///dir/../").lexically_normal().get_uri(), "file:///C%3A/");
    }
}

TEST(resource_location, lexically_normal_change_root_dir)
{
    if (is_windows())
    {
        EXPECT_EQ(resource_location("file:///C:/../D:/").lexically_normal().get_uri(), "file:///d%3A/");

        EXPECT_EQ(resource_location("file:///C:/../../../file").lexically_normal().get_uri(), "file:///c%3A/file");
        EXPECT_EQ(resource_location("file:///C:/../../../dir/").lexically_normal().get_uri(), "file:///c%3A/dir/");
        EXPECT_EQ(resource_location("file:///C:/../../../dir/subdir/file").lexically_normal().get_uri(),
            "file:///c%3A/dir/subdir/file");
        EXPECT_EQ(resource_location("file:///C:/../../../dir/..").lexically_normal().get_uri(), "file:///c%3A/");
        EXPECT_EQ(resource_location("file:///C:/../../../.").lexically_normal().get_uri(), "file:///c%3A/");
    }
    else
    {
        EXPECT_EQ(resource_location("file:///C:/../D:/").lexically_normal().get_uri(), "file:///D%3A/");
        EXPECT_EQ(resource_location("file:///C:/../../..").lexically_normal().get_uri(), "file:///");
        EXPECT_EQ(resource_location("file:///C:/../../../file").lexically_normal().get_uri(), "file:///file");
        EXPECT_EQ(resource_location("file:///C:/../../../root/").lexically_normal().get_uri(), "file:///root/");
        EXPECT_EQ(resource_location("file:///C:/../../../root/dir/file").lexically_normal().get_uri(),
            "file:///root/dir/file");
        EXPECT_EQ(resource_location("file:///C:/../../../root/..").lexically_normal().get_uri(), "file:///");
        EXPECT_EQ(resource_location("file:///C:/../../../.").lexically_normal().get_uri(), "file:///");
    }
}

TEST(resource_location, lexically_normal_diff_schemes)
{
    EXPECT_EQ(resource_location("aaa:").lexically_normal().get_uri(), "aaa:");
    EXPECT_EQ(resource_location("aaa:././file").lexically_normal().get_uri(), "aaa:file");
    EXPECT_EQ(resource_location("aaa:C:/file").lexically_normal().get_uri(), "aaa:C%3A/file");
    EXPECT_EQ(resource_location("aaa:C:/dir/").lexically_normal().get_uri(), "aaa:C%3A/dir/");
    EXPECT_EQ(resource_location("aaa:C:/dir/.").lexically_normal().get_uri(), "aaa:C%3A/dir/");
    EXPECT_EQ(resource_location("aaa:C:/dir../.").lexically_normal().get_uri(), "aaa:C%3A/dir../");
    EXPECT_EQ(resource_location("aaa:C:/dir./.").lexically_normal().get_uri(), "aaa:C%3A/dir./");
    EXPECT_EQ(resource_location("aaa:C:/dir./%2f").lexically_normal().get_uri(), "aaa:C%3A/dir./%2F");
    EXPECT_EQ(resource_location("aaa:C:/dir/..").lexically_normal().get_uri(), "aaa:C%3A/");
    EXPECT_EQ(resource_location("aaa:C:///dir/../").lexically_normal().get_uri(), "aaa:C%3A/");
}

TEST(resource_location, lexically_normal_no_schemes)
{
    EXPECT_EQ(resource_location("").lexically_normal().get_uri(), "");
    EXPECT_EQ(resource_location("./").lexically_normal().get_uri(), "");
    EXPECT_EQ(resource_location("././").lexically_normal().get_uri(), "");
    EXPECT_EQ(resource_location("../").lexically_normal().get_uri(), "../");
    EXPECT_EQ(resource_location("./../").lexically_normal().get_uri(), "../");
    EXPECT_EQ(resource_location("./a").lexically_normal().get_uri(), "a");
    EXPECT_EQ(resource_location("../a").lexically_normal().get_uri(), "../a");
}

TEST(resource_location, lexically_normal_change_root_dir_diff_schemes)
{
    EXPECT_EQ(resource_location("aaa:C:/../../../D:").lexically_normal().get_uri(), "aaa:D%3A");
    EXPECT_EQ(resource_location("aaa:C:/../../../D:/").lexically_normal().get_uri(), "aaa:D%3A/");
    EXPECT_EQ(resource_location("aaa:C:/../../../D:/..").lexically_normal().get_uri(), "aaa:");
    EXPECT_EQ(resource_location("aaa:C:/../../../.").lexically_normal().get_uri(), "aaa:");
}

TEST(resource_location, lexically_normal_slashes)
{
    EXPECT_EQ(resource_location("file:///.\\.\\file").lexically_normal().get_uri(), "file:///file");

    if (is_windows())
    {
        EXPECT_EQ(resource_location("file:\\\\\\C:\\file").lexically_normal().get_uri(), "file:///c%3A/file");
        EXPECT_EQ(resource_location("file:\\\\\\C:\\.\\dir\\.").lexically_normal().get_uri(), "file:///c%3A/dir/");
    }
    else
    {
        EXPECT_EQ(resource_location("file:\\\\\\C:\\file").lexically_normal().get_uri(), "file:///C%3A/file");
        EXPECT_EQ(resource_location("file:\\\\\\C:\\.\\dir\\.").lexically_normal().get_uri(), "file:///C%3A/dir/");
    }
}

TEST(resource_location, lexically_normal_file_scheme)
{
    if (hlasm_plugin::utils::platform::is_windows())
    {
        EXPECT_EQ(resource_location("file:c:/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:c%3a/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:c%3A/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");

        EXPECT_EQ(resource_location("file:C:/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:C%3a/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:C%3A/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");

        EXPECT_EQ(resource_location("file:/C:/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:/C%3a/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:/C%3A/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:\\C:/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:\\C%3a/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:\\C%3A/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");

        EXPECT_EQ(resource_location("file://C:/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file://C%3a/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file://C%3A/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:\\\\C:/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:\\\\C%3a/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:\\\\C%3A/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");

        EXPECT_EQ(resource_location("file:///C:/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:///C%3a/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:///C%3A/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:\\\\\\C:/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:\\\\\\C%3a/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:\\\\\\C%3A/Dir/").lexically_normal().get_uri(), "file:///c%3A/Dir/");

        EXPECT_EQ(resource_location("file://///C://Dir//").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file://///C%3a//Dir//").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file://///C%3A//Dir//").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:\\\\\\\\\\C://Dir//").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:\\\\\\\\\\C%3a//Dir//").lexically_normal().get_uri(), "file:///c%3A/Dir/");
        EXPECT_EQ(resource_location("file:\\\\\\\\\\C%3A//Dir//").lexically_normal().get_uri(), "file:///c%3A/Dir/");

        EXPECT_EQ(resource_location("file://C/Dir//").lexically_normal().get_uri(), "file://C/Dir/");
        EXPECT_EQ(resource_location("file://host/Dir//").lexically_normal().get_uri(), "file://host/Dir/");
    }
    else
    {
        EXPECT_EQ(resource_location("file:relative/Dir/").lexically_normal().get_uri(), "file:relative/Dir/");
        EXPECT_EQ(resource_location("file:relative\\Dir/").lexically_normal().get_uri(), "file:relative/Dir/");
        EXPECT_EQ(resource_location("file:/absolute/Dir/colon:name").lexically_normal().get_uri(),
            "file:/absolute/Dir/colon%3Aname");
        EXPECT_EQ(resource_location("file:\\absolute\\Dir/colon:name").lexically_normal().get_uri(),
            "file:/absolute/Dir/colon%3Aname");
        EXPECT_EQ(resource_location("file://host/Dir/").lexically_normal().get_uri(), "file://host/Dir/");
        EXPECT_EQ(resource_location("file:\\\\host\\Dir/").lexically_normal().get_uri(), "file://host/Dir/");
    }

    EXPECT_EQ(resource_location("").lexically_normal().get_uri(), "");
    EXPECT_EQ(resource_location("file:").lexically_normal().get_uri(), "file:");
    EXPECT_EQ(resource_location("file://host/C:/Dir/").lexically_normal().get_uri(), "file://host/C%3A/Dir/");
    EXPECT_EQ(resource_location("file:\\\\host/C:/Dir/").lexically_normal().get_uri(), "file://host/C%3A/Dir/");
    EXPECT_EQ(resource_location("aaa:C:/Dir/").lexically_normal().get_uri(), "aaa:C%3A/Dir/");
    EXPECT_EQ(resource_location(":C:/Dir/").lexically_normal().get_uri(), ":C:/Dir/");
}

TEST(resource_location, lexically_normal_rfc_3986)
{
    EXPECT_EQ(resource_location("aaa:/a/b/c/./../../g").lexically_normal().get_uri(), "aaa:/a/g");
    EXPECT_EQ(resource_location("aaa:mid/content=5/../6").lexically_normal().get_uri(), "aaa:mid/6");
}

TEST(resource_location, lexically_normal_rfc_3986_syntax_based)
{
    // EXPECT_EQ(resource_location("example://a/b/c/%7Bfoo%7D").lexically_normal(),
    //     resource_location("eXAMPLE://a/./b/../b/%63/%7bfoo%7d").lexically_normal()); // todo

    EXPECT_EQ(resource_location("example://a/b/c/%7Bfoo%7D").lexically_normal(),
        resource_location("eXAMPLE://a/./b/../b/c/%7bfoo%7d").lexically_normal());

    EXPECT_EQ(resource_location("aaa:/directory/colon:file").lexically_normal(),
        resource_location("aaa:/directory/colon%3Afile").lexically_normal());

    if (is_windows())
    {
        EXPECT_EQ(resource_location("file:///C:/a/b/c/%7Bfoo%7D").lexically_normal(),
            resource_location("file:///C:/a/./b/../b/c/%7bfoo%7d").lexically_normal());

        EXPECT_EQ(resource_location("file:/C%3a/a/b/c/%7Bfoo%7D").lexically_normal(),
            resource_location("file:/C:/a/./b/../b/c/%7bfoo%7d").lexically_normal());

        EXPECT_EQ(resource_location("file:/C%3a/a/b/c/%7Bfoo%7D").lexically_normal(),
            resource_location("file:/C%3A/a/./b/../b/c/%7bfoo%7d").lexically_normal());
    }
}

TEST(resource_location, join_empty)
{
    resource_location rl("aaa://src/dir/");

    rl.join("");
    EXPECT_EQ(rl, resource_location("aaa://src/dir/"));
}

TEST(resource_location, join_empty_2)
{
    resource_location rl("");

    rl.join("aaa://src/dir/");
    EXPECT_EQ(rl, resource_location("aaa://src/dir/"));
}

TEST(resource_location, join_uri)
{
    resource_location rl("aaa://src/dir/");

    rl.join("scheme://a/b/c");
    EXPECT_EQ(rl, resource_location("scheme://a/b/c"));
}

TEST(resource_location, join_with_root_dir)
{
    resource_location res("aaa://src/");
    std::string rel_loc = ".hlasmplugin";

    EXPECT_EQ(resource_location::join(res, rel_loc), resource_location("aaa://src/.hlasmplugin"));
}
TEST(resource_location, join_with_regular_dir)
{
    resource_location res("aaa://src/temp/");
    std::string rel_loc = ".hlasmplugin";

    EXPECT_EQ(resource_location::join(res, rel_loc), resource_location("aaa://src/temp/.hlasmplugin"));
}

TEST(resource_location, join_with_root_dir_no_auth)
{
    resource_location res("aaa:src/");
    std::string rel_loc = ".hlasmplugin";

    EXPECT_EQ(resource_location::join(res, rel_loc), resource_location("aaa:src/.hlasmplugin"));
}
TEST(resource_location, join_with_regular_dir_no_auth)
{
    resource_location res("aaa:src/temp/");
    std::string rel_loc = ".hlasmplugin";

    EXPECT_EQ(resource_location::join(res, rel_loc), resource_location("aaa:src/temp/.hlasmplugin"));
}

TEST(resource_location, join_with_root_file)
{
    resource_location res("aaa://src");
    std::string rel_loc = ".hlasmplugin";

    EXPECT_EQ(resource_location::join(res, rel_loc), resource_location("aaa://src/.hlasmplugin"));
}

TEST(resource_location, join_with_regular_file)
{
    resource_location res("aaa://src/temp");
    std::string rel_loc = ".hlasmplugin";

    EXPECT_EQ(resource_location::join(res, rel_loc), resource_location("aaa://src/temp/.hlasmplugin"));
}

TEST(resource_location, join_with_root_file_no_auth)
{
    resource_location res("aaa:src");
    std::string rel_loc = ".hlasmplugin";

    EXPECT_EQ(resource_location::join(res, rel_loc), resource_location("aaa:src/.hlasmplugin"));
}

TEST(resource_location, join_with_regular_file_no_auth)
{
    resource_location res("aaa:src/temp");
    std::string rel_loc = ".hlasmplugin";

    EXPECT_EQ(resource_location::join(res, rel_loc), resource_location("aaa:src/temp/.hlasmplugin"));
}

TEST(resource_location, join_prepending_slash)
{
    resource_location rl("aaa://src/dir/a");

    rl.join("/b");
    EXPECT_EQ(rl, resource_location("aaa://src/b"));
}

TEST(resource_location, join_prepending_slash_2)
{
    resource_location rl("aaa://src/dir/a/");

    rl.join("/b");
    EXPECT_EQ(rl, resource_location("aaa://src/b"));
}

TEST(resource_location, join_mulitple_slashes)
{
    resource_location rl("aaa://src/dir/////a///");

    rl.join("b///");
    EXPECT_EQ(rl, resource_location("aaa://src/dir/////a///b///"));
}

TEST(resource_location, relative_reference_resolution_rfc_3986_normal)
{
    resource_location rl("http://a/b/c/d;p?q");

    // TODO Enable this when we start supporting 1 letter schemes
    // EXPECT_EQ(resource_location::relative_reference_resolution(rl, "g:h").get_uri(), "g:h");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "aaa:h").get_uri(), "aaa:h");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "g").get_uri(), "http://a/b/c/g");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "./g").get_uri(), "http://a/b/c/g");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "g/").get_uri(), "http://a/b/c/g/");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "/g").get_uri(), "http://a/g");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "//g").get_uri(), "http://g");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "?y").get_uri(), "http://a/b/c/d;p?y");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "g?y").get_uri(), "http://a/b/c/g?y");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "#s").get_uri(), "http://a/b/c/d;p?q#s");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "g#s").get_uri(), "http://a/b/c/g#s");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "g?y#s").get_uri(), "http://a/b/c/g?y#s");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, ";x").get_uri(), "http://a/b/c/;x");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "g;x").get_uri(), "http://a/b/c/g;x");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "g;x?y#s").get_uri(), "http://a/b/c/g;x?y#s");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "").get_uri(), "http://a/b/c/d;p?q");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, ".").get_uri(), "http://a/b/c/");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "./").get_uri(), "http://a/b/c/");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "..").get_uri(), "http://a/b/");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "../").get_uri(), "http://a/b/");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "../g").get_uri(), "http://a/b/g");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "../..").get_uri(), "http://a/");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "../../").get_uri(), "http://a/");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "../../g").get_uri(), "http://a/g");
}

TEST(resource_location, relative_reference_resolution_rfc_3986_abnormal)
{
    resource_location rl("http://a/b/c/d;p?q");

    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "../../../g").get_uri(), "http://a/g");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "../../../../g").get_uri(), "http://a/g");
}

TEST(resource_location, relative_reference_resolution_rfc_3986_dot_double_dot_2)
{
    resource_location rl("http://a/b/c/d;p?q");

    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "/./g").get_uri(), "http://a/g");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "/../g").get_uri(), "http://a/g");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "g.").get_uri(), "http://a/b/c/g.");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, ".g").get_uri(), "http://a/b/c/.g");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "g..").get_uri(), "http://a/b/c/g..");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "..g").get_uri(), "http://a/b/c/..g");
}

TEST(resource_location, relative_reference_resolution_rfc_3986_query_fragment)
{
    resource_location rl("http://a/b/c/d;p?q");

    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "./../g").get_uri(), "http://a/b/g");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "./g/.").get_uri(), "http://a/b/c/g/");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "g/./h").get_uri(), "http://a/b/c/g/h");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "g/../h").get_uri(), "http://a/b/c/h");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "g;x=1/./y").get_uri(), "http://a/b/c/g;x=1/y");
    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "g;x=1/../y").get_uri(), "http://a/b/c/y");
}

TEST(resource_location, relative_reference_resolution_rfc_3986_strict_parsers)
{
    resource_location rl("http://a/b/c/d");

    EXPECT_EQ(resource_location::relative_reference_resolution(rl, "http:g").get_uri(), "http:g");
}

namespace {
std::vector<std::string> get_file_roots()
{
    return is_windows() ? std::vector<std::string> { "file:///C:/",
        "file:///c:/",
        "file:///C%3A/",
        "file:///c%3A/",
        "file:///C%3a/",
        "file:///c%3a/" }
                        : std::vector<std::string> { "file:///home/" };
}

std::pair<std::vector<resource_location>, resource_location> construct_combinations(
    std::string_view decoded_suffix, std::vector<std::string_view> encoded_suffixes)
{
    std::vector<resource_location> equivalent;
    for (const auto& file_root : get_file_roots())
        for (const auto& suffix : encoded_suffixes)
            equivalent.emplace_back(file_root + std::string(suffix));

    if (is_windows())
        return { equivalent, resource_location("file:///C:/" + std::string(decoded_suffix)).lexically_normal() };
    else
        return { equivalent, resource_location("file:///home/" + std::string(decoded_suffix)).lexically_normal() };
}
} // namespace

TEST(resource_location, lexically_normal_percent_encoded_chars)
{
    std::vector<std::pair<std::vector<resource_location>, resource_location>> test_cases;
    test_cases.emplace_back(construct_combinations("temp+", { "temp%2B", "temp%2b" }));
    test_cases.emplace_back(construct_combinations("temp+/", { "temp%2B/", "temp%2b/" }));
    test_cases.emplace_back(
        construct_combinations("temp++", { "temp%2B%2B", "temp%2B%2b", "temp%2b%2B", "temp%2b%2b" }));
    test_cases.emplace_back(
        construct_combinations("temp++/", { "temp%2B%2B/", "temp%2B%2b/", "temp%2b%2B/", "temp%2b%2b/" }));

    for (const auto& [equivalents, expected] : test_cases)
        for (const auto& equivalent : equivalents)
        {
            auto rl = equivalent.lexically_normal();
            EXPECT_TRUE(rl == expected) << rl.get_uri() << " should be equal to " << expected.get_uri();
        }
}

TEST(resource_location, replace_filename)
{
    EXPECT_EQ(resource_location::replace_filename(resource_location("a"), "b").get_uri(), "b");
    EXPECT_EQ(resource_location::replace_filename(resource_location("x/a"), "b").get_uri(), "x/b");
    EXPECT_EQ(resource_location::replace_filename(resource_location("x/"), "b").get_uri(), "x/b");
    EXPECT_EQ(resource_location::replace_filename(resource_location("schema:a"), "b").get_uri(), "schema:b");
    EXPECT_EQ(resource_location::replace_filename(resource_location("schema://h/a"), "b").get_uri(), "schema://h/b");
    EXPECT_EQ(
        resource_location::replace_filename(resource_location("schema://h/a/x/"), "b").get_uri(), "schema://h/a/x/b");
    EXPECT_EQ(resource_location::replace_filename(resource_location("schema://h/a"), "").get_uri(), "schema://h/");
    EXPECT_EQ(resource_location::replace_filename(resource_location("schema://h/a/x/f?zzz"), "b").get_uri(),
        "schema://h/a/x/b?zzz");
}

TEST(resource_location, filename)
{
    EXPECT_EQ(resource_location("a").filename(), "a");
    EXPECT_EQ(resource_location("x/a").filename(), "a");
    EXPECT_EQ(resource_location("x/").filename(), "");
    EXPECT_EQ(resource_location("schema:a").filename(), "a");
    EXPECT_EQ(resource_location("schema://h/a").filename(), "a");
    EXPECT_EQ(resource_location("schema://h/a/x/").filename(), "");
    EXPECT_EQ(resource_location("schema://h/a").filename(), "a");
    EXPECT_EQ(resource_location("schema://h/a/x/f?zzz").filename(), "f");
}

TEST(resource_location, parent)
{
    EXPECT_EQ(resource_location("a").parent().get_uri(), "");
    EXPECT_EQ(resource_location("x/a").parent().get_uri(), "x");
    EXPECT_EQ(resource_location("x/").parent().get_uri(), "x");
    EXPECT_EQ(resource_location("schema:a").parent().get_uri(), "schema:");
    EXPECT_EQ(resource_location("schema://h/a").parent().get_uri(), "schema://h");
    EXPECT_EQ(resource_location("schema://h/a/x/").parent().get_uri(), "schema://h/a/x");
    EXPECT_EQ(resource_location("schema://h/a").parent().get_uri(), "schema://h");
    EXPECT_EQ(resource_location("schema://h/a/x/f?zzz").parent().get_uri(), "schema://h/a/x?zzz");
}

TEST(resource_location, get_local_path_or_uri)
{
    EXPECT_EQ(resource_location("schema://h/a").get_local_path_or_uri(), "schema://h/a");
    EXPECT_EQ(resource_location("schema:h/a").get_local_path_or_uri(), "schema:h/a");
    EXPECT_EQ(resource_location("x/a").get_local_path_or_uri(), "x/a");
    EXPECT_EQ((is_windows() ? resource_location("file:///C:/dir/file") : resource_location("file:///home/file"))
                  .get_local_path_or_uri(),
        is_windows() ? "c:\\dir\\file" : "/home/file");
}

TEST(resource_location, is_prefix)
{
    const resource_location base("scheme://d1/d2/f1");

    EXPECT_TRUE(resource_location::is_prefix(resource_location("scheme://d1/d2/f1/"), base));
    EXPECT_TRUE(resource_location::is_prefix(resource_location("scheme://d1/d2/f1"), base));
    EXPECT_TRUE(resource_location::is_prefix(resource_location("scheme://d1/d2/"), base));
    EXPECT_TRUE(resource_location::is_prefix(resource_location("scheme://d1/d2"), base));
    EXPECT_TRUE(resource_location::is_prefix(resource_location("scheme://d1/"), base));
    EXPECT_TRUE(resource_location::is_prefix(resource_location("scheme://d1"), base));

    EXPECT_FALSE(resource_location::is_prefix(resource_location("scheme://d1/d2/f"), base));
    EXPECT_FALSE(resource_location::is_prefix(resource_location("scheme://d1/d2/f1/f2"), base));
    EXPECT_FALSE(resource_location::is_prefix(resource_location("scheme://d1/d2/d3"), base));
    EXPECT_FALSE(resource_location::is_prefix(resource_location("scheme://d1/d2/d3/"), base));
    EXPECT_FALSE(resource_location::is_prefix(resource_location("scheme://d1/d2/d3/f1"), base));
    EXPECT_FALSE(resource_location::is_prefix(resource_location("scheme://d9/d8/f1"), base));
    EXPECT_FALSE(resource_location::is_prefix(resource_location("diff_scheme://d1/d2/f1"), base));
}
