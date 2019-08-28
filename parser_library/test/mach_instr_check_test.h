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

	one_operand op_val_0 = one_operand(0);
	one_operand op_val_2 = one_operand(2);
	one_operand op_val_4 = one_operand(4);
	one_operand op_val_11 = one_operand(11);
	one_operand op_val_12 = one_operand(12);
	one_operand op_val_13 = one_operand(13);
	one_operand op_val_14 = one_operand(14);
	one_operand op_val_15 = one_operand(15);

	diagnostic_collector collector;

	std::vector<const checking::machine_operand*> test_no_operand_true = std::vector<const checking::machine_operand*>();
	std::vector<const checking::machine_operand*> test_balr_true = std::vector<const checking::machine_operand*>();
	std::vector<const checking::machine_operand*> test_lr_true = std::vector<const checking::machine_operand*>();
	std::vector<const checking::machine_operand*> test_br_true = std::vector<const checking::machine_operand*>();
	std::vector<const checking::machine_operand*> test_mvcl_true = std::vector<const checking::machine_operand*>();
	std::vector<const checking::machine_operand*> test_xr_true = std::vector<const checking::machine_operand*>();
};

TEST_F(machine_instr_test, checker_test)
{
	std::string balr_name = "BALR";
	std::string lr_name = "LR";
	std::string mvcl_name = "MVCL";
	std::string xr_name = "XR";

	auto balr_true = (&hlasm_plugin::parser_library::context::instruction::machine_instructions.at(balr_name))->get()->check(balr_name, test_balr_true, range(), collector);
	EXPECT_TRUE(balr_true);
	auto lr_true = (&hlasm_plugin::parser_library::context::instruction::machine_instructions.at(lr_name))->get()->check(lr_name, test_lr_true, range(), collector);
	EXPECT_TRUE(lr_true);
	auto mvcl_true = (&hlasm_plugin::parser_library::context::instruction::machine_instructions.at(mvcl_name))->get()->check(mvcl_name, test_mvcl_true, range(), collector);
	EXPECT_TRUE(mvcl_true);
	auto xr_true = (&hlasm_plugin::parser_library::context::instruction::machine_instructions.at(xr_name))->get()->check(xr_name, test_xr_true, range(), collector);
	EXPECT_TRUE(xr_true);

	(&hlasm_plugin::parser_library::context::instruction::machine_instructions.at(balr_name))->get()->clear_diagnostics();
	(&hlasm_plugin::parser_library::context::instruction::machine_instructions.at(lr_name))->get()->clear_diagnostics();
	(&hlasm_plugin::parser_library::context::instruction::machine_instructions.at(mvcl_name))->get()->clear_diagnostics();
	(&hlasm_plugin::parser_library::context::instruction::machine_instructions.at(xr_name))->get()->clear_diagnostics();
}

#endif
