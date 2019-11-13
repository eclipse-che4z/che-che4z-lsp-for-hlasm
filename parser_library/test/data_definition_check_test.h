
#include "data_definition_common.h"

using namespace hlasm_plugin::parser_library::checking;
using namespace hlasm_plugin::parser_library;


TEST(data_def_checker, unknown_type)
{
	dc d({}, "DC");
	
	data_definition_operand op = setup_data_def_op('W', '\0', "");

	diag_collector col;
	
	ASSERT_FALSE(d.check({ &op }, {}, ADD_DIAG(col)));
	ASSERT_EQ(col.diags().size(), 1U);
	ASSERT_EQ(col.diags()[0].code, "D012");
}

TEST(data_def_checker, unknown_extension)
{
	dc d({}, "DC");

	diag_collector col;

	data_definition_operand op = setup_data_def_op('B', 'A', "");
	ASSERT_FALSE(d.check({ &op }, {}, ADD_DIAG(col)));
	ASSERT_EQ(col.diags().size(), 1U);
	ASSERT_EQ(col.diags()[0].code, "D013");
}

TEST(data_def_checker, operands_too_long)
{
	ds_dxd d({}, "DC");

	diag_collector col;

	data_definition_operand op = setup_data_def_op('C', '\0', 1);
	op.dupl_factor = 1 << 30;

	ASSERT_FALSE(d.check({ &op, &op }, {}, ADD_DIAG(col)));
	ASSERT_EQ(col.diags().size(), 1U);
	ASSERT_EQ(col.diags()[0].code, "D029");
}

TEST(data_def_checker, unknown_extension_analyzer)
{
	std::string input = " DC BA'1'";
	analyzer a(input);
	a.analyze();
	a.collect_diags();

	ASSERT_EQ(a.diags().size(), 1U);
	EXPECT_EQ(a.diags()[0].code, "D013");
}

TEST(data_def_checker, unexpected_expr)
{
	data_def_type_B B;
	data_definition_operand op = setup_data_def_expr_op('B', '\0', 0, expr_type::ABS, 0, expr_type::ABS);
	
	diag_collector col;
	EXPECT_FALSE(B.check_DC(op, ADD_DIAG(col)));

	EXPECT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D018");
}

TEST(data_def_checker, unexpected_string)
{
	data_def_type_A t;
	data_definition_operand op = setup_data_def_op('A', '\0', "sth");


	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	EXPECT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D017");
}

TEST(data_def_checker, unexpected_address)
{
	data_def_type_A t;
	nominal_value_expressions exprs;
	exprs.push_back(data_def_expr{ 0, expr_type::ABS, range() });
	exprs.push_back(data_def_address{ data_def_field(true, 14, range()) , data_def_field(true, 2, range()) });
	data_definition_operand op = setup_data_def_op('A', '\0', std::move(exprs));

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	EXPECT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D020");
}

TEST(data_def_checker, DC_nominal_expected)
{
	data_def_type_A t;
	data_definition_operand op = setup_data_def_op('A', '\0');

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	EXPECT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D016");
}

TEST(data_def_checker, DS_no_nominal_ok)
{
	data_def_type_A t;
	data_definition_operand op = setup_data_def_op('A', '\0');

	diag_collector col;
	EXPECT_TRUE(t.check_DS(op, ADD_DIAG(col)));

	EXPECT_EQ(col.diags().size(), (size_t)0);
}

TEST(data_def_checker, nominal_checked_when_dupl_zero)
{
	data_def_type_S t;
	
	data_definition_operand op = setup_data_def_expr_op('S', '\0', -1, expr_type::ABS);
	op.dupl_factor = data_def_field(true, 0, range());
	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	EXPECT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D022");
}

TEST(data_def_checker, operand_too_long)
{
	data_def_type_C t;
	data_definition_operand op = setup_data_def_op('C', '\0', "char", 256);
	op.dupl_factor = 1 << 28;
	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	EXPECT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D028");
}

TEST(data_def_checker, B_correct)
{
	data_def_type_B t;

	data_definition_operand op;
	op.type = data_def_field(true, 'B', range());
	op.nominal_value.present = true;
	op.nominal_value.value = "101011010";

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	EXPECT_EQ(col.diags().size(), 0U);
}

TEST(data_def_checker, B_multiple_correct)
{
	data_def_type_B t;

	data_definition_operand op = setup_data_def_op('B', '\0', "11100101,10100010,011");

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	EXPECT_EQ(col.diags().size(), 0U);
}

TEST(data_def_checker, B_multiple_wrong_end)
{
	data_def_type_B t;

	data_definition_operand op = setup_data_def_op('B', '\0', "11100101,10100010,011,");

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), 1U);
	EXPECT_EQ(col.diags()[0].code, "D010");
}

TEST(data_def_checker, DS_nominal_is_checked)
{
	data_def_type_B t;

	data_definition_operand op = setup_data_def_op('B', '\0', "12");

	diag_collector col;
	EXPECT_FALSE(t.check_DS(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), 1U);
	EXPECT_EQ(col.diags()[0].code, "D010");
}

TEST(data_def_checker, B_multiple_wrong_middle)
{
	data_def_type_B t;

	data_definition_operand op = setup_data_def_op('B', '\0', "11100101,,10100010,011");

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), 1U);
	EXPECT_EQ(col.diags()[0].code, "D010");
}

TEST(data_def_checker, B_multiple_wrong_beginning)
{
	data_def_type_B t;

	data_definition_operand op = setup_data_def_op('B', '\0', ",011");

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), 1U);
	EXPECT_EQ(col.diags()[0].code, "D010");
}

TEST(data_def_checker, B_wrong)
{
	data_def_type_B t;

	data_definition_operand op;
	op.type = data_def_field(true, 'B', range());
	op.nominal_value.present = true;
	op.nominal_value.value = "10101101023";

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), 1U);
	EXPECT_EQ(col.diags()[0].code, "D010");
}

TEST(data_def_checker, C_DC_length_too_big)
{
	data_def_type_C t;

	data_definition_operand op = setup_data_def_op('C', '\0', "ASCII", 30000);

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), 1U);
	EXPECT_EQ(col.diags()[0].code, "D008");
}

TEST(data_def_checker, C_DS_length)
{
	data_def_type_C t;

	data_definition_operand op = setup_data_def_op('C', '\0', "ASCII", 65535);

	diag_collector col;
	EXPECT_TRUE(t.check_DS(op, ADD_DIAG(col)));
	EXPECT_EQ(col.diags().size(), 0U);
}

TEST(data_def_checker, C_DS_length_too_big)
{
	data_def_type_C t;

	data_definition_operand op = setup_data_def_op('C', '\0', "ASCII", 65536);

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), 1U);
	EXPECT_EQ(col.diags()[0].code, "D008");
}

TEST(data_def_checker, CA_correct)
{
	data_def_type_C t;

	data_definition_operand op = setup_data_def_op('C', '\0', "ASCII");

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));
	EXPECT_EQ(col.diags().size(), 0U);
}

TEST(data_def_checker, CE_correct)
{
	data_def_type_CE CE;

	data_definition_operand op = setup_data_def_op('C', 'E', "ebcdic");

	diag_collector col;
	EXPECT_TRUE(CE.check_DC(op, ADD_DIAG(col)));

	EXPECT_EQ(col.diags().size(), 0U);
}

TEST(data_def_checker, CU_correct)
{
	data_def_type_CU CU;

	data_definition_operand op = setup_data_def_op('C', 'U', "utf16");

	diag_collector col;
	EXPECT_TRUE(CU.check_DC(op, ADD_DIAG(col)));

	EXPECT_EQ(col.diags().size(), 0U);
}

TEST(data_def_checker, CU_not_even)
{
	data_def_type_CU CU;

	data_definition_operand op = setup_data_def_op('C', 'U', "utf16");
	op.length = data_def_field(true, 35, range());

	diag_collector col;
	EXPECT_FALSE(CU.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), 1U);
	EXPECT_EQ(col.diags()[0].code, "D014");
}

TEST(data_def_checker, CU_bit_length_not_allowed)
{
	data_def_type_CU CU;

	data_definition_operand op = setup_data_def_op('C', 'U', "utf16");
	op.length = data_def_field(true, 480, range());
	op.length.len_type = data_def_length_t::BIT;

	diag_collector col;
	EXPECT_FALSE(CU.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), 1U);
	EXPECT_EQ(col.diags()[0].code, "D007");
}

TEST(data_def_checker, G_not_even)
{
	data_def_type_G t;

	data_definition_operand op = setup_data_def_op('G', '\0', "utf16");
	op.length = data_def_field(true, 35, range());

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), 1U);
	EXPECT_EQ(col.diags()[0].code, "D014");
}

TEST(data_def_checker, X_correct)
{
	data_def_type_X t;

	data_definition_operand op = setup_data_def_op('X', '\0', " ABCDEFabc def1 234567890 ");
	op.length = data_def_field(true, 35, range());

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), 0U);
}

TEST(data_def_checker, X_multiple)
{
	data_def_type_X t;

	data_definition_operand op = setup_data_def_op('X', '\0', "ABCDEF,abcdef12345,67890");
	op.length = data_def_field(true, 35, range());

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), 0U);
}

TEST(data_def_checker, X_hexa_digit_expected)
{
	data_def_type_X t;

	data_definition_operand op = setup_data_def_op('X', '\0', "G");

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), 1U);
	EXPECT_EQ(col.diags()[0].code, "D010");
}


TEST(data_def_checker, F_correct)
{
	data_def_type_F t;

	data_definition_operand op = setup_data_def_op('F', '\0', "1.23E3");

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t) 0);
}

TEST(data_def_checker, FD_correct)
{
	data_def_type_FD t;

	data_definition_operand op = setup_data_def_op('F', 'D', "1.23E3");

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)0);
}

TEST(data_def_checker, H_correct)
{
	data_def_type_H t;

	data_definition_operand op = setup_data_def_op('H', '\0', "U12345");

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)0);
}

TEST(data_def_checker, H_correct_no_fractional)
{
	data_def_type_H t;

	data_definition_operand op = setup_data_def_op('H', '\0', "3.");

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)0);
}

TEST(data_def_checker, H_sign_after_dot)
{
	data_def_type_H t;

	data_definition_operand op = setup_data_def_op('H', '\0', "3.+0");

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D010");
}

TEST(data_def_checker, H_two_signs)
{
	data_def_type_H t;

	data_definition_operand op = setup_data_def_op('H', '\0', "U-12345");

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), 1U);
	EXPECT_EQ(col.diags()[0].code, "D010");
}

TEST(data_def_checker, H_no_exponent)
{
	data_def_type_H t;

	data_definition_operand op = setup_data_def_op('H', '\0', "U12345E");

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), 1U);
	EXPECT_EQ(col.diags()[0].code, "D010");
}

TEST(data_def_checker, H_no_fraction)
{
	data_def_type_H t;

	data_definition_operand op = setup_data_def_op('H', '\0', "U123123.");

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), 0U);
}

TEST(data_def_checker, H_no_integral)
{
	data_def_type_H t;

	data_definition_operand op = setup_data_def_op('H', '\0', ".3,+.3,-.3,U.3");

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), 0U);
}


TEST(data_def_checker, P_correct)
{
	data_def_type_P t;

	data_definition_operand op = setup_data_def_op('P', '\0', "456");

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)0);
}

TEST(data_def_checker, Z_correct)
{
	data_def_type_Z t;

	data_definition_operand op = setup_data_def_op('Z', '\0', "+456,-01453,-12.13,+10.19");

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)0);
}

TEST(data_def_checker, Z_two_signs)
{
	data_def_type_Z t;

	data_definition_operand op = setup_data_def_op('Z', '\0', "+-456");

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D010");
}

TEST(data_def_checker, Z_two_dots)
{
	data_def_type_Z t;

	data_definition_operand op = setup_data_def_op('Z', '\0', "456..4");

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D010");
}

TEST(data_def_checker, Z_two_commas)
{
	data_def_type_Z t;

	data_definition_operand op = setup_data_def_op('Z', '\0', "456,,45");

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D010");
}

TEST(data_def_checker, Z_plus_only)
{
	data_def_type_Z t;

	data_definition_operand op = setup_data_def_op('Z', '\0', "+,45");

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D010");
}

TEST(data_def_checker, Z_plus_end)
{
	data_def_type_Z t;

	data_definition_operand op = setup_data_def_op('Z', '\0', "45+,45");

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D010");
}

TEST(data_def_checker, Z_comma_only)
{
	data_def_type_Z t;

	data_definition_operand op = setup_data_def_op('Z', '\0', ",");

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D010");
}

TEST(data_def_checker, A_sym_bit_length)
{
	data_def_type_A t;

	data_definition_operand op = setup_data_def_expr_op_length('A', '\0', 10, 0, expr_type::RELOC, 0, expr_type::COMPLEX);
	op.length.len_type = data_def_length_t::BIT;

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D007");
}

TEST(data_def_checker, A_sym_length_too_big)
{
	data_def_type_A t;
	data_definition_operand op = setup_data_def_expr_op_length('A', '\0', 8, 0, expr_type::RELOC, 0, expr_type::ABS);

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D008");
}

TEST(data_def_checker, A_abs_length_ok)
{
	data_def_type_A t;
	data_definition_operand op = setup_data_def_expr_op_length('A', '\0', 8, 0, expr_type::ABS, 0, expr_type::ABS);

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)0);
}

TEST(data_def_checker, RD_wrong_length)
{
	data_def_type_RD t;
	data_definition_operand op = setup_data_def_expr_op_length('R', 'D', 6, 0, expr_type::RELOC, 0, expr_type::RELOC);

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	ASSERT_EQ(col.diags()[0].code, "D021");
}

TEST(data_def_checker, S_expr_out_of_range)
{
	data_def_type_S t;
	data_definition_operand op = setup_data_def_expr_op('S', '\0', 4097, expr_type::ABS);

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	ASSERT_EQ(col.diags()[0].code, "D022");
}

TEST(data_def_checker, S_expr_out_of_range_negative)
{
	data_def_type_S t;
	data_definition_operand op = setup_data_def_expr_op('S', '\0', -10, expr_type::ABS);

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	ASSERT_EQ(col.diags()[0].code, "D022");
}

TEST(data_def_checker, S_displacement_out_of_range)
{
	data_def_type_S t;
	data_definition_operand op = setup_data_def_addr_op('S', '\0', 3, -10);

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	ASSERT_EQ(col.diags()[0].code, "D022");
}

TEST(data_def_checker, S_base_out_of_range)
{
	data_def_type_S t;
	data_definition_operand op = setup_data_def_addr_op('S', '\0', 30, 30);

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	ASSERT_EQ(col.diags()[0].code, "D023");
}

TEST(data_def_checker, SY_negative_displacement_OK)
{
	data_def_type_SY t;
	data_definition_operand op = setup_data_def_addr_op('S', 'Y', 3, -10);

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)0);
}

TEST(data_def_checker, J_wrong_length)
{
	data_def_type_J t;
	data_definition_operand op = setup_data_def_expr_op_length('J', '\0', 6, 0, expr_type::RELOC);

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	ASSERT_EQ(col.diags()[0].code, "D024");
}

TEST(data_def_checker, J_length_ok)
{
	data_def_type_J t;
	data_definition_operand op = setup_data_def_expr_op_length('J', '\0', 2, 0, expr_type::RELOC);

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)0);
}

TEST(data_def_checker, V_ok)
{
	std::string input = " DC V(SYMBOL)";
	analyzer a(input);
	a.analyze();
	a.collect_diags();
	ASSERT_EQ(a.diags().size(), 0U);
}

TEST(data_def_checker, V_single_symbol_expected)
{
	std::string input = " DC V(SYMBOL+5)";
	analyzer a(input);
	a.analyze();
	a.collect_diags();
	ASSERT_EQ(a.diags().size(), 1U);
	EXPECT_EQ(a.diags()[0].code, "D030");

	std::string input2 = " DC V(12(13))";
	analyzer a2(input);
	a2.analyze();
	a2.collect_diags();
	ASSERT_EQ(a2.diags().size(), 1U);
	EXPECT_EQ(a2.diags()[0].code, "D030");
}

TEST(data_def_checker, L_two_commas)
{
	data_def_type_L t;

	data_definition_operand op = setup_data_def_op('L', '\0', "456,,45");

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D010");
}

TEST(data_def_checker, L_correct)
{
	data_def_type_L t;

	data_definition_operand op = setup_data_def_op('L', '\0', "456E10,5");

	diag_collector col;
	EXPECT_TRUE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)0);
}

TEST(data_def_checker, L_invalid_round_mode)
{
	data_def_type_L t;

	data_definition_operand op = setup_data_def_op('L', '\0', "456E10R5");

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D026");
}

TEST(data_def_checker, LD_invalid_round_mode)
{
	data_def_type_LD t;

	data_definition_operand op = setup_data_def_op('L', 'D', "456E7R5");

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D026");
}

TEST(data_def_checker, LD_no_round_mode)
{
	data_def_type_LD t;

	data_definition_operand op = setup_data_def_op('L', 'D', "456E7R");

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D026");
}

TEST(data_def_checker, LD_no_exponent)
{
	data_def_type_LD t;

	data_definition_operand op = setup_data_def_op('L', 'D', "456ER10");

	diag_collector col;
	EXPECT_FALSE(t.check_DC(op, ADD_DIAG(col)));

	ASSERT_EQ(col.diags().size(), (size_t)1);
	EXPECT_EQ(col.diags()[0].code, "D010");
}
