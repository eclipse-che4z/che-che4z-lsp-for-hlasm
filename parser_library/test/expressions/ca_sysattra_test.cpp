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

TEST(ca_sysattra, equ)
{
    std::string input = R"(
VR0     EQU 0,,,,VR
&RES    SETC SYSATTRA('VR0')
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "RES"), "VR");
}

TEST(ca_sysattra, nothing)
{
    std::string input = R"(
VR0     EQU 0
&RES    SETC SYSATTRA('VR0')
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "RES"), "");
}

TEST(ca_sysattra, empty)
{
    std::string input = R"(
&RES    SETC SYSATTRA('')
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "RES"), "");
}

TEST(ca_sysattra, missing)
{
    std::string input = R"(
&RES    SETC SYSATTRA('AR0')
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "RES"), "");
}
