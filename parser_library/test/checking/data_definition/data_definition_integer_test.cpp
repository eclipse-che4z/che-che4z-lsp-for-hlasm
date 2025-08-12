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

#include "../../common_testing.h"

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library;

TEST(data_def_integer_attribute, H)
{
    std::string input = R"(
V   DC  HS6'-25.93'
RES EQU I'V
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "RES"), 9);
}

TEST(data_def_integer_attribute, F)
{
    std::string input = R"(
V   DC  FS8'100.3E-2'
RES EQU I'V
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "RES"), 23);
}

TEST(data_def_integer_attribute, E)
{
    std::string input = R"(
V   DC  ES2'46.415'
RES EQU I'V
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "RES"), 4);
}

TEST(data_def_integer_attribute, D)
{
    std::string input = R"(
V   DC  DS5'-3.729'
RES EQU I'V
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "RES"), 9);
}

TEST(data_def_integer_attribute, L)
{
    std::string input = R"(
V   DC  LS10'5.312'
RES EQU I'V
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "RES"), 18);
}

TEST(data_def_integer_attribute, P)
{
    std::string input = R"(
V   DC  P'+3.513'
RES EQU I'V
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "RES"), 2);
}

TEST(data_def_integer_attribute, Z)
{
    std::string input = R"(
V   DC  Z'3.513'
RES EQU I'V
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "RES"), 1);
}

TEST(data_def_integer_attribute, LD)
{
    std::string input = R"(
V   DC  LD'3.513'
RES EQU I'V
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "RES"), 28);
}

TEST(data_def_integer_attribute, LB)
{
    std::string input = R"(
V   DC  LB'3.513'
RES EQU I'V
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "RES"), 28);
}
