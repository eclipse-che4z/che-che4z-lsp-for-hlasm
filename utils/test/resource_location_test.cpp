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

TEST(resource_location, lexically_relative_subset_of_base)
{
    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/")),
        "file_a");

    EXPECT_EQ(resource_location("file:///C:/dir/subdir/").lexically_relative(resource_location("file:///C:/dir/")),
        "subdir/");

    EXPECT_EQ(
        resource_location("file:///C:/dir/subdir/file_a").lexically_relative(resource_location("file:///C:/dir/")),
        "subdir/file_a");
}

TEST(resource_location, lexically_relative)
{
    EXPECT_EQ(resource_location("file:///C:/dir/file_b")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/file_a")),
        "../../file_b");

    EXPECT_EQ(resource_location("file:///C:/dir/subdir/")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/file_a")),
        "../");

    EXPECT_EQ(
        resource_location("file:///C:/").lexically_relative(resource_location("file:///C:/dir/subdir/")), "../../");

    EXPECT_EQ(resource_location("file:///C:/").lexically_relative(resource_location("file:///C:/dir/subdir/file_a")),
        "../../../");

    EXPECT_EQ(resource_location("file:///C:/dir/file_b").lexically_relative(resource_location("file:///C:/dir/file_a")),
        "../file_b");

    EXPECT_EQ(resource_location("file:///C:/dir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/file_a")),
        "../../file_a");

    EXPECT_EQ(
        resource_location("file:///C:/dir/file_a").lexically_relative(resource_location("file:///C:/dir/subdir/")),
        "../file_a");

    EXPECT_EQ(
        resource_location("file:///C:/file_a").lexically_relative(resource_location("file:///C:/dir/subdir/file_a")),
        "../../../file_a");
}

TEST(resource_location, lexically_relative_single_dots)
{
    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/.")),
        "file_a");

    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/./")),
        "file_a");

    EXPECT_EQ(resource_location("file:///C:/dir/./subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/./")),
        ".././subdir/file_a");

    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/./subdir")),
        "../subdir/file_a");

    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/./subdir/")),
        "../subdir/file_a");
}

TEST(resource_location, lexically_relative_double_dots)
{
    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/..")),
        "");

    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/../")),
        "");

    EXPECT_EQ(resource_location("file:///C:/dir/../subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir/../")),
        "../subdir/file_a");

    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/../file_b")),
        "subdir/file_a");

    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/../subdir/")),
        "subdir/file_a");
}

TEST(resource_location, lexically_relative_multiple_slashes)
{
    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:/dir/subdir////")),
        "file_a");

    EXPECT_EQ(resource_location("file:///C:/dir/subdir/file_a")
                  .lexically_relative(resource_location("file:///C:///dir/subdir////")),
        "file_a");

    EXPECT_EQ(
        resource_location("file:///C:/dir/subdir///file_a").lexically_relative(resource_location("file:///C:/dir/")),
        "subdir/file_a");
}

TEST(resource_location, lexically_relative_win_drive_letter)
{
    EXPECT_EQ(
        resource_location("file:///C:/dir/subdir/").lexically_relative(resource_location("file:///C:/dir/subdir/")),
        ".");

    EXPECT_EQ(
        resource_location("file:///c:/dir/subdir/").lexically_relative(resource_location("file:///c:/dir/subdir/")),
        ".");

    EXPECT_EQ(
        resource_location("file:///C%3A/dir/subdir/").lexically_relative(resource_location("file:///C%3A/dir/subdir/")),
        ".");

    EXPECT_EQ(
        resource_location("file:///c%3A/dir/subdir/").lexically_relative(resource_location("file:///c%3A/dir/subdir/")),
        ".");

    EXPECT_EQ(resource_location("file:///C:/dir/file_a").lexically_relative(resource_location("file:///D:/dir/file_a")),
        "../../../C:/dir/file_a");

    EXPECT_EQ(resource_location("file:///D:/dir/file_a").lexically_relative(resource_location("file:///C:/dir/file_a")),
        "../../../D:/dir/file_a");
}

TEST(resource_location, lexically_relative_different_schemes)
{
    EXPECT_EQ(resource_location("file:///home/").lexically_relative(resource_location("aaa:dir/file_a")), "");

    EXPECT_EQ(resource_location("aaa:dir/file_a").lexically_relative(resource_location("file:///C:/dir/file_a")), "");
}

TEST(resource_location, lexically_normal)
{
    EXPECT_EQ(resource_location("file:///").lexically_normal(), "file:///");
    EXPECT_EQ(resource_location("file:///././file").lexically_normal(), "file:///file");
    EXPECT_EQ(resource_location("file:///C:/./file").lexically_normal(), "file:///C:/file");
    EXPECT_EQ(resource_location("file:///C:/./dir/").lexically_normal(), "file:///C:/dir/");
    EXPECT_EQ(resource_location("file:///C:/./dir/.").lexically_normal(), "file:///C:/dir/");
    EXPECT_EQ(resource_location("file:///C:/./dir../.").lexically_normal(), "file:///C:/dir../");
    EXPECT_EQ(resource_location("file:///C:/./dir./.").lexically_normal(), "file:///C:/dir./");
    EXPECT_EQ(resource_location("file:///C:/./dir/..").lexically_normal(), "file:///C:/");
    EXPECT_EQ(resource_location("file:///C:/.///dir/../").lexically_normal(), "file:///C:/");
}

TEST(resource_location, lexically_normal_change_root_dir)
{
    EXPECT_EQ(resource_location("file:///C:/../D:/").lexically_normal(), "file:///D:/");
    EXPECT_EQ(resource_location("file:///C:/../../../hostname").lexically_normal(), "file://hostname");
    EXPECT_EQ(resource_location("file:///C:/../../../hostname/").lexically_normal(), "file://hostname/");
    EXPECT_EQ(
        resource_location("file:///C:/../../../hostname/dir/file").lexically_normal(), "file://hostname/dir/file");
    EXPECT_EQ(resource_location("file:///C:/../../../hostname/..").lexically_normal(), "file://");
    EXPECT_EQ(resource_location("file:///C:/../../../.").lexically_normal(), "file://");
}

TEST(resource_location, lexically_normal_diff_schemes)
{
    EXPECT_EQ(resource_location("aaa:").lexically_normal(), "aaa:");
    EXPECT_EQ(resource_location("aaa:././file").lexically_normal(), "aaa:file");
    EXPECT_EQ(resource_location("aaa:C:/file").lexically_normal(), "aaa:C:/file");
    EXPECT_EQ(resource_location("aaa:C:/dir/").lexically_normal(), "aaa:C:/dir/");
    EXPECT_EQ(resource_location("aaa:C:/dir/.").lexically_normal(), "aaa:C:/dir/");
    EXPECT_EQ(resource_location("aaa:C:/dir../.").lexically_normal(), "aaa:C:/dir../");
    EXPECT_EQ(resource_location("aaa:C:/dir./.").lexically_normal(), "aaa:C:/dir./");
    EXPECT_EQ(resource_location("aaa:C:/dir/..").lexically_normal(), "aaa:C:/");
    EXPECT_EQ(resource_location("aaa:C:///dir/../").lexically_normal(), "aaa:C:/");
}

TEST(resource_location, lexically_normal_change_root_dir_diff_schemes)
{
    EXPECT_EQ(resource_location("aaa:C:/../../../D:").lexically_normal(), "aaa:D:");
    EXPECT_EQ(resource_location("aaa:C:/../../../D:/").lexically_normal(), "aaa:D:/");
    EXPECT_EQ(resource_location("aaa:C:/../../../D:/..").lexically_normal(), "aaa:");
    EXPECT_EQ(resource_location("aaa:C:/../../../.").lexically_normal(), "aaa:");
}

TEST(resource_location, lexically_normal_slashes)
{
    EXPECT_EQ(resource_location("file:///.\\.\\file").lexically_normal(), "file:///file");
    EXPECT_EQ(resource_location("file:\\\\\\C:\\file").lexically_normal(), "file:///C:/file");
    EXPECT_EQ(resource_location("file:\\\\\\C:\\.\\dir\\.").lexically_normal(), "file:///C:/dir/");
}

TEST(resource_location, lexically_normal_file_scheme)
{
    if (hlasm_plugin::utils::platform::is_windows())
    {
        EXPECT_EQ(resource_location("file:C:/Dir/").lexically_normal(), "file:///C:/Dir/");
        EXPECT_EQ(resource_location("file:C%3a/Dir/").lexically_normal(), "file:///C%3a/Dir/");
        EXPECT_EQ(resource_location("file:C%3A/Dir/").lexically_normal(), "file:///C%3A/Dir/");

        EXPECT_EQ(resource_location("file:/C:/Dir/").lexically_normal(), "file:///C:/Dir/");
        EXPECT_EQ(resource_location("file:/C%3a/Dir/").lexically_normal(), "file:///C%3a/Dir/");
        EXPECT_EQ(resource_location("file:/C%3A/Dir/").lexically_normal(), "file:///C%3A/Dir/");
        EXPECT_EQ(resource_location("file:\\C:/Dir/").lexically_normal(), "file:///C:/Dir/");
        EXPECT_EQ(resource_location("file:\\C%3a/Dir/").lexically_normal(), "file:///C%3a/Dir/");
        EXPECT_EQ(resource_location("file:\\C%3A/Dir/").lexically_normal(), "file:///C%3A/Dir/");

        EXPECT_EQ(resource_location("file://C:/Dir/").lexically_normal(), "file:///C:/Dir/");
        EXPECT_EQ(resource_location("file://C%3a/Dir/").lexically_normal(), "file:///C%3a/Dir/");
        EXPECT_EQ(resource_location("file://C%3A/Dir/").lexically_normal(), "file:///C%3A/Dir/");
        EXPECT_EQ(resource_location("file:\\\\C:/Dir/").lexically_normal(), "file:///C:/Dir/");
        EXPECT_EQ(resource_location("file:\\\\C%3a/Dir/").lexically_normal(), "file:///C%3a/Dir/");
        EXPECT_EQ(resource_location("file:\\\\C%3A/Dir/").lexically_normal(), "file:///C%3A/Dir/");

        EXPECT_EQ(resource_location("file:///C:/Dir/").lexically_normal(), "file:///C:/Dir/");
        EXPECT_EQ(resource_location("file:///C%3a/Dir/").lexically_normal(), "file:///C%3a/Dir/");
        EXPECT_EQ(resource_location("file:///C%3A/Dir/").lexically_normal(), "file:///C%3A/Dir/");
        EXPECT_EQ(resource_location("file:\\\\\\C:/Dir/").lexically_normal(), "file:///C:/Dir/");
        EXPECT_EQ(resource_location("file:\\\\\\C%3a/Dir/").lexically_normal(), "file:///C%3a/Dir/");
        EXPECT_EQ(resource_location("file:\\\\\\C%3A/Dir/").lexically_normal(), "file:///C%3A/Dir/");

        EXPECT_EQ(resource_location("file://///C://Dir//").lexically_normal(), "file:///C:/Dir/");
        EXPECT_EQ(resource_location("file://///C%3a//Dir//").lexically_normal(), "file:///C%3a/Dir/");
        EXPECT_EQ(resource_location("file://///C%3A//Dir//").lexically_normal(), "file:///C%3A/Dir/");
        EXPECT_EQ(resource_location("file:\\\\\\\\\\C://Dir//").lexically_normal(), "file:///C:/Dir/");
        EXPECT_EQ(resource_location("file:\\\\\\\\\\C%3a//Dir//").lexically_normal(), "file:///C%3a/Dir/");
        EXPECT_EQ(resource_location("file:\\\\\\\\\\C%3A//Dir//").lexically_normal(), "file:///C%3A/Dir/");

        EXPECT_EQ(resource_location("file://C/Dir//").lexically_normal(), "file://C/Dir/");
        EXPECT_EQ(resource_location("file://host/Dir//").lexically_normal(), "file://host/Dir/");
    }
    else
    {
        EXPECT_EQ(resource_location("file:relative/Dir/").lexically_normal(), "file:relative/Dir/");
        EXPECT_EQ(resource_location("file:relative\\Dir/").lexically_normal(), "file:relative/Dir/");
        EXPECT_EQ(resource_location("file:/absolute/Dir/").lexically_normal(), "file:/absolute/Dir/");
        EXPECT_EQ(resource_location("file:\\absolute\\Dir/").lexically_normal(), "file:/absolute/Dir/");
        EXPECT_EQ(resource_location("file://host/Dir/").lexically_normal(), "file://host/Dir/");
        EXPECT_EQ(resource_location("file:\\\\host\\Dir/").lexically_normal(), "file://host/Dir/");
    }

    EXPECT_EQ(resource_location("").lexically_normal(), "");
    EXPECT_EQ(resource_location("file:").lexically_normal(), "file:");
    EXPECT_EQ(resource_location("file://host/C:/Dir/").lexically_normal(), "file://host/C:/Dir/");
    EXPECT_EQ(resource_location("file:\\\\host/C:/Dir/").lexically_normal(), "file://host/C:/Dir/");
    EXPECT_EQ(resource_location("aaa:C:/Dir/").lexically_normal(), "aaa:C:/Dir/");
    EXPECT_EQ(resource_location(":C:/Dir/").lexically_normal(), ":C:/Dir/");
}

TEST(resource_location, lexically_normal_rfc_3986)
{
    EXPECT_EQ(resource_location("aaa:/a/b/c/./../../g").lexically_normal(), "aaa:/a/g");
    EXPECT_EQ(resource_location("aaa:mid/content=5/../6").lexically_normal(), "aaa:mid/6");
}

// TEST(resource_location, lexically_normal_rfc_3986_syntax_based) // todo
//{
//     EXPECT_EQ(resource_location("example://a/b/c/%7Bfoo%7D").lexically_normal(),
//         resource_location("eXAMPLE://a/./b/../b/%63/%7bfoo%7d").lexically_normal());
// }

// TEST(resource_location, lexically_normal_rfc_3986_case) // todo
//{
//     EXPECT_EQ(resource_location("example://a/b/c/%7Bfoo%7D").lexically_normal(),
//         resource_location("eXAMPLE://a/./b/../b/%63/%7bfoo%7d").lexically_normal());
// }

TEST(resource_location, join_empty)
{
    resource_location rl("aaa://src/dir/");

    rl.join("");
    EXPECT_EQ(rl.get_uri(), "aaa://src/dir/");
}

TEST(resource_location, join_empty_2)
{
    resource_location rl("");

    rl.join("aaa://src/dir/");
    EXPECT_EQ(rl.get_uri(), "aaa://src/dir/");
}

TEST(resource_location, join_uri)
{
    resource_location rl("aaa://src/dir/");

    rl.join("scheme://a/b/c");
    EXPECT_EQ(rl.get_uri(), "scheme://a/b/c");
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
    EXPECT_EQ(rl.get_uri(), "aaa://src/b");
}

TEST(resource_location, join_prepending_slash_2)
{
    resource_location rl("aaa://src/dir/a/");

    rl.join("/b");
    EXPECT_EQ(rl.get_uri(), "aaa://src/b");
}

TEST(resource_location, join_mulitple_slashes)
{
    resource_location rl("aaa://src/dir/////a///");

    rl.join("b///");
    EXPECT_EQ(rl.get_uri(), "aaa://src/dir/////a///b///");
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