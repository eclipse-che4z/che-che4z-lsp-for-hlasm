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
#include "expressions/conditional_assembly/ca_operator_binary.h"
#include "expressions/conditional_assembly/ca_operator_unary.h"
#include "expressions/conditional_assembly/terms/ca_constant.h"
#include "expressions/conditional_assembly/terms/ca_string.h"
#include "expressions/evaluation_context.h"

using namespace std::string_literals;
using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library::context;

struct op_test_param
{
    ca_expr_ops operation;
    SET_t_enum kind;
    std::vector<SET_t> params;
    SET_t true_result;
    std::string name;
};

std::ostream& operator<<(std::ostream& os, const op_test_param& param)
{
    for (auto& p : param.params)
    {
        if (p.type == context::SET_t_enum::A_TYPE)
            os << p.access_a();
        else if (p.type == context::SET_t_enum::B_TYPE)
            os << p.access_b();
        else if (p.type == context::SET_t_enum::C_TYPE)
            os << p.access_c();
        os << " ";
    }
    return os;
}

struct stringer
{
    std::string operator()(::testing::TestParamInfo<op_test_param> p) { return p.param.name; }
};

class ca_op : public ::testing::TestWithParam<op_test_param>
{
protected:
    hlasm_context ctx;
    lib_prov_mock prov;
    evaluation_context eval_ctx { ctx, prov };

    SET_t get_result()
    {
        if (GetParam().operation == ca_expr_ops::NOT || GetParam().kind == SET_t_enum::C_TYPE)
        {
            ca_function_unary_operator op(nullptr, GetParam().operation, GetParam().kind, range());

            return op.operation(GetParam().params[0], eval_ctx);
        }
        else
        {
            ca_expr_ptr left;
            if (GetParam().params[0].type == SET_t_enum::A_TYPE)
                left = std::make_unique<ca_constant>(1, range());
            else
                left =
                    std::make_unique<ca_string>(semantics::concat_chain {}, nullptr, ca_string::substring_t(), range());

            ca_function_binary_operator op(std::move(left), nullptr, GetParam().operation, GetParam().kind, range());

            return op.operation(GetParam().params[0], GetParam().params[1], eval_ctx);
        }
    }
};

INSTANTIATE_TEST_SUITE_P(op_parameters_suite,
    ca_op,
    ::testing::Values(op_test_param { ca_expr_ops::SLA, SET_t_enum::A_TYPE, { 2, 2 }, 8, "SLA_positive" },
        op_test_param { ca_expr_ops::SLA, SET_t_enum::A_TYPE, { -2, 2 }, -8, "SLA_negative_zero" },
        op_test_param { ca_expr_ops::SLA, SET_t_enum::A_TYPE, { -2147483647, 1 }, -2147483646, "SLA_negative_one" },
        op_test_param { ca_expr_ops::SLA, SET_t_enum::A_TYPE, { 2, -2147483646 }, 8, "SLA_trunc" },

        op_test_param { ca_expr_ops::SLL, SET_t_enum::A_TYPE, { 10, 2 }, 40, "SLL_positive" },
        op_test_param { ca_expr_ops::SLL, SET_t_enum::A_TYPE, { -2, 2 }, -8, "SLL_negative_zero" },
        op_test_param { ca_expr_ops::SLL, SET_t_enum::A_TYPE, { -2147483647, 1 }, 2, "SLL_negative_one" },
        op_test_param { ca_expr_ops::SLL, SET_t_enum::A_TYPE, { 2, -2147483646 }, 8, "SLL_trunc" },

        op_test_param { ca_expr_ops::SRA, SET_t_enum::A_TYPE, { 10, 2 }, 2, "SRA_positive" },
        op_test_param { ca_expr_ops::SRA, SET_t_enum::A_TYPE, { -344, 40 }, -1, "SRA_negative_all" },
        op_test_param { ca_expr_ops::SRA, SET_t_enum::A_TYPE, { -2147483390, 2 }, -536870848, "SRA_negative_some" },
        op_test_param { ca_expr_ops::SRA, SET_t_enum::A_TYPE, { 10, -2147483646 }, 2, "SRA_trunc" },

        op_test_param { ca_expr_ops::SRL, SET_t_enum::A_TYPE, { 10, 2 }, 2, "SRL_positive" },
        op_test_param { ca_expr_ops::SRL, SET_t_enum::A_TYPE, { -344, 40 }, 0, "SRL_negative_all" },
        op_test_param { ca_expr_ops::SRL, SET_t_enum::A_TYPE, { -2147483390, 2 }, 536870976, "SRL_negative_some" },
        op_test_param { ca_expr_ops::SRL, SET_t_enum::A_TYPE, { 10, -2147483646 }, 2, "SRL_trunc" },

        op_test_param { ca_expr_ops::FIND, SET_t_enum::A_TYPE, { "abcdef", "cde" }, 3, "FIND" },

        op_test_param { ca_expr_ops::INDEX, SET_t_enum::A_TYPE, { "abc", "b" }, 2, "INDEX" },

        op_test_param { ca_expr_ops::AND, SET_t_enum::A_TYPE, { 10, 2 }, 2, "ANDA" },
        op_test_param { ca_expr_ops::OR, SET_t_enum::A_TYPE, { 10, 2 }, 10, "ORA" },
        op_test_param { ca_expr_ops::XOR, SET_t_enum::A_TYPE, { 10, 2 }, 8, "XORA" },
        op_test_param { ca_expr_ops::NOT, SET_t_enum::A_TYPE, { 10 }, -11, "NOTA" },

        op_test_param { ca_expr_ops::AND, SET_t_enum::B_TYPE, { false, true }, false, "ANDB" },
        op_test_param { ca_expr_ops::AND_NOT, SET_t_enum::B_TYPE, { true, false }, true, "AND_NOT" },
        op_test_param { ca_expr_ops::OR, SET_t_enum::B_TYPE, { false, true }, true, "ORB" },
        op_test_param { ca_expr_ops::OR_NOT, SET_t_enum::B_TYPE, { false, true }, false, "OR_NOT" },
        op_test_param { ca_expr_ops::XOR, SET_t_enum::B_TYPE, { false, true }, true, "XORB" },
        op_test_param { ca_expr_ops::XOR_NOT, SET_t_enum::B_TYPE, { true, false }, false, "XOR_NOT" },
        op_test_param { ca_expr_ops::NOT, SET_t_enum::B_TYPE, { false }, true, "NOTB" },

        op_test_param { ca_expr_ops::EQ, SET_t_enum::B_TYPE, { 1, 0 }, false, "EQA" },
        op_test_param { ca_expr_ops::NE, SET_t_enum::B_TYPE, { 1, 0 }, true, "NEA" },
        op_test_param { ca_expr_ops::LE, SET_t_enum::B_TYPE, { 1, 0 }, false, "LEA" },
        op_test_param { ca_expr_ops::LT, SET_t_enum::B_TYPE, { 1, 0 }, false, "LTA" },
        op_test_param { ca_expr_ops::GE, SET_t_enum::B_TYPE, { 1, 0 }, true, "GEA" },
        op_test_param { ca_expr_ops::GT, SET_t_enum::B_TYPE, { 1, 0 }, true, "GTA" },

        op_test_param { ca_expr_ops::EQ, SET_t_enum::B_TYPE, { "ab", "c" }, false, "EQB" },
        op_test_param { ca_expr_ops::EQ, SET_t_enum::B_TYPE, { "ab", "ac" }, false, "EQB_same_length" },
        op_test_param { ca_expr_ops::NE, SET_t_enum::B_TYPE, { "ab", "c" }, true, "NEB" },
        op_test_param { ca_expr_ops::LE, SET_t_enum::B_TYPE, { "ab", "c" }, false, "LEB" },
        op_test_param { ca_expr_ops::LE, SET_t_enum::B_TYPE, { "ab", "ac" }, true, "LEB_same_length" },
        op_test_param { ca_expr_ops::LT, SET_t_enum::B_TYPE, { "ab", "c" }, false, "LTB" },
        op_test_param { ca_expr_ops::GE, SET_t_enum::B_TYPE, { "ab", "c" }, true, "GEB" },
        op_test_param { ca_expr_ops::GT, SET_t_enum::B_TYPE, { "ab", "c" }, true, "GTB" },

        op_test_param { ca_expr_ops::BYTE, SET_t_enum::C_TYPE, { 97 }, "/", "BYTE" },

        op_test_param { ca_expr_ops::DOUBLE, SET_t_enum::C_TYPE, { "a&&''&b" }, "a&&&&''''&&b", "DOUBLE" },

        op_test_param { ca_expr_ops::LOWER, SET_t_enum::C_TYPE, { "aBcDefG321&^%$" }, "abcdefg321&^%$", "LOWER" },

        op_test_param { ca_expr_ops::SIGNED, SET_t_enum::C_TYPE, { 0 }, "0", "SIGNED" },

        op_test_param { ca_expr_ops::UPPER, SET_t_enum::C_TYPE, { "aBcDefG321&^%$" }, "ABCDEFG321&^%$", "UPPER" }),

    stringer());

TEST_P(ca_op, test)
{
    auto result = get_result();

    ASSERT_EQ(result.type, GetParam().true_result.type);

    if (result.type == SET_t_enum::A_TYPE)
        EXPECT_EQ(result.access_a(), GetParam().true_result.access_a());
    else if (result.type == SET_t_enum::B_TYPE)
        EXPECT_EQ(result.access_b(), GetParam().true_result.access_b());
    else if (result.type == SET_t_enum::C_TYPE)
        EXPECT_EQ(result.access_c(), GetParam().true_result.access_c());
    else
        FAIL();
}
