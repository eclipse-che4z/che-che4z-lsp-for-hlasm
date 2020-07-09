/*
 * Copyright (c){} 2019 Broadcom.
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

#include "expressions/conditional_assembly/terms/ca_function.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::expressions;

struct test_param
{
    ca_expr_funcs function;
    std::vector<context::SET_t> params;
    context::SET_t true_result;
    bool erroneous;
    std::string name;
};

std::ostream& operator<<(std::ostream& os, const test_param& param)
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

bool compare(const context::SET_t& lhs, const context::SET_t& rhs)
{
    if (lhs.type != rhs.type)
        return false;

    if (lhs.type == context::SET_t_enum::A_TYPE)
        return lhs.access_a() == rhs.access_a();
    else if (lhs.type == context::SET_t_enum::B_TYPE)
        return lhs.access_b() == rhs.access_b();
    else if (lhs.type == context::SET_t_enum::C_TYPE)
        return lhs.access_c() == rhs.access_c();
    return false;
}


struct stringer
{
    std::string operator()(::testing::TestParamInfo<test_param> p) { return p.param.name; }
};

class ca_func : public ::testing::TestWithParam<test_param>
{
protected:
    ranged_diagnostic_collector add_diags;

    context::SET_t get_result()
    {
        switch (GetParam().function)
        {
            case ca_expr_funcs::B2A:
                return ca_function::B2A(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::C2A:
                return ca_function::C2A(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::D2A:
                return ca_function::D2A(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::DCLEN:
                return ca_function::DCLEN(GetParam().params.front().access_c());
            case ca_expr_funcs::FIND:
                return ca_function::FIND(GetParam().params[0].access_c(), GetParam().params[1].access_c());
            case ca_expr_funcs::INDEX:
                return ca_function::INDEX(GetParam().params[0].access_c(), GetParam().params[1].access_c());
            case ca_expr_funcs::ISBIN:
                return ca_function::ISBIN(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::ISDEC:
                return ca_function::ISDEC(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::ISHEX:
                return ca_function::ISHEX(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::ISSYM:
                return ca_function::ISSYM(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::X2A:
                return ca_function::X2A(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::A2B:
                return ca_function::A2B(GetParam().params.front().access_a());
            case ca_expr_funcs::A2C:
                return ca_function::A2C(GetParam().params.front().access_a());
            case ca_expr_funcs::A2D:
                return ca_function::A2D(GetParam().params.front().access_a());
            case ca_expr_funcs::A2X:
                return ca_function::A2X(GetParam().params.front().access_a());
            case ca_expr_funcs::B2C:
                return ca_function::B2C(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::B2D:
                return ca_function::B2D(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::B2X:
                return ca_function::B2X(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::BYTE:
                return ca_function::BYTE(GetParam().params.front().access_a(), add_diags);
            case ca_expr_funcs::C2B:
                return ca_function::C2B(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::C2D:
                return ca_function::C2D(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::C2X:
                return ca_function::C2X(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::D2B:
                return ca_function::D2B(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::D2C:
                return ca_function::D2C(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::D2X:
                return ca_function::D2X(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::DCVAL:
                return ca_function::DCVAL(GetParam().params.front().access_c());
            case ca_expr_funcs::DEQUOTE:
                return ca_function::DEQUOTE(GetParam().params.front().access_c());
            case ca_expr_funcs::DOUBLE:
                return ca_function::DOUBLE(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::LOWER:
                return ca_function::LOWER(GetParam().params.front().access_c());
            case ca_expr_funcs::SIGNED:
                return ca_function::SIGNED(GetParam().params.front().access_a());
            case ca_expr_funcs::UPPER:
                return ca_function::UPPER(GetParam().params.front().access_c());
            case ca_expr_funcs::X2B:
                return ca_function::X2B(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::X2C:
                return ca_function::X2C(GetParam().params.front().access_c(), add_diags);
            case ca_expr_funcs::X2D:
                return ca_function::X2D(GetParam().params.front().access_c(), add_diags);
            default:
                return context::SET_t();
        }
    }
};

std::string big_string(char c = '1')
{
    std::string s;
    s.reserve(1000);
    for (size_t i = 0; i < 4000; i++)
        s.push_back(c);
    return s;
}

INSTANTIATE_TEST_SUITE_P(parameters_suite,
    ca_func,
    ::testing::Values(test_param { ca_expr_funcs::B2A, { "" }, 0, false, "B2A_empty" },
        test_param { ca_expr_funcs::B2A, { "0000000101" }, 5, false, "B2A_padding" },
        test_param { ca_expr_funcs::B2A, { "11111111111111111111111111111110" }, -2, false, "B2A_whole" },
        test_param { ca_expr_funcs::B2A, { "1021" }, {}, true, "B2A_bad_char" },
        test_param { ca_expr_funcs::B2A, { "111111111111111111111111111111101" }, {}, true, "B2A_exceeds" },

        test_param { ca_expr_funcs::C2A, { "" }, 0, false, "C2A_empty" },
        test_param { ca_expr_funcs::C2A, { "+" }, 78, false, "C2A_padding" },
        test_param { ca_expr_funcs::C2A, { "0000" }, -252645136, false, "C2A_whole" },
        test_param { ca_expr_funcs::C2A, { "00001" }, {}, true, "C2A_exceeds" },

        test_param { ca_expr_funcs::D2A, { "" }, 0, false, "D2A_empty" },
        test_param { ca_expr_funcs::D2A, { "000" }, 0, false, "D2A_zeros" },
        test_param { ca_expr_funcs::D2A, { "10" }, 10, false, "D2A_basic" },
        test_param { ca_expr_funcs::D2A, { "+12" }, 12, false, "D2A_leading_plus" },
        test_param { ca_expr_funcs::D2A, { "-45" }, -45, false, "D2A_leading_minus" },
        test_param { ca_expr_funcs::D2A, { "0f1" }, {}, true, "D2A_bad_char" },
        test_param { ca_expr_funcs::D2A, { "0000000000001" }, {}, true, "D2A_too_large" },
        test_param { ca_expr_funcs::D2A, { "99999999999" }, {}, true, "D2A_overflow" },
        test_param { ca_expr_funcs::D2A, { "+" }, {}, true, "D2A_only_sign" },
        test_param { ca_expr_funcs::D2A, { "--1" }, {}, true, "D2A_multiple_signs" },

        test_param { ca_expr_funcs::DCLEN, { "" }, 0, false, "DCLEN_empty" },
        test_param { ca_expr_funcs::DCLEN, { "'" }, 1, false, "DCLEN_single_apo" },
        test_param { ca_expr_funcs::DCLEN, { "''" }, 1, false, "DCLEN_double_apo" },
        test_param { ca_expr_funcs::DCLEN, { "&&" }, 1, false, "DCLEN_single_amp" },
        test_param { ca_expr_funcs::DCLEN, { "a''b" }, 3, false, "DCLEN_apo_char" },
        test_param { ca_expr_funcs::DCLEN, { "a''b&&c" }, 5, false, "DCLEN_apo_amp_char" },

        test_param { ca_expr_funcs::FIND, { "abcdef", "cde" }, 1, false, "FIND_basic1" },
        test_param { ca_expr_funcs::FIND, { "abcdef", "gde" }, 4, false, "FIND_basic2" },
        test_param { ca_expr_funcs::FIND, { "", "" }, 0, false, "FIND_empty" },
        test_param { ca_expr_funcs::FIND, { "", "a" }, 0, false, "FIND_l_empty" },
        test_param { ca_expr_funcs::FIND, { "a", "" }, 0, false, "FIND_r_empty" },

        test_param { ca_expr_funcs::INDEX, { "abc", "b" }, 2, false, "INDEX_found" },
        test_param { ca_expr_funcs::INDEX, { "abc", "ca" }, 0, false, "INDEX_not_found" },

        test_param { ca_expr_funcs::ISBIN, { "10101" }, 1, false, "ISBIN_valid" },
        test_param { ca_expr_funcs::ISBIN, { "101010101010101010101010101010101" }, 0, false, "ISBIN_exceeds" },
        test_param { ca_expr_funcs::ISBIN, { "1210" }, 0, false, "ISBIN_bad_char" },
        test_param { ca_expr_funcs::ISBIN, { "" }, {}, true, "ISBIN_empty" },

        test_param { ca_expr_funcs::ISDEC, { "12345678" }, 1, false, "ISDEC_valid" },
        test_param { ca_expr_funcs::ISDEC, { "+25" }, 0, false, "ISDEC_bad_char" },
        test_param { ca_expr_funcs::ISDEC, { "2147483648" }, 0, false, "ISDEC_overflow" },
        test_param { ca_expr_funcs::ISDEC, { "00000000005" }, 0, false, "ISDEC_exceeds" },
        test_param { ca_expr_funcs::ISDEC, { "" }, {}, true, "ISDEC_empty" },

        test_param { ca_expr_funcs::ISHEX, { "ab34CD9F" }, 1, false, "ISHEX_valid" },
        test_param { ca_expr_funcs::ISHEX, { "abcdEFGH" }, 0, false, "ISHEX_bad_char" },
        test_param { ca_expr_funcs::ISHEX, { "123456789" }, 0, false, "ISHEX_exceeds" },
        test_param { ca_expr_funcs::ISHEX, { "" }, {}, true, "ISHEX_empty" },

        test_param { ca_expr_funcs::ISSYM, { "Abcd_1234" }, 1, false, "ISSYM_valid1" },
        test_param { ca_expr_funcs::ISSYM, { "_abcd1234" }, 1, false, "ISSYM_valid2" },
        test_param { ca_expr_funcs::ISSYM, { "##@$_" }, 1, false, "ISSYM_valid3" },
        test_param { ca_expr_funcs::ISSYM, { "1234_Abcd" }, 0, false, "ISSYM_invalid" },
        test_param { ca_expr_funcs::ISSYM, { "" }, {}, true, "ISSYM_empty" },

        test_param { ca_expr_funcs::X2A, { "" }, 0, false, "X2A_empty" },
        test_param { ca_expr_funcs::X2A, { "C1" }, 193, false, "X2A_padding" },
        test_param { ca_expr_funcs::X2A, { "FFFFfFF0" }, -16, false, "X2A_whole" },
        test_param { ca_expr_funcs::X2A, { "FfFFFFF00" }, {}, true, "X2A_exceeds" },
        test_param { ca_expr_funcs::X2A, { "FFFfG0" }, {}, true, "X2A_bad_char" },

        test_param { ca_expr_funcs::A2B, { 0 }, "00000000000000000000000000000000", false, "A2B_zero" },
        test_param { ca_expr_funcs::A2B, { 1022 }, "00000000000000000000001111111110", false, "A2B_positive" },
        test_param { ca_expr_funcs::A2B, { -7 }, "11111111111111111111111111111001", false, "A2B_negative" },

        test_param { ca_expr_funcs::A2C, { 0 }, "\0\0\0\0", false, "A2C_zero" },
        test_param { ca_expr_funcs::A2C, { 241 }, "\0\0\01", false, "A2C_positive" },
        test_param { ca_expr_funcs::A2C, { -252645136 }, "0000", false, "A2C_negative" },

        test_param { ca_expr_funcs::A2D, { 0 }, "+0", false, "A2D_zero" },
        test_param { ca_expr_funcs::A2D, { 241 }, "+241", false, "A2D_positive" },
        test_param { ca_expr_funcs::A2D, { -3 }, "-3", false, "A2D_negative" },

        test_param { ca_expr_funcs::A2X, { 0 }, "00000000", false, "A2X_zero" },
        test_param { ca_expr_funcs::A2X, { 1022 }, "00000101", false, "A2X_positive" },
        test_param { ca_expr_funcs::A2X, { -7 }, "FFFFFFF9", false, "A2X_negative" },

        test_param { ca_expr_funcs::B2C, { "101110011110001" }, "*1", false, "B2C_valid" },
        test_param { ca_expr_funcs::B2C, { "0" }, "\0", false, "B2C_padding" },
        test_param { ca_expr_funcs::B2C, { "00010010001" }, "\0j", false, "B2C_padding_zero" },
        test_param { ca_expr_funcs::B2C, { "" }, "", false, "B2C_empty" },
        test_param { ca_expr_funcs::B2C, { "12" }, {}, true, "B2C_bad_char" },

        test_param { ca_expr_funcs::B2D, { "11110001" }, "+241", false, "B2D_positive" },
        test_param { ca_expr_funcs::B2D, { "11111111111111111111111111110001" }, "-15", false, "B2D_negative" },
        test_param { ca_expr_funcs::B2D, { "" }, "+0", false, "B2D_empty" },
        test_param { ca_expr_funcs::B2D, { "111111111111111111111111111100011" }, {}, true, "B2D_exceeds" },
        test_param { ca_expr_funcs::B2D, { "12" }, {}, true, "B2D_bad_char" },

        test_param { ca_expr_funcs::B2X, { "0001001000" }, "0091", false, "B2X_padding" },
        test_param { ca_expr_funcs::B2X, { "" }, "", false, "B2X_empty" },
        test_param { ca_expr_funcs::B2X, { "12" }, {}, true, "B2X_bad_char" },

        test_param { ca_expr_funcs::BYTE, { 0 }, "\0", false, "B2X_zero" },
        test_param { ca_expr_funcs::BYTE, { 97 }, "/", false, "B2X_valid" },
        test_param { ca_expr_funcs::BYTE, { 200 }, {}, true, "B2X_exceeds" },

        test_param { ca_expr_funcs::C2B, { "" }, "", false, "C2B_empty" },
        test_param { ca_expr_funcs::C2B, { "\0" }, "00000000", false, "C2B_zero" },
        test_param { ca_expr_funcs::C2B, { "1234" }, "11110001111100101111001111110100", false, "C2B_valid" },
        test_param { ca_expr_funcs::C2B, { big_string() }, {}, true, "C2B_exceeds" },

        test_param { ca_expr_funcs::C2D, { "" }, "+0", false, "C2D_empty" },
        test_param { ca_expr_funcs::C2D, { "\0" }, "+0", false, "C2D_zero" },
        test_param { ca_expr_funcs::C2D, { "\0j" }, "+145", false, "C2D_positive" },
        test_param { ca_expr_funcs::C2D, { "0000" }, "-252645136", false, "C2D_negative" },
        test_param { ca_expr_funcs::C2D, { big_string() }, {}, true, "C2D_exceeds" },

        test_param { ca_expr_funcs::C2X, { "" }, "", false, "C2X_empty" },
        test_param { ca_expr_funcs::C2X, { "\0" }, "00", false, "C2X_zero" },
        test_param { ca_expr_funcs::C2X, { "1234567R" }, "F1F2F3F4F5F6F7D9", false, "C2X_valid" },
        test_param { ca_expr_funcs::C2X, { big_string() }, {}, true, "C2X_exceeds" },

        test_param { ca_expr_funcs::D2B, { "" }, "", false, "D2B_empty" },
        test_param { ca_expr_funcs::D2B, { "000" }, "00000000000000000000000000000000", false, "D2B_zeros" },
        test_param { ca_expr_funcs::D2B, { "1022" }, "00000000000000000000001111111110", false, "D2B_basic" },
        test_param { ca_expr_funcs::D2B, { "+5" }, "00000000000000000000000000000101", false, "D2B_leading_plus" },
        test_param { ca_expr_funcs::D2B, { "-7" }, "11111111111111111111111111111001", false, "D2B_leading_minus" },
        test_param { ca_expr_funcs::D2B, { "0f1" }, {}, true, "D2B_bad_char" },
        test_param { ca_expr_funcs::D2B, { "0000000000001" }, {}, true, "D2B_too_large" },
        test_param { ca_expr_funcs::D2B, { "99999999999" }, {}, true, "D2B_overflow" },
        test_param { ca_expr_funcs::D2B, { "+" }, {}, true, "D2B_only_sign" },
        test_param { ca_expr_funcs::D2B, { "--1" }, {}, true, "D2B_multiple_signs" },

        test_param { ca_expr_funcs::D2C, { "" }, {}, true, "D2C_empty" },
        test_param { ca_expr_funcs::D2C, { "000" }, "\0\0\0\0", false, "D2C_zeros" },
        test_param { ca_expr_funcs::D2C, { "23793" }, "\0\0*1", false, "D2C_basic" },
        test_param { ca_expr_funcs::D2C, { "0f1" }, {}, true, "D2C_bad_char" },
        test_param { ca_expr_funcs::D2C, { "0000000000001" }, {}, true, "D2C_too_large" },
        test_param { ca_expr_funcs::D2C, { "99999999999" }, {}, true, "D2C_overflow" },
        test_param { ca_expr_funcs::D2C, { "+" }, {}, true, "D2C_only_sign" },
        test_param { ca_expr_funcs::D2C, { "--1" }, {}, true, "D2C_multiple_signs" },

        test_param { ca_expr_funcs::D2X, { "" }, {}, true, "D2X_empty" },
        test_param { ca_expr_funcs::D2X, { "000" }, "00000000", false, "D2X_zeros" },
        test_param { ca_expr_funcs::D2X, { "1022" }, "000003FE", false, "D2X_basic" },
        test_param { ca_expr_funcs::D2X, { "+5" }, "00000005", false, "D2X_leading_plus" },
        test_param { ca_expr_funcs::D2X, { "-7" }, "FFFFFFF9", false, "D2X_leading_minus" },
        test_param { ca_expr_funcs::D2X, { "0f1" }, {}, true, "D2X_bad_char" },
        test_param { ca_expr_funcs::D2X, { "0000000000001" }, {}, true, "D2X_too_large" },
        test_param { ca_expr_funcs::D2X, { "99999999999" }, {}, true, "D2X_overflow" },
        test_param { ca_expr_funcs::D2X, { "+" }, {}, true, "D2X_only_sign" },
        test_param { ca_expr_funcs::D2X, { "--1" }, {}, true, "D2X_multiple_signs" },

        test_param { ca_expr_funcs::DCVAL, { "" }, "", false, "DCVAL_empty" },
        test_param { ca_expr_funcs::DCVAL, { "'" }, "'", false, "DCVAL_single_apo" },
        test_param { ca_expr_funcs::DCVAL, { "''" }, "'", false, "DCVAL_double_apo" },
        test_param { ca_expr_funcs::DCVAL, { "&&" }, "&", false, "DCVAL_single_amp" },
        test_param { ca_expr_funcs::DCVAL, { "a''b" }, "a'b", false, "DCVAL_apo_char" },
        test_param { ca_expr_funcs::DCVAL, { "a''b&&c" }, "a'b&c", false, "DCVAL_apo_amp_char" },

        test_param { ca_expr_funcs::DEQUOTE, { "adam" }, "adam", false, "DEQUOTE_char" },
        test_param { ca_expr_funcs::DEQUOTE, { "" }, "", false, "DEQUOTE_empty" },
        test_param { ca_expr_funcs::DEQUOTE, { "''abc'''" }, "'abc''", false, "DEQUOTE_apo_char" },
        test_param { ca_expr_funcs::DEQUOTE, { "''" }, "''", false, "DEQUOTE_apo" },
        test_param { ca_expr_funcs::DEQUOTE, { "''a" }, "a", false, "DEQUOTE_apo_char_one_side" },
        test_param { ca_expr_funcs::DEQUOTE, { "'" }, "", false, "DEQUOTE_apo_one_side" },

        test_param { ca_expr_funcs::DOUBLE, { "a&&''&b" }, "a&&&&''''&&b", false, "DOUBLE_simple" },
        test_param { ca_expr_funcs::DOUBLE, { big_string('\'') }, {}, true, "DOUBLE_exceeds" },

        test_param { ca_expr_funcs::LOWER, { "aBcDefG321&^%$" }, "abcdefg321&^%$", true, "LOWER_simple" },

        test_param { ca_expr_funcs::SIGNED, { 0 }, "0", false, "SIGNED_zero" },
        test_param { ca_expr_funcs::SIGNED, { 241 }, "241", false, "SIGNED_positive" },
        test_param { ca_expr_funcs::SIGNED, { -3 }, "-3", false, "SIGNED_negative" },

        test_param { ca_expr_funcs::UPPER, { "aBcDefG321&^%$" }, "ABCDEFG321&^%$", true, "UPPER_simple" },

        test_param { ca_expr_funcs::X2B, { "" }, "", false, "X2B_empty" },
        test_param { ca_expr_funcs::X2B, { "00" }, "00000000", false, "X2B_zeros" },
        test_param { ca_expr_funcs::X2B, { "f3" }, "0000000011110011", false, "X2B_basic" },
        test_param { ca_expr_funcs::X2B, { "0g1" }, {}, true, "X2B_bad_char" },
        test_param { ca_expr_funcs::X2B, { big_string() }, {}, true, "X2B_exceeds" },

        test_param { ca_expr_funcs::X2C, { "" }, "", false, "X2C_empty" },
        test_param { ca_expr_funcs::X2C, { "0" }, "\n", false, "X2C_zeros" },
        test_param { ca_expr_funcs::X2C, { "F1f2F3F4F5" }, "12345", false, "X2C_basic" },
        test_param { ca_expr_funcs::X2C, { "000F1" }, "\0\01", false, "X2C_basic2" },
        test_param { ca_expr_funcs::X2C, { "0g1" }, {}, true, "X2C_bad_char" },

        test_param { ca_expr_funcs::X2D, { "" }, "+0", false, "X2D_empty" },
        test_param { ca_expr_funcs::X2D, { "00" }, "+0", false, "X2D_zeros" },
        test_param { ca_expr_funcs::X2D, { "7FFfFFFF" }, "+2147483647", false, "X2D_positive" },
        test_param { ca_expr_funcs::X2D, { "FFFFfFF1" }, "-15", false, "X2D_negative" },
        test_param { ca_expr_funcs::X2D, { "0g1" }, {}, true, "X2D_bad_char" },
        test_param { ca_expr_funcs::X2D, { "000000f000"}, {}, true, "X2D_exceeds" }
        ),
    stringer());

TEST_P(ca_func, test)
{
    auto result = get_result();

    ASSERT_EQ(add_diags.diagnostics_present, GetParam().erroneous);

    if (!GetParam().erroneous)
        EXPECT_TRUE(compare(result, GetParam().true_result));
}
