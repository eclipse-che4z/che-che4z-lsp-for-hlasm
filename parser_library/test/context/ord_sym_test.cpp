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
#include "../mock_parse_lib_provider.h"
#include "context/hlasm_context.h"
// tests for ordinary symbols feature:
// relocatable/absolute value and attribute value
// space/alignment creation
// cyclic definition/location counter

using namespace hlasm_plugin::utils::resource;
TEST(ordinary_symbols, machine_instruction_duplication)
{
    std::string input(R"(
lbl lr 1,1
lcl lr 1,1
lbl lr 1,1
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(context::id_index("LBL")));
    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(context::id_index("LCL")));

    EXPECT_EQ(a.diags().size(), (size_t)1);
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
    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.section_defined(context::id_index("A"), section_kind::EXECUTABLE));
    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.section_defined(context::id_index("B"), section_kind::DUMMY));

    EXPECT_TRUE(a.diags().empty());
}

TEST(ordinary_symbols, section_duplication)
{
    std::string input(R"(
A CSECT
A DSECT
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.section_defined(context::id_index("A"), section_kind::EXECUTABLE));

    EXPECT_EQ(a.diags().size(), (size_t)1);
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

    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(context::id_index("A")));
    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(context::id_index("B")));
    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(context::id_index("C")));

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "A"), 12);
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "B"), 11);
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "C"), 10);

    EXPECT_TRUE(a.diags().empty());
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

    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(context::id_index("D")));
    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(context::id_index("X")));
    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(context::id_index("Y")));

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "D"), 2);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "X")->value().value_kind(), symbol_value_kind::RELOC);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "Y")->value().value_kind(), symbol_value_kind::RELOC);

    EXPECT_TRUE(a.diags().empty());
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

    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(context::id_index("A")));
    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(context::id_index("B")));
    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(context::id_index("C")));

    EXPECT_TRUE(get_symbol(a.hlasm_ctx(), "A")->kind() == symbol_value_kind::ABS);
    EXPECT_TRUE(get_symbol(a.hlasm_ctx(), "B")->kind() == symbol_value_kind::ABS);
    EXPECT_TRUE(get_symbol(a.hlasm_ctx(), "C")->kind() == symbol_value_kind::ABS);

    EXPECT_EQ(a.diags().size(), (size_t)1);
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

    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(context::id_index("A")));
    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(context::id_index("B")));

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "A"), 101);
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "B"), 100);

    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(ordinary_symbols, relocatable_bad_place)
{
    std::string input(R"(
 LR A,1
A LR 1,1
)");
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(context::id_index("A")));
    EXPECT_TRUE(get_symbol(a.hlasm_ctx(), "A")->kind() == symbol_value_kind::RELOC);

    EXPECT_TRUE(matches_message_codes(a.diags(), { "M110" }));
}

TEST(ordinary_symbols, relocatable_address_multiplication)
{
    std::string input(R"(
A LR 1,1
B LR A*2,1
)");
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(context::id_index("A")));
    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(context::id_index("B")));
    EXPECT_TRUE(get_symbol(a.hlasm_ctx(), "A")->kind() == symbol_value_kind::RELOC);

    EXPECT_EQ(a.diags().size(), (size_t)2);
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

    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(context::id_index("X")));
    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(context::id_index("Y")));
    EXPECT_TRUE(get_symbol(a.hlasm_ctx(), "X")->kind() == symbol_value_kind::RELOC);
    EXPECT_TRUE(get_symbol(a.hlasm_ctx(), "Y")->kind() == symbol_value_kind::RELOC);

    EXPECT_TRUE(a.diags().empty());
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

    EXPECT_TRUE(get_symbol(a.hlasm_ctx(), "X1")->kind() == symbol_value_kind::RELOC);
    EXPECT_TRUE(get_symbol(a.hlasm_ctx(), "Y1")->kind() == symbol_value_kind::RELOC);
    EXPECT_TRUE(get_symbol(a.hlasm_ctx(), "X2")->kind() == symbol_value_kind::RELOC);
    EXPECT_TRUE(get_symbol(a.hlasm_ctx(), "Y2")->kind() == symbol_value_kind::RELOC);
    EXPECT_TRUE(get_symbol(a.hlasm_ctx(), "U")->kind() == symbol_value_kind::RELOC);
    EXPECT_TRUE(get_symbol(a.hlasm_ctx(), "V")->kind() == symbol_value_kind::RELOC);
    EXPECT_TRUE(get_symbol(a.hlasm_ctx(), "F")->kind() == symbol_value_kind::ABS);

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "F"), 4);

    EXPECT_TRUE(a.diags().empty());
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

    EXPECT_TRUE(get_symbol(a.hlasm_ctx(), "A")->kind() == symbol_value_kind::RELOC);
    ASSERT_EQ(get_symbol_abs(a.hlasm_ctx(), "B"), 2);
    ASSERT_EQ(get_symbol_abs(a.hlasm_ctx(), "C"), 0);
    ASSERT_EQ(get_symbol_abs(a.hlasm_ctx(), "X1"), 1);
    ASSERT_EQ(get_symbol_abs(a.hlasm_ctx(), "X2"), 0);
    ASSERT_EQ(get_symbol_abs(a.hlasm_ctx(), "X3"), 2);

    EXPECT_TRUE(a.diags().empty());
}

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
    std::string lib_data("XXX EQU 1");
    mock_parse_lib_provider mock { { "COPYF", lib_data } };
    analyzer a(input, analyzer_options { resource_location("test"), &mock });
    a.analyze();

    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "X")->symbol_location(), location({ 6, 0 }, resource_location("test")));
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "XX")->symbol_location(), location({ 3, 0 }, resource_location("test")));
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "XXX")->symbol_location(), location({ 0, 0 }, resource_location("COPYF")));
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

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "X"), 24);

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

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "X"), 24);

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

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(ordinary_symbols, postponed_statement_in_macro)
{
    std::string input = R"(
   MACRO
   MAC
   DS XL(*-AAA+(YD-XD))
   MEND
AAA DS C
   MAC
   MAC
   MAC
TEST_AAA EQU *-AAA

XD DSECT
  DS F
YD DS 0C
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "TEST_AAA"), 36);
}

TEST(ordinary_symbols, private_sections_valid)
{
    for (std::string sect_type : { "CSECT", "RSECT", "COM" })
    {
        std::string input = R"(
 DSECT
 )" + sect_type;
        analyzer a(input);
        a.analyze();

        EXPECT_EQ(a.diags().size(), (size_t)0) << sect_type;
    }
}

TEST(ordinary_symbols, private_sections_invalid)
{
    std::initializer_list<std::string> types = { "CSECT", "RSECT", "COM" };

    for (const auto& t1 : types)
    {
        for (const auto& t2 : types)
        {
            std::string input = " " + t1 + "\n " + t2;
            analyzer a(input);
            a.analyze();

            EXPECT_EQ(a.diags().size(), t1 != t2) << t1 << t2;
        }
    }
}

TEST(ordinary_symbols, reduced_cyclic_dependency)
{
    std::string input(R"(
A   DS    0CL(L)
AA  DS    0H
    DS    CL2
    DS    CL(X)
    DS    H
L   EQU   *-AA
X   EQU   1
)");
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "L"), 6);
}

TEST(ordinary_symbols, reduced_cyclic_dependency_impossible)
{
    std::string input(R"(
A   DS    0CL(L)
AA  DS    0H
    DS    CL2
    DS    CL(X)
    DS    F
L   EQU   *-AA
X   EQU   1
)");
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(contains_message_codes(a.diags(), { "E033" }));
}

TEST(ordinary_symbols, first_operand_dependency_after_rebuilt)
{
    std::string input(R"(
&C  SETC '0'
A   DC AL(L'A)(&C)
)");
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(contains_message_codes(a.diags(), { "E033" }));
}

TEST(ordinary_symbols, reduced_cyclic_dependency_cnop)
{
    std::string input(R"(
A   DS    0CL(L)
AA  DS    0FD
    CNOP  2,8
    DS    CL(X)
    CNOP  2,4
    DS    H
L   EQU   *-AA
X   EQU   1
)");
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "L"), 8);
}
