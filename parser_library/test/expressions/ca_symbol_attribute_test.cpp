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

#include "ebcdic_encoding.h"
#include "expr_mocks.h"
#include "expressions/conditional_assembly/terms/ca_constant.h"
#include "expressions/conditional_assembly/terms/ca_symbol_attribute.h"
#include "expressions/evaluation_context.h"

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library;

TEST(ca_symbol_attr, undefined_attributes)
{
    context::hlasm_context ctx;
    lib_prov_mock lib;
    evaluation_context eval_ctx { ctx, lib };

    std::string name = "n";
    std::vector<ca_expr_ptr> subscript;

    subscript.push_back(std::make_unique<ca_constant>(1, range()));

    auto vs = std::make_unique<basic_variable_symbol>(&name, std::move(subscript), range());

    ca_symbol_attribute attr(std::move(vs), context::data_attr_kind::D, range());

    auto res = attr.get_undefined_attributed_symbols(eval_ctx);

    ASSERT_EQ(res.size(), 0U);
}

ca_symbol_attribute create_var_sym_attr(context::data_attr_kind kind, context::id_index name)
{
    auto vs = std::make_unique<basic_variable_symbol>(name, std::vector<ca_expr_ptr> {}, range());

    return ca_symbol_attribute(std::move(vs), kind, range());
}

TEST(ca_symbol_attr, evaluate_undef_varsym)
{
    context::hlasm_context ctx;
    lib_prov_mock lib;
    evaluation_context eval_ctx { ctx, lib };

    auto res = create_var_sym_attr(context::data_attr_kind::D, ctx.ids().add("n")).evaluate(eval_ctx);

    ASSERT_EQ(eval_ctx.diags().size(), 1U);
    EXPECT_EQ(eval_ctx.diags().front().code, "E010");
}

TEST(ca_symbol_attr, evaluate_substituted_varsym_not_char)
{
    context::hlasm_context ctx;
    lib_prov_mock lib;
    evaluation_context eval_ctx { ctx, lib };

    auto name = ctx.ids().add("n");

    auto var = ctx.create_local_variable<int>(name, true);
    var->access_set_symbol<int>()->set_value(12);

    auto res = create_var_sym_attr(context::data_attr_kind::L, name).evaluate(eval_ctx);

    ASSERT_EQ(eval_ctx.diags().size(), 1U);
    EXPECT_EQ(eval_ctx.diags().front().code, "E066");
}

TEST(ca_symbol_attr, evaluate_substituted_varsym_char_not_sym)
{
    context::hlasm_context ctx;
    lib_prov_mock lib;
    evaluation_context eval_ctx { ctx, lib };

    auto name = ctx.ids().add("n");

    auto var = ctx.create_local_variable<context::C_t>(name, true);
    var->access_set_symbol<context::C_t>()->set_value("(abc");

    auto res = create_var_sym_attr(context::data_attr_kind::L, name).evaluate(eval_ctx);

    ASSERT_EQ(eval_ctx.diags().size(), 1U);
    EXPECT_EQ(eval_ctx.diags().front().code, "E065");
}

struct attr_test_param
{
    std::string value;
    context::data_attr_kind attr;
    context::SET_t result;
    std::string name;
};

std::ostream& operator<<(std::ostream& os, const attr_test_param& param)
{
    os << param.value;
    return os;
}

struct stringer
{
    std::string operator()(::testing::TestParamInfo<attr_test_param> p) { return p.param.name; }
};

class ca_attr : public ::testing::TestWithParam<attr_test_param>
{
protected:
    context::hlasm_context ctx;
    lib_prov_mock lib;
    evaluation_context eval_ctx { ctx, lib };
};

INSTANTIATE_TEST_SUITE_P(ca_attr_suite,
    ca_attr,
    ::testing::Values(attr_test_param { "C-A", context::data_attr_kind::T, "w", "T_mach_expr" },
        attr_test_param { "C(R1)", context::data_attr_kind::T, "w", "T_address" },
        attr_test_param { "C'TEXT'", context::data_attr_kind::T, "N", "T_self_def_term" },
        attr_test_param { "C'T''T'", context::data_attr_kind::T, "N", "T_self_def_term_apo" },
        attr_test_param { "C'TEXT'", context::data_attr_kind::L, 1, "L_self_def_term" },
        attr_test_param { "(C)", context::data_attr_kind::L, 10, "T_mach_expr_pars" },
        attr_test_param { "4(,R15)", context::data_attr_kind::T, "U", "T_mach_expr_number" },
        attr_test_param { "A.C", context::data_attr_kind::L, 10, "L_using_prefix" },
        attr_test_param { "T'C", context::data_attr_kind::T, "U", "T_attribute" }),
    stringer());

TEST_P(ca_attr, test)
{
    auto name = ctx.ids().add("VAR");

    (void)ctx.ord_ctx.create_symbol(ctx.ids().add("C"),
        context::symbol_value(),
        context::symbol_attributes(context::symbol_origin::EQU, 'w'_ebcdic, 10),
        location());

    (void)ctx.ord_ctx.create_symbol(ctx.ids().add("T"),
        context::symbol_value(),
        context::symbol_attributes(context::symbol_origin::EQU, 'w'_ebcdic, 10),
        location());

    auto var = ctx.create_local_variable<context::C_t>(name, true);
    var->access_set_symbol<context::C_t>()->set_value(GetParam().value);

    auto result = create_var_sym_attr(GetParam().attr, name).evaluate(eval_ctx);

    ASSERT_EQ(result.type, GetParam().result.type);

    if (result.type == context::SET_t_enum::A_TYPE)
        EXPECT_EQ(result.access_a(), GetParam().result.access_a());
    else if (result.type == context::SET_t_enum::B_TYPE)
        EXPECT_EQ(result.access_b(), GetParam().result.access_b());
    else if (result.type == context::SET_t_enum::C_TYPE)
        EXPECT_EQ(result.access_c(), GetParam().result.access_c());
    else
        FAIL();
}
