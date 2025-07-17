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
#include "context/ordinary_assembly/address.h"
#include "context/ordinary_assembly/symbol.h"

// tests for DC instruction

TEST(DC, not_defined_symbol_in_length)
{
    std::string input = R"(
A DC CL(D-C)'1'
B LR 1,1
C LR 1,1

R EQU B-A
)";

    analyzer a(input);
    a.analyze();
    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(DC, non_previously_defined_length)
{
    std::string input = R"(
A DC CL(C-B)'1'
B LR 1,1
C LR 1,1

R EQU B-A
)";

    analyzer a(input);
    a.analyze();
    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "R"), 2);
}

TEST(DC, previously_defined_length)
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
    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "R"), 4);
}

TEST(DC, implicit_length)
{
    std::string input = R"(
A LR 1,1
B DC S(1,1)
C LR 1,1

R EQU C-B
)";

    analyzer a(input);
    a.analyze();
    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "R"), 4);
}

TEST(DC, implicit_length_deferred_checking)
{
    std::string input = R"(
  USING *,12
B DC S(C,-1)
C LR 1,1

R EQU C-B
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D022" }));
}

TEST(DC, simple_cycle)
{
    std::string input = R"(
A DC BL(B-A)'101'
B LR 1,1

)";

    analyzer a(input);
    a.analyze();
    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(DC, space_cycle)
{
    std::string input = R"(
A DC AL(D-C)(1,1,1)
B LR 1,1
C DC CL(B-A)'1'
D LR 1,1
)";

    analyzer a(input);
    a.analyze();
    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(DC, len_attr)
{
    std::string input = R"(
X EQU -12
A DC CL(X+14)'A'
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "a")->attributes().length(), (symbol_attributes::len_attr)2);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(DC, scale_attr)
{
    std::string input = R"(
X EQU 22
A DC FS(X+14)'1'
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "a")->attributes().scale(), (symbol_attributes::scale_attr)36);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(DC, scale_with_unary_op)
{
    std::string input = R"(
P DC FS+12'1'
M DC FS-12'1'
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "P")->attributes().scale(), (symbol_attributes::scale_attr)12);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "M")->attributes().scale(), (symbol_attributes::scale_attr)-12);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(DC, cyclic_len_non_forward)
{
    std::string input = R"(
X DC CL(A+1)'X'
A EQU L'X
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "A"), 0);

    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(DC, cyclic_len_forward)
{
    std::string input = R"(
A EQU L'X
X DC CL(A+1)'X'
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "A"), 1);

    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(DC, valid_len_ref)
{
    std::string input = R"(
A EQU L'X
X DC CL(Y)'X'
Y EQU L'A
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "A"), 1);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "X")->attributes().length(), (symbol_attributes::len_attr)1);
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "Y"), 1);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(DC, invalid_len_ref)
{
    std::string input = R"(
A EQU L'X
X DC CL(Y+1)'X'
Y EQU A
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "A"), 1);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "X")->attributes().length(), (symbol_attributes::len_attr)1);
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "Y"), 0);

    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(DC, self_cycle)
{
    std::string input = R"(
X DC CL(L'X)'X'
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(DC, manual_alignment)
{
    std::string input = R"(
S   CSECT
    DC    C' '
    DS    XL(((((*-S)+4095)/4096)*4096)-(*-S))'00'
T   EQU   *
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    auto t = get_symbol_reloc(a.hlasm_ctx(), "T");

    ASSERT_TRUE(t);

    EXPECT_EQ(t->offset(), 0x1000);
}

TEST(DC, tolerate_incorrect_nominal_value)
{
    std::string input = R"(
    DC C(0)
    DC A'0'
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D018", "D017" }));
}

TEST(DC, reloc_in_length_and_dupl_fields)
{
    std::string input = R"(
TEST CSECT
     DC CL(TEST)' '
     DC (TEST)C' '
     DS CL(TEST)
     DS (TEST)C
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D034", "D034", "D034", "D034" }));
}

TEST(DC, correctly_count_long_utf8_chars)
{
    std::string input = (const char*)u8"X    DC  C'\u00A6'\n"
                                     u8"LEN  EQU *-X\n"
                                     u8"XA   DC  CA'\u00A6'\n"
                                     u8"LENA EQU *-XA\n"
                                     u8"XE   DC  CE'\u00A6'\n"
                                     u8"LENE EQU *-XE\n"
                                     u8"XU   DC  CU'\u00A6'\n"
                                     u8"LENU EQU *-XU\n";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "LEN"), 1);
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "LENA"), 1);
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "LENE"), 1);
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "LENU"), 2);
}

TEST(DC, validate_attribute_in_nominal)
{
    std::string input = R"(
    DC A(L'X)
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E010" }));
}

TEST(DC, validate_attribute_in_nominal_self_reference)
{
    std::string input = R"(
C        CSECT
         DS    C
H        DS    CL(H-C)
         DC    A(L'H)
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(DC, invalid_self_reference)
{
    std::string input = R"(
H        DS    CL(L'H)
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E033" }));
}

TEST(DC, r_type_invalid)
{
    std::string input = " DC R(X)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D012" }));
}

TEST(DC, r_type_invalid_length)
{
    std::string input = R"(
    DC R(X)
A   DS 0F
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol_address(a.hlasm_ctx(), "A"), std::pair(0, std::string()));
}

TEST(DC, r_type_valid)
{
    std::string input = " DC R(X)";

    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(DC, p_attr)
{
    std::string input = R"(
P   DC  AP(X'12345678')(0)
)";

    analyzer a(input);
    a.analyze();

    const auto p = get_symbol(a.hlasm_ctx(), "P");
    ASSERT_TRUE(p);

    EXPECT_EQ(p->attributes().prog_type(), program_type(0x12345678));
}

TEST(DC, p_attr_invalid)
{
    std::string input = R"(
P   DC  AP(X)(0)
X   EQU 0
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A175" }));
}
