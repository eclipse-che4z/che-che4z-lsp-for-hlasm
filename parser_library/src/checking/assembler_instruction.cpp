
#include "assembler_instruction.h"

namespace hlasm_plugin
{
namespace parser_library
{
namespace checking
{

xattr::xattr(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) : assembler_instruction(allowed_types, name_of_instruction, 1, -1) {};

bool xattr::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	for (const auto& operand : to_check)
	{
		if (is_operand_simple(operand)) //instruction can have only complex operands
		{
			diagnostic = diagnostic_op::error_I011_complex_expected();
			return false;
		}

		complex_operand* current_operand = (complex_operand*) operand;
		if (std::any_of(current_operand->operand_parameters.begin(), current_operand->operand_parameters.end(), [](auto inner) { return is_operand_complex(inner); }))
		{
			diagnostic = diagnostic_op::error_I010_simple_expected();
			return false;
		}

		if (current_operand->operand_identifier == "ATTRIBUTES" || current_operand->operand_identifier == "ATTR"
			|| current_operand->operand_identifier == "LINKAGE" || current_operand->operand_identifier == "LINK"
			|| current_operand->operand_identifier == "SCOPE" || current_operand->operand_identifier == "PSECT")
		{
			if (current_operand->operand_parameters.size() != 1)
			{
				diagnostic = diagnostic_op::error_I031_exact(name_of_instruction, false, 1);
				return false;
			}
			if (current_operand->operand_identifier == "SCOPE")
			{
				const static std::vector<std::string> scope_operands = { "SECTION", "MODULE", "LIBRARY", "IMPORT", "EXPORT", "S", "M", "L", "X" };
				if (!is_param_in_vector(current_operand->operand_parameters[0]->operand_identifier, scope_operands))
				{
					diagnostic = diagnostic_op::error_I020_format(current_operand->operand_identifier, false);
					return false;
				}
			}
			else if (current_operand->operand_identifier == "LINKAGE" || current_operand->operand_identifier == "LINK")
			{
				if (current_operand->operand_parameters[0]->operand_identifier != "OS" && current_operand->operand_parameters[0]->operand_identifier != "XPLINK")
				{
					diagnostic = diagnostic_op::error_I020_format(current_operand->operand_identifier, false);
					return false;
				}
			}
		}
		else if (current_operand->operand_identifier == "REFERENCE" || current_operand->operand_identifier == "REF")
		{
			if ((current_operand->operand_parameters.size() == 0) || (current_operand->operand_parameters.size() > 2))
			{
				diagnostic = diagnostic_op::error_I033_either(name_of_instruction, false, 1, 2);
				return false;
			}
			bool code_data_option = false;
			bool direct_option = false;
			for (const auto& parameter: current_operand->operand_parameters)
			{
				if ((parameter->operand_identifier == "DIRECT" || parameter->operand_identifier == "INDIRECT") && !direct_option)
					direct_option = true;
				else if ((parameter->operand_identifier == "DATA" || parameter->operand_identifier == "CODE") && !code_data_option)
					code_data_option = true;
				else
				{
					diagnostic = diagnostic_op::error_I020_format(current_operand->operand_identifier, false);
					return false;
				}
			}
		}
		else
		{
			diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
			return false;
		}
	}
	return true;
};

using_instr::using_instr(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 2, -1) {};

bool using_instr::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	//check first operand
	if (is_operand_complex(to_check[0])) //first operand must be therefore in the form of (base, end)
	{
		complex_operand* first_operand = (complex_operand*)to_check[0];
		if (first_operand->operand_identifier != "" || first_operand->operand_parameters.size() != 2 || !is_positive_value(first_operand->operand_parameters[0]->operand_identifier)
			|| !is_positive_value(first_operand->operand_parameters[1]->operand_identifier)
			|| std::stoi(first_operand->operand_parameters[1]->operand_identifier) <= std::stoi(first_operand->operand_parameters[0]->operand_identifier))
		{
			diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
			return false;
		}
		if (is_operand_complex(first_operand->operand_parameters[0]) || is_operand_complex(first_operand->operand_parameters[1]))
		{
			diagnostic = diagnostic_op::error_I010_simple_expected();
			return false;
		}
	}
	else //first operand specifies only base
	{
		if (!is_positive_value(to_check[0]->operand_identifier))
		{
			diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
			return false;
		}
	}
	//check other operands
	if (to_check.size() == 2) //therefore there can be either address or one base register
	{
		if (is_operand_complex(to_check[1]))
		{
			diagnostic = diagnostic_op::error_I010_simple_expected();
			return false;
		}
		if (!is_base_register(to_check[1]->operand_identifier) || !is_positive_value(to_check[1]->operand_identifier))
		{
			diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
			return false;
		}
	}
	else //all other parameters are base registers
	{
		for (size_t i = 1; i < to_check.size(); i++)
		{
			if (is_operand_complex(to_check[i]))
			{
				diagnostic = diagnostic_op::error_I010_simple_expected();
				return false;
			}
			if (!is_base_register(to_check[i]->operand_identifier))
			{
				diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
				return false;
			}
		}
	}
	return true;
}

title::title(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) : assembler_instruction(allowed_types, name_of_instruction, 1, 1) {};

bool title::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	if (is_operand_complex(to_check[0]))
	{
		diagnostic = diagnostic_op::error_I010_simple_expected();
		return false;
	}
	if (const auto& op_id = to_check[0]->operand_identifier; op_id.size() > 102 || op_id.size() < 3
		|| op_id.front() != '\'' || op_id.back() != '\'')
	{
		diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
		return false;
	}
	return true;
}

rmode::rmode(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 1, 1) {};

bool rmode::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	if (is_operand_complex(to_check[0]))
	{
		diagnostic = diagnostic_op::error_I010_simple_expected();
		return false;
	}
	const static std::vector<std::string> amode_options = { "24", "31", "64", "ANY" };
	if (is_param_in_vector(to_check[0]->operand_identifier, amode_options))
		return true;
	diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
	return false;
}

punch::punch(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 1, 1) {};

bool punch::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check)) //check number of operands
		return false;
	if (is_operand_complex(to_check[0])) //check complexity of operand
	{
		diagnostic = diagnostic_op::error_I010_simple_expected();
		return false;
	}
	if (to_check[0]->operand_identifier[0] != '\'' || to_check[0]->operand_identifier[to_check[0]->operand_identifier.size() - 1] != '\'')
	{
		diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
		return false;
	}
	int size_of_string = 0;
	size_t i = 0;
	for (const auto& op_id = to_check[0]->operand_identifier; i < op_id.size() - 1; ++size_of_string)
	{
		if (op_id[i] == '&' &&  op_id[i + 1] == '&')
			i += 2;
		else if (op_id[i] == '\'' &&  op_id[i + 1] == '\'')
			i += 2;
		else
			i++;
	}
	if (size_of_string <= 80)
		return true;
	diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
	return false;
}

print::print(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 1, -1) {};

bool print::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	const static std::vector<std::string> print_pair_operands = { "GEN", "DATA", "MCALL", "MSOURCE", "UHEAD", "NOGEN", "NODATA", "NOMCALL", "NOMSOURCE", "NOUHEAD" };
	const static std::vector<std::string> print_other_operands = { "ON", "OFF", "NOPRINT", "" };
	for (const auto & operand : to_check)
	{
		if (is_operand_complex(operand))
		{
			diagnostic = diagnostic_op::error_I010_simple_expected();
			return false;
		}
		if (!is_param_in_vector(operand->operand_identifier, print_pair_operands) && !is_param_in_vector(operand->operand_identifier, print_other_operands))
		{
			diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
			return false;
		}
	}
	return true;
}

stack_instr::stack_instr(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 1, 4) {};

bool stack_instr::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	bool acontrol_operand = false;
	bool print_operand = false;
	bool using_operand = false;
	size_t i = 0;
	for (const auto& operand : to_check)
	{
		if (is_operand_complex(operand))
		{
			diagnostic = diagnostic_op::error_I010_simple_expected();
			return false;
		}
		if (operand->operand_identifier == "ACONTROL" && !acontrol_operand)
			acontrol_operand = true;
		else if (operand->operand_identifier == "PRINT" && !print_operand)
			print_operand = true;
		else if (operand->operand_identifier == "USING" && !using_operand)
			using_operand = true;
		else if (operand->operand_identifier == "NOPRINT" && i == to_check.size() - 1)
			return true;
		else
		{
			diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
			return false;
		}
		i++;
	}
	return true;
}

org::org(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 0, 3) {};

bool org::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	for (const auto& operand : to_check)
	{
		if (is_operand_complex(operand)) //check complexity of operands
		{
			diagnostic = diagnostic_op::error_I010_simple_expected();
			return false;
		}
		if (!is_positive_number(operand->operand_identifier) && (operand->operand_identifier != "")) //check whether operands are numbers
		{
			diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
			return false;
		}
	}
	if (to_check.size() == 0)
		return true;
	else if ((to_check.size() >= 1 && to_check[0]->operand_identifier == "")
			|| (to_check.size() >= 2 && to_check[1]->operand_identifier != "" && is_number(to_check[1]->operand_identifier) &&
				(!is_power_of_two(std::stoi(to_check[1]->operand_identifier)) || std::stoi(to_check[1]->operand_identifier) > 4096 || std::stoi(to_check[1]->operand_identifier) < 2))
			|| (to_check.size() == 3 && to_check[2]->operand_identifier == ""))
	{
		diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
		return false;
	}
	else
		return true;
}

opsyn::opsyn(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 0, 1) {};

bool opsyn::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	if (to_check.size() == 1)
	{
		if (is_operand_complex(to_check[0]))
		{
			diagnostic = diagnostic_op::error_I010_simple_expected();
			return false;
		}
		//TO DO - check operation code parameter
		return true;
	}
	return true;
}

mnote::mnote(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 1, 2) {};

bool mnote::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	for (const auto& operand : to_check)
	{
		if (is_operand_complex(operand))
		{
			diagnostic = diagnostic_op::error_I010_simple_expected();
			return false;
		}
	}
	if (to_check.back()->operand_identifier[0] == '\'' && to_check.back()->operand_identifier.size() <= 1020
		&& to_check.back()->operand_identifier.back() == '\'')
	{
		if (to_check.size() == 1)
			return true;
		else if (to_check.size() == 2 && (to_check[0]->operand_identifier.size() + to_check[1]->operand_identifier.size() <= 1024)
			&& (to_check[0]->operand_identifier == "*" || to_check[0]->operand_identifier == "" || is_byte_value(to_check[0]->operand_identifier)))
			return true;
		else
		{
			diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
			return false;
		}
	}
	diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
	return false;
}

iseq::iseq(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 0, 2) {};

bool iseq::check(const std::vector<const one_operand*> & to_check)
{
	if (to_check.size() == 0)
		return true;
	else if (to_check.size() == 2)
	{
		if (is_operand_complex(to_check[0]) || is_operand_complex(to_check[1]))
		{
			diagnostic = diagnostic_op::error_I010_simple_expected();
			return false;
		}
		if (!is_number(to_check[0]->operand_identifier) || !is_number(to_check[1]->operand_identifier))
		{
			diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
			return false;
		}
		try
		{
			int left = std::stoi(to_check[0]->operand_identifier);
			int right = std::stoi(to_check[1]->operand_identifier);
			if (left >= 1 && left <= 80 && right >= 1 && right <= 80 && right >= left)
				return true;
			else
			{
				diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
				return false;
			}
		}
		catch (...)
		{
			diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
			return false;
		}
	}
	else
	{
		diagnostic = diagnostic_op::error_I033_either(name_of_instruction, true, 0, 2);
		return false;
	}
}

ictl::ictl(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 1, 3) {};

bool ictl::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	int begin = 1;
	int end = 72;
	int continuation = 16;
	for (const auto & operand : to_check)
	{
		if (is_operand_complex(operand))
		{
			diagnostic = diagnostic_op::error_I010_simple_expected();
			return false;
		}
		if (!is_positive_number(operand->operand_identifier))
		{
			diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
			return false;
		}
	}
	try {
		if (to_check.size() > 0)
			begin = std::stoi(to_check[0]->operand_identifier);
		if (to_check.size() > 1)
			end = std::stoi(to_check[1]->operand_identifier);
		if (to_check.size() > 2)
			continuation = std::stoi(to_check[2]->operand_identifier);
	}
	catch (...)
	{
		diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
		return false;
	}
	if (to_check.size() == 2 || end == 80)
		continuation = -1;
	if (check_ictl_parameters(begin, end, continuation))
		return true;
	diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
	return false;
}

external::external(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 1, -1) {};

bool external::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	else if (to_check.size() == 1 && is_operand_complex(to_check[0])) //check complexity of operands
	{
		complex_operand* part_operand = (complex_operand*)to_check[0];
		if (part_operand->operand_identifier == "PART")
		{
			for (const auto& parameter: part_operand->operand_parameters)
			{
				if (is_operand_complex(parameter))
				{
					diagnostic = diagnostic_op::error_I010_simple_expected();
					return false;
				}
				if (parameter->operand_identifier == "")
				{
					diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
					return false;
				}
			}
			return true;
		}
		else
		{
			diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
			return false;
		}
	}
	else //every item is an external symbol
	{
		for (const auto & operand : to_check)
		{
			if (is_operand_complex(operand))
			{
				diagnostic = diagnostic_op::error_I010_simple_expected();
				return false;
			}
			if (operand->operand_identifier == "")
			{
				diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
				return false;
			}
		}
		return true;
	}
}

exitctl::exitctl(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 2, 5) {};

bool exitctl::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	const static std::vector<std::string> exit_type = { "SOURCE", "LIBRARY", "LISTING", "PUNCH", "ADATA", "TERM", "OBJECT" };
	if (is_operand_complex(to_check[0]))
	{
		diagnostic = diagnostic_op::error_I010_simple_expected();
		return false;
	}
	if (!is_param_in_vector(to_check[0]->operand_identifier, exit_type))
	{
		diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
		return false;
	}
	for (size_t i = 1; i < to_check.size(); i++)
	{
		if (is_operand_complex(to_check[i]))
		{
			diagnostic = diagnostic_op::error_I010_simple_expected();
			return false;
		}
		if (!is_number(to_check[i]->operand_identifier))
		{
			diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
			return false;
		}
	}
	return true;
}

equ::equ(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 1, 5) {};

bool equ::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	for (const auto& operand : to_check) //check complexity of all operands
	{
		if (is_operand_complex(operand))
		{
			diagnostic = diagnostic_op::error_I010_simple_expected();
			return false;
		}
	}
	if (!is_number(to_check[0]->operand_identifier)
		|| (to_check.size() > 1 && (to_check[1]->operand_identifier.size() > 0)
			&& (!is_positive_number(to_check[1]->operand_identifier) || !is_number(to_check[1]->operand_identifier))) //checking length attribute value 
		||
		(to_check.size() > 2 && (to_check[2]->operand_identifier.size() > 0)
			&& (!is_positive_number(to_check[2]->operand_identifier) || !is_byte_value(to_check[2]->operand_identifier))) //checking type attribute value
		||
		(to_check.size() > 4 && !check_assembler_type_value(to_check[4]->operand_identifier))) //checking assembler type value
	{
		diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
		return false;
	}
	return true;
}

entry::entry(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 1, -1) {};

bool entry::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	for (const auto& operand : to_check)
	{
		if (is_operand_complex(operand))
		{
			diagnostic = diagnostic_op::error_I010_simple_expected();
			return false;
		}
		if (operand->operand_identifier == "")
		{
			diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
			return false;
		}
	}
	return true;
}

end::end(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 0, 2) {};

bool end::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	if (to_check.size() > 0) //there is at least one parameter
	{
		if (is_operand_complex(to_check[0])) //check complexity of first operand
		{
			diagnostic = diagnostic_op::error_I010_simple_expected();
			return false;
		}
		if (to_check.size() > 1) // therefore there is a second operand
		{
			if (is_operand_simple(to_check[1])) //check complexity of second operand
			{
				diagnostic = diagnostic_op::error_I011_complex_expected();
				return false;
			}
			complex_operand* language_operand = (complex_operand*)to_check[1];
			for (const auto& param : language_operand->operand_parameters)
			{
				if (is_operand_complex(param)) //check complexity of second operand
				{
					diagnostic = diagnostic_op::error_I010_simple_expected();
					return false;
				}
			}
			if (language_operand->operand_identifier != "" || language_operand->operand_parameters.size() != 3
				|| language_operand->operand_parameters[0]->operand_identifier.size() < 1 || language_operand->operand_parameters[0]->operand_identifier.size() > 10
				|| language_operand->operand_parameters[1]->operand_identifier.size() != 4 || language_operand->operand_parameters[2]->operand_identifier.size() != 5
				|| !is_positive_number(language_operand->operand_parameters[2]->operand_identifier)) //operand identifier needs to be empty
			{
				diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
				return false;
			}

		}
	}
	return true;
}

drop::drop(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 0, -1) {};

bool drop::check(const std::vector<const one_operand*> & to_check)
{
	for (const auto& operand : to_check)
	{
		if (is_operand_complex(operand))
		{
			diagnostic = diagnostic_op::error_I010_simple_expected();
			return false;
		}
		if (operand->operand_identifier.empty() || (!is_ord_symbol(operand->operand_identifier) && !is_var_symbol(operand->operand_identifier) && !is_base_register(operand->operand_identifier)))
		{
			diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
			return false;
		}
	}
	return true;
}

data::data(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 1, -1) {};

bool data::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	for (const auto& operand : to_check)
	{
		if (is_operand_complex(operand))
		{
			diagnostic = diagnostic_op::error_I010_simple_expected();
			return false;
		}
	}
	return true;
}

copy::copy(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 1, 1) {};

bool copy::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	if (is_operand_complex(to_check[0]))
	{
		diagnostic = diagnostic_op::error_I010_simple_expected();
		return false;
	}
	if (to_check[0]->operand_identifier == "")
	{
		diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
		return false;
	}
	return true;
}

cnop::cnop(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 2, 2) {};

bool cnop::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	if (is_operand_complex(to_check[0]) || is_operand_complex(to_check[1])) //check complexity of operands
	{
		diagnostic = diagnostic_op::error_I010_simple_expected();
		return false;
	}
	try
	{
		int byte = std::stoi(to_check[0]->operand_identifier);
		int boundary = std::stoi(to_check[1]->operand_identifier);
		if (byte % 2 == 1 || byte > boundary - 2 || boundary > 4096 || !is_power_of_two(boundary))
		{
			diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
			return false;
		}
	}
	catch (...)
	{
		diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
		return false;
	}
	return true;
}

ccw::ccw(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 4, 4) {};

bool ccw::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	else
	{
		for (const auto& operand : to_check)
		{
			if (is_operand_complex(operand))
			{
				diagnostic = diagnostic_op::error_I010_simple_expected();
				return false;
			}
		}
	}
	return true;
}

expression_instruction::expression_instruction(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 0, 1) {};

bool expression_instruction::check(const std::vector<const one_operand*> & to_check)
{
	if (to_check.size() == 0)
		return true;
	if (!check_vector_size(to_check))
		return false;
	if (is_operand_complex(to_check[0]))
	{
		diagnostic = diagnostic_op::error_I010_simple_expected();
		return false;
	}
	return true;
}

cattr::cattr(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) : assembler_instruction(allowed_types, name_of_instruction, 1, -1) {};

bool cattr::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	const static std::vector<std::string> simple_operands = { "DEFLOAD", "EXECUTABLE", "MOVABLE", "NOLOAD", "NOTEXECUTABLE", "NOTREUS", "READONLY", "REFR", "REMOVABLE", "RENT", "REUS" };
	for (const auto& operand : to_check)
	{
		if (is_operand_simple(operand)) //operand is simple
		{
			if (is_param_in_vector(operand->operand_identifier, simple_operands))
				continue;
			else
			{
				diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
				return false;
			}
		}
		else //operand is complex
		{
			complex_operand* current_operand = (complex_operand*)operand;
			if (current_operand->operand_parameters.size() != 1) //has to have only one operand
			{
				diagnostic = diagnostic_op::error_I031_exact(name_of_instruction, false, 1);
				return false;
			}
			if (is_operand_complex(current_operand->operand_parameters[0]))
			{
				diagnostic = diagnostic_op::error_I010_simple_expected();
				return false;
			}
			if (current_operand->operand_identifier == "RMODE")
			{
				const static std::vector<std::string> rmode_options = { "24", "31", "64", "ANY" };
				if (!is_param_in_vector(current_operand->operand_parameters[0]->operand_identifier, rmode_options))
				{
					diagnostic = diagnostic_op::error_I020_format(current_operand->operand_identifier, false);
					return false;
				}
			}
			else if (current_operand->operand_identifier == "ALIGN") //upon sending, an empty operand has to come in a vector (not an empty vector)
			{
				const static std::vector<std::string> align_options = { "0", "1", "2", "3", "4", "12", "" };
				if (!is_param_in_vector(current_operand->operand_parameters[0]->operand_identifier, align_options))
				{
					diagnostic = diagnostic_op::error_I020_format(current_operand->operand_identifier, false);
					return false;
				}
			}
			else if (current_operand->operand_identifier == "FILL")
			{
				if (!is_number(current_operand->operand_parameters[0]->operand_identifier) || !is_byte_value(current_operand->operand_parameters[0]->operand_identifier))
				{
					diagnostic = diagnostic_op::error_I020_format(current_operand->operand_identifier, false);
					return false;
				}
			}
			else if (current_operand->operand_identifier == "PART")
			{
				if (current_operand->operand_parameters[0]->operand_identifier.empty() || current_operand->operand_parameters[0]->operand_identifier.length() > 63)
				{
					diagnostic = diagnostic_op::error_I020_format(current_operand->operand_identifier, false);
					return false;
				}
			}
			else if (current_operand->operand_identifier == "PRIORITY")
			{
				if (!is_number(current_operand->operand_parameters[0]->operand_identifier) || !isdigit(current_operand->operand_parameters[0]->operand_identifier.at(0)))
				{
					diagnostic = diagnostic_op::error_I020_format(current_operand->operand_identifier, false);
					return false;
				}
			}
			else //operand name does not exist
			{
				diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
				return false;
			}
		}
	}
	return true;
}

amode::amode(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 1, 1) {};

bool amode::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	if (is_operand_complex(to_check[0]))
	{
		diagnostic = diagnostic_op::error_I010_simple_expected();
		return false;
	}
	const static std::vector<std::string> amode_options = { "24", "31", "64", "ANY", "ANY31", "ANY64" };
	if (!is_param_in_vector(to_check[0]->operand_identifier, amode_options))
	{
		diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
		return false;
	}
	return true;
}

alias::alias(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 1, 1) {};

bool alias::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	if (is_operand_complex(to_check[0]))
	{
		diagnostic = diagnostic_op::error_I010_simple_expected();
		return false;
	}
	if (to_check[0]->operand_identifier.size() < 3)
	{
		diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
		return false;
	}
	if (to_check[0]->operand_identifier[1] == '\'' && to_check[0]->operand_identifier[to_check[0]->operand_identifier.size() - 1] == '\'')
	{
		if (to_check[0]->operand_identifier[0] == 'C')
		{
			return true;
		}
		else if (to_check[0]->operand_identifier[0] == 'X' && to_check[0]->operand_identifier.size() % 2 == 1)
		{
			int max_value = 0xFE;
			int min_value = 0x42;
			for (size_t i = 2; i < to_check[0]->operand_identifier.size() - 1; i+=2)
			{
				std::string tocomp = "";
				tocomp.push_back(to_check[0]->operand_identifier[i]);
				tocomp.push_back(to_check[0]->operand_identifier[i + 1]);
				int comparing = 0;
				try
				{
					comparing = std::stoul(tocomp, nullptr, 16);
				}
				catch (...)
				{
					diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
					return false;
				}
				if (comparing < min_value || comparing > max_value)
				{
					diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
					return false;
				}
			}
			return true;
		}
		else
		{
			return false;
		}
	}
	diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
	return false;
}

ainsert::ainsert(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 2, 2) {};

bool ainsert::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	if (is_operand_complex(to_check[0]) || is_operand_complex(to_check[1])) //check complexity
	{
		diagnostic = diagnostic_op::error_I010_simple_expected();
		return false;
	}
	if (to_check[0]->operand_identifier.size() == 1
		|| to_check[0]->operand_identifier[0] != '\'' || to_check[0]->operand_identifier[to_check[0]->operand_identifier.size() - 1] != '\''
		) //check first parameter
	{
		diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
		return false;
	}
	if (to_check[1]->operand_identifier != "BACK" && to_check[1]->operand_identifier != "FRONT") //check second parameter
	{
		diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
		return false;
	}
	return true;
}

adata::adata(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 5, 5) {};

bool adata::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	//first four operands must be values or empty
	for (size_t i = 0; i < to_check.size() - 1; ++i)
	{
		//operand can't be complex (can't be casted to complex)
		if (is_operand_complex(to_check[i]))
		{
			diagnostic = diagnostic_op::error_I010_simple_expected();
			return false;
		}
		if (!(to_check[i]->operand_identifier == "" || is_number(to_check[i]->operand_identifier)))
		{
			diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
			return false;
		}
	}
	//last operand must be character_string
	if (is_operand_complex(to_check.back()))
	{
		diagnostic = diagnostic_op::error_I010_simple_expected();
		return false;
	}
	if (!(to_check.back()->operand_identifier == "" || is_character_string(to_check.back()->operand_identifier)))
	{
		diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
		return false;
	}
	return true;
}

no_operands::no_operands(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 0, 0) {};

bool no_operands::check(const std::vector<const one_operand*> & to_check)
{
	return check_vector_size(to_check);
}

process::process(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 1, -1) {};

bool process::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	if (to_check.size() == 1 && is_operand_complex(to_check[0]) && to_check[0]->operand_identifier == "OVERRIDE") //everything parsed is parameter of operand
	{
		complex_operand* process_operands = (complex_operand*)to_check[0];
		return std::all_of(process_operands->operand_parameters.cbegin(), process_operands->operand_parameters.cend(),
			[this](const auto& parameter) { return check_assembler_process_operand(parameter); });
	}
	else //everything is an operand
	{
		return std::all_of(to_check.cbegin(), to_check.cend(),
			[this](const auto& operand) { return check_assembler_process_operand(operand); });
	}
}

acontrol::acontrol(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction) :assembler_instruction(allowed_types, name_of_instruction, 1, -1) {};

bool acontrol::check(const std::vector<const one_operand*> & to_check)
{
	if (!check_vector_size(to_check))
		return false;
	for (const auto& operand : to_check)
	{
		if (is_operand_simple(operand)) //checking simple operands
		{
			const static std::vector<std::string> pair_option_vector = { "AFPR", "LIBMAC", "RA2", "LMAC", "NOAFPR", "NOLIBMAC", "NORA2", "NOLMAC" }; //possible simple operands to check, need to also check for NOCOMPAT and NOTYPECHECK
			if (is_param_in_vector(operand->operand_identifier, pair_option_vector) || (operand->operand_identifier == "NOCOMPAT"
				|| operand->operand_identifier == "NOTYPECHECK" || operand->operand_identifier == "NOCPAT" || operand->operand_identifier == "NOTC"))
				continue;
			else
			{
				diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
				return false;
			}
		}
		else //checking complex operands
		{
			//if operand is not simple, complex operands need to be parsed
			complex_operand* current_operand = (complex_operand*)operand;
			if (current_operand->operand_parameters.cbegin() == current_operand->operand_parameters.cend())
			{
				diagnostic = diagnostic_op::error_I030_minimum(name_of_instruction, false, 1);
				return false;
			}
			if (current_operand->operand_identifier == "COMPAT" || current_operand->operand_identifier == "CPAT")
			{
				if (!check_compat_operands(current_operand->operand_parameters))
					return false;
			}
			else if (current_operand->operand_identifier == "FLAG")
			{
				for (size_t j = 0; j < current_operand->operand_parameters.size(); j++)
				{
					if (!check_flag_operand(current_operand->operand_parameters[j]))
					{
						diagnostic = diagnostic_op::error_I020_format(current_operand->operand_identifier, false);
						return false;
					}
				}
			}
			else if (current_operand->operand_identifier == "OPTABLE")
			{
				if (current_operand->operand_parameters.size() > 2 || current_operand->operand_parameters.size() == 0)
				{
					diagnostic = diagnostic_op::error_I033_either(name_of_instruction, false, 1, 2);
					return false;
				}
				if (!check_optable_operands(current_operand->operand_parameters))
					return false;
			}
			else if (current_operand->operand_identifier == "TYPECHECK" || current_operand->operand_identifier == "TC")
			{
				if (!check_typecheck_operands(current_operand->operand_parameters))
					return false;
			}
			else
			{
				diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
				return false;
			}
		}
	}
	return true;
}


}
}
}
