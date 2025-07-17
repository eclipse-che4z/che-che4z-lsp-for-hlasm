/*
 * Copyright (c) 2025 Broadcom.
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

#include "../common_testing.h"

TEST(ca_sysattrp, equ)
{
    std::string input = R"(
P       EQU 0,,,C'ABCD'
&RES    SETC SYSATTRP('P')
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "RES"), "ABCD");
}

TEST(ca_sysattrp, empty)
{
    std::string input = R"(
&RES    SETC SYSATTRP('')
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "RES"), "");
}

TEST(ca_sysattrp, missing)
{
    std::string input = R"(
&RES    SETC SYSATTRP('P')
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "RES"), "");
}

TEST(ca_sysattrp, dc)
{
    std::string input = R"(
P       DC   AP(C'ABCD')(0)
&RES    SETC SYSATTRP('P')
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "RES"), "ABCD");
}

TEST(ca_sysattrp, nothing)
{
    std::string input = R"(
P       DC   A(0)
&RES    SETC SYSATTRP('P')
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "RES"), "");
}
