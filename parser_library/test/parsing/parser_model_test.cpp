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

// tests for parsing model statements:
// various invalid statements parsing
// checking correct ranges

auto parse_model(std::string s,
    range r,
    bool after_substitution = false,
    diagnostic_op_consumer* diag_consumer = nullptr,
    processing_form form = processing_form::MACH)
{
    hlasm_context context;
    diagnostic_op_consumer_container fallback_container;
    return statement_fields_parser(&context).parse_operand_field(std::move(s),
        after_substitution,
        range_provider(r, adjusting_state::NONE),
        std::make_pair(processing_format(processing_kind::ORDINARY, form), op_code()),
        diag_consumer ? *diag_consumer : fallback_container);
}

TEST(parser, parse_model)
{
    {
        range r(position(0, 4), position(0, 10));
        auto [op, rem] = parse_model("&var,1", r);

        ASSERT_EQ(op.value.size(), (size_t)1);
        ASSERT_EQ(rem.value.size(), (size_t)0);

        EXPECT_EQ(op.field_range, r);
        EXPECT_EQ(rem.field_range, range(position(0, 10)));
        EXPECT_TRUE(op.value[0]->access_model());
    }
    {
        range r(position(0, 4), position(0, 8));
        auto [op, rem] = parse_model("&var", r);

        ASSERT_EQ(op.value.size(), (size_t)1);
        ASSERT_EQ(rem.value.size(), (size_t)0);

        EXPECT_EQ(op.field_range, r);
        EXPECT_EQ(rem.field_range, range(position(0, 8)));
        EXPECT_TRUE(op.value[0]->access_model());
    }
}

TEST(parser, parse_model_with_remark_alone)
{
    range r(position(0, 4), position(0, 18));
    auto [op, rem] = parse_model("&var rem,. ???", r);

    ASSERT_EQ(op.value.size(), (size_t)1);
    ASSERT_EQ(rem.value.size(), (size_t)1);

    EXPECT_EQ(op.field_range, range(position(0, 4), position(0, 8)));
    EXPECT_EQ(rem.field_range, range(position(0, 9), position(0, 18)));
    EXPECT_TRUE(op.value[0]->access_model());
}

TEST(parser, parse_model_with_remark_before)
{
    range r(position(0, 4), position(0, 20));
    auto [op, rem] = parse_model("1,&var rem,. ???", r);

    ASSERT_EQ(op.value.size(), (size_t)1);
    ASSERT_EQ(rem.value.size(), (size_t)1);

    EXPECT_EQ(op.field_range, range(position(0, 4), position(0, 10)));
    EXPECT_EQ(rem.field_range, range(position(0, 11), position(0, 20)));
    EXPECT_TRUE(op.value[0]->access_model());
}

TEST(parser, parse_model_with_remark_after)
{
    range r(position(0, 4), position(0, 20));
    auto [op, rem] = parse_model("&var,1 rem,. ???", r);

    ASSERT_EQ(op.value.size(), (size_t)1);
    ASSERT_EQ(rem.value.size(), (size_t)1);

    EXPECT_EQ(op.field_range, range(position(0, 4), position(0, 10)));
    EXPECT_EQ(rem.field_range, range(position(0, 11), position(0, 20)));
    EXPECT_TRUE(op.value[0]->access_model());
}

TEST(parser, parse_model_with_remark_before_after)
{
    range r(position(0, 4), position(0, 26));
    auto [op, rem] = parse_model("1,&var,'&v'4 rem,. ???", r);

    ASSERT_EQ(op.value.size(), (size_t)1);
    ASSERT_EQ(rem.value.size(), (size_t)1);

    EXPECT_EQ(op.field_range, range(position(0, 4), position(0, 16)));
    EXPECT_EQ(rem.field_range, range(position(0, 17), position(0, 26)));
    EXPECT_TRUE(op.value[0]->access_model());
}

TEST(parser, parse_model_with_remark_string)
{
    range r(position(0, 4), position(0, 26));
    auto [op, rem] = parse_model("1,'&var',h,. rem,. ???", r);

    ASSERT_EQ(op.value.size(), (size_t)1);
    ASSERT_EQ(rem.value.size(), (size_t)1);

    EXPECT_EQ(op.field_range, range(position(0, 4), position(0, 16)));
    EXPECT_EQ(rem.field_range, range(position(0, 17), position(0, 26)));
    EXPECT_TRUE(op.value[0]->access_model());
}

TEST(parser, parse_model_with_apostrophe_escaping)
{
    range r(position(0, 4), position(0, 26));
    auto [op, rem] = parse_model("*,'%GEN=''''&CFARG '''", r);

    ASSERT_EQ(op.value.size(), (size_t)1);
    ASSERT_EQ(rem.value.size(), (size_t)0);

    EXPECT_EQ(op.field_range, range(position(0, 4), position(0, 26)));
    EXPECT_TRUE(op.value[0]->access_model());
}

TEST(parser, parse_bad_model)
{
    diagnostic_op_consumer_container diag_container;

    range r(position(0, 4), position(0, 5));
    auto [op, rem] = parse_model("'", r, true, &diag_container);

    ASSERT_EQ(op.value.size(), 0U);
    ASSERT_EQ(rem.value.size(), 0U);

    std::vector<diagnostic_op>& diags = diag_container.diags;

    ASSERT_EQ(diags.size(), 1U);
    EXPECT_EQ(diags[0].message, "While evaluating the result of substitution ''' => Unexpected end of statement");

    range expected_range = { { 0, 5 }, { 0, 5 } };
    EXPECT_EQ(diags[0].diag_range, expected_range);
}

TEST(parser, parse_bad_model_no_substitution)
{
    diagnostic_op_consumer_container diag_container;

    range r(position(0, 4), position(0, 5));
    auto [op, rem] = parse_model("'", r, false, &diag_container);

    std::vector<diagnostic_op>& diags = diag_container.diags;
    ASSERT_EQ(diags.size(), 1U);
    EXPECT_EQ(diags[0].message, "Unexpected end of statement");
    range expected_range = { { 0, 5 }, { 0, 5 } };
    EXPECT_EQ(diags[0].diag_range, expected_range);
}

TEST(parser, invalid_self_def)
{
    diagnostic_op_consumer_container diag_container;

    range r(position(0, 5), position(0, 12));
    auto [op, rem] = parse_model("1,A'10'", r, false, &diag_container);

    std::vector<diagnostic_op>& diags = diag_container.diags;
    ASSERT_EQ(diags.size(), 1U);
    EXPECT_EQ(diags[0].code, "CE015");

    range expected_range = { { 0, 7 }, { 0, 12 } };
    EXPECT_EQ(diags[0].diag_range, expected_range);
}

TEST(parser, invalid_macro_param_alternative)
{
    diagnostic_op_consumer_container diag_container;

    range r(position(0, 3), position(0, 16));
    std::string input = R"(op1,   remark                                                       X
               ()";
    auto [op, rem] = parse_model(input, r, false, &diag_container, processing_form::MAC);

    std::vector<diagnostic_op>& diags = diag_container.diags;
    ASSERT_EQ(diags.size(), 1U);
    EXPECT_EQ(diags[0].code, "S0003");

    range expected_range = { { 1, 16 }, { 1, 16 } };
    EXPECT_EQ(diags[0].diag_range, expected_range);
}
