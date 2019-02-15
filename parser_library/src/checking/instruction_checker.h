#ifndef HLASMPLUGIN_PARSERLIBRARY_INSTRUCTION_CHECKER_H
#define HLASMPLUGIN_PARSERLIBRARY_INSTRUCTION_CHECKER_H

#include <map>

#include "assembler_instruction.h"

namespace hlasm_plugin
{
namespace parser_library
{
namespace checking
{
	class assembler_instruction_checker
	{
	public:
		assembler_instruction_checker();
		bool check(const std::string& instruction_name, const std::vector<hlasm_plugin::parser_library::checking::one_operand*>& operand_vector) const;

		std::vector<diagnostic_op *> get_diagnostics();
		void clear_diagnostics();
	protected:
		std::map <std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>> assembler_instruction_map;
		void initialize_assembler_map();
	};

	class machine_instruction_checker
	{
	public:
		machine_instruction_checker();
		
		const std::vector<diagnostic_op> & get_diagnostics();
		void clear_diagnostic();

		bool mach_instr_check(const std::string & instruction_name, const std::vector<one_operand*>& operand_vector);
	private:
		std::vector<diagnostic_op> diagnostic;
		int find_instruction_index(const std::string & instruction_name);
		bool resolve_operand(const context::machine_instruction & current_instruction, const address_operand* curr_operand, size_t it);
	};


}
}
}

#endif
