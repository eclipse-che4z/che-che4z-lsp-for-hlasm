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

// tests for ordinary symbols feature:
// relocatable/absolute value and attribute value
// space/alignment creation
// cyclic definition/location counter

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
    EXPECT_TRUE(a.context().ord_ctx.section_defined(a.context().ids().add("A"), section_kind::EXECUTABLE));
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
    EXPECT_EQ(
        a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->value().value_kind(), symbol_value_kind::RELOC);
    EXPECT_EQ(
        a.context().ord_ctx.get_symbol(a.context().ids().add("Y"))->value().value_kind(), symbol_value_kind::RELOC);

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

    ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("F"))->value().get_abs(), 4);

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
    ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("B"))->value().get_abs(), 2);
    ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("C"))->value().get_abs(), 0);
    ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X1"))->value().get_abs(), 1);
    ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X2"))->value().get_abs(), 0);
    ASSERT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X3"))->value().get_abs(), 2);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

class loc_mock : public workspaces::parse_lib_provider
{
    asm_option asm_options;
    virtual workspaces::parse_result parse_library(
        const std::string& library, context::hlasm_context& hlasm_ctx, const workspaces::library_data data)
    {
        std::string lib_data("XXX EQU 1");
        analyzer a(lib_data, library, hlasm_ctx, *this, data);
        a.analyze();
        return true;
    }

    virtual bool has_library(const std::string&, context::hlasm_context&) const { return true; }

    virtual const asm_option& get_asm_options(const std::string&) { return asm_options; }
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
    EXPECT_EQ(
        a.context().ord_ctx.get_symbol(a.context().ids().add("XXX"))->symbol_location, location({ 0, 0 }, "COPYF"));
}

TEST(ordinary_symbols, alignment_cycle)
{
    std::string input = R"(
     DS   (X)B
S1   EQU  *
S2   DS   0F
X    EQU  S2-S1
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(ordinary_symbols, no_alignment_cycle)
{
    std::string input = R"(
     DS   (X)F
S1   EQU  *
S2   DS   0F
X    EQU  S2-S1
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, space_valid_alignment)
{
    std::string input = R"(
A    CSECT
     DS   3C
     DS   (X)D
B    EQU  *
     DS   1F
     DS   3X
     DS   2D
X    EQU  *-B
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->value().get_abs(), 24);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, valid_alignment_resolution)
{
    std::string input = R"(
A    CSECT
     DS     3C
B    LOCTR
     DS     2F
C    LOCTR
     DS     1D

X    EQU    *-A
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.context().ord_ctx.get_symbol(a.context().ids().add("X"))->value().get_abs(), 24);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, relocatable_ca_invalid)
{
    std::string input = R"(
A LR 1,1
  DS (D)C
B LR 1,1
D EQU 6
C EQU B-A
  AIF (C EQ 0).A
.A ANOP
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(ordinary_symbols, relocatable_ca_valid)
{
    std::string input = R"(
A LR 1,1
D EQU 6
  DS (D)C
B LR 1,1
C EQU B-A
  AIF (C EQ 0).A
.A ANOP
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);
}
