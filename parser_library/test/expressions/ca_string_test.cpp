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

#include "context/hlasm_context.h"
#include "expressions/conditional_assembly/terms/ca_constant.h"
#include "expressions/conditional_assembly/terms/ca_string.h"
#include "expressions/evaluation_context.h"
#include "library_info_transitional.h"
#include "semantics/concatenation.h"
#include "semantics/variable_symbol.h"

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library;

TEST(ca_string, undefined_attributes)
{
    diagnostic_op_consumer_container diags;
    context::hlasm_context ctx;
    evaluation_context eval_ctx { ctx, library_info_transitional::empty, diags };

    concat_chain value;
    value.emplace_back(char_str_conc("gfds", range()));

    ca_expr_ptr dupl = std::make_unique<ca_constant>(1, range());

    ca_string::substring_t substr;
    substr.start = std::make_unique<ca_constant>(1, range());
    substr.count = std::make_unique<ca_constant>(1, range());


    ca_string s(std::move(value), std::move(dupl), std::move(substr), range());

    std::vector<context::id_index> references;
    EXPECT_FALSE(s.get_undefined_attributed_symbols(references, eval_ctx));

    EXPECT_EQ(references.size(), 0U);
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
        test_param { std::string(8000, '1'), 1, std::string(4064, '1'), true, "too_big_value" },
        test_param { std::string(4000, '1'), 2, std::string(4064, '1'), true, "too_big_dupl_factor" }),
    stringer());

TEST(ca_string, test)
{
    concat_chain value;
    value.emplace_back(char_str_conc("gfds", range()));

    ca_expr_ptr dupl = std::make_unique<ca_constant>(0, range());

    ca_string s(std::move(value), std::move(dupl), ca_string::substring_t(), range());

    diagnostic_op_consumer_container diags;
    context::hlasm_context ctx;
    evaluation_context eval_ctx { ctx, library_info_transitional::empty, diags };

    auto res = s.evaluate(eval_ctx);

    EXPECT_TRUE(diags.diags.empty());

    EXPECT_EQ(res.access_c(), "");
}

TEST_P(ca_string_suite, dupl)
{
    concat_chain value;
    value.emplace_back(char_str_conc(GetParam().value, range()));

    ca_expr_ptr dupl = std::make_unique<ca_constant>(GetParam().factor, range());

    ca_string s(std::move(value), std::move(dupl), ca_string::substring_t(), range());

    diagnostic_op_consumer_container diags;
    context::hlasm_context ctx;
    evaluation_context eval_ctx { ctx, library_info_transitional::empty, diags };

    auto res = s.evaluate(eval_ctx);

    EXPECT_EQ(diags.diags.size(), GetParam().error);
    EXPECT_EQ(res.access_c(), GetParam().result);
}
