#ifndef HLASMPLUGIN_PARSERLIBRARY_DIAGNOSTIC_H
#define HLASMPLUGIN_PARSERLIBRARY_DIAGNOSTIC_H

#include <string>
#include <vector>

#include "shared/protocol.h"

namespace hlasm_plugin::parser_library
{

/*
diagnostic_op errors:
I000 - NONE
ASSEMBLER INSTR:
I01x - complexity of operands error
	- I010 - simple expected
	- I011 - complex expected
I02x - wrong format of operands error
	- I020 - wrong format of operands
I03x - wrong number of operands
	- I030 - number of operands is min x
	- I031 - number of operands is exactly x
	- I032 - number of operands is from x to y
	- I033 - number of operands is either x or y
MACHINE INSTR:
M01x - format problems
	- M010 - wrong format of address operand parameters
	- M011 - wrong format of machine instruction operand
	- M012 - wrong format of the last optional parameter
M02x - size checking problems
	- M020 - wrong size of address operand parameters
	- M021 - wrong size of machine instruction operand 
M03x - wrong instruction
	- M030 - wrong number of operands
	- M031 - machine instruction does not exist
M04x - validation problem
	- M040 - address operand is not valid
*/

struct diagnostic_op
{
	diagnostic_severity severity;
	std::string code;
	std::string message;
	diagnostic_op() {};
	diagnostic_op(diagnostic_severity severity, std::string code, std::string message) :
		severity(severity), code(std::move(code)), message(std::move(message)) {};
	diagnostic_op(diagnostic_op&& d) noexcept
		:severity(d.severity), code(std::move(d.code)), message(std::move(d.message)) {};
	diagnostic_op(const diagnostic_op&d) :
		severity(d.severity), code((d.code)), message((d.message)) {}

	diagnostic_op & operator= (diagnostic_op&& d) noexcept
	{
		code = std::move(d.code);
		message = std::move(d.message);
		severity = d.severity;
		return *this;
	}

	static std::string add_to_instruction_name_I02x(bool is_instruction);

	static std::string add_to_instruction_name_I03x(bool is_instruction);

	static diagnostic_op error_I010_simple_expected();

	static diagnostic_op error_I011_complex_expected();

	static diagnostic_op error_I020_format(std::string instruction_name, bool is_instruction);

	static diagnostic_op error_I030_minimum(std::string instruction_name, bool is_instruction, size_t min_params);

	static diagnostic_op error_I031_exact(std::string instruction_name, bool is_instruction, size_t number_of_params);

	static diagnostic_op error_I032_from_to(std::string instruction_name, bool is_instruction, size_t number_from, size_t number_to);

	static diagnostic_op error_I033_either(std::string instruction_name, bool is_instruction, int option_one, int option_two);

	static diagnostic_op error_NOERR();

	static bool is_error(const diagnostic_op & diag);

	static diagnostic_op error_M010(std::string instruction_name);

	static diagnostic_op error_M011(std::string instruction_name);

	static diagnostic_op error_M012(std::string instruction_name);

	static diagnostic_op error_M020(std::string instruction_name);

	static diagnostic_op error_M021(std::string instruction_name);

	static diagnostic_op error_M030(std::string instruction_name);

	static diagnostic_op error_M031(std::string instruction_name);

	static diagnostic_op error_M040(std::string instruction_name);
	static diagnostic_op warning_M041(std::string instruction_name);
	//static diagnostic_op error_MXXX(std::string instruction_name);
		
};


class diagnostic_related_info_s
{
public:
	diagnostic_related_info_s() {}
	diagnostic_related_info_s(position location, std::string message) : location(location), message(std::move(message)) {}
	position location;
	std::string message;
};

class diagnostic_s
{
public:
	diagnostic_s() {}
	diagnostic_s(std::string file_name, range range, std::string code, std::string message) :
		file_name(std::move(file_name)), diag_range(range), code(code), severity(diagnostic_severity::unspecified), message(std::move(message)) {}
	diagnostic_s(std::string file_name, range range, diagnostic_severity severity, std::string code, std::string source, std::string message, std::vector<diagnostic_related_info_s> related) :
		file_name(std::move(file_name)), diag_range(range), severity(severity), code(std::move(code)), source(std::move(source)), message(std::move(message)), related(std::move(related)) {}

	std::string file_name;
	range diag_range;
	diagnostic_severity severity;
	std::string code;
	std::string source;
	std::string message;
	std::vector<diagnostic_related_info_s> related;

	static diagnostic_s error_E010(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range);

	static diagnostic_s error_E011(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range);

	static diagnostic_s error_E012(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range);

	static diagnostic_s error_E013(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range);

	static diagnostic_s error_E020(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range);

	static diagnostic_s error_E021(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range);

	static diagnostic_s error_E022(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range);

	static diagnostic_s error_E030(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range);

	static diagnostic_s error_E031(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range);

	static diagnostic_s warning_W010(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range);

	static diagnostic_s error_EQU1(const std::string& filename, hlasm_plugin::parser_library::range range);

	static diagnostic_s error_EQU2(const std::string& filename, hlasm_plugin::parser_library::range range);

/*
E01x - wrong format
- E010 - unknown name
- E011 - operand/param already exist and are defined
- E012 - wrong format (exists but contains space etc) 
- E013 - inconsistent format (with instruction, operation etc)

E02x - operand/parameter/instruction number issues
- E020 - operands number problem (too many)
- E021 - operands number problem (too little)
- E022 - missing operand 

E03x - runtime problems
- E030 - assigment not allowed
- E031 - naming problem - name already exists

W01x - wrong format
- W010 - unexpected field/name/instr

*/

};

}

#endif
