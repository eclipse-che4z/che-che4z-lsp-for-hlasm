/*
 * Copyright (c) 2019 Broadcom.
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

#include "common_testing.h"

 //tests for LOCTR instruction

TEST(LOCTR, relocatable_values)
{
    std::string input = R"(
A CSECT
X LR 1,1
B LOCTR
Y LR 1,1
A LOCTR
  LR 1,1
  LR 1,1
  LR 1,1
  LR 1,1
  LR 1,1
  LR 1,1

Z EQU Y-X
 LR Z,Z
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->kind() == symbol_value_kind::RELOC);
    EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->kind() == symbol_value_kind::RELOC);
    EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("Z"))->kind() == symbol_value_kind::ABS);

    ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Z"))->value().get_abs(), 16);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)2);
}

TEST(LOCTR, different_counter)
{
    std::string input = R"(
A CSECT
  LR 1,1
L LOCTR
X LR 1,1
B CSECT
  LR 1,1
L LOCTR
Y LR 1,1

Z EQU Y-X
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->kind() == symbol_value_kind::RELOC);
    EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->kind() == symbol_value_kind::RELOC);
    EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("Z"))->kind() == symbol_value_kind::ABS);

    ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Z"))->value().get_abs(), 2);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(LOCTR, valid_alignment)
{
    std::string input = R"(
A    CSECT
X1   DS   0F
B    LOCTR 
X2   DS   0H
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

