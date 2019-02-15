#ifndef HLASM_PLUGIN_PARSER_LIBRARY_TEST_MACH_INSTR_CHECK_TEST_H
#define HLASM_PLUGIN_PARSER_LIBRARY_TEST_MACH_INSTR_CHECK_TEST_H

#include "../src/checking/instruction_checker.h"

class machine_instr_test : public testing::Test
{
public:
	virtual void SetUp(std::string param) {}
	virtual void TearDown() {}
	machine_instr_test() {}

protected:
	hlasm_plugin::parser_library::checking::one_operand n_15 = { "15" };
	hlasm_plugin::parser_library::checking::one_operand n_4 = { "4" };
	hlasm_plugin::parser_library::checking::one_operand n_0 = { "0" };
	hlasm_plugin::parser_library::checking::one_operand n_1022 = { "1022" };
	hlasm_plugin::parser_library::checking::one_operand n_63 = { "63" };
	hlasm_plugin::parser_library::checking::one_operand n_7 = { "7" };
	hlasm_plugin::parser_library::checking::one_operand n_10 = { "10" };
	hlasm_plugin::parser_library::checking::one_operand n_1 = { "1" };
	hlasm_plugin::parser_library::checking::one_operand n_45 = { "45" };
	hlasm_plugin::parser_library::checking::one_operand n_60 = { "60" };
	hlasm_plugin::parser_library::checking::one_operand n_2 = { "2" };
	hlasm_plugin::parser_library::checking::one_operand n_min1 = { "-1" };
	hlasm_plugin::parser_library::checking::one_operand n_1024 = { "1024" };
	hlasm_plugin::parser_library::checking::one_operand n_143 = { "143" };

	hlasm_plugin::parser_library::checking::address_operand dxb4_one{ hlasm_plugin::parser_library::checking::address_state::UNRES, 143, 15, 7 };
	hlasm_plugin::parser_library::checking::address_operand dxb4_two = { hlasm_plugin::parser_library::checking::address_state::RES_VALID, 143, 15, 7 };
	hlasm_plugin::parser_library::checking::address_operand db_one = { hlasm_plugin::parser_library::checking::address_state::UNRES, 8, 0, -1 };
	hlasm_plugin::parser_library::checking::address_operand dxb8_one = { hlasm_plugin::parser_library::checking::address_state::UNRES, 143, 63, 15 };
	hlasm_plugin::parser_library::checking::address_operand dxb4_three = { hlasm_plugin::parser_library::checking::address_state::UNRES, 0, 0, 0 };
	hlasm_plugin::parser_library::checking::address_operand db_two = { hlasm_plugin::parser_library::checking::address_state::UNRES, 100, 4, -1 };
	hlasm_plugin::parser_library::checking::address_operand dxb4_four = { hlasm_plugin::parser_library::checking::address_state::UNRES, 143, 15, 8 };
	hlasm_plugin::parser_library::checking::address_operand db_three = { hlasm_plugin::parser_library::checking::address_state::UNRES, 100, 4, 1 };
	hlasm_plugin::parser_library::checking::address_operand dxb4_five = { hlasm_plugin::parser_library::checking::address_state::UNRES, 100, 4, 40 };
	hlasm_plugin::parser_library::checking::address_operand dxb4_six = { hlasm_plugin::parser_library::checking::address_state::RES_INVALID, 143, 15, 7 };


	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_true_simple_one { std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_15, &n_4, &n_0 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_true_simple_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_4, &n_1022 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_true_simple_three{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_15, &n_15, &n_63, &n_0, &n_7 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_true_simple_four{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_10, &n_1, &n_45, &n_60 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_true_addr_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &dxb4_one, &dxb4_two } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_true_addr_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &dxb8_one, &db_one } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_true_mixed_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &dxb4_three, &db_two, &n_2 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_false_simple_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_15, &n_4, &n_min1 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_false_simple_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_1024, &n_4, &n_15 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_false_addr_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &dxb4_two, &dxb4_six} };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_false_addr_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &dxb8_one, &dxb4_four } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_false_addr_three{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &db_three, &db_three } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_false_addr_four{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &dxb4_five } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_true_no_operands{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{} };

	checking::machine_instruction_checker checker;
};

TEST_F(machine_instr_test, checker_test)
{
	EXPECT_TRUE(checker.mach_instr_check("CSCH", test_true_no_operands));
	EXPECT_TRUE(checker.mach_instr_check("ADTR", test_true_simple_one));
	EXPECT_TRUE(checker.mach_instr_check("VLEIB", test_true_simple_one));
	EXPECT_TRUE(checker.mach_instr_check("SSKE", test_true_simple_one));
	EXPECT_TRUE(checker.mach_instr_check("CLRL", test_true_simple_two));
	EXPECT_TRUE(checker.mach_instr_check("ROSBG", test_true_simple_three));
	EXPECT_TRUE(checker.mach_instr_check("ROSBG", test_true_simple_four));
	EXPECT_TRUE(checker.mach_instr_check("UNPK", test_true_addr_one));
	EXPECT_TRUE(checker.mach_instr_check("TR", test_true_addr_two));
	EXPECT_TRUE(checker.mach_instr_check("SRP", test_true_mixed_one));

	EXPECT_FALSE(checker.mach_instr_check("SQER", test_true_no_operands));
	EXPECT_TRUE(checker.mach_instr_check("SRXT", test_true_simple_one));
	EXPECT_FALSE(checker.mach_instr_check("VMAE", test_true_simple_one));
	EXPECT_FALSE(checker.mach_instr_check("TAR", test_true_simple_two));
	EXPECT_FALSE(checker.mach_instr_check("VFM", test_true_simple_three));
	EXPECT_FALSE(checker.mach_instr_check("TROO", test_true_simple_four));
	EXPECT_FALSE(checker.mach_instr_check("VL", test_true_addr_one));
	EXPECT_FALSE(checker.mach_instr_check("ZAP", test_true_addr_two));
	EXPECT_FALSE(checker.mach_instr_check("TBDR", test_false_simple_one));
	EXPECT_FALSE(checker.mach_instr_check("TRTT", test_false_simple_two));
	EXPECT_FALSE(checker.mach_instr_check("ZAP", test_false_addr_one));
	EXPECT_FALSE(checker.mach_instr_check("XC", test_false_addr_two));
	EXPECT_FALSE(checker.mach_instr_check("XC", test_false_addr_three));
	EXPECT_FALSE(checker.mach_instr_check("TP", test_false_addr_four));
	EXPECT_TRUE(checker.mach_instr_check("TRACE", test_true_simple_one));
}

#endif
