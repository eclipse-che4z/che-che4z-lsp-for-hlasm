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

#include "gtest/gtest.h"

#include "../common_testing.h"

TEST(system_id, basic_properties)
{
    std::string input =
        R"(
&A    SETC '&SYSTEM_ID'
&B    SETC T'&SYSTEM_ID
&C    SETA K'&SYSTEM_ID
)";
    analyzer a(input, analyzer_options { asm_option { "", "", instruction_set_version::UNI, "VSE" } });
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "A"), "VSE");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "B"), "U");
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "C"), 3);
}

TEST(system_id, check_defaults)
{
    std::string input =
        R"(
&A    SETC '&SYSTEM_ID'
&B    SETC T'&SYSTEM_ID
&C    SETA K'&SYSTEM_ID
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "A"), "z/OS 02.04.00");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "B"), "U");
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "C"), 13);
}
