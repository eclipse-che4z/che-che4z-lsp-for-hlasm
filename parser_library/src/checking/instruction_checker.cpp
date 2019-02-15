#include <map>

#include "instruction_checker.h"

namespace hlasm_plugin
{
namespace parser_library
{
namespace checking
{
	assembler_instruction_checker::assembler_instruction_checker()
	{
		initialize_assembler_map();
	}

	bool assembler_instruction_checker::check(const std::string & instruction_name, const std::vector<one_operand*>& operand_vector) const
	{
		try 
		{
			return assembler_instruction_map.at(instruction_name)->check(operand_vector);
		}
		catch (...)
		{
			return false;
		}
	}

	std::vector<diagnostic_op *> assembler_instruction_checker::get_diagnostics()
	{
		std::vector<diagnostic_op *> diags;
		for (auto& [key, instr] : assembler_instruction_map)
		{
			diags.push_back(&instr->diagnostic);
		}
		return diags;
	}

	void assembler_instruction_checker::clear_diagnostics()
	{
		for (auto &[key, instr] : assembler_instruction_map)
		{
			instr->diagnostic = diagnostic_op::error_NOERR();
		}
	}

	void assembler_instruction_checker::initialize_assembler_map()
	{
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"*PROCESS",
				std::make_unique<hlasm_plugin::parser_library::checking::process>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::NO_LABEL}, "*PROCESS")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"ACONTROL",
				std::make_unique<hlasm_plugin::parser_library::checking::acontrol>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
					hlasm_plugin::parser_library::checking::label_types::OPTIONAL}, "ACONTROL")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"ADATA",
				std::make_unique<hlasm_plugin::parser_library::checking::adata>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
					hlasm_plugin::parser_library::checking::label_types::OPTIONAL}, "ADATA")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"AINSERT",
				std::make_unique<hlasm_plugin::parser_library::checking::ainsert>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL,
					hlasm_plugin::parser_library::checking::label_types::OPTIONAL}, "AINSERT")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"ALIAS",
				std::make_unique<hlasm_plugin::parser_library::checking::alias>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
					hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL}, "ALIAS")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"AMODE",
				std::make_unique<hlasm_plugin::parser_library::checking::amode>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::NAME}, "AMODE")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"CATTR",
				std::make_unique<hlasm_plugin::parser_library::checking::cattr>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::CLASS_NAME}, "CATTR")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"CCW",
				std::make_unique<hlasm_plugin::parser_library::checking::ccw>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL, hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL, hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL}, "CCW")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"CCW0",
				std::make_unique<hlasm_plugin::parser_library::checking::ccw>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL, hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL, hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL}, "CCW0")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"CCW1",
				std::make_unique<hlasm_plugin::parser_library::checking::ccw>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL, hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL, hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL}, "CCW1")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"CEJECT",
				std::make_unique<hlasm_plugin::parser_library::checking::expression_instruction>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL}, "CEJECT")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"CNOP",
				std::make_unique<hlasm_plugin::parser_library::checking::cnop>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL, hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL, hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL}, "CNOP")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"COM",
				std::make_unique<hlasm_plugin::parser_library::checking::no_operands>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL, hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL, hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL}, "COM")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"COPY",
				std::make_unique<hlasm_plugin::parser_library::checking::copy>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL}, "COPY")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"COPY",
				std::make_unique<hlasm_plugin::parser_library::checking::copy>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL}, "COPY")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"CSECT",
				std::make_unique<hlasm_plugin::parser_library::checking::no_operands>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL, hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL, hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL}, "CSECT")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"CXD",
				std::make_unique<hlasm_plugin::parser_library::checking::no_operands>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL, hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL, hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL}, "CXD")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"DC",
				std::make_unique<hlasm_plugin::parser_library::checking::data>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL, hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL, hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL}, "DC")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"DROP",
				std::make_unique<hlasm_plugin::parser_library::checking::drop>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL}, "DROP")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"DS",
				std::make_unique<hlasm_plugin::parser_library::checking::data>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL, hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL, hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL}, "DS")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"DSECT",
				std::make_unique<hlasm_plugin::parser_library::checking::no_operands>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL, hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL, hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL}, "DSECT")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"DXD",
				std::make_unique<hlasm_plugin::parser_library::checking::data>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL, hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL, hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL}, "DXD")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"EJECT",
				std::make_unique<hlasm_plugin::parser_library::checking::no_operands>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL}, "EJECT")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"END",
				std::make_unique<hlasm_plugin::parser_library::checking::end>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL}, "END")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"ENTRY",
				std::make_unique<hlasm_plugin::parser_library::checking::entry>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL}, "ENTRY")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"EQU",
				std::make_unique<hlasm_plugin::parser_library::checking::equ>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
					hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL}, "EQU")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"EXITCTL",
				std::make_unique<hlasm_plugin::parser_library::checking::exitctl>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL}, "EXITCTL")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"EXTRN",
				std::make_unique<hlasm_plugin::parser_library::checking::external>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL}, "EXTRN")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"ICTL",
				std::make_unique<hlasm_plugin::parser_library::checking::ictl>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::NO_LABEL}, "ICTL")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"ISEQ",
				std::make_unique<hlasm_plugin::parser_library::checking::iseq>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL}, "ISEQ")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"LOCTR",
				std::make_unique<hlasm_plugin::parser_library::checking::no_operands>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
					hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL}, "LOCTR")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"LTORG",
				std::make_unique<hlasm_plugin::parser_library::checking::no_operands>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
					hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL, hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL, hlasm_plugin::parser_library::checking::label_types::OPTIONAL}, "LTORG")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"MNOTE",
				std::make_unique<hlasm_plugin::parser_library::checking::mnote>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL}, "MNOTE")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"OPSYN",
				std::make_unique<hlasm_plugin::parser_library::checking::opsyn>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
					hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL, hlasm_plugin::parser_library::checking::label_types::OPERATION_CODE}, "OPSYN")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"ORG",
				std::make_unique<hlasm_plugin::parser_library::checking::org>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
					hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL, hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL, hlasm_plugin::parser_library::checking::label_types::OPTIONAL}, "ORG")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"POP",
				std::make_unique<hlasm_plugin::parser_library::checking::stack_instr>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL}, "POP")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"PRINT",
				std::make_unique<hlasm_plugin::parser_library::checking::print>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL}, "PRINT")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"PUNCH",
				std::make_unique<hlasm_plugin::parser_library::checking::punch>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL}, "PUNCH")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"PUSH",
				std::make_unique<hlasm_plugin::parser_library::checking::stack_instr>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL}, "PUSH")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"REPRO",
				std::make_unique<hlasm_plugin::parser_library::checking::no_operands>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL}, "REPRO")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"RMODE",
				std::make_unique<hlasm_plugin::parser_library::checking::rmode>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::NAME}, "RMODE")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"RSECT",
				std::make_unique<hlasm_plugin::parser_library::checking::no_operands>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL, hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL, hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL}, "RSECT")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"SPACE",
				std::make_unique<hlasm_plugin::parser_library::checking::expression_instruction>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL}, "SPACE")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"START",
				std::make_unique<hlasm_plugin::parser_library::checking::expression_instruction>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL, hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL, hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL}, "START")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"TITLE",
				std::make_unique<hlasm_plugin::parser_library::checking::title>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL, hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL, hlasm_plugin::parser_library::checking::label_types::STRING}, "TITLE")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"USING",
				std::make_unique<hlasm_plugin::parser_library::checking::using_instr>(std::vector<hlasm_plugin::parser_library::checking::label_types>{hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL, hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL, hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL}, "USING")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"WXTRN",
				std::make_unique<hlasm_plugin::parser_library::checking::external>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::OPTIONAL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL}, "WXTRN")
				));
		assembler_instruction_map.insert(std::pair < std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>>
			(
				"XATTR",
				std::make_unique<hlasm_plugin::parser_library::checking::xattr>(std::vector<hlasm_plugin::parser_library::checking::label_types>{ hlasm_plugin::parser_library::checking::label_types::ORD_SYMBOL,
					hlasm_plugin::parser_library::checking::label_types::SEQUENCE_SYMBOL, hlasm_plugin::parser_library::checking::label_types::VAR_SYMBOL}, "XATTR")
				));
	}

	machine_instruction_checker::machine_instruction_checker() {}

	int machine_instruction_checker::find_instruction_index(const std::string & instruction_name)
	{
		int i = 0;
		while (i < context::instruction::machine_instructions.size() && instruction_name != context::instruction::machine_instructions[i].name)
			i++;
		if (i == context::instruction::machine_instructions.size())
		{
			// machine instruction does not exist
			diagnostic.push_back(diagnostic_op::error_M031(instruction_name));
			return -1;
		}
		return i;
	}

	bool machine_instruction_checker::resolve_operand(const context::machine_instruction & current_instruction, const address_operand* curr_operand, size_t it)
	{
		if (!is_size_corresponding_int(12, curr_operand->displacement))
		{
			// displacement always needs to be 12 bit
			diagnostic.push_back(diagnostic_op::error_M020(current_instruction.name)); // wrong size of address operand parameters
			return false;
		}
		if (current_instruction.operands[it] == context::operand_format::DB)
		{
			// check instructions with DB format, need to be parsed in a displacement(base, -1) format
			// first_param is therefore base and second is -1, base needs to be 4bit
			if (!is_size_corresponding_int(4, curr_operand->first_par))
			{
				diagnostic.push_back(diagnostic_op::error_M020(current_instruction.name)); // wrong size of address operand parameters
				return false;
			}
			else if (curr_operand->second_par != -1)
			{
				diagnostic.push_back(diagnostic_op::error_M010(current_instruction.name)); // wrong format
				return false;
			}
		}
		else if (current_instruction.operands[it] == context::operand_format::DXB_4b || current_instruction.operands[it] == context::operand_format::DXB_8b)
		{
			// check instruction with DXB format, 
			// need to be parsed in displacement(index,-1), displacement(index,base) or displacement(0,base) format
			if (curr_operand->second_par != -1 && !is_size_corresponding_int(4, curr_operand->second_par))
			{
				//check the base, if if fails, wrong format
				diagnostic.push_back(diagnostic_op::error_M010(current_instruction.name)); // wrong format
				return false;
			}
			if ((current_instruction.operands[it] == context::operand_format::DXB_4b && !is_size_corresponding_int(4, curr_operand->first_par))
				|| (current_instruction.operands[it] == context::operand_format::DXB_8b && !is_size_corresponding_int(8, curr_operand->first_par))
				)
			{
				//therefore index has a wrong format
				diagnostic.push_back(diagnostic_op::error_M020(current_instruction.name)); // wrong size of address operand parameters
				return false;
			}
		}
		return true;
	}

	bool machine_instruction_checker::mach_instr_check(const std::string & instruction_name, const std::vector<one_operand*>& input)
	{
		int i = 0;
		if ((i = find_instruction_index(instruction_name)) == -1)
			return false;

		// otherwise instruction is found at the ith position
		context::machine_instruction current_instruction = context::instruction::machine_instructions[i];
		size_t operands_size = current_instruction.operands.size();

		if (!(input.size() == operands_size || (input.size() + 1 == operands_size && current_instruction.has_optional_operand)))
		{
			// wrong size of vector
			diagnostic.push_back(diagnostic_op::error_M030(instruction_name));
			return false;
		}

		for (size_t j = 0; j < input.size(); j++) // iterate through all required parameters
		{
			if (is_operand_address(input[j])) // resolve the address here
			{
				address_operand* curr_operand = (address_operand*)input[j];
				if (curr_operand->operand_identifier == "" && is_displacement_operand(context::instruction::machine_instructions[i].operands[j]))
				{
					if (curr_operand->state == address_state::RES_VALID)
						continue;
					else if (curr_operand->state == address_state::RES_INVALID)
					{
						// invalid address
						diagnostic.push_back(diagnostic_op::error_M040(instruction_name));
						return false;
					}
					else if (curr_operand->state == address_state::UNRES && !resolve_operand(current_instruction, curr_operand, j))
					{
						return false;
					}
				}
				else
				{
					diagnostic.push_back(diagnostic_op::error_M011(instruction_name)); // wrong format of machine instruction operand
					return false;
				}
			}
			else if (is_operand_simple(input[j]))
			{
				int size_of_operand = get_size_of_operand(current_instruction.operands[j]);
				if (size_of_operand == -1)
				{
					if ((current_instruction.operands[j] == context::operand_format::DB || current_instruction.operands[j] == context::operand_format::DXB_4b)
						&& is_positive_number(input[j]->operand_identifier))
					{
						diagnostic.push_back(diagnostic_op::warning_M041(instruction_name)); // wrong format of machine instruction operand
						continue;
					}
					else
					{
						diagnostic.push_back(diagnostic_op::error_M011(instruction_name)); // wrong format of machine instruction operand
						return false;
					}
				}
				if (!is_size_corresponding_str(size_of_operand, input[j]->operand_identifier))
				{
					diagnostic.push_back(diagnostic_op::error_M021(instruction_name)); // wrong size of machine instruction operand
					return false;
				}
			}
			else
			{
				diagnostic.push_back(diagnostic_op::error_M011(instruction_name)); // wrong format of machine instruction operand
				return false;
			}
		}
		return true;
	}

	const std::vector<diagnostic_op> & machine_instruction_checker::get_diagnostics()
	{
		return diagnostic;
	}

	void machine_instruction_checker::clear_diagnostic()
	{
		diagnostic.clear();
	}

}
}
}