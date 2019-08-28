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

TEST(ordinary_symbols, previously_defined_symbol)
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

	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("A"))->kind() == symbol_kind::UNDEF);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->kind() == symbol_kind::UNDEF);
	EXPECT_TRUE(a.context().ord_ctx.get_symbol(a.context().ids().add("C"))->kind() == symbol_kind::UNDEF);

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)3);
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

TEST(ordinary_symbols, relocatable_no_loctr_cycle)
{
	id_storage s;
	ordinary_assembly_context ord_ctx(s);
	symbol_dependency_tables tables(ord_ctx);

	//created space in location counter value
	//  DC (A)C'D'
	auto space = ord_ctx.register_space();
	auto dep = expressions::mach_expr_symbol(s.add("A"),range());
	ASSERT_TRUE(tables.add_dependency(space, &dep, nullptr));

	//created symbols each dependent on location counter value
	//  LR 1,1
	//X LR 1, 1
	//Y LR 1, 1
	auto x = ord_ctx.reserve_storage_area(4, context::no_align);
	auto y = ord_ctx.reserve_storage_area(4, context::no_align);
	ord_ctx.reserve_storage_area(4, context::no_align);
	ord_ctx.create_symbol(s.add("X"), x, context::symbol_attributes());
	ord_ctx.create_symbol(s.add("Y"), y, context::symbol_attributes());
	ASSERT_FALSE(x.spaces.empty());
	ASSERT_FALSE(y.spaces.empty());
	ASSERT_TRUE(tables.add_dependency(s.add("X"),x,nullptr));
	ASSERT_TRUE(tables.add_dependency(s.add("Y"),y,nullptr));

	//expr should not be dependent on space in location counter value
	//A EQU Y-X
	auto expr = mach_expr_binary<sub>(
		std::make_unique<mach_expr_symbol>(s.add("Y"),range()),
		std::make_unique<mach_expr_symbol>(s.add("X"),range()),
		range()
		);

	auto deps = expr.get_dependencies(ord_ctx);

	ASSERT_FALSE(deps.contains_dependencies());
}

TEST(ordinary_symbols, relocatable_loctr_cycle)
{
	id_storage s;
	ordinary_assembly_context ord_ctx(s);
	symbol_dependency_tables tables(ord_ctx);

	//created symbol before space
	//X LR 1,1
	auto x = ord_ctx.align(context::no_align);
	ord_ctx.reserve_storage_area(4, context::no_align);
	ord_ctx.create_symbol(s.add("X"), x, context::symbol_attributes());
	ASSERT_TRUE(x.spaces.empty());

	//created space in location counter value
	//  DC (A)C'D'
	auto space = ord_ctx.register_space();
	auto dep = expressions::mach_expr_symbol(s.add("A"), range());
	ASSERT_TRUE(tables.add_dependency(space, &dep, nullptr));

	//created symbol dependent on location counter value
	//Y LR 1, 1
	auto y = ord_ctx.align(context::no_align);
	ord_ctx.reserve_storage_area(4, context::no_align);
	ord_ctx.create_symbol(s.add("Y"), y, context::symbol_attributes());
	ASSERT_FALSE(y.spaces.empty());
	ASSERT_TRUE(tables.add_dependency(s.add("Y"), y, nullptr));

	//expr should be dependent on space in location counter value
	//A EQU Y-X
	auto expr = mach_expr_binary<sub>(
		std::make_unique<mach_expr_symbol>(s.add("Y"), range()),
		std::make_unique<mach_expr_symbol>(s.add("X"), range()),
		range()
		);

	auto deps = expr.get_dependencies(ord_ctx);

	ASSERT_TRUE(deps.contains_dependencies());
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

TEST(ordinary_symbols, relocatable_bad_layout)
{
	id_storage s;
	ordinary_assembly_context ord_ctx(s);
	symbol_dependency_tables tables(ord_ctx);

	//A CSECT
	ord_ctx.set_section(s.add("A"), section_kind::COMMON);

	//created space in location counter value
	//  DC (D)C'A'
	auto space = ord_ctx.register_space();
	auto dep = expressions::mach_expr_symbol(s.add("D"), range());
	ASSERT_TRUE(tables.add_dependency(space, &dep, nullptr));

	//created symbol in A
	//X LR 1,1
	auto x = ord_ctx.align(context::no_align);
	ord_ctx.reserve_storage_area(4, context::no_align);
	ord_ctx.create_symbol(s.add("X"), x, context::symbol_attributes());
	ASSERT_FALSE(x.spaces.empty());
	ASSERT_TRUE(tables.add_dependency(s.add("X"), x, nullptr));

	//change location counter
	ord_ctx.set_location_counter(s.add("B"));

	//created symbol in B
	//Y LR 1,1
	auto y = ord_ctx.align(context::no_align);
	ord_ctx.reserve_storage_area(4, context::no_align);
	ord_ctx.create_symbol(s.add("Y"), y, context::symbol_attributes());
	ASSERT_FALSE(y.spaces.empty());
	ASSERT_TRUE(tables.add_dependency(s.add("Y"), y, nullptr));

	//expr should be dependent on layout of the section
	//D EQU Y-X
	auto expr = mach_expr_binary<sub>(
		std::make_unique<mach_expr_symbol>(s.add("Y"), range()),
		std::make_unique<mach_expr_symbol>(s.add("X"), range()),
		range()
		);

	auto deps = expr.get_dependencies(ord_ctx);

	ASSERT_TRUE(deps.contains_dependencies());
	ASSERT_FALSE(deps.is_address());

	ord_ctx.create_symbol(s.add("D"), context::symbol_value(), context::symbol_attributes());

	std::vector<const context::resolvable*> tmp = { &expr };
	ASSERT_TRUE(tables.add_dependency(s.add("D"), tmp, nullptr));

	bool bad_layout = false;

	try 
	{
		ord_ctx.finish_module_layout();
	}
	catch (std::runtime_error& )
	{
		bad_layout = true;
	}

	ASSERT_TRUE(bad_layout);
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

#endif
