/*
 * Copyright (c) 2023 Broadcom.
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

#include "../common_testing.h"

TEST(label_parsing, pass)
{
    for (std::string p : {
             "=A'",
             "=$C'",
             "=AC'",
             "=C'=C'",
             "=C'&X'",
             "A'",
             "C'=C'",
             "C'&X'",
             "=C'   '",
             "=A(",
             "=C(",
             "' '=C' '",
             "' 'C' '",
             "' '' '",
             "'&X'(1)",
             "&&",
             "'&&'",
             "I'&X",
             "L'&X",
         })
    {
        std::string input = R"(
&X     SETC  ' '
&Y(1)  SETC  ' '
       MACRO
&L     MAC
&A     SETC  'A(0) '.'&L'
       DC    &A
       MEND
)" + p + " MAC";

        analyzer a(input);
        a.analyze();
        a.collect_diags();

        EXPECT_TRUE(a.diags().empty()) << p;
    }
}

TEST(label_parsing, fail)
{
    for (std::string p : {
             "=C'",
             "=.C'",
             "=-C'",
             "=*C'",
             "=(C'",
             "=C'&X",
             "=CL'&X",
             "C'",
             "C'&X",
             "CL'&X",
             "=A'   '",
             "=A(   )",
             "' '=' '",
             "'",
             "'&&",
             "A'&X",
             "D'&X",
         })
    {
        std::string input = R"(
&X     SETC  ' '
&Y(1)  SETC  ' '
       MACRO
&L     MAC
&A     SETC  'A(0) '.'&L'
       DC    &A
       MEND
)" + p + " MAC";

        analyzer a(input);
        a.analyze();
        a.collect_diags();

        EXPECT_FALSE(a.diags().empty()) << p;
    }
}
