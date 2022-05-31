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
#include "../mock_parse_lib_provider.h"

// test for
// symbol data attributes

TEST(data_attributes, N)
{
    hlasm_context ctx;


    auto id = ctx.ids().add("id");
    auto id2 = ctx.ids().add("id2");

    macro_data_ptr data = std::make_unique<macro_param_data_single>("ada");
    EXPECT_EQ(positional_param(id, 0, *data).number({}), 1);
    data = std::make_unique<macro_param_data_dummy>();
    EXPECT_EQ(positional_param(id, 0, *data).number({}), 0);


    auto s0 = std::make_unique<macro_param_data_single>("first");
    auto s1 = std::make_unique<macro_param_data_single>("second");
    auto s2 = std::make_unique<macro_param_data_single>("third");
    std::vector<macro_data_ptr> v;
    v.push_back(move(s0));
    v.push_back(move(s1));
    v.push_back(move(s2));

    EXPECT_EQ(keyword_param(id, std::make_unique<macro_param_data_dummy>(), nullptr).number({}), 0);
    data = std::make_unique<macro_param_data_composite>(move(v));
    auto kp = keyword_param(id, std::make_unique<macro_param_data_dummy>(), std::move(data));
    EXPECT_EQ(kp.number({}), 3);
    EXPECT_EQ(kp.number({ 1 }), 1);
    EXPECT_EQ(kp.number({ 4 }), 0);

    auto var = ctx.create_local_variable<A_t>(id, true)->access_set_symbol<A_t>();

    EXPECT_EQ(var->number({}), 0);
    EXPECT_EQ(var->number({ 1 }), 0);

    var->set_value(1);
    EXPECT_EQ(var->number({}), 0);
    EXPECT_EQ(var->number({ 1 }), 0);

    auto var_ns = ctx.create_local_variable<A_t>(id2, false)->access_set_symbol<A_t>();

    EXPECT_EQ(var_ns->number({}), 0);
    EXPECT_EQ(var_ns->number({ 1 }), 0);

    var_ns->set_value(1, 0);
    EXPECT_EQ(var_ns->number({}), 1);
    EXPECT_EQ(var_ns->number({ 1 }), 1);

    var_ns->set_value(1, 14);
    EXPECT_EQ(var_ns->number({}), 15);
    EXPECT_EQ(var_ns->number({ 1 }), 15);
}

TEST(data_attributes, K)
{
    hlasm_context ctx;


    auto idA = ctx.ids().add("id1");
    auto idB = ctx.ids().add("id2");
    auto idC = ctx.ids().add("id3");

    macro_data_ptr data = std::make_unique<macro_param_data_single>("ada");
    EXPECT_EQ(positional_param(idA, 0, *data).count({}), 3);
    data = std::make_unique<macro_param_data_dummy>();
    EXPECT_EQ(positional_param(idA, 0, *data).count({}), 0);


    auto s0 = std::make_unique<macro_param_data_single>("first");
    auto s1 = std::make_unique<macro_param_data_single>("second");
    auto s2 = std::make_unique<macro_param_data_single>("third");
    std::vector<macro_data_ptr> v;
    v.push_back(move(s0));
    v.push_back(move(s1));
    v.push_back(move(s2));

    EXPECT_EQ(keyword_param(idA, std::make_unique<macro_param_data_dummy>(), nullptr).count({}), 0);
    auto kp = keyword_param(
        idA, std::make_unique<macro_param_data_dummy>(), std::make_unique<macro_param_data_composite>(move(v)));
    EXPECT_EQ(kp.count({}), 20);
    EXPECT_EQ(kp.count({ 2 }), 6);
    EXPECT_EQ(kp.count({ 4 }), 0);

    auto varA = ctx.create_local_variable<A_t>(idA, true)->access_set_symbol<A_t>();
    auto varB = ctx.create_local_variable<B_t>(idB, true)->access_set_symbol<B_t>();
    auto varC = ctx.create_local_variable<C_t>(idC, true)->access_set_symbol<C_t>();

    EXPECT_EQ(varA->count({}), 1);
    EXPECT_EQ(varA->count({ 1 }), 1);
    EXPECT_EQ(varB->count({}), 1);
    EXPECT_EQ(varB->count({ 1 }), 1);
    EXPECT_EQ(varC->count({}), 0);
    EXPECT_EQ(varC->count({ 1 }), 0);

    varA->set_value(1);
    EXPECT_EQ(varA->count({}), 1);
    varA->set_value(10);
    EXPECT_EQ(varA->count({}), 2);
    varA->set_value(-10);
    EXPECT_EQ(varA->count({}), 3);
}

TEST(data_attributes, N_var_syms)
{
    std::string input = R"(
&var(1) seta 1,2,3
&var2 seta 1
&N1 seta N'&var
&N2 seta N'&var2
&N3 seta N'&var(2)
&N4 seta N'&var2(2)

 macro
 m  &a
 gbla nn1,nn2,nn3
&nn1 seta N'&a
&nn2 seta N'&a(1)
&nn3 seta N'&syslist(2)
 mend

 gbla nn1,nn2,nn3
 m (1,2,3),4
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "N1"), 3);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "N2"), 0);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "N3"), 3);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "N4"), 0);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "NN1"), 3);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "NN2"), 1);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "NN3"), 1);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_attributes, N_var_syms_2)
{
    std::string input = R"(
      MACRO
      MAC_KEY_PAR &A=()
      GBLA &VAR1
&VAR1 SETA N'&A
      MEND

      MACRO
      MAC_POS_PAR &A
      GBLA &VAR2
&VAR2 SETA N'&A
      MEND

      MACRO
      MAC2 &A=()
      GBLA &VAR1,&VAR2
      MAC_KEY_PAR A=&A
      MAC_POS_PAR &A
      MEND

      GBLA &VAR1,&VAR2
      MAC2
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "VAR1"), 1);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "VAR2"), 1);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_attributes, K_var_syms_good)
{
    std::string input = R"(
&var(1) seta 1,2,3
&var2 seta 1
&N1 seta K'&var(1)
&N2 seta K'&var2

 macro
 m  &a
 gbla nn1,nn2,nn3
&nn1 seta K'&a
&nn2 seta K'&a(1)
&nn3 seta K'&syslist(2)
 mend

 gbla nn1,nn2,nn3
 m (1,2,3),three
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "N1"), 1);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "N2"), 1);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "NN1"), 7);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "NN2"), 1);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "NN3"), 5);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_attributes, K_var_syms_bad)
{
    std::string input = R"(
&var(1) seta 1,2,3
&N1 seta K'&var
)";

    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(data_attributes, T_ord_syms)
{
    std::string input = R"(
A LR 1,1
B CSECT
C LOCTR
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "A")->attributes().type(), ebcdic_encoding::a2e[U'I']);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "B")->attributes().type(), ebcdic_encoding::a2e[U'J']);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "C")->attributes().type(), ebcdic_encoding::a2e[U'J']);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_attributes, T_var_syms)
{
    std::string input = R"(
&v1 setc ''
&v2 seta 11
&v3 setb 1
&v4 setc '11'
&v5 setc 'c''1'''
&v6 setc '**c''1'''

&t1 setc t'&v1
&t2 setc t'&v2
&t3 setc t'&v3
&t4 setc t'&v4
&t5 setc t'&v5
&t6 setc t'&v6
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "T1"), "O");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "T2"), "N");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "T3"), "N");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "T4"), "N");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "T5"), "N");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "T6"), "U");

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_attributes, T_macro_params)
{
    std::string input = R"(
 MACRO
 M &OP1,&OP2
 GBLC T1,T2
&T1 SETC T'&OP1
&T2 SETC T'&OP2(2)
 MEND
 
 GBLC T1,T2
 M (,),(,1)

)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "T1"), "O");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "T2"), "N");

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_attributes, T_var_to_ord_syms)
{
    std::string input = R"(
LAB     LR 1,1
SYMC    DC C''
SYMA    DC A(0)

&v1     SETC 'LAB'
&arr(1) SETC 'SYMC','SYMA'

&t1     SETC t'&v1
&t2     SETC t'&arr(1)
&t3     SETC t'&arr(2)

)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "T1"), "I");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "T2"), "C");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "T3"), "A");

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), 0U);
}

TEST(data_attributes, T_var_zero_subscript)
{
    std::string input = R"(
SYMC    DC C''
SYMA    DC A(0)

&arr(1) SETC 'SYMC','SYMA'

&t1     SETC t'&arr(0)

)";

    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), 1U);
    EXPECT_EQ(a.diags()[0].code, "E012");
}

TEST(data_attributes, D)
{
    std::string input = R"(
A EQU 11,
B LR 1,1
&V1 SETB (D'A)
&V2 SETB (D'B)
&V3 SETB (D'C)

&SYM SETC 'A'
&BAD_SYM SETC 'C'

&V4 SETB (D'&SYM)
&V5 SETB (D'&BAD_SYM)
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "V1"), true);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "V2"), true);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "V3"), false);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "V4"), true);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "V5"), false);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}


TEST(data_attributes, L_ord_syms)
{
    std::string input = R"(
TEST CSECT
LBL LR 1,1
A EQU L'TEST
B EQU L'LBL
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "A"), 1);
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "B"), 2);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_attributes, L_var_syms)
{
    std::string input = R"(
TEST EQU 11,10

&LBL setc 'TEST'

&V1 SETA L'&LBL
&V2 SETA L'TEST

)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "V1"), 10);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "V2"), 10);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_attributes, S_ord_syms)
{
    std::string input = R"(
A DC FS12'1'

B EQU S'A

)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "B"), 12);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_attributes, S_var_syms)
{
    std::string input = R"(
A DC FS12'1'

&V1 SETA S'A

)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "V1"), 12);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_attributes, I)
{
    std::string input = R"(
HALFCON DC HS6'-25.93'
ONECON DC FS8'100.3E-2'
SHORT DC ES2'46.415'
LONG DC DS5'-3.729'
EXTEND DC LS10'5.312'
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "HALFCON")->attributes().integer(), (symbol_attributes::len_attr)9);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "ONECON")->attributes().integer(), (symbol_attributes::len_attr)23);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "SHORT")->attributes().integer(), (symbol_attributes::len_attr)4);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "LONG")->attributes().integer(), (symbol_attributes::len_attr)9);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "EXTEND")->attributes().integer(), (symbol_attributes::len_attr)18);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_attributes, I_ord_syms)
{
    std::string input = R"(
HALFCON DC HS6'-25.93'
X EQU I'HALFCON
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "X"), 9);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_attributes, I_var_syms)
{
    std::string input = R"(
HALFCON DC HS6'-25.93'
&V SETA I'HALFCON
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "V"), 9);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_attributes, O_opencode_ord)
{
    std::string input = R"(
 MACRO
 M
 MEND

&V1 SETC O'LR
&V2 SETC O'ORG
&V3 SETC O'SETC
&V4 SETC O'J
&V5 SETC O'M
&V6 SETC O'UNKNOWN

)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "V1"), "O");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "V2"), "A");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "V3"), "A");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "V4"), "E");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "V5"), "M");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "V6"), "U");

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_attributes, O_opencode_var)
{
    std::string input = R"(

&A1 SETC 'M'

&V1 SETC O'&A1

 MACRO
 M
 MEND

&V2 SETC O'&A1

)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "V1"), "O");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "V2"), "M");

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}


TEST(data_attributes, O_libraries)
{
    std::string input = R"(
&V1 SETC O'MAC
&V2 SETC O'M2
 MAC 1
&V3 SETC O'MAC

)";
    const std::string mac_def =
        R"(   MACRO
       MAC  
       LR   1,1
       MEND
)";

    mock_parse_lib_provider prov { { "MAC", mac_def } };
    analyzer a(input, analyzer_options { &prov });
    a.analyze();

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "V1"), "S");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "V2"), "U");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "V3"), "M");

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), 0U);
}

TEST(data_attributes, basic_attr_ref)
{
    std::string input = R"(
Q EQU 1
W EQU 4

A EQU L'Q
B EQU T'W

)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "A"), 1);
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "B"), symbol_attributes::undef_type);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_attributes, forward_attr_ref)
{
    std::string input = R"(
A EQU L'Q
B EQU T'W

Q EQU 1,2,3
W EQU 4,5,6

)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "A"), 2);
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "B"), 6);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_attributes, forward_attr_ref_CA)
{
    std::string input = R"(
A EQU B

&V1 SETA L'A

B EQU 1,11

)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "V1"), 1);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "A")->attributes().length(), (symbol_attributes::len_attr)1);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_attributes, forward_attr_ref_fail)
{
    std::string input = R"(
A EQU L'Q
B EQU T'W

)";

    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)2);
}

TEST(data_attributes, attr_cycle_ok)
{
    std::string input = R"(
A EQU B,11
B EQU L'A

)";

    analyzer a(input);
    a.analyze();

    ASSERT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(a.hlasm_ctx().ids().add("A")));
    ASSERT_TRUE(a.hlasm_ctx().ord_ctx.symbol_defined(a.hlasm_ctx().ids().add("B")));

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "A"), 11);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "A")->attributes().length(), (symbol_attributes::len_attr)11);
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "B"), 11);
    EXPECT_EQ(get_symbol(a.hlasm_ctx(), "B")->attributes().length(), (symbol_attributes::len_attr)1);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_attributes, attr_value_reloc_expr)
{
    std::string input = R"(
A LR 1,1
B LR 1,1

X EQU B-A+L'C

C EQU 1,12

)";

    analyzer a(input);
    a.analyze();

    ASSERT_EQ(get_symbol(a.hlasm_ctx(), "X")->kind(), symbol_value_kind::ABS);

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_attributes, bad_attr_access)
{
    std::string input = R"(
C EQU C'A'
V EQU I'C

)";

    analyzer a(input);
    a.analyze();

    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(data_attributes, loctr_length)
{
    std::string input = R"(
X1   DS   (L'*)C
A    EQU  *-X1
X2   DC   (L'*)C' '
B    EQU  *-X2
C    EQU  L'*
)";

    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "A"), 1);
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "B"), 1);
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "C"), 1);
}

TEST(data_attributes, expression_without_spaces)
{
    std::string input = R"(
        MACRO
        MAC &P=
&B      SETB ('&P'EQ'E'OR'&P'EQ'D'OR'&P'EQ'B')
&P      EQU  &B
        MEND

        MAC P=A
        MAC P=E
        MAC P=D
        MAC P=B
)";

    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "A"), 0);
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "E"), 1);
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "D"), 1);
    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "B"), 1);
}

TEST(data_attributes, attribute_after_paren)
{
    std::string input = R"(
    MACRO
    MAC2 &P
    GBLB &X
&X  SETB (&P)
    MEND
    MACRO
    MAC
    MAC2 &SYSLIST(N'&SYSLIST)
    MEND

    GBLB &X
    MAC  1
)";

    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "X"), true);
}

TEST(data_attributes, variable_type)
{
    // Just to make clear what is going on here
    // This special 'M' behavior is triggerred ONLY
    // when the tested symbol is equal to the name field on the macro call
    std::string input = R"(
         MACRO
&L       MAC   &E
         GBLA  &I
         GBLC  &RES(1)
&I       SETA  &I+1
&C       SETC  (UPPER '&E')
&R(1)    SETC  T'&L,T'&E,T'&C
&TEXT    SETC  DOUBLE('&L')
&RES(&I) SETC '&TEXT = &R(1) &R(2) &R(3)'
         MEND

         GBLC  &RES(1)
A        MAC   A
A        EQU   1
A        MAC   A
$        MAC   $
3        MAC   3
_        MAC   _
A_       MAC   A_
=        MAC   =
* TODO: not supported because of outstanding label parsing issue
* =C' '    MAC   =C' '
* =C'a'    MAC   =C'A'
=A(0)    MAC   =A(0)
=A(0)    MAC   =A(0+0)
         END
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_vector<C_t>(a.hlasm_ctx(), "RES"),
        std::vector<std::string>({
            "A = M M M",
            "A = U U U",
            "$ = M M M",
            "3 = N N N",
            "_ = M M M",
            "A_ = M M M",
            "= = M M M",
            /* TODO "=C' ' = M M M", */
            /* TODO "=C'a' = C M M", */
            "=A(0) = M M M",
            "=A(0) = M A A",
        }));
}

TEST(data_attributes, delayed_literal_definition)
{
    std::string input = R"(
         MACRO
&L       MAC
         GBLB &B
         GBLC &C
&C       SETC T'&L
&B       SETB (D'&L)
         MEND
         GBLB &B
         GBLC &C
=A(0)    MAC
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "B"), false);
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "C"), "M");
}
