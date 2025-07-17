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
#include "context/hlasm_context.h"
#include "context/ordinary_assembly/symbol.h"
#include "ebcdic_encoding.h"

// tests for EQU instruction

TEST(EQU, simple)
{
    std::string input(R"(
A EQU 1
 LR A,A
)");
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "A"), 1);

    EXPECT_TRUE(a.diags().empty());
}

TEST(EQU, complex)
{
    std::string input(R"(
A EQU 1
B EQU A+A-10
 LR A,B
)");
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(id_index("A")));

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "B"), -8);

    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(EQU, length_explicit)
{
    std::string input = R"(
Y EQU X,12
X EQU 5,2
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "X"), 5);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "X")->attributes().length(), (symbol_attributes::len_attr)2);

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "Y"), 5);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "Y")->attributes().length(), (symbol_attributes::len_attr)12);

    EXPECT_TRUE(a.diags().empty());
}

TEST(EQU, length_implicit)
{
    std::string input = R"(
X EQU 5,2
Y EQU X
Z EQU 1+X
ZZ EQU *+X
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "Y")->attributes().length(), (symbol_attributes::len_attr)2);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "Z")->attributes().length(), (symbol_attributes::len_attr)1);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "ZZ")->attributes().length(), (symbol_attributes::len_attr)1);

    EXPECT_TRUE(a.diags().empty());
}

TEST(EQU, length_dep)
{
    std::string input = R"(
LEN EQU 11
X EQU UNKNOWN,LEN
UNKNOWN EQU L'X
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "X"), 11);

    EXPECT_TRUE(a.diags().empty());
}

TEST(EQU, length_bounds)
{
    std::string input = R"(
A EQU 1,12
LEN EQU 1+A,-100
LEM EQU A+1,100000
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "LEN")->attributes().length(), (symbol_attributes::len_attr)1);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "LEM")->attributes().length(), (symbol_attributes::len_attr)12);

    EXPECT_EQ(a.diags().size(), (size_t)2);
}

TEST(EQU, type_explicit)
{
    std::string input = R"(
LEN EQU 11,3,4
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "LEN")->attributes().type(), 4);

    EXPECT_TRUE(a.diags().empty());
}

TEST(EQU, type_implicit)
{
    std::string input = R"(
LEN EQU 11,3
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "LEN")->attributes().type(), symbol_attributes::undef_type);

    EXPECT_TRUE(a.diags().empty());
}

TEST(EQU, type_bounds)
{
    std::string input = R"(
LEN EQU 11,1,-1
LEM EQU 11,1,300
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "LEN")->attributes().type(), symbol_attributes::undef_type);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "LEM")->attributes().type(), symbol_attributes::undef_type);

    EXPECT_EQ(a.diags().size(), (size_t)2);
}

TEST(EQU, loctr_use)
{
    std::string input = R"(
A  DS    A
B  EQU   256
C  EQU   B-*+A
   DS    XL(C)
)";

    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(a.diags().empty());
}

TEST(EQU, deps_with_multiplication)
{
    std::string input = R"(
TEST CSECT
     DS    XL(D)
A    DS    A
L    EQU   (A-TEST)*(A-TEST)
     DS    XL(L)
D    EQU   4
RES  EQU   *-TEST
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "RES"), 24);
}

TEST(EQU, reloc_deps_with_multiplication)
{
    std::string input = R"(
TEST CSECT
     DS    XL(D)
A    DS    A
L    EQU   TEST+(A-TEST)*(A-TEST)
     DS    XL(L-TEST)
D    EQU   4
RES  EQU   *-TEST
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "RES"), 24);
}

TEST(EQU, op_count)
{
    std::string input = R"(
A   EQU 
B   EQU 0,0,C'X',0,GR,0
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A012", "A012" }));
}

TEST(EQU, mandatory_value)
{
    std::string input = R"(
A   EQU ,0
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A132" }));
}

TEST(EQU, t_attr_non_existing_symbol)
{
    std::string input = R"(
A   DS  A
B   EQU A,*-A,T'TYPO
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
    auto b = get_symbol(a.hlasm_ctx(), "B");
    ASSERT_TRUE(b);

    EXPECT_EQ(b->attributes().type(), 'U'_ebcdic);
}

TEST(EQU, p_attr)
{
    std::string input = R"(
N   EQU 0
P   EQU 0,,,X'12345678'
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
    const auto n = get_symbol(a.hlasm_ctx(), "N");
    const auto p = get_symbol(a.hlasm_ctx(), "P");
    ASSERT_TRUE(n);
    ASSERT_TRUE(p);

    EXPECT_EQ(n->attributes().prog_type(), program_type());
    EXPECT_EQ(p->attributes().prog_type(), program_type(0x12345678));
}

TEST(EQU, p_attr_invalid)
{
    std::string input = R"(
P   EQU 0,,,X
X   EQU 0
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A174" }));
}

TEST(EQU, a_attr)
{
    std::string input = R"(
N   EQU 0
AR0 EQU 0,,,,AR
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
    const auto n = get_symbol(a.hlasm_ctx(), "N");
    const auto ar0 = get_symbol(a.hlasm_ctx(), "AR0");
    ASSERT_TRUE(n);
    ASSERT_TRUE(ar0);

    EXPECT_EQ(n->attributes().asm_type(), assembler_type::NONE);
    EXPECT_EQ(ar0->attributes().asm_type(), assembler_type::AR);
}

TEST(EQU, a_attr_invalid)
{
    std::string input = R"(
X   EQU 0
P   EQU 0,,,,X
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A135" }));
}
