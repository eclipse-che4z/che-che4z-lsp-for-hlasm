#ifndef HLASM_PLUGIN_PARSER_LIBRARY_TEST_MACH_INSTR_CHECK_TEST_H
#define HLASM_PLUGIN_PARSER_LIBRARY_TEST_MACH_INSTR_CHECK_TEST_H

#include "../src/checking/instruction_checker.h"

using namespace hlasm_plugin::parser_library::checking;

class machine_instr_test : public testing::Test
{
public:
	virtual void SetUp(std::string param) {}
	virtual void TearDown() {}
	machine_instr_test() {
	
		test_balr_true.push_back(&op_val_14);
		test_balr_true.push_back(&op_val_15);
	
		test_lr_true.push_back(&op_val_0);
		test_lr_true.push_back(&op_val_2);
	
		test_br_true.push_back(&op_val_11);

		test_mvcl_true.push_back(&op_val_4);
		test_mvcl_true.push_back(&op_val_0);
	
		test_xr_true.push_back(&op_val_15);
		test_xr_true.push_back(&op_val_15);
	}

protected:

	simple_operand_value op_val_0 = simple_operand_value(0);
	simple_operand_value op_val_2 = simple_operand_value(2);
	simple_operand_value op_val_4 = simple_operand_value(4);
	simple_operand_value op_val_11 = simple_operand_value(11);
	simple_operand_value op_val_12 = simple_operand_value(12);
	simple_operand_value op_val_13 = simple_operand_value(13);
	simple_operand_value op_val_14 = simple_operand_value(14);
	simple_operand_value op_val_15 = simple_operand_value(15);

	std::vector<machine_operand_value*> test_no_operand_true = std::vector<machine_operand_value*>();
	std::vector<machine_operand_value*> test_balr_true = std::vector<machine_operand_value*>();
	std::vector<machine_operand_value*> test_lr_true = std::vector<machine_operand_value*>();
	std::vector<machine_operand_value*> test_br_true = std::vector<machine_operand_value*>();
	std::vector<machine_operand_value*> test_mvcl_true = std::vector<machine_operand_value*>();
	std::vector<machine_operand_value*> test_xr_true = std::vector<machine_operand_value*>();
};

TEST_F(machine_instr_test, checker_test)
{
	std::string balr_name = "BALR";
	std::string lr_name = "LR";
	std::string mvcl_name = "MVCL";
	std::string xr_name = "XR";

	auto balr_true = (&hlasm_plugin::parser_library::context::instruction::machine_instructions.at(balr_name))->get()->check(balr_name, test_balr_true);
	EXPECT_TRUE(balr_true);
	auto lr_true = (&hlasm_plugin::parser_library::context::instruction::machine_instructions.at(lr_name))->get()->check(lr_name, test_lr_true);
	EXPECT_TRUE(lr_true);
	auto mvcl_true = (&hlasm_plugin::parser_library::context::instruction::machine_instructions.at(mvcl_name))->get()->check(mvcl_name, test_mvcl_true);
	EXPECT_TRUE(mvcl_true);
	auto xr_true = (&hlasm_plugin::parser_library::context::instruction::machine_instructions.at(xr_name))->get()->check(xr_name, test_xr_true);
	EXPECT_TRUE(xr_true);
}

#endif
