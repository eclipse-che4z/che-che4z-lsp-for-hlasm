#ifndef HLASMPLUGIN_PARSERLIBRARY_INSTRUCTION_CHECKER_H
#define HLASMPLUGIN_PARSERLIBRARY_INSTRUCTION_CHECKER_H

#include <map>

#include "asm_instr_check.h"

namespace hlasm_plugin{
namespace parser_library{
namespace checking{

//interface for unified checking
class instruction_checker
{
public:
	virtual bool check(const std::string& instruction_name, const std::vector<const operand*>& operand_vector, const range & stmt_range, const diagnostic_collector& add_diagnostic) const = 0;
};

class assembler_checker : public instruction_checker

{
public:
	assembler_checker();
	virtual bool check(const std::string& instruction_name, const std::vector<const operand*>& operand_vector, const range& stmt_range, const diagnostic_collector& add_diagnostic) const override;
	static std::map <std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>> assembler_instruction_map;
protected:
	void initialize_assembler_map();
};

class machine_checker : public instruction_checker
{
public:
	virtual bool check(const std::string& instruction_name, const std::vector<const operand*>& operand_vector, const range& stmt_range, const diagnostic_collector& add_diagnostic) const override;
};

}
}
}

#endif
