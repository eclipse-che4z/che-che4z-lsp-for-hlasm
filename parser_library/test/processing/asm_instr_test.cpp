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

#include "analyzer.h"
#include "ebcdic_encoding.h"

using namespace hlasm_plugin::parser_library;

TEST(asm_instr_processing, CCW)
{
    std::string input = R"(
       LR 1,1   random instruction to test CCW alignment
LEN    EQU 4
CCWSYM    CCW X'04',ADDR,0,LEN
ADDR   DS CL4
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_EQ(a.diags().size(), 0U);

    auto& ctx = *a.context().hlasm_ctx;

    auto symbol = ctx.ord_ctx.get_symbol(ctx.ids().add("CCWSYM"));
    ASSERT_NE(symbol, nullptr);
    EXPECT_EQ(symbol->value().get_reloc().offset(), 8);

    EXPECT_EQ(symbol->attributes().get_attribute_value(context::data_attr_kind::T), 'W'_ebcdic);
    EXPECT_EQ(symbol->attributes().get_attribute_value(context::data_attr_kind::L), 8);
}

TEST(asm_instr_processing, CCW_loctr_correct_assign)
{
    std::string input = R"(
SYM  DS   CL249
     CCW  *-SYM,0,0,0
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), 1U);
    EXPECT_EQ(a.diags()[0].code, "M122");
}
