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
		bool check(const std::string& instruction_name, const std::vector<const hlasm_plugin::parser_library::checking::asm_operand*>& operand_vector) const;

		std::vector<diagnostic_op *> get_diagnostics();
		void clear_diagnostics();
	protected:
		std::map <std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>> assembler_instruction_map;
		void initialize_assembler_map();
	};

}
}
}

#endif
