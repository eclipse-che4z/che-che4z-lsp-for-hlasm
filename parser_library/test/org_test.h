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

#ifndef HLASMPLUGIN_PARSERLIBRARY_ORGTEST_H
#define HLASMPLUGIN_PARSERLIBRARY_ORGTEST_H

#include "common_testing.h"

//tests for ORG instruction
TEST(org, duplicate_labels)
{
	std::string input(R"(
A ORG ,
A ORG ,
)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(org, symbol_non_prev_defined)
{
	std::string input(R"(
  ORG S
S LR  1,1
)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(org, symbol_in_different_loctr)
{
	std::string input(R"(
A  CSECT
   LR 1,1
B  LOCTR
B1 LR 1,1
A  LOCTR
   ORG B1 
)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(org, symbol_in_different_section)
{
	std::string input(R"(
A  CSECT
A1 LR 1,1
B  CSECT
B1 LR 1,1
   ORG A1 
)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(org, section_underflow)
{
	std::string input(R"(
A  CSECT
   LR 1,1
   ORG *-3
)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(org, loctr_underflow)
{
	std::string input(R"(
A  CSECT
   LR 1,1
B LOCTR
   ORG *-1
)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(org, subtract_from_asterisk)
{
	std::string input(R"(
A DS  (16)B
  ORG *-1
B EQU *-A
  LR  1,B
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->value().get_abs(), 15);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, add_to_asterisk)
{
	std::string input(R"(
A DS  (15)B
  ORG *+1
B EQU *-A
  LR  1,B
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->value().get_abs(), 16);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(org, jump_to_ord_sym)
{
	std::string input(R"(
A DS  (X)B
  ORG A
X EQU 1
)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, jump_to_ord_sym_plus_const)
{
	std::string input(R"(
A DS  (15)B
  ORG A+3
B EQU *-A
  LR  1,B
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->value().get_abs(), 3);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, jump_after_last_space_and_back_asterisk)
{
	std::string input(R"(
   LR  1,1
S0 DS  (X)C
   LR  1,1
S  ORG *-2
X  EQU S-*
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->value().get_abs(), 2);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, jump_before_last_space_and_back_asterisk)
{
	std::string input(R"(
   LR  1,1
S0 DS  (X)C
   LR  1,1
S  ORG *-3
X  EQU S-*
)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(org, jump_after_last_space_and_back_ord_sym)
{
	std::string input(R"(
S0 DS  (2)C
S1 DS  (X)C
S2 DS  (2)C
S3 ORG S2
   ORG S3
X  EQU *-S2
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->value().get_abs(), 2);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, jump_before_last_space_and_back_ord_sym)
{
	std::string input(R"(
S0 DS  (2)C
S1 DS  (X)C
S2 DS  (2)C
S3 ORG S1
   ORG S3
X  EQU *-S2
)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(org, jump_before_space)
{
	std::string input(R"(
A      DS (8)C
B      DS (X)H
       ORG B
       DS 2H
       DS 1F
Y      EQU *-A
X      EQU 3
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->value().get_abs(), 16);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, jump_before_space_inverted)
{
	std::string input(R"(
S DS 1C
A DS (-X)C
 ORG A
X EQU S-*
)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(org, jump_before_alignment_space)
{
	std::string input(R"(
A      DS (8)C
B      DS (X)H
C      DS 2H
       DS 1F
D      ORG C
X      EQU D-*
)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(org, second_param_invalid)
{
	std::string input(R"(
 ORG *,0
 ORG *,0
 ORG *,3
 ORG *,-2

)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)4);
}

TEST(org, second_param_use)
{
	std::string input(R"(
A  DS  1C
   ORG *,4
   DS  1F
B  EQU *-A
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->value().get_abs(), 8);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, second_param_use_ord_sym)
{
	std::string input(R"(
A  DS  1C
B  DS  (X)C
   ORG B,4
C  EQU *
   DS  1F
Y  EQU *-C
X  EQU 1
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->value().get_abs(), 4);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, second_param_use_ord_sym_false_alignment)
{
	std::string input(R"(
A  DS  1C
B  DS  (X)C
   ORG B,4
C  EQU *
   DS  1F
X  EQU *-C
)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(org, second_param_use_ord_sym_alignment_cycle)
{
	std::string input(R"(
A  DS  (X)C
B  DS  (4)C
   ORG B,4
   DS  1F
X  EQU *-B
)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(org, third_param_invalid)
{
	std::string input(R"(
  ORG *,,A
A EQU 1
  ORG *,,*
S EQU *+1
  ORG *,,S

)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)3);
}

TEST(org, third_param_use)
{
	std::string input(R"(
A  DS  1C
   ORG *,,19
B  EQU *-A
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->value().get_abs(), 20);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, third_param_bad_use)
{
	std::string input(R"(
A  DS  1C
   ORG *,,-3
)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(org, third_param_use_ord_sym)
{
	std::string input(R"(
   DS  (X)C
A  DS  (4)C
   ORG A,,-3
X  EQU A-*
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->value().get_abs(), 3);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, all_params_use)
{
	std::string input(R"(
A  DS  (3)C
B  ORG *,2,3
X  EQU *-B
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->value().get_abs(), 4);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, all_params_false_use)
{
	std::string input(R"(
   DS  (X)C
A  DS  (4)C
   ORG A,2,-3
X  EQU A-*
)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(org, available_empty_params)
{
	std::string input(R"(
A  DS  (4)C
B  ORG
X  EQU *-A
Y  EQU *-B
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->value().get_abs(), 4);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->value().get_abs(), 0);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, available_use_ord_sym)
{
	std::string input(R"(
A  DS  (4)C
B  ORG A
C  ORG
X  EQU *-A
Y  EQU *-B
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->value().get_abs(), 4);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->value().get_abs(), 0);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, available_use_ord_sym_loctr)
{
	std::string input(R"(
L  LOCTR
A  DS  (4)C
B  ORG A
C  ORG
X  EQU *-A
Y  EQU *-B
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->value().get_abs(), 4);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->value().get_abs(), 0);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, available_cycle)
{
	std::string input(R"(
A  DS  (X)C
B  DS  (4)C
   ORG
X  EQU *-B
)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(org, available_competing_simple)
{
	std::string input(R"(
A  DS  (4)C
B  DS  (4)C
   ORG B
C  DS  (X)C
   ORG
Y  EQU *-B
X  EQU 2
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->value().get_abs(), 4);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);

	std::string input2(R"(
A  DS  (4)C
B  DS  (4)C
   ORG B
C  DS  (X)C
   ORG
Y  EQU *-B
X  EQU 5
)");
	analyzer a2(input2);
	a2.analyze();

	EXPECT_EQ(a2.context().ord_ctx.get_symbol(a2.context().ids().add("Y"))->value().get_abs(), 5);

	a2.collect_diags();
	ASSERT_EQ(a2.diags().size(), (size_t)0);
}

TEST(org, available_competing_complex)
{
	std::string input(R"(
A  DS  (X)C
   ORG A
   DS  (Y)C
B  DS  (4)C
   ORG B+6
   ORG
Y  EQU 4
X  EQU 2
Z  EQU *-A
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Z"))->value().get_abs(), 10);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, available_all_param)
{
	std::string input(R"(
A  DS  (4)C
B  DS  (4)C
   ORG B
C  DS  (X)C
   ORG ,4,2
Y  EQU *-B
X  EQU 2
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->value().get_abs(), 6);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, unknown_absolute_part_simple)
{
	std::string input(R"(
A DS (5)C
B ORG *+Y
Y EQU 5
Z EQU *-A
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Z"))->value().get_abs(), 10);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, unknown_absolute_part_space)
{
	std::string input(R"(
A DS (X)C
B ORG *+Y
X EQU *-B
Y EQU 5
Z EQU *-A
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->value().get_abs(), 5);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Z"))->value().get_abs(), 10);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, unknown_absolute_part_simple_all_params)
{
	std::string input(R"(
A DS (5)C
B ORG *+Y,2,3
X EQU *-B
Y EQU 5
Z EQU *-A
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->value().get_abs(), 8);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Z"))->value().get_abs(), 13);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, unknown_absolute_part_space_all_params)
{
	std::string input(R"(
A DS (X)C
B ORG *+Y,2,3
X EQU *-B
Y EQU 5
Z EQU *-A
)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(org, unknown_absolute_part_multiple_spaces)
{
	std::string input(R"(
A DS (X)C
  ORG *+Y
  DS (X)H
  ORG A
  DS (X)F
B EQU *
Y EQU 5
X EQU 5
  ORG
Z EQU *-A
Z2 EQU B-A
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Z"))->value().get_abs(), 20);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Z2"))->value().get_abs(), 20);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}


TEST(org, multiple_unknown_absolute_parts)
{
	std::string input(R"(
A DS (X)C
  ORG *+Y
  DS (X)H
  ORG *+Y2
  DS (X)F
Y EQU 5
Y2 EQU 5
X EQU 5
Z EQU *-A
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Z"))->value().get_abs(), 48);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, multiple_unknown_absolute_parts_loctr)
{
	std::string input(R"(
L LOCTR
A DS (X)C
  ORG *+Y
  DS (X)H
  ORG *+Y2
  DS (X)F
Y EQU 5
Y2 EQU 5
X EQU 5
Z EQU *-A
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Z"))->value().get_abs(), 48);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, unknown_absolute_part_jump_before_space_simple)
{
	std::string input(R"(
A DS (X)C
  ORG *-Y
B EQU *-A
Y EQU 4
X EQU 10
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->value().get_abs(), 6);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);

	std::string input2(R"(
A DS (X)C
  ORG *-Y
B EQU *-A
X EQU 10
Y EQU 4
)");
	analyzer a2(input2);
	a2.analyze();

	EXPECT_EQ(a2.context().ord_ctx.get_symbol(a2.context().ids().add("B"))->value().get_abs(), 6);

	a2.collect_diags();
	ASSERT_EQ(a2.diags().size(), (size_t)0);
}

TEST(org, unknown_absolute_part_jump_before_space_loctr)
{
	std::string input(R"(
L LOCTR
A DS (X)C
  ORG *-Y
B EQU *-A
Y EQU 4
X EQU 10
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->value().get_abs(), 6);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, unknown_absolute_part_jump_before_space_twice_resolve_in_order)
{
	std::string input(R"(
A DS (X)C
  ORG *-Y
B EQU *-A
  ORG *-Y
C EQU *-A
X EQU 10
Y EQU 4
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->value().get_abs(), 6);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("C"))->value().get_abs(), 2);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, unknown_absolute_part_jump_before_space_twice_resolve_in_reverse_order)
{
	std::string input(R"(
A DS (X)C
  ORG *-Y1
B EQU *-A
  ORG *-Y2
C EQU *-A
X EQU 10
Y2 EQU 4
Y1 EQU 4
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->value().get_abs(), 6);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("C"))->value().get_abs(), 2);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(org, unknown_absolute_part_with_available_value)
{
	std::string input(R"(
A  DS (X)C
   ORG *-Y1
   ORG
B EQU *-A
Y1 EQU 4
X EQU 10
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->value().get_abs(), 10);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);

	std::string input2(R"(
A  DS (X)C
   ORG *+Y1
   ORG
B EQU *-A
Y1 EQU 4
X EQU 10
)");
	analyzer a2(input2);
	a2.analyze();

	EXPECT_EQ(a2.context().ord_ctx.get_symbol(a2.context().ids().add("B"))->value().get_abs(), 14);

	a2.collect_diags();
	ASSERT_EQ(a2.diags().size(), (size_t)0);
}

TEST(org, unknown_absolute_part_with_available_value_loctr)
{
	std::string input(R"(
L LOCTR
A  DS (X)C
   ORG *-Y1
   ORG
B EQU *-A
Y1 EQU 4
X EQU 10
)");
	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->value().get_abs(), 10);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);

	std::string input2(R"(
L LOCTR
A  DS (X)C
   ORG *+Y1
   ORG
B EQU *-A
Y1 EQU 4
X EQU 10
)");
	analyzer a2(input2);
	a2.analyze();

	EXPECT_EQ(a2.context().ord_ctx.get_symbol(a2.context().ids().add("B"))->value().get_abs(), 14);

	a2.collect_diags();
	ASSERT_EQ(a2.diags().size(), (size_t)0);
}

TEST(org, unknown_part_non_absolute)
{
	std::string input(R"(
 LR 1,1
 ORG *+Y
Y LR 1,1
)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(org, unknown_part_underflow)
{
	std::string input(R"(
 LR 1,1
 ORG *-Y
Y EQU 3

A CSECT
B LOCTR
 ORG *-Y
Y EQU 3

A2 CSECT
 DS (X)C
 ORG *,,-5
X EQU 4

)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)3);
}

TEST(org, true_negative_check)
{
	std::string input(R"(
TEST CSECT
  DS  (Y)C
A DS  (4)C
  ORG *-X
Y EQU *-A
X EQU 2
)");
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

#endif