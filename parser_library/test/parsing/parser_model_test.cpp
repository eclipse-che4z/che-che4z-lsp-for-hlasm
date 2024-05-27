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

#include <numeric>

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "context/hlasm_context.h"
#include "processing/statement_fields_parser.h"
#include "semantics/operand_impls.h"

// tests for parsing model statements:
// various invalid statements parsing
// checking correct ranges

auto parse_model(std::string s,
    range r,
    bool after_substitution = false,
    diagnostic_op_consumer* diag_consumer = nullptr,
    processing_form form = processing_form::MACH,
    hlasm_context* context = nullptr)
{
    hlasm_context fallback_context;
    diagnostic_op_consumer_container fallback_container;
    return statement_fields_parser(context ? context : &fallback_context)
        .parse_operand_field(std::move(s),
            after_substitution,
            range_provider(r, adjusting_state::NONE),
            r.start.column,
            std::make_pair(processing_format(processing_kind::ORDINARY, form), op_code()),
            diag_consumer ? *diag_consumer : fallback_container);
}

TEST(parser, parse_model)
{
    {
        range r(position(0, 4), position(0, 10));
        auto [op, rem, lit] = parse_model("&var,1", r);

        ASSERT_EQ(op.value.size(), (size_t)1);
        ASSERT_EQ(rem.value.size(), (size_t)0);
        EXPECT_TRUE(lit.empty());

        EXPECT_EQ(op.field_range, r);
        EXPECT_EQ(rem.field_range, range(position(0, 10)));
        EXPECT_TRUE(op.value[0]->access_model());
    }
    {
        range r(position(0, 4), position(0, 8));
        auto [op, rem, lit] = parse_model("&var", r);

        ASSERT_EQ(op.value.size(), (size_t)1);
        ASSERT_EQ(rem.value.size(), (size_t)0);
        EXPECT_TRUE(lit.empty());

        EXPECT_EQ(op.field_range, r);
        EXPECT_EQ(rem.field_range, range(position(0, 8)));
        EXPECT_TRUE(op.value[0]->access_model());
    }
}

TEST(parser, parse_model_with_remark_alone)
{
    range r(position(0, 4), position(0, 18));
    auto [op, rem, lit] = parse_model("&var rem,. ???", r);

    ASSERT_EQ(op.value.size(), (size_t)1);
    ASSERT_EQ(rem.value.size(), (size_t)1);
    EXPECT_TRUE(lit.empty());

    EXPECT_EQ(op.field_range, range(position(0, 4), position(0, 8)));
    EXPECT_EQ(rem.field_range, range(position(0, 9), position(0, 18)));
    EXPECT_TRUE(op.value[0]->access_model());
}

TEST(parser, parse_model_with_remark_before)
{
    range r(position(0, 4), position(0, 20));
    auto [op, rem, lit] = parse_model("1,&var rem,. ???", r);

    ASSERT_EQ(op.value.size(), (size_t)1);
    ASSERT_EQ(rem.value.size(), (size_t)1);
    EXPECT_TRUE(lit.empty());

    EXPECT_EQ(op.field_range, range(position(0, 4), position(0, 10)));
    EXPECT_EQ(rem.field_range, range(position(0, 11), position(0, 20)));
    EXPECT_TRUE(op.value[0]->access_model());
}

TEST(parser, parse_model_with_remark_after)
{
    range r(position(0, 4), position(0, 20));
    auto [op, rem, lit] = parse_model("&var,1 rem,. ???", r);

    ASSERT_EQ(op.value.size(), (size_t)1);
    ASSERT_EQ(rem.value.size(), (size_t)1);
    EXPECT_TRUE(lit.empty());

    EXPECT_EQ(op.field_range, range(position(0, 4), position(0, 10)));
    EXPECT_EQ(rem.field_range, range(position(0, 11), position(0, 20)));
    EXPECT_TRUE(op.value[0]->access_model());
}

TEST(parser, parse_model_with_remark_before_after)
{
    range r(position(0, 4), position(0, 26));
    auto [op, rem, lit] = parse_model("1,&var,'&v'4 rem,. ???", r);

    ASSERT_EQ(op.value.size(), (size_t)1);
    ASSERT_EQ(rem.value.size(), (size_t)1);
    EXPECT_TRUE(lit.empty());

    EXPECT_EQ(op.field_range, range(position(0, 4), position(0, 16)));
    EXPECT_EQ(rem.field_range, range(position(0, 17), position(0, 26)));
    EXPECT_TRUE(op.value[0]->access_model());
}

TEST(parser, parse_model_with_remark_string)
{
    range r(position(0, 4), position(0, 26));
    auto [op, rem, lit] = parse_model("1,'&var',h,. rem,. ???", r);

    ASSERT_EQ(op.value.size(), (size_t)1);
    ASSERT_EQ(rem.value.size(), (size_t)1);
    EXPECT_TRUE(lit.empty());

    EXPECT_EQ(op.field_range, range(position(0, 4), position(0, 16)));
    EXPECT_EQ(rem.field_range, range(position(0, 17), position(0, 26)));
    EXPECT_TRUE(op.value[0]->access_model());
}

TEST(parser, parse_model_with_apostrophe_escaping)
{
    range r(position(0, 4), position(0, 26));
    auto [op, rem, lit] = parse_model("*,'%GEN=''''&CFARG '''", r);

    ASSERT_EQ(op.value.size(), (size_t)1);
    ASSERT_EQ(rem.value.size(), (size_t)0);
    EXPECT_TRUE(lit.empty());

    EXPECT_EQ(op.field_range, range(position(0, 4), position(0, 26)));
    EXPECT_TRUE(op.value[0]->access_model());
}

TEST(parser, parse_bad_model)
{
    diagnostic_op_consumer_container diag_container;

    range r(position(0, 4), position(0, 5));
    auto [op, rem, lit] = parse_model("'", r, true, &diag_container);

    ASSERT_EQ(op.value.size(), 0U);
    ASSERT_EQ(rem.value.size(), 0U);
    EXPECT_TRUE(lit.empty());

    std::vector<diagnostic_op>& diags = diag_container.diags;

    EXPECT_TRUE(matches_message_text(
        diags, { "While evaluating the result of substitution ''' => Unexpected end of statement" }));
    EXPECT_TRUE(matches_message_properties(diags, { range({ 0, 5 }, { 0, 5 }) }, &diagnostic_op::diag_range));
}

TEST(parser, parse_bad_model_no_substitution)
{
    diagnostic_op_consumer_container diag_container;

    range r(position(0, 4), position(0, 5));
    auto [op, rem, lit] = parse_model("'", r, false, &diag_container);

    std::vector<diagnostic_op>& diags = diag_container.diags;
    EXPECT_TRUE(matches_message_text(diags, { "Unexpected end of statement" }));
    EXPECT_TRUE(matches_message_properties(diags, { range({ 0, 5 }, { 0, 5 }) }, &diagnostic_op::diag_range));
}

TEST(parser, invalid_self_def)
{
    diagnostic_op_consumer_container diag_container;

    range r(position(0, 5), position(0, 12));
    auto [op, rem, lit] = parse_model("1,A'10'", r, false, &diag_container);

    std::vector<diagnostic_op>& diags = diag_container.diags;

    EXPECT_TRUE(matches_message_codes(diags, { "CE015" }));
    EXPECT_TRUE(matches_message_properties(diags, { range({ 0, 7 }, { 0, 12 }) }, &diagnostic_op::diag_range));
}

TEST(parser, invalid_macro_param_alternative)
{
    diagnostic_op_consumer_container diag_container;

    range r(position(0, 3), position(0, 16));
    std::string input = R"(op1,   remark                                                       X
               ()";
    auto [op, rem, lit] = parse_model(input, r, false, &diag_container, processing_form::MAC);

    std::vector<diagnostic_op>& diags = diag_container.diags;

    EXPECT_TRUE(matches_message_codes(diags, { "S0003" }));
    EXPECT_TRUE(matches_message_properties(diags, { range({ 1, 16 }, { 1, 16 }) }, &diagnostic_op::diag_range));
}

TEST(parser, parse_single_apostrophe_string)
{
    diagnostic_op_consumer_container diag_container;
    hlasm_context context;

    range r(position(0, 10), position(0, 20));
    auto [op, rem, lit] =
        parse_model("&VAR,C''''", r, false, &diag_container, processing::processing_form::MACH, &context);

    EXPECT_TRUE(diag_container.diags.empty());

    ASSERT_EQ(op.value.size(), 1);
    auto* model = op.value[0]->access_model();
    ASSERT_TRUE(model);
    auto cc = concatenation_point::to_string(model->chain);
    EXPECT_EQ(std::ranges::count(cc, '\''), 4);
}

TEST(parser, parse_single_apostrophe_literal)
{
    diagnostic_op_consumer_container diag_container;
    hlasm_context context;

    range r(position(0, 10), position(0, 21));
    auto [op, rem, lit] =
        parse_model("&VAR,=C''''", r, false, &diag_container, processing::processing_form::MACH, &context);

    EXPECT_TRUE(diag_container.diags.empty());

    ASSERT_EQ(op.value.size(), 1);
    auto* model = op.value[0]->access_model();
    ASSERT_TRUE(model);
    auto cc = concatenation_point::to_string(model->chain);
    EXPECT_EQ(std::ranges::count(cc, '\''), 4);
}

TEST(parser, sanitize_message_content_replace)
{
    diagnostic_op_consumer_container diag_container;

    range r(position(0, 10), position(0, 15));
    auto [op, rem, lit] = parse_model("=C'\xC2'", r, true, &diag_container);

    ASSERT_EQ(diag_container.diags.size(), 1);

    const auto& msg = diag_container.diags[0].message;

    EXPECT_TRUE(std::ranges::all_of(msg, [](unsigned char c) { return c < 0x80; }));
}

TEST(parser, sanitize_message_content_valid_multibyte)
{
    diagnostic_op_consumer_container diag_container;

    range r(position(0, 10), position(0, 14));
    std::string line = "=C'\xC2\x80";
    auto [op, rem, lit] = parse_model(line, r, true, &diag_container);

    ASSERT_EQ(diag_container.diags.size(), 1);

    const auto& msg = diag_container.diags[0].message;

    EXPECT_NE(msg.find(line), std::string::npos);
}
