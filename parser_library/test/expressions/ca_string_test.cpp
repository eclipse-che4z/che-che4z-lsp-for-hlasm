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

#include "gmock/gmock.h"

#include "expr_mocks.h"
#include "expressions/conditional_assembly/terms/ca_constant.h"
#include "expressions/conditional_assembly/terms/ca_string.h"
#include "expressions/evaluation_context.h"
#include "semantics/concatenation_term.h"

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library;

TEST(ca_string, undefined_attributes)
{
    context::hlasm_context ctx;
    evaluation_context eval_ctx { ctx, workspaces::empty_parse_lib_provider::instance };

    concat_chain value;
    value.push_back(std::make_unique<char_str_conc>("gfds", range()));

    ca_expr_ptr dupl = std::make_unique<ca_constant>(1, range());

    ca_string::substring_t substr;
    substr.start = std::make_unique<ca_constant>(1, range());
    substr.count = std::make_unique<ca_constant>(1, range());


    ca_string s(std::move(value), std::move(dupl), std::move(substr), range());

    auto res = s.get_undefined_attributed_symbols(eval_ctx);

    ASSERT_EQ(res.size(), 0U);
}

struct test_param
{
    std::string value;
    int factor;
    std::string result;
    bool error;
    std::string name;
};

struct stringer
{
    std::string operator()(::testing::TestParamInfo<test_param> p) { return p.param.name; }
};

class ca_string_suite : public ::testing::TestWithParam<test_param>
{};

INSTANTIATE_TEST_SUITE_P(param_suite,
    ca_string_suite,
    ::testing::Values(test_param { "abc", 0, "", false, "dupl_zero" },
        test_param { "abc", -5, "", true, "dupl_negative" },
        test_param { big_string() + big_string(), 1, "", true, "too_big_value" },
        test_param { big_string(), 2, "", true, "too_big_dupl_factor" }),
    stringer());

TEST(ca_string, test)
{
    concat_chain value;
    value.push_back(std::make_unique<char_str_conc>("gfds", range()));

    ca_expr_ptr dupl = std::make_unique<ca_constant>(0, range());

    ca_string s(std::move(value), std::move(dupl), ca_string::substring_t(), range());

    context::hlasm_context ctx;
    evaluation_context eval_ctx { ctx, workspaces::empty_parse_lib_provider::instance };

    auto res = s.evaluate(eval_ctx);

    ASSERT_EQ(eval_ctx.diags().size(), 0U);

    EXPECT_EQ(res.access_c(), "");
}

TEST_P(ca_string_suite, dupl)
{
    concat_chain value;
    value.push_back(std::make_unique<char_str_conc>(GetParam().value, range()));

    ca_expr_ptr dupl = std::make_unique<ca_constant>(GetParam().factor, range());

    ca_string s(std::move(value), std::move(dupl), ca_string::substring_t(), range());

    context::hlasm_context ctx;
    evaluation_context eval_ctx { ctx, workspaces::empty_parse_lib_provider::instance };

    auto res = s.evaluate(eval_ctx);

    ASSERT_EQ(eval_ctx.diags().size(), GetParam().error);

    if (!GetParam().error)
        EXPECT_EQ(res.access_c(), GetParam().result);
}
