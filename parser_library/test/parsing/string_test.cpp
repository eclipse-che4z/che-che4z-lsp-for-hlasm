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

// tests parsing of hlasm strings

TEST(parser, mach_string_double_ampersand)
{
    std::string input("A EQU C'&&'");
    analyzer a(input);
    a.analyze();

    auto& ctx = a.hlasm_ctx();

    auto it = ctx.ids().find("A");

    ASSERT_EQ(ctx.ord_ctx.get_symbol(it)->value().get_abs(), 80);
}

TEST(parser, ca_string_double_ampersand)
{
    std::string input("&A SETC '&&'");
    analyzer a(input);
    a.analyze();

    auto& ctx = a.hlasm_ctx();

    auto it = ctx.ids().find("A");

    ASSERT_EQ(ctx.get_var_sym(it)->access_set_symbol_base()->access_set_symbol<context::C_t>()->get_value(), "&&");
}