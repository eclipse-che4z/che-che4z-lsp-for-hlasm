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

#ifndef HLASMPLUGIN_PARSERLIBARY_ORDSYMTEST_H
#define HLASMPLUGIN_PARSERLIBARY_ORDSYMTEST_H
#include "common_testing.h"

TEST(ordinary_symbols, machine_instruction_duplication)
{
	std::string input(R"(
lbl lr 1,1
lcl lr 1,1
lbl lr 1,1
)");
	analyzer a(input);
	a.analyze();
	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("lbl")));
	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("lcl")));

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(ordinary_symbols, section_continuation)
{
	std::string input(R"(
A CSECT
B DSECT
A CSECT
B DSECT
)");
	analyzer a(input);
	a.analyze();
	EXPECT_TRUE(a.context().ord_ctx.section_defined(a.context().ids().add("A"),section_kind::EXECUTABLE));
	EXPECT_TRUE(a.context().ord_ctx.section_defined(a.context().ids().add("B"), section_kind::DUMMY));

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, section_duplication)
{
	std::string input(R"(
A CSECT
A DSECT
)");
	analyzer a(input);
	a.analyze();
	EXPECT_TRUE(a.context().ord_ctx.section_defined(a.context().ids().add("A"), section_kind::EXECUTABLE));

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(ordinary_symbols, simple_EQU)
{
	std::string input(R"(
A EQU 1
 LR A,A
)");
	analyzer a(input);
	a.analyze();

	auto id = a.context().ids().add("A");

	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(id));
	EXPECT_EQ(a.context().ord_ctx.get_symbol(id)->value().get_abs(), 1);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, complex_EQU)
{
	std::string input(R"(
A EQU 1
B EQU A+A-10
 LR A,B
)");
	analyzer a(input);
	a.analyze();

	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("A")));
	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("B")));

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->value().get_abs(), -8);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(ordinary_symbols, previously_defined_symbol_abs)
{
	std::string input(R"(
A EQU B+1
B EQU C+1
C EQU 10
 LR A,B
)");
	analyzer a(input);
	a.analyze();

	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("A")));
	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("B")));
	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("C")));

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("A"))->value().get_abs(), 12);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->value().get_abs(), 11);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("C"))->value().get_abs(), 10);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, previously_defined_symbol_reloc)
{
	std::string input(R"(
D EQU Y-X

X LR 1,1
Y LR 1,1
 LR D,D
)");
	analyzer a(input);
	a.analyze();

	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("D")));
	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("X")));
	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("Y")));

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("D"))->value().get_abs(), 2);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->value().value_kind(), symbol_value_kind::RELOC);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->value().value_kind(), symbol_value_kind::RELOC);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, cyclic_dependency)
{
	std::string input(R"(
A EQU B+1
B EQU C+1
C EQU A
)");
	analyzer a(input);
	a.analyze();

	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("A")));
	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("B")));
	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("C")));

	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("A"))->kind() == symbol_value_kind::ABS);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->kind() == symbol_value_kind::ABS);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("C"))->kind() == symbol_value_kind::ABS);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(ordinary_symbols, previously_defined_mach_err)
{
	std::string input(R"(
 LR A,1
A EQU B+1
B EQU 100
)");
	analyzer a(input);
	a.analyze();

	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("A")));
	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("B")));

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("A"))->value().get_abs(), 101);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->value().get_abs(), 100);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(ordinary_symbols, relocatable_bad_place)
{
	std::string input(R"(
 LR A,1
A LR 1,1
)");
	analyzer a(input);
	a.analyze();

	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("A")));
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("A"))->kind() == symbol_value_kind::RELOC);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(ordinary_symbols, relocatable_address_multiplication)
{
	std::string input(R"(
A LR 1,1
B LR A*2,1
)");
	analyzer a(input);
	a.analyze();

	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("A")));
	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("B")));
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("A"))->kind() == symbol_value_kind::RELOC);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)2);
}

TEST(ordinary_symbols, relocatable_to_absolute)
{
	std::string input(R"(
X LR 1,1
Y LR 1,1
  LR -X+Y,1
)");
	analyzer a(input);
	a.analyze();

	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("X")));
	EXPECT_TRUE(a.context().ord_ctx.symbol_defined(a.context().ids().add("Y")));
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->kind() == symbol_value_kind::RELOC);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->kind() == symbol_value_kind::RELOC);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, complex_relocatable_address)
{
	std::string input = R"(
A CSECT
X1 LR 1,1
Y1 LR 1,1

B CSECT
X2 LR 1,1
Y2 LR 1,1

C CSECT
U EQU X1+X2
V EQU Y1+Y2
F EQU V-U

 LR F,F
)";

	analyzer a(input);
	a.analyze();

	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("X1"))->kind() == symbol_value_kind::RELOC);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("Y1"))->kind() == symbol_value_kind::RELOC);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("X2"))->kind() == symbol_value_kind::RELOC);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("Y2"))->kind() == symbol_value_kind::RELOC);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("U"))->kind() == symbol_value_kind::RELOC);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("V"))->kind() == symbol_value_kind::RELOC);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("F"))->kind() == symbol_value_kind::ABS);

	ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("F"))->value().get_abs(),4);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, relocatable_LOCTR)
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

TEST(ordinary_symbols, different_LOCTR)
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

TEST(ordinary_symbols, location_counter_simple)
{
	std::string input = R"(
A LR 1,1
B EQU *-A
C EQU *-*
D EQU *
E EQU *
 LR 1,1
F EQU *
G EQU *+1

X1 EQU G-F
X2 EQU E-D
X3 EQU F-E

 LR B,1
)";

	analyzer a(input);
	a.analyze();

	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("A"))->kind() == symbol_value_kind::RELOC);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->kind() == symbol_value_kind::ABS);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("C"))->kind() == symbol_value_kind::ABS);
	ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->value().get_abs(),2);
	ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("C"))->value().get_abs(), 0);
	ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X1"))->value().get_abs(), 1);
	ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X2"))->value().get_abs(), 0);
	ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X3"))->value().get_abs(), 2);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, EQU_length_explicit)
{
	std::string input = R"(
Y EQU X,12
X EQU 5,2
)";

	analyzer a(input);
	a.analyze();

	ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->kind(), symbol_value_kind::ABS);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->value().get_abs(), 5);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->attributes().length(), (symbol_attributes::len_attr) (symbol_attributes::len_attr)2);

	ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->kind(), symbol_value_kind::ABS);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->value().get_abs(), 5);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->attributes().length(), (symbol_attributes::len_attr) 12);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, EQU_length_implicit)
{
	std::string input = R"(
X EQU 5,2
Y EQU X
Z EQU 1+X
ZZ EQU *+X
)";

	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->attributes().length(), (symbol_attributes::len_attr) 2);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Z"))->attributes().length(), (symbol_attributes::len_attr) 1);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("ZZ"))->attributes().length(), (symbol_attributes::len_attr) 1);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, EQU_length_dep)
{
	std::string input = R"(
LEN EQU 11
X EQU UNKNOWN,LEN
UNKNOWN EQU L'X
)";

	analyzer a(input);
	a.analyze();

	ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->kind(), symbol_value_kind::ABS);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->value().get_abs(), 11);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, EQU_length_bounds)
{
	std::string input = R"(
A EQU 1,12
LEN EQU 1+A,-100
LEM EQU A+1,100000
)";

	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("LEN"))->attributes().length(), (symbol_attributes::len_attr) 1);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("LEM"))->attributes().length(), (symbol_attributes::len_attr) 12);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)2);
}

TEST(ordinary_symbols, EQU_type_explicit)
{
	std::string input = R"(
LEN EQU 11,3,4
)";

	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("LEN"))->attributes().type(), 4);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, EQU_type_implicit)
{
	std::string input = R"(
LEN EQU 11,3
)";

	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("LEN"))->attributes().type(), symbol_attributes::undef_type);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, EQU_type_bounds)
{
	std::string input = R"(
LEN EQU 11,1,-1
LEM EQU 11,1,300
)";

	analyzer a(input);
	a.analyze();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("LEN"))->attributes().type(), symbol_attributes::undef_type);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("LEM"))->attributes().type(), symbol_attributes::undef_type);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)2);
}

TEST(ordinary_symbols, DC_not_defined_symbol_in_length)
{
	std::string input = R"(
A DC CL(D-C)'1'
B LR 1,1
C LR 1,1

R EQU B-A
)";

	analyzer a(input);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(a.diags().size(), (size_t)2);
}

TEST(ordinary_symbols, DC_non_previously_defined_length)
{
	std::string input = R"(
A DC CL(C-B)'1'
B LR 1,1
C LR 1,1

R EQU B-A
)";

	analyzer a(input);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(a.diags().size(), (size_t)0);

	id_index R = a.context().ids().add("R");
	EXPECT_EQ(a.context().ord_ctx.get_symbol(R)->value().get_abs(), 2);
}

TEST(ordinary_symbols, DC_previously_defined_length)
{
	std::string input = R"(
A LR 1,1
  LR 1,1
B DC CL(B-A)'ABCD'
C LR 1,1

R EQU C-B
)";

	analyzer a(input);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(a.diags().size(), (size_t)0);

	id_index R = a.context().ids().add("R");
	EXPECT_EQ(a.context().ord_ctx.get_symbol(R)->value().get_abs(), 4);
}

TEST(ordinary_symbols, DC_implicit_length)
{
	std::string input = R"(
A LR 1,1
B DC S(1,1)
C LR 1,1

R EQU C-B
)";

	analyzer a(input);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(a.diags().size(), (size_t)0);

	id_index R = a.context().ids().add("R");
	EXPECT_EQ(a.context().ord_ctx.get_symbol(R)->value().get_abs(), 4);
}

TEST(ordinary_symbols, DC_implicit_length_deferred_checking)
{
	std::string input = R"(
B DC S(C,-1)
C LR 1,1

R EQU C-B
)";

	analyzer a(input);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(a.diags().size(), (size_t)1);
	EXPECT_EQ(a.diags()[0].code, "D022");
}

TEST(ordinary_symbols, DC_simple_cycle)
{
	std::string input = R"(
A DC BL(B-A)'101'
B LR 1,1

)";

	analyzer a(input);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(ordinary_symbols, DC_space_cycle)
{
	std::string input = R"(
A DC AL(D-C)(1,1,1)
B LR 1,1
C DC CL(B-A)'1'
D LR 1,1
)";

	analyzer a(input);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(ordinary_symbols, DC_len_attr)
{
	std::string input = R"(
X EQU -12
A DC CL(X+14)'A'
)";

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("a"))->attributes().length(), (symbol_attributes::len_attr)2);

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, DC_scale_attr)
{
	std::string input = R"(
X EQU 22
A DC FS(X+14)'1'
)";

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("a"))->attributes().scale(), (symbol_attributes::scale_attr)36);

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, DC_cyclic_len_non_forward)
{
	std::string input = R"(
X DC CL(A+1)'X'
A EQU L'X
)";

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("A"))->value().get_abs(), 0);

	EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(ordinary_symbols, DC_cyclic_len_forward)
{
	std::string input = R"(
A EQU L'X
X DC CL(A+1)'X'
)";

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("A"))->value().get_abs(), 1);

	EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(ordinary_symbols, DC_valid_len_ref)
{
	std::string input = R"(
A EQU L'X
X DC CL(Y)'X'
Y EQU L'A
)";

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("A"))->value().get_abs(), 1);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->attributes().length(), (symbol_attributes::len_attr)1);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->value().get_abs(), 1);

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, DC_invalid_len_ref)
{
	std::string input = R"(
A EQU L'X
X DC CL(Y+1)'X'
Y EQU A
)";

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("A"))->value().get_abs(), 1);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->attributes().length(), (symbol_attributes::len_attr)1);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->value().get_abs(), 0);

	EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(ordinary_symbols, DC_self_cycle)
{
	std::string input = R"(
X DC CL(L'X)'X'
)";

	analyzer a(input);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.diags().size(), (size_t)1);
}

class loc_mock : public parse_lib_provider
{
	virtual parse_result parse_library(const std::string& library, context::hlasm_context& hlasm_ctx, const library_data data)
	{
		std::string lib_data("XXX EQU 1");
		analyzer a(lib_data, library, hlasm_ctx, *this, data);
		a.analyze();
		return true;
	}

	virtual bool has_library(const std::string&, context::hlasm_context&) const { return true; }

};

TEST(ordinary_symbols, symbol_location)
{
	std::string input = R"(
 MACRO
 M
XX EQU 1
 MEND

X EQU 1
 M
 COPY COPYF

)";
	loc_mock tmp;
	analyzer a(input, "test", tmp);
	a.analyze();
	a.collect_diags();

	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->symbol_location, location({ 6, 0 }, "test"));
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("XX"))->symbol_location, location({ 3, 0 }, "test"));
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("XXX"))->symbol_location, location({ 0, 0 }, "COPYF"));

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

#endif
