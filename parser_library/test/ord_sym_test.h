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
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->value().value_kind(), symbol_kind::RELOC);
	EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->value().value_kind(), symbol_kind::RELOC);

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

	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("A"))->kind() == symbol_kind::ABS);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->kind() == symbol_kind::ABS);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("C"))->kind() == symbol_kind::ABS);

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
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("A"))->kind() == symbol_kind::RELOC);

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
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("A"))->kind() == symbol_kind::RELOC);

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
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->kind() == symbol_kind::RELOC);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->kind() == symbol_kind::RELOC);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, complex_relocatable_address)
{
	auto input = R"(
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

	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("X1"))->kind() == symbol_kind::RELOC);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("Y1"))->kind() == symbol_kind::RELOC);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("X2"))->kind() == symbol_kind::RELOC);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("Y2"))->kind() == symbol_kind::RELOC);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("U"))->kind() == symbol_kind::RELOC);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("V"))->kind() == symbol_kind::RELOC);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("F"))->kind() == symbol_kind::ABS);

	ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("F"))->value().get_abs(),4);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, relocatable_LOCTR)
{
	auto input = R"(
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

	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->kind() == symbol_kind::RELOC);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->kind() == symbol_kind::RELOC);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("Z"))->kind() == symbol_kind::ABS);

	ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("Z"))->value().get_abs(), 16);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)2);
}

TEST(ordinary_symbols, location_counter_simple)
{
	auto input = R"(
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

	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("A"))->kind() == symbol_kind::RELOC);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->kind() == symbol_kind::ABS);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("C"))->kind() == symbol_kind::ABS);
	ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->value().get_abs(),2);
	ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("C"))->value().get_abs(), 0);
	ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X1"))->value().get_abs(), 1);
	ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X2"))->value().get_abs(), 0);
	ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X3"))->value().get_abs(), 2);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, DC_not_defined_symbol_in_length)
{
	auto input = R"(
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
	auto input = R"(
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
	auto input = R"(
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
	auto input = R"(
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
	auto input = R"(
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
	auto input = R"(
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
	auto input = R"(
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


#endif
