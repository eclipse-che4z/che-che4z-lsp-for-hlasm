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

#ifndef HLASMPLUGIN_PARSER_PARSER_MODEL_TEST_H
#define HLASMPLUGIN_PARSER_PARSER_MODEL_TEST_H
#include "common_testing.h"

auto parse_model(std::string s,range r)
{
	std::string input(" LR &var,1");
	analyzer a(input);
	return a.parser().parse_operand_field(
		&a.context(), std::move(s), false, range_provider(r, adjusting_state::NONE),
		std::make_pair(processing_format(processing_kind::ORDINARY, processing_form::MACH), op_code()));
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


#endif