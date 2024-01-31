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
#include "expressions/conditional_assembly/terms/ca_function.h"
#include "expressions/evaluation_context.h"
#include "library_info_transitional.h"

using namespace std::string_literals;
using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::expressions;

struct func_test_param
{
    ca_expr_funcs function;
    std::vector<context::SET_t> params;
    context::SET_t true_result;
    bool erroneous;
    std::string name;
};

std::ostream& operator<<(std::ostream& os, const func_test_param& param)
{
    for (auto& p : param.params)
    {
        if (p.type() == context::SET_t_enum::A_TYPE)
            os << p.access_a();
        else if (p.type() == context::SET_t_enum::B_TYPE)
            os << p.access_b();
        else if (p.type() == context::SET_t_enum::C_TYPE)
            os << p.access_c();
        os << " ";
    }
    return os;
}

struct stringer
{
    std::string operator()(::testing::TestParamInfo<func_test_param> p) { return p.param.name; }
};

class set_expr final : public ca_expression
{
public:
    context::SET_t value;

    set_expr(context::SET_t value)
        : ca_expression(context::SET_t_enum::A_TYPE, range())
        , value(std::move(value))
    {}

    bool get_undefined_attributed_symbols(std::vector<context::id_index>&, const evaluation_context&) const override
    {
        return false;
    }

    void resolve_expression_tree(ca_expression_ctx, diagnostic_op_consumer&) override {}

    bool is_character_expression(character_expression_purpose) const override { return false; }

    void apply(ca_expr_visitor&) const override {}

    context::SET_t evaluate(const evaluation_context&) const override { return value; }
};

class ca_func : public ::testing::TestWithParam<func_test_param>
{
protected:
    context::hlasm_context ctx;
    diagnostic_op_consumer_container diags;
    evaluation_context eval_ctx { ctx, library_info_transitional::empty, diags };

    context::SET_t get_result()
    {
        std::vector<ca_expr_ptr> params;
        for (const auto& param : GetParam().params)
            params.push_back(std::make_unique<set_expr>(param));


        ca_function f(context::id_index(), GetParam().function, std::move(params), nullptr, range());

        return f.evaluate(eval_ctx);
    }
};

INSTANTIATE_TEST_SUITE_P(func_parameters_suite,
    ca_func,
    ::testing::Values(func_test_param { ca_expr_funcs::B2A, { "" }, 0, false, "B2A_empty" },
        func_test_param { ca_expr_funcs::B2A, { "0000000101" }, 5, false, "B2A_padding" },
        func_test_param { ca_expr_funcs::B2A, { "11111111111111111111111111111110" }, -2, false, "B2A_whole" },
        func_test_param { ca_expr_funcs::B2A, { "1021" }, {}, true, "B2A_bad_char" },
        func_test_param { ca_expr_funcs::B2A, { "111111111111111111111111111111101" }, {}, true, "B2A_exceeds" },

        func_test_param { ca_expr_funcs::C2A, { "" }, 0, false, "C2A_empty" },
        func_test_param { ca_expr_funcs::C2A, { "+" }, 78, false, "C2A_padding" },
        func_test_param { ca_expr_funcs::C2A, { "0000" }, -252645136, false, "C2A_whole" },
        func_test_param { ca_expr_funcs::C2A, { "00001" }, {}, true, "C2A_exceeds" },
        func_test_param {
            ca_expr_funcs::C2A,
            { (const char*)u8"\U0001F600\U0001F600\U0001F600\U0001F600" },
            0x3F3F3F3F,
            false,
            "C2A_multibyte",
        },

        func_test_param { ca_expr_funcs::D2A, { "" }, 0, false, "D2A_empty" },
        func_test_param { ca_expr_funcs::D2A, { "000" }, 0, false, "D2A_zeros" },
        func_test_param { ca_expr_funcs::D2A, { "10" }, 10, false, "D2A_basic" },
        func_test_param { ca_expr_funcs::D2A, { "+12" }, 12, false, "D2A_leading_plus" },
        func_test_param { ca_expr_funcs::D2A, { "-45" }, -45, false, "D2A_leading_minus" },
        func_test_param { ca_expr_funcs::D2A, { "0f1" }, {}, true, "D2A_bad_char" },
        func_test_param { ca_expr_funcs::D2A, { "0000000000001" }, {}, true, "D2A_too_large" },
        func_test_param { ca_expr_funcs::D2A, { "99999999999" }, {}, true, "D2A_overflow" },
        func_test_param { ca_expr_funcs::D2A, { "+" }, {}, true, "D2A_only_sign" },
        func_test_param { ca_expr_funcs::D2A, { "--1" }, {}, true, "D2A_multiple_signs" },

        func_test_param { ca_expr_funcs::DCLEN, { "" }, 0, false, "DCLEN_empty" },
        func_test_param { ca_expr_funcs::DCLEN, { "'" }, 1, false, "DCLEN_single_apo" },
        func_test_param { ca_expr_funcs::DCLEN, { "''" }, 1, false, "DCLEN_double_apo" },
        func_test_param { ca_expr_funcs::DCLEN, { "&&" }, 1, false, "DCLEN_single_amp" },
        func_test_param { ca_expr_funcs::DCLEN, { "a''b" }, 3, false, "DCLEN_apo_char" },
        func_test_param { ca_expr_funcs::DCLEN, { "a''b&&c" }, 5, false, "DCLEN_apo_amp_char" },

        func_test_param { ca_expr_funcs::FIND, { "abcdef", "cde" }, 3, false, "FIND_basic1" },
        func_test_param { ca_expr_funcs::FIND, { "abcdef", "gde" }, 4, false, "FIND_basic2" },
        func_test_param { ca_expr_funcs::FIND, { "", "" }, 0, false, "FIND_empty" },
        func_test_param { ca_expr_funcs::FIND, { "", "a" }, 0, false, "FIND_l_empty" },
        func_test_param { ca_expr_funcs::FIND, { "a", "" }, 0, false, "FIND_r_empty" },

        func_test_param { ca_expr_funcs::INDEX, { "abc", "b" }, 2, false, "INDEX_found" },
        func_test_param { ca_expr_funcs::INDEX, { "abc", "ca" }, 0, false, "INDEX_not_found" },

        func_test_param { ca_expr_funcs::ISBIN, { "10101" }, true, false, "ISBIN_valid" },
        func_test_param {
            ca_expr_funcs::ISBIN, { "101010101010101010101010101010101" }, false, false, "ISBIN_exceeds" },
        func_test_param { ca_expr_funcs::ISBIN, { "1210" }, false, false, "ISBIN_bad_char" },
        func_test_param { ca_expr_funcs::ISBIN, { "" }, {}, true, "ISBIN_empty" },

        func_test_param { ca_expr_funcs::ISDEC, { "12345678" }, true, false, "ISDEC_valid" },
        func_test_param { ca_expr_funcs::ISDEC, { "+25" }, false, false, "ISDEC_bad_char" },
        func_test_param { ca_expr_funcs::ISDEC, { "2147483648" }, false, false, "ISDEC_overflow" },
        func_test_param { ca_expr_funcs::ISDEC, { "00000000005" }, false, false, "ISDEC_exceeds" },
        func_test_param { ca_expr_funcs::ISDEC, { "" }, {}, true, "ISDEC_empty" },

        func_test_param { ca_expr_funcs::ISHEX, { "ab34CD9F" }, true, false, "ISHEX_valid" },
        func_test_param { ca_expr_funcs::ISHEX, { "abcdEFGH" }, false, false, "ISHEX_bad_char" },
        func_test_param { ca_expr_funcs::ISHEX, { "123456789" }, false, false, "ISHEX_exceeds" },
        func_test_param { ca_expr_funcs::ISHEX, { "" }, {}, true, "ISHEX_empty" },

        func_test_param { ca_expr_funcs::ISSYM, { "Abcd_1234" }, true, false, "ISSYM_valid1" },
        func_test_param { ca_expr_funcs::ISSYM, { "_abcd1234" }, true, false, "ISSYM_valid2" },
        func_test_param { ca_expr_funcs::ISSYM, { "##@$_" }, true, false, "ISSYM_valid3" },
        func_test_param { ca_expr_funcs::ISSYM, { "1234_Abcd" }, false, false, "ISSYM_invalid" },
        func_test_param { ca_expr_funcs::ISSYM, { "" }, {}, true, "ISSYM_empty" },

        func_test_param { ca_expr_funcs::X2A, { "" }, 0, false, "X2A_empty" },
        func_test_param { ca_expr_funcs::X2A, { "C1" }, 193, false, "X2A_padding" },
        func_test_param { ca_expr_funcs::X2A, { "FFFFfFF0" }, -16, false, "X2A_whole" },
        func_test_param { ca_expr_funcs::X2A, { "FfFFFFF00" }, {}, true, "X2A_exceeds" },
        func_test_param { ca_expr_funcs::X2A, { "FFFfG0" }, {}, true, "X2A_bad_char" },

        func_test_param { ca_expr_funcs::A2B, { 0 }, "00000000000000000000000000000000", false, "A2B_zero" },
        func_test_param { ca_expr_funcs::A2B, { 1022 }, "00000000000000000000001111111110", false, "A2B_positive" },
        func_test_param { ca_expr_funcs::A2B, { -7 }, "11111111111111111111111111111001", false, "A2B_negative" },

        func_test_param { ca_expr_funcs::A2C, { 0 }, "\0\0\0\0"s, false, "A2C_zero" },
        func_test_param { ca_expr_funcs::A2C, { 241 }, "\0\0\0"s + "1", false, "A2C_positive" },
        func_test_param { ca_expr_funcs::A2C, { -252645136 }, "0000", false, "A2C_negative" },

        func_test_param { ca_expr_funcs::A2D, { 0 }, "+0", false, "A2D_zero" },
        func_test_param { ca_expr_funcs::A2D, { 241 }, "+241", false, "A2D_positive" },
        func_test_param { ca_expr_funcs::A2D, { -3 }, "-3", false, "A2D_negative" },

        func_test_param { ca_expr_funcs::A2X, { 0 }, "00000000", false, "A2X_zero" },
        func_test_param { ca_expr_funcs::A2X, { 1022 }, "000003FE", false, "A2X_positive" },
        func_test_param { ca_expr_funcs::A2X, { -7 }, "FFFFFFF9", false, "A2X_negative" },

        func_test_param { ca_expr_funcs::B2C, { "101110011110001" }, "*1", false, "B2C_valid" },
        func_test_param { ca_expr_funcs::B2C, { "0" }, "\0"s, false, "B2C_padding" },
        func_test_param { ca_expr_funcs::B2C, { "00010010001" }, "\0j"s, false, "B2C_padding_zero" },
        func_test_param { ca_expr_funcs::B2C, { "" }, "", false, "B2C_empty" },
        func_test_param { ca_expr_funcs::B2C, { "12" }, {}, true, "B2C_bad_char" },

        func_test_param { ca_expr_funcs::B2D, { "11110001" }, "+241", false, "B2D_positive" },
        func_test_param { ca_expr_funcs::B2D, { "11111111111111111111111111110001" }, "-15", false, "B2D_negative" },
        func_test_param { ca_expr_funcs::B2D, { "" }, "+0", false, "B2D_empty" },
        func_test_param { ca_expr_funcs::B2D, { "111111111111111111111111111100011" }, {}, true, "B2D_exceeds" },
        func_test_param { ca_expr_funcs::B2D, { "12" }, {}, true, "B2D_bad_char" },

        func_test_param { ca_expr_funcs::B2X, { "0000010010001" }, "0091", false, "B2X_padding" },
        func_test_param { ca_expr_funcs::B2X, { "" }, "", false, "B2X_empty" },
        func_test_param { ca_expr_funcs::B2X, { "12" }, {}, true, "B2X_bad_char" },

        func_test_param { ca_expr_funcs::BYTE, { 0 }, "\0"s, false, "BYTE_zero" },
        func_test_param { ca_expr_funcs::BYTE, { 97 }, "/", false, "BYTE_valid" },
        func_test_param { ca_expr_funcs::BYTE, { 2000 }, {}, true, "BYTE_exceeds" },

        func_test_param { ca_expr_funcs::C2B, { "" }, "", false, "C2B_empty" },
        func_test_param { ca_expr_funcs::C2B, { "\0"s }, "00000000", false, "C2B_zero" },
        func_test_param { ca_expr_funcs::C2B, { "1234" }, "11110001111100101111001111110100", false, "C2B_valid" },
        func_test_param { ca_expr_funcs::C2B, { std::string(4000, '1') }, {}, true, "C2B_exceeds" },

        func_test_param { ca_expr_funcs::C2D, { "" }, "+0", false, "C2D_empty" },
        func_test_param { ca_expr_funcs::C2D, { "\0"s }, "+0", false, "C2D_zero" },
        func_test_param { ca_expr_funcs::C2D, { "\0j"s }, "+145", false, "C2D_positive" },
        func_test_param { ca_expr_funcs::C2D, { "0000" }, "-252645136", false, "C2D_negative" },
        func_test_param { ca_expr_funcs::C2D, { std::string(4000, '1') }, {}, true, "C2D_exceeds" },

        func_test_param { ca_expr_funcs::C2X, { "" }, "", false, "C2X_empty" },
        func_test_param { ca_expr_funcs::C2X, { "\0"s }, "00", false, "C2X_zero" },
        func_test_param { ca_expr_funcs::C2X, { "1234567R" }, "F1F2F3F4F5F6F7D9", false, "C2X_valid" },
        func_test_param { ca_expr_funcs::C2X, { std::string(4000, '1') }, {}, true, "C2X_exceeds" },

        func_test_param { ca_expr_funcs::D2B, { "" }, "", false, "D2B_empty" },
        func_test_param { ca_expr_funcs::D2B, { "000" }, "00000000000000000000000000000000", false, "D2B_zeros" },
        func_test_param { ca_expr_funcs::D2B, { "1022" }, "00000000000000000000001111111110", false, "D2B_basic" },
        func_test_param { ca_expr_funcs::D2B, { "+5" }, "00000000000000000000000000000101", false, "D2B_leading_plus" },
        func_test_param {
            ca_expr_funcs::D2B, { "-7" }, "11111111111111111111111111111001", false, "D2B_leading_minus" },
        func_test_param { ca_expr_funcs::D2B, { "0f1" }, {}, true, "D2B_bad_char" },
        func_test_param { ca_expr_funcs::D2B, { "0000000000001" }, {}, true, "D2B_too_large" },
        func_test_param { ca_expr_funcs::D2B, { "99999999999" }, {}, true, "D2B_overflow" },
        func_test_param { ca_expr_funcs::D2B, { "+" }, {}, true, "D2B_only_sign" },
        func_test_param { ca_expr_funcs::D2B, { "--1" }, {}, true, "D2B_multiple_signs" },

        func_test_param { ca_expr_funcs::D2C, { "" }, {}, true, "D2C_empty" },
        func_test_param { ca_expr_funcs::D2C, { "000" }, "\0\0\0\0"s, false, "D2C_zeros" },
        func_test_param { ca_expr_funcs::D2C, { "23793" }, "\0\0*1"s, false, "D2C_basic" },
        func_test_param { ca_expr_funcs::D2C, { "0f1" }, {}, true, "D2C_bad_char" },
        func_test_param { ca_expr_funcs::D2C, { "0000000000001" }, {}, true, "D2C_too_large" },
        func_test_param { ca_expr_funcs::D2C, { "99999999999" }, {}, true, "D2C_overflow" },
        func_test_param { ca_expr_funcs::D2C, { "+" }, {}, true, "D2C_only_sign" },
        func_test_param { ca_expr_funcs::D2C, { "--1" }, {}, true, "D2C_multiple_signs" },

        func_test_param { ca_expr_funcs::D2X, { "" }, {}, true, "D2X_empty" },
        func_test_param { ca_expr_funcs::D2X, { "000" }, "00000000", false, "D2X_zeros" },
        func_test_param { ca_expr_funcs::D2X, { "1022" }, "000003FE", false, "D2X_basic" },
        func_test_param { ca_expr_funcs::D2X, { "+5" }, "00000005", false, "D2X_leading_plus" },
        func_test_param { ca_expr_funcs::D2X, { "-7" }, "FFFFFFF9", false, "D2X_leading_minus" },
        func_test_param { ca_expr_funcs::D2X, { "0f1" }, {}, true, "D2X_bad_char" },
        func_test_param { ca_expr_funcs::D2X, { "0000000000001" }, {}, true, "D2X_too_large" },
        func_test_param { ca_expr_funcs::D2X, { "99999999999" }, {}, true, "D2X_overflow" },
        func_test_param { ca_expr_funcs::D2X, { "+" }, {}, true, "D2X_only_sign" },
        func_test_param { ca_expr_funcs::D2X, { "--1" }, {}, true, "D2X_multiple_signs" },

        func_test_param { ca_expr_funcs::DCVAL, { "" }, "", false, "DCVAL_empty" },
        func_test_param { ca_expr_funcs::DCVAL, { "'" }, "'", false, "DCVAL_single_apo" },
        func_test_param { ca_expr_funcs::DCVAL, { "''" }, "'", false, "DCVAL_double_apo" },
        func_test_param { ca_expr_funcs::DCVAL, { "&&" }, "&", false, "DCVAL_single_amp" },
        func_test_param { ca_expr_funcs::DCVAL, { "a''b" }, "a'b", false, "DCVAL_apo_char" },
        func_test_param { ca_expr_funcs::DCVAL, { "a''b&&c" }, "a'b&c", false, "DCVAL_apo_amp_char" },

        func_test_param { ca_expr_funcs::DEQUOTE, { "adam" }, "adam", false, "DEQUOTE_char" },
        func_test_param { ca_expr_funcs::DEQUOTE, { "" }, "", false, "DEQUOTE_empty" },
        func_test_param { ca_expr_funcs::DEQUOTE, { "''abc'''" }, "'abc''", false, "DEQUOTE_apo_char" },
        func_test_param { ca_expr_funcs::DEQUOTE, { "''" }, "", false, "DEQUOTE_apo" },
        func_test_param { ca_expr_funcs::DEQUOTE, { "'a" }, "a", false, "DEQUOTE_apo_char_one_side" },
        func_test_param { ca_expr_funcs::DEQUOTE, { "'" }, "", false, "DEQUOTE_apo_one_side" },

        func_test_param { ca_expr_funcs::DOUBLE, { "a&&''&b" }, "a&&&&''''&&b", false, "DOUBLE_simple" },
        func_test_param { ca_expr_funcs::DOUBLE, { std::string(4000, '\'') }, {}, true, "DOUBLE_exceeds" },

        func_test_param { ca_expr_funcs::LOWER, { "aBcDefG321&^%$" }, "abcdefg321&^%$", false, "LOWER_simple" },

        func_test_param { ca_expr_funcs::SIGNED, { 0 }, "0", false, "SIGNED_zero" },
        func_test_param { ca_expr_funcs::SIGNED, { 241 }, "241", false, "SIGNED_positive" },
        func_test_param { ca_expr_funcs::SIGNED, { -3 }, "-3", false, "SIGNED_negative" },

        func_test_param { ca_expr_funcs::UPPER, { "aBcDefG321&^%$" }, "ABCDEFG321&^%$", false, "UPPER_simple" },

        func_test_param { ca_expr_funcs::X2B, { "" }, "", false, "X2B_empty" },
        func_test_param { ca_expr_funcs::X2B, { "00" }, "00000000", false, "X2B_zeros" },
        func_test_param { ca_expr_funcs::X2B, { "f3" }, "11110011", false, "X2B_basic" },
        func_test_param { ca_expr_funcs::X2B, { "0g1" }, {}, true, "X2B_bad_char" },
        func_test_param { ca_expr_funcs::X2B, { std::string(4000, '1') }, {}, true, "X2B_exceeds" },

        func_test_param { ca_expr_funcs::X2C, { "" }, "", false, "X2C_empty" },
        func_test_param { ca_expr_funcs::X2C, { "0" }, "\0"s, false, "X2C_zeros" },
        func_test_param { ca_expr_funcs::X2C, { "F1f2F3F4F5" }, "12345", false, "X2C_basic" },
        func_test_param { ca_expr_funcs::X2C, { "000F1" }, "\0\0"s + "1", false, "X2C_basic2" },
        func_test_param { ca_expr_funcs::X2C, { "0g1" }, {}, true, "X2C_bad_char" },

        func_test_param { ca_expr_funcs::X2D, { "" }, "+0", false, "X2D_empty" },
        func_test_param { ca_expr_funcs::X2D, { "00" }, "+0", false, "X2D_zeros" },
        func_test_param { ca_expr_funcs::X2D, { "7FFfFFFF" }, "+2147483647", false, "X2D_positive" },
        func_test_param { ca_expr_funcs::X2D, { "FFFFfFF1" }, "-15", false, "X2D_negative" },
        func_test_param { ca_expr_funcs::X2D, { "0g1" }, {}, true, "X2D_bad_char" },
        func_test_param { ca_expr_funcs::X2D, { "000000f000" }, {}, true, "X2D_exceeds" }),
    stringer());

TEST_P(ca_func, test)
{
    auto result = get_result();

    EXPECT_EQ(diags.diags.size(), GetParam().erroneous);

    if (!GetParam().erroneous)
    {
        EXPECT_EQ(result, GetParam().true_result);
    }
}
