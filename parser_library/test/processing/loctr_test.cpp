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

#include "../common_testing.h"
#include "context/ordinary_assembly/symbol.h"

// tests for LOCTR instruction

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

    EXPECT_TRUE(get_symbol(a.hlasm_ctx(), "X")->kind() == symbol_value_kind::RELOC);
    EXPECT_TRUE(get_symbol(a.hlasm_ctx(), "Y")->kind() == symbol_value_kind::RELOC);

    ASSERT_EQ(get_symbol_abs(a.hlasm_ctx(), "Z"), 16);

    EXPECT_EQ(a.diags().size(), (size_t)2);
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

    EXPECT_TRUE(get_symbol(a.hlasm_ctx(), "X")->kind() == symbol_value_kind::RELOC);
    EXPECT_TRUE(get_symbol(a.hlasm_ctx(), "Y")->kind() == symbol_value_kind::RELOC);

    ASSERT_EQ(get_symbol_abs(a.hlasm_ctx(), "Z"), 2);

    EXPECT_TRUE(a.diags().empty());
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

    EXPECT_TRUE(a.diags().empty());
}
