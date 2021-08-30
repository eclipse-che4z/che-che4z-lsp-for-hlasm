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
extern std::regex pathmask_to_regex(std::string input);
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

    EXPECT_TRUE(check_path("/path/**/test/", "\\path\\test\\"));
    EXPECT_TRUE(check_path("/path/**/test/", "\\path\\a\\test\\"));
    EXPECT_TRUE(check_path("/path/**/test/", "\\path\\a\\b\\test\\"));

    EXPECT_TRUE(check_path("/path/*/test/", "\\path\\a\\test\\"));

    EXPECT_TRUE(check_path("/path/a*/test/", "\\path\\a\\test\\"));
    EXPECT_TRUE(check_path("/path/*b/test/", "\\path\\b\\test\\"));
    EXPECT_TRUE(check_path("/path/a*b/test/", "\\path\\ab\\test\\"));

    EXPECT_TRUE(check_path("/path/a**/test/", "\\path\\a\\test\\"));
    EXPECT_TRUE(check_path("/path/**b/test/", "\\path\\b\\test\\"));
    EXPECT_TRUE(check_path("/path/a**b/test/", "\\path\\ab\\test\\"));
    EXPECT_TRUE(check_path("/path/a**b/test/", "\\path\\a\\b\\test\\"));
    EXPECT_TRUE(check_path("/path/a**b/test/", "\\path\\a\\c\\b\\test\\"));

    EXPECT_TRUE(check_path("/path/**/", "\\path\\"));
    EXPECT_TRUE(check_path("/path/**/", "\\path\\test\\"));
    EXPECT_TRUE(check_path("/path/**/", "\\path\\a\\test\\"));
    EXPECT_TRUE(check_path("/path/**/", "\\path\\a\\b\\test\\"));
    EXPECT_TRUE(check_path("/path/**", "\\path\\"));
    EXPECT_TRUE(check_path("/path/**", "\\path\\test\\"));
    EXPECT_TRUE(check_path("/path/**", "\\path\\a\\test\\"));
    EXPECT_TRUE(check_path("/path/**", "\\path\\a\\b\\test\\"));
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
}
