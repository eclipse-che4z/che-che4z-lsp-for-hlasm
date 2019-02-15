#include <string>

#include "diagnostic.h"

namespace hlasm_plugin::parser_library
{

//diagnostic_op errors

//asembler instruction errors

std::string diagnostic_op::add_to_instruction_name_I02x(bool is_instruction)
{
	if (is_instruction)
		return " instruction operand";
	else
		return " option parameter";
}

std::string diagnostic_op::add_to_instruction_name_I03x(bool is_instruction)
{
	if (is_instruction)
		return " instruction ";
	else
		return " option ";
}

diagnostic_op diagnostic_op::error_I010_simple_expected()
{
	return diagnostic_op(diagnostic_severity::error, "I010", "Simple operand expected");
}

diagnostic_op diagnostic_op::error_I011_complex_expected()
{
	return diagnostic_op(diagnostic_severity::error, "I010", "Complex operand expected");
}

diagnostic_op diagnostic_op::error_I030_minimum(std::string instruction_name, bool is_instruction, size_t min_params)
{
	return diagnostic_op(diagnostic_severity::error, "I030", "Number of operands of " + instruction_name + add_to_instruction_name_I03x(is_instruction) +
		+ "has to be at least " + std::to_string(min_params));

}

diagnostic_op diagnostic_op::error_I031_exact(std::string instruction_name, bool is_instruction, size_t number_of_params)
{
	return diagnostic_op(diagnostic_severity::error, "I030", "Number of operands of " + instruction_name + add_to_instruction_name_I03x(is_instruction) +
		+ "has to be " + std::to_string(number_of_params));
}

diagnostic_op diagnostic_op::error_I032_from_to(std::string instruction_name, bool is_instruction, size_t number_from, size_t number_to)
{
	return diagnostic_op(diagnostic_severity::error, "I030", "Number of operands of " + instruction_name + add_to_instruction_name_I03x(is_instruction) +
		+ "has to be from " + std::to_string(number_from) + " to " + std::to_string(number_to));
}

diagnostic_op diagnostic_op::error_I033_either(std::string instruction_name, bool is_instruction, int option_one, int option_two)
{
	return diagnostic_op(diagnostic_severity::error, "I030", "Number of operands of " + instruction_name + add_to_instruction_name_I03x(is_instruction) +
		+"has to be either " + std::to_string(option_one) + " or " + std::to_string(option_two));
}

diagnostic_op diagnostic_op::error_I020_format(std::string instruction_name, bool is_instruction)
{
	return diagnostic_op(diagnostic_severity::error, "I020", "Wrong format of " + instruction_name + add_to_instruction_name_I02x(is_instruction));
}

diagnostic_op diagnostic_op::error_NOERR ()
{
	return diagnostic_op(diagnostic_severity::error, "NOERR", "No error found");
}

bool diagnostic_op::is_error(const diagnostic_op & diag)
{
	return diag.code != "NOERR";
}



//machine instruction errors

diagnostic_op hlasm_plugin::parser_library::diagnostic_op::error_M010(std::string instruction_name)
{
	return diagnostic_op(diagnostic_severity::error, "M010", "Error at " + instruction_name + " instruction: wrong format of address operands" );
}

diagnostic_op hlasm_plugin::parser_library::diagnostic_op::error_M011(std::string instruction_name)
{
	return diagnostic_op(diagnostic_severity::error, "M011", "Error at " + instruction_name + " instruction: wrong format of machine instruction operands");
}

diagnostic_op hlasm_plugin::parser_library::diagnostic_op::error_M012(std::string instruction_name)
{
	return diagnostic_op(diagnostic_severity::error, "M012", "Error at " + instruction_name + " instruction: wrong format of the last optional parameter");
}

diagnostic_op hlasm_plugin::parser_library::diagnostic_op::error_M020(std::string instruction_name)
{
	return diagnostic_op(diagnostic_severity::error, "M020", "Error at " + instruction_name + " instruction: wrong size of address operand parameters");
}

diagnostic_op hlasm_plugin::parser_library::diagnostic_op::error_M021(std::string instruction_name)
{
	return diagnostic_op(diagnostic_severity::error, "M021", "Error at " + instruction_name + " instruction: wrong size of machine instruction operands");
}

diagnostic_op hlasm_plugin::parser_library::diagnostic_op::error_M030(std::string instruction_name)
{
	return diagnostic_op(diagnostic_severity::error, "M030", "Error at " + instruction_name + " instruction: wrong number of operands");
}

diagnostic_op hlasm_plugin::parser_library::diagnostic_op::error_M031(std::string instruction_name)
{
	return diagnostic_op(diagnostic_severity::error, "M031", "Error at " + instruction_name + " : machine instruction does not exist");
}

diagnostic_op hlasm_plugin::parser_library::diagnostic_op::error_M040(std::string instruction_name)
{
	return diagnostic_op(diagnostic_severity::error, "M040", "Error at " + instruction_name + " instruction: address operand is not valid");
}

diagnostic_op diagnostic_op::warning_M041(std::string instruction_name)
{
	return diagnostic_op(diagnostic_severity::warning, "M041", "Warning at " + instruction_name + " non-standard address format.");
}


// diagnostic_s errors

diagnostic_s diagnostic_s::error_E010(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range)
{
	return diagnostic_s(filename, range, diagnostic_severity::error, "E010", "HLASM Plugin", "Unknown name of " + message, {});
}

diagnostic_s diagnostic_s::error_E011(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range)
{
	return diagnostic_s(filename, range, diagnostic_severity::error, "E011", "HLASM Plugin", message + " already specified", {});
}

diagnostic_s diagnostic_s::error_E012(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range)
{
	return diagnostic_s(filename, range, diagnostic_severity::error, "E012", "HLASM Plugin", "Wrong format: " + message, {});
}

diagnostic_s diagnostic_s::error_E013(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range)
{
	return diagnostic_s(filename, range, diagnostic_severity::error, "E013", "HLASM Plugin", "Inconsistent format: " + message, {});
}

diagnostic_s diagnostic_s::error_E020(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range)
{
	return diagnostic_s(filename, range, diagnostic_severity::error, "E020", "HLASM Plugin", "Error at " + message + " - too many operands", {});
}

diagnostic_s diagnostic_s::error_E021(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range)
{
	return diagnostic_s(filename, range, diagnostic_severity::error, "E021", "HLASM Plugin", "Error at " + message + " - operand number too low", {});
}

diagnostic_s diagnostic_s::error_E022(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range)
{
	return diagnostic_s(filename, range, diagnostic_severity::error, "E022", "HLASM Plugin", "Error at " + message + " - operand missing", {});
}

diagnostic_s diagnostic_s::error_E030(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range)
{
	return diagnostic_s(filename, range, diagnostic_severity::error, "E030", "HLASM Plugin", "Can't assign value to " + message, {});
}

diagnostic_s diagnostic_s::error_E031(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range)
{
	return diagnostic_s(filename, range, diagnostic_severity::error, "E031", "HLASM Plugin", "Cannot declare " + message + " with the same name", {});
}

diagnostic_s diagnostic_s::warning_W010(const std::string& filename, const std::string& message, hlasm_plugin::parser_library::range range)
{
	return diagnostic_s(filename, range, diagnostic_severity::error, "W010", "HLASM Plugin", message + " not expected", {});
}

diagnostic_s diagnostic_s::error_EQU1(const std::string& filename, hlasm_plugin::parser_library::range range)
{
	return diagnostic_s(filename, range, diagnostic_severity::error, "EQU1", "HLASM Plugin", "Constant redefinition", {});
}

diagnostic_s diagnostic_s::error_EQU2(const std::string& filename, hlasm_plugin::parser_library::range range)
{
	return diagnostic_s(filename, range, diagnostic_severity::error, "EQU2", "HLASM Plugin", "Label redefinition", {});
}

}
