/*
 * Copyright (c) 2025 Broadcom.
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

#include "../common_testing.h"
#include "context/hlasm_context.h"
#include "parsing/parser_impl.h"

auto prepare_edge_case(std::string_view text)
{
    struct result_t
    {
        hlasm_context ctx;
        parsing::parser_holder h = { ctx, nullptr };

        result_t(std::string_view t) { h.reset(lexing::u8string_view_with_newlines(t), {}, 10); }
    };
    return result_t(text);
}

TEST(parser_edge_cases, mac_ops_empty)
{
    auto [_, h] = prepare_edge_case("(");

    auto ops = h.macro_ops(true);

    EXPECT_TRUE(ops.empty());
}

TEST(parser_edge_cases, ca_args_empty)
{
    auto [_, h] = prepare_edge_case(" (");

    h.op_rem_body_ca_branch();

    EXPECT_TRUE(h.collector.current_operands().value.empty());
}

TEST(parser_edge_cases, ca_args_generic)
{
    auto [_, h] = prepare_edge_case("(");

    auto res = h.op_rem_body_asm(processing::processing_form::ASM_GENERIC, true, false);

    ASSERT_TRUE(res);

    EXPECT_TRUE(res->operands.empty());
}

TEST(parser_edge_cases, ca_args_generic_pseudo_empty)
{
    auto [_, h] = prepare_edge_case(",");

    auto res = h.op_rem_body_asm(processing::processing_form::ASM_GENERIC, true, false);

    ASSERT_TRUE(res);

    EXPECT_EQ(res->operands.size(), 2);
}

TEST(parser_edge_cases, ca_args_alias)
{
    auto [_, h] = prepare_edge_case(",");

    auto res = h.op_rem_body_asm(processing::processing_form::ASM_ALIAS, true, false);

    ASSERT_TRUE(res);

    EXPECT_TRUE(res->operands.empty());
}

TEST(parser_edge_cases, lookahead_invalid_arg)
{
    auto [_, h] = prepare_edge_case(" (");

    h.lookahead_operands_and_remarks_asm();

    EXPECT_TRUE(h.collector.current_operands().value.empty());
}
