#ifndef HLASMPLUGIN_PARSERLIBRARY_INSTR_CLASS_H
#define HLASMPLUGIN_PARSERLIBRARY_INSTR_CLASS_H

#include <iomanip>
#include <string>
#include <algorithm>
#include <vector>

#include "../diagnosable.h"
#include "checker_helper.h"
#include "instr_operand.h"

namespace hlasm_plugin
{
namespace parser_library
{
namespace checking
{

// defining label types before instruction, used as parameter in assembler_instruction class
enum label_types { SEQUENCE_SYMBOL, VAR_SYMBOL, ORD_SYMBOL, NAME, CLASS_NAME, NO_LABEL, OPTIONAL, OPERATION_CODE, STRING };

class assembler_instruction
{
public:
	const std::vector<label_types> allowed_types;
	const std::string name_of_instruction;
	std::vector<diagnostic_op> diagnostics;
	int min_operands;
	int max_operands; // maximum number of operands, if not specified, value is -1
	virtual bool check(const std::vector<const asm_operand*> & to_check) { (void)to_check; return true; };
	assembler_instruction(std::vector<label_types> allowed_types, std::string name_of_instruction, int min_operands, int max_operands) :
		allowed_types(allowed_types), name_of_instruction(name_of_instruction),
		min_operands(min_operands), max_operands(max_operands) {};
	assembler_instruction(): min_operands(0), max_operands(0) {};
	virtual ~assembler_instruction() {};

protected:

	const std::vector<std::string> rmode_options = { "24", "31", "64", "ANY" };

	void add_diagnostic(diagnostic_op diag);

	bool all_operands_simple(const std::vector<std::unique_ptr<asm_operand>>& input);

	bool is_param_in_vector(const std::string& parameter, const std::vector<std::string>& options);

	bool operands_size_corresponding(const std::vector<const asm_operand*>& to_check);

	// functions for checking complex operands
	bool check_compat_operands(const std::vector<std::unique_ptr<asm_operand>>& input, const std::string& instr_name, const std::string& op_name);

	bool check_flag_operand(const one_operand* input, const std::string& instr_name);

	bool check_process_flag_parameters(const std::vector<std::unique_ptr<asm_operand>>& input, const std::string& instr_name);

	bool check_optable_operands(const std::vector<std::unique_ptr<asm_operand>>& input, const std::string& instr_name);

	bool check_typecheck_operands(const std::vector<std::unique_ptr<asm_operand>>& input, const std::string& instr_name, const std::string op_name);

	// process instruction functions
	bool check_codepage_parameter(const std::string& input_str);

	bool check_fail_parameters(const std::vector<std::unique_ptr<asm_operand>>& input, const std::string& instr_name);

	bool check_first_machine_operand(const std::string& input_str);

	bool check_pcontrol_parameters(const std::vector<std::unique_ptr<asm_operand>>& input, const std::string& instr_name);

	bool check_using_parameters(const std::vector<std::unique_ptr<asm_operand>>& input, const std::string& instr_name);

	bool check_xref_parameters(const std::vector<std::unique_ptr<asm_operand>>& input, const std::string& instr_name);

	bool check_suprwarn_parameters(const std::vector<std::unique_ptr<asm_operand>>& input, const std::string instr_name, const std::string& op_name); 

	bool check_assembler_process_operand(const asm_operand* input);

};


}
}
}

#endif
