/*
 * Copyright (c) 2021 Broadcom.
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

#include <regex>

#include "gtest/gtest.h"

#include "../common_testing.h"

namespace hlasm_plugin::parser_library::workspaces {
extern std::regex pathmask_to_regex(const std::string& input);
} // namespace hlasm_plugin::parser_library::workspaces

bool check_path(std::string pattern, std::string path)
{
    return std::regex_match(path, hlasm_plugin::parser_library::workspaces::pathmask_to_regex(pattern));
}

TEST(pathmask, pass)
{
    EXPECT_TRUE(check_path("/path/**/test/", "/path/test/"));
    EXPECT_TRUE(check_path("/path/**/test/", "/path/a/test/"));
    EXPECT_TRUE(check_path("/path/**/test/", "/path/a/b/test/"));

    EXPECT_TRUE(check_path("/path/*/test/", "/path/a/test/"));

    EXPECT_TRUE(check_path("/path/a*/test/", "/path/a/test/"));
    EXPECT_TRUE(check_path("/path/*b/test/", "/path/b/test/"));
    EXPECT_TRUE(check_path("/path/a*b/test/", "/path/ab/test/"));

    EXPECT_TRUE(check_path("/path/a**/test/", "/path/a/test/"));
    EXPECT_TRUE(check_path("/path/**b/test/", "/path/b/test/"));
    EXPECT_TRUE(check_path("/path/a**b/test/", "/path/ab/test/"));
    EXPECT_TRUE(check_path("/path/a**b/test/", "/path/a/b/test/"));
    EXPECT_TRUE(check_path("/path/a**b/test/", "/path/a/c/b/test/"));

    EXPECT_TRUE(check_path("/path/**/", "/path/"));
    EXPECT_TRUE(check_path("/path/**/", "/path/test/"));
    EXPECT_TRUE(check_path("/path/**/", "/path/a/test/"));
    EXPECT_TRUE(check_path("/path/**/", "/path/a/b/test/"));
    EXPECT_TRUE(check_path("/path/**", "/path/"));
    EXPECT_TRUE(check_path("/path/**", "/path/test/"));
    EXPECT_TRUE(check_path("/path/**", "/path/a/test/"));
    EXPECT_TRUE(check_path("/path/**", "/path/a/b/test/"));

    EXPECT_TRUE(check_path("file:///C%3A/path/**/", "file:///C%3A/path/a/test/"));
    EXPECT_TRUE(check_path("file:///C%3A/path/**/test/", "file:///C%3A/path/a/test/"));
    EXPECT_TRUE(check_path("file:///c%3A/path/**/", "file:///C%3A/path/a/test/"));
    EXPECT_TRUE(check_path("file:///c%3A/path/**/test/", "file:///C%3A/path/a/test/"));
    EXPECT_TRUE(check_path("file:///c%3A/path/**/", "file:///c%3A/path/a/test/"));
    EXPECT_TRUE(check_path("file:///c%3A/path/**/test/", "file:///c%3A/path/a/test/"));
    EXPECT_TRUE(check_path("file:///C%3A/path/**/", "file:///c%3A/path/a/test/"));
    EXPECT_TRUE(check_path("file:///C%3A/path/**/test/", "file:///c%3A/path/a/test/"));

    EXPECT_TRUE(check_path("file:///C%3a/path/**/", "file:///C%3A/path/a/test/"));
    EXPECT_TRUE(check_path("file:///C%3a/path/**/test/", "file:///C%3A/path/a/test/"));
    EXPECT_TRUE(check_path("file:///c%3a/path/**/", "file:///C%3A/path/a/test/"));
    EXPECT_TRUE(check_path("file:///c%3a/path/**/test/", "file:///C%3A/path/a/test/"));
    EXPECT_TRUE(check_path("file:///c%3a/path/**/", "file:///c%3A/path/a/test/"));
    EXPECT_TRUE(check_path("file:///c%3a/path/**/test/", "file:///c%3A/path/a/test/"));
    EXPECT_TRUE(check_path("file:///C%3a/path/**/", "file:///c%3A/path/a/test/"));
    EXPECT_TRUE(check_path("file:///C%3a/path/**/test/", "file:///c%3A/path/a/test/"));

    EXPECT_TRUE(check_path("file:///C:/path/**/", "file:///C%3A/path/a/test/"));
    EXPECT_TRUE(check_path("file:///C:/path/**/test/", "file:///C%3a/path/a/test/"));
    EXPECT_TRUE(check_path("file:///c:/path/**/", "file:///C%3a/path/a/test/"));
    EXPECT_TRUE(check_path("file:///c:/path/**/test/", "file:///C%3A/path/a/test/"));
    EXPECT_TRUE(check_path("file:///c:/path/**/", "file:///c%3A/path/a/test/"));
    EXPECT_TRUE(check_path("file:///c:/path/**/test/", "file:///c%3a/path/a/test/"));
    EXPECT_TRUE(check_path("file:///C:/path/**/", "file:///c%3a/path/a/test/"));
    EXPECT_TRUE(check_path("file:///C:/path/**/test/", "file:///c%3A/path/a/test/"));

    // The following tests just check that there is no longer a SEH exception
    EXPECT_TRUE(check_path("file:///C%3A/User/ws/symlinks/inf/**",
        "file:///C%3A/User/ws/symlinks/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/"
        "inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/"
        "inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/"
        "inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf"));

    EXPECT_TRUE(check_path("file:///C%3A/User/ws/symlinks/inf/**",
        "file:///C%3A/User/ws/symlinks/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/"
        "inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/"
        "inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/"
        "inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/"));

    EXPECT_TRUE(check_path("file:///C%3A/User/ws/symlinks/inf/**/",
        "file:///C%3A/User/ws/symlinks/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/"
        "inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/"
        "inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/"
        "inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/inf/"));
}

TEST(pathmask, fail)
{
    EXPECT_FALSE(check_path("/path/**/test/", "/path/test2/"));
    EXPECT_FALSE(check_path("/path/**/test/", "/path/a/test2/"));
    EXPECT_FALSE(check_path("/path/**/test/", "/path/a/b/test2/"));
    EXPECT_FALSE(check_path("/path/**/test/", "/path/test/test2/"));

    EXPECT_FALSE(check_path("/path/*/test/", "/path/test/"));
    EXPECT_FALSE(check_path("/path/*/test/", "/path/a/b/test/"));

    EXPECT_FALSE(check_path("/path/a*/test/", "/path/b/test/"));
    EXPECT_FALSE(check_path("/path/*b/test/", "/path/a/test/"));
    EXPECT_FALSE(check_path("/path/a*b/test/", "/path/ba/test/"));

    EXPECT_FALSE(check_path("file:///C%3A/path/**/", "file:///c%3A/Path/a/test/"));
    EXPECT_FALSE(check_path("file:///C%3A/path/**/test/", "file:///c%3A/path/a/tEst/"));
    EXPECT_FALSE(check_path("file:///c%3A/path/**/", "file:///C%3A/Path/a/test/"));
    EXPECT_FALSE(check_path("file:///c%3A/path/**/test/", "file:///C%3A/path/a/tEst/"));
}
