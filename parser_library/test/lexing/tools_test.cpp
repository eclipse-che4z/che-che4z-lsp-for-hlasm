/*
 * Copyright (c) 2026 Broadcom.
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

#include <string>

#include "gtest/gtest.h"

#include "lexing/tools.h"

TEST(lexing_tools, is_continued_line)
{
    using hlasm_plugin::parser_library::lexing::is_continued_line;

    const std::string line71(71, 'A');

    EXPECT_FALSE(is_continued_line(""));
    EXPECT_FALSE(is_continued_line("\n"));
    EXPECT_FALSE(is_continued_line("         SHORT LINE\n"));

    EXPECT_FALSE(is_continued_line(line71));
    EXPECT_FALSE(is_continued_line(line71 + "\n"));
    EXPECT_FALSE(is_continued_line(line71 + "\r\n"));

    EXPECT_FALSE(is_continued_line(line71 + " "));
    EXPECT_FALSE(is_continued_line(line71 + " \n"));
    EXPECT_FALSE(is_continued_line(line71 + " \r\n"));

    EXPECT_TRUE(is_continued_line(line71 + "X"));
    EXPECT_TRUE(is_continued_line(line71 + "X\n"));
    EXPECT_TRUE(is_continued_line(line71 + "X\r\n"));

    EXPECT_TRUE(is_continued_line(line71 + "X12345678\n"));
    EXPECT_FALSE(is_continued_line(line71 + " 12345678\n"));
}
