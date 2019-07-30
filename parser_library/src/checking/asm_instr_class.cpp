#include "asm_instr_class.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::checking;


bool assembler_instruction::all_operands_simple(const std::vector<std::unique_ptr<asm_operand>>& input)
{
	for (const auto& operand : input)
	{
		if (!is_operand_simple(operand.get()))
			return false;
	}
	return true;
}

void assembler_instruction::add_diagnostic(diagnostic_op diag)
{
	diagnostics.push_back(std::move(diag));
}

bool assembler_instruction::is_param_in_vector(const std::string& parameter, const std::vector<std::string>& options)
{
	return std::find(options.cbegin(), options.cend(), parameter) != options.cend();
}

bool assembler_instruction::is_param_in_vector(int parameter, const std::vector<int>& options)
{
	return std::find(options.cbegin(), options.cend(), parameter) != options.cend();
}

bool assembler_instruction::operands_size_corresponding(const std::vector<const asm_operand*>& to_check)
{
	if ((int)to_check.size() >= min_operands && ((int)to_check.size() <= max_operands || max_operands == -1))
		return true;
	if (max_operands == -1)
		add_diagnostic(diagnostic_op::error_A010_minimum(name_of_instruction, min_operands));
	else if (min_operands == max_operands)
		add_diagnostic(diagnostic_op::error_A011_exact(name_of_instruction, min_operands));
	else if (max_operands - min_operands == 1)
		add_diagnostic(diagnostic_op::error_A013_either(name_of_instruction, min_operands, max_operands));
	else
		add_diagnostic(diagnostic_op::error_A012_from_to(name_of_instruction, min_operands, max_operands));
	return false;
}

// functions for checking complex operands
bool assembler_instruction::check_compat_operands(const std::vector<std::unique_ptr<asm_operand>>& input, const std::string& instr_name, const std::string& op_name)
{
	//vector of keywords that can be used in compat option
	const static std::vector<std::string> keywords = { "NOCASE", "NOLITTYPE", "NOMACROCASE", "NOSYSLIST", "NOTRANSDT", "NOLIT", "NOMC", "NOSYSL", "NOTRS", "CASE", "LITTYPE", "MACROCASE", "SYSLIST", "TRANSDT", "LIT", "MC", "SYSL", "TRS" };
	for (const auto& operand : input)
	{
		auto simple_op = get_simple_operand(operand.get());
		if (simple_op == nullptr || !is_param_in_vector(simple_op->operand_identifier, keywords))
		{
			add_diagnostic(diagnostic_op::error_A209_COMPAT_param_format(instr_name));
			return false;
		}
	}
	return true;
}

bool assembler_instruction::check_flag_operand(const one_operand* input, const std::string& instr_name)
{
	//vector of keywords that can be used in flag option
	const static std::vector<std::string> flag_operands = { "NOALIGN", "NOCONT", "NOEXLITW", "NOIMPLEN", "NOPAGE0", "NOSUBSTR", "NOUSING0", "NOAL", "NOSUB", "NOUS0", "ALIGN", "CONT", "EXLITW", "IMPLEN", "PAGE0", "SUBSTR", "USING0", "AL", "SUB", "US0" };
	//either param can be in flag_operands vector or is an integer
	if (is_param_in_vector(input->operand_identifier, flag_operands))
		return true;
	if (has_all_digits(input->operand_identifier) && !input->is_default)
	{
		if (input->value < 0 || input->value > 255)
		{
			add_diagnostic(diagnostic_op::error_A210_FLAG_integer_size(instr_name));
			return false;
		}
		return true;
	}
	add_diagnostic(diagnostic_op::error_A211_FLAG_op_format(instr_name));
	return false;
}

bool assembler_instruction::check_process_flag_parameters(const std::vector<std::unique_ptr<asm_operand>>& input, const std::string& instr_name)
{
	const static std::vector<std::string> other_pair_options = { "NOPUSH", "NORECORD", "NOPU", "NORC", "PUSH", "RECORD", "PU", "RC" };
	for (size_t i = 0; i < input.size(); i++)
	{
		auto simple_op = get_simple_operand(input[i].get());
		if (simple_op == nullptr)
		{
			add_diagnostic(diagnostic_op::error_A211_FLAG_op_format(instr_name));
			return false;
		}
		if (!check_flag_operand(simple_op, instr_name) && !is_param_in_vector(simple_op->operand_identifier, other_pair_options))
			return false;
	}
	return true;
}

bool assembler_instruction::check_optable_operands(const std::vector<std::unique_ptr<asm_operand>>& input, const std::string& instr_name)
{
	if (input.size() > 2)
		assert(false);
	//declare array of options for first parameter
	const static std::array<std::string, 13> optable_array = { "DOS", "ESA", "XA", "370", "YOP", "ZOP", "ZS3", "ZS4", "ZS5", "ZS6", "ZS7", "ZS8" };
	auto first = get_simple_operand(input[0].get());
	const one_operand* second = nullptr;
	if (input.size() == 2)
		second = get_simple_operand(input[1].get());
	//check first parameter
	if (first == nullptr || std::find(optable_array.cbegin(), optable_array.cend(), first->operand_identifier) == optable_array.end())
	{
		// first parameter was wrong
		add_diagnostic(diagnostic_op::error_A212_OPTABLE_first_op(instr_name));
		return false;
	}
	// check second parameter
	if (input.size() == 2 && (second == nullptr || (second->operand_identifier != "LIST" && second->operand_identifier != "NOLIST")))
	{
		add_diagnostic(diagnostic_op::error_A213_OPTABLE_second_op(instr_name));
		return false;
	}
	return true;
}

bool assembler_instruction::check_typecheck_operands(const std::vector<std::unique_ptr<asm_operand>>& input, const std::string& instr_name, const std::string op_name)
{
	const static std::vector<std::string> typecheck_operands = { "MAGNITUDE", "REGISTER", "MAG", "REG", "NOMAGNITUDE", "NOREGISTER", "NOMAG", "NOREG" };
	for (const auto& operand : input)
	{
		// convert to simple
		auto simple_op = get_simple_operand(operand.get());
		if (simple_op == nullptr || !is_param_in_vector(simple_op->operand_identifier, typecheck_operands))
		{
			add_diagnostic(diagnostic_op::error_A214_TYPECHECK_format(instr_name));
			return false;
		}
	}
	return true;
}

// process instruction functions
bool assembler_instruction::check_codepage_parameter(const std::string& input_str)
{
	//hexa value
	if (input_str.front() == 'X')
	{
		if (input_str.size() < 3 || input_str[1] != '\'' || input_str.back() != '\'')
		{
			add_diagnostic(diagnostic_op::error_A215_CODEPAGE_format(name_of_instruction));
			return false;
		}
		// get value 
		std::string value = "";
		for (size_t i = 2; i < input_str.size() - 1; i++)
		{
			if (!isxdigit(input_str[i]))
			{
				add_diagnostic(diagnostic_op::error_A215_CODEPAGE_format(name_of_instruction));
				return false;
			}
			value += input_str[i];
		}
		try
		{
			int val_hexa = std::stoul(value, nullptr, 16);
			int min_value = 0x0474;
			int max_value = 0x047C;
			if (val_hexa < min_value || val_hexa > max_value)
			{
				add_diagnostic(diagnostic_op::error_A216_CODEPAGE_value(name_of_instruction));
				return false;
			}
		}
		catch (...)
		{
			assert(false);
		}
	}
	//decimal value
	else
	{
		if (!std::all_of(input_str.cbegin(), input_str.cend(), ::isdigit))
		{
			add_diagnostic(diagnostic_op::error_A215_CODEPAGE_format(name_of_instruction));
			return false;
		}
		auto val = std::stoi(input_str);
		if (val < 1140 || val > 1148)
		{
			add_diagnostic(diagnostic_op::error_A216_CODEPAGE_value(name_of_instruction));
			return false;
		}
	}
	return true;
}

bool assembler_instruction::check_fail_parameters(const std::vector<std::unique_ptr<asm_operand>>& input, const std::string& instr_name)
{
	for (const auto& operand : input)
	{
		// check simple parameters 
		if (is_operand_simple(operand.get()))
		{
			auto simple_op = get_simple_operand(operand.get());
			if (simple_op->operand_identifier == "NOMSG" || simple_op->operand_identifier == "NOMNOTE")
				continue;
			add_diagnostic(diagnostic_op::error_A233_FAIL_param_format(instr_name));
			return false;
		}
		else if (is_operand_complex(operand.get()))
		{
			complex_operand* current_operand = (complex_operand*)operand.get();
			if (current_operand->operand_identifier != "MNOTE" && current_operand->operand_identifier != "MSG" ||
				current_operand->operand_identifier != "MAXERRS" && current_operand->operand_identifier != "NOMAXERRS")
			{
				add_diagnostic(diagnostic_op::error_A233_FAIL_param_format(instr_name));
				return false;
			}
			if (current_operand->operand_parameters.size() != 1 || !is_operand_simple(current_operand->operand_parameters[0].get()))
			{
				add_diagnostic(diagnostic_op::error_A234_FAIL_complex_param_no(instr_name, current_operand->operand_identifier));
				return false;
			}
			auto ident = get_simple_operand(current_operand->operand_parameters[0].get());
			int ident_val = 0;
			try
			{
				ident_val = std::stoi(ident->operand_identifier);
			}
			catch (...)
			{
				add_diagnostic(diagnostic_op::error_A235_FAIL_param_number_format(instr_name, current_operand->operand_identifier));
				return false;
			}
			if (current_operand->operand_identifier == "MNOTE" || current_operand->operand_identifier == "MSG")
			{
				if (ident_val < 0 || ident_val > 7)
				{
					add_diagnostic(diagnostic_op::error_A237_FAIL_severity_message(instr_name, current_operand->operand_identifier));
					return false;
				}
			}
			else if (current_operand->operand_identifier == "MAXERRS" || current_operand->operand_identifier == "NOMAXERRS")
			{
				if (ident_val < 32 || ident_val > 65535)
				{
					add_diagnostic(diagnostic_op::error_A236_FAIL_MAXXERS_value(instr_name, current_operand->operand_identifier));
					return false;
				}
			}
			else
			{
				add_diagnostic(diagnostic_op::error_A233_FAIL_param_format(instr_name));
				return false;
			}
		}
		else
		{
			add_diagnostic(diagnostic_op::error_A021_cannot_be_empty(name_of_instruction));
			return false;
		}
	}
	return true;
}

bool assembler_instruction::check_first_machine_operand(const std::string& input_str)
{
	const static std::vector<std::string> machine_operand_options = { "S370", "S370XA", "S370ESA", "S390", "S390E", "ZSERIES", "ZS", "ZSERIES-2",
		"ZS-2", "ZSERIES-3", "ZS-3", "ZSERIES-4", "ZS-4", "ZSERIES-5", "ZS-5", "ZSERIES-6", "ZS-6", "ZSERIES-7", "ZS-7", "ZSERIES-8", "ZS-8" };
	return is_param_in_vector(input_str, machine_operand_options);
}

bool assembler_instruction::check_pcontrol_parameters(const std::vector<std::unique_ptr<asm_operand>>& input, const std::string& instr_name)
{
	const static std::vector<std::string> pcontrol_pair_options = { "DATA", "GEN", "MCALL", "MSOURCE", "UHEAD", "MC", "MS", "UHD", "NODATA", "NOGEN", "NOMCALL", "NOMSOURCE", "NOUHEAD", "NOMC", "NOMS", "NOUHD" };
	for (const auto& operand : input)
	{
		auto simple = get_simple_operand(operand.get());
		if (simple == nullptr)
			return false;
		if (!is_param_in_vector(simple->operand_identifier, pcontrol_pair_options) && simple->operand_identifier != "ON" && simple->operand_identifier != "OFF")
		{
			add_diagnostic(diagnostic_op::error_A223_PCONTROL_par_format(instr_name));
			return false;
		}
	}
	return true;
}

bool assembler_instruction::check_using_parameters(const std::vector<std::unique_ptr<asm_operand>>& input, const std::string& instr_name)
{
	for (const auto& operand : input)
	{
		//check simple operands
		if (is_operand_simple(operand.get()))
		{
			auto simple_op = get_simple_operand(operand.get());
			if (simple_op->operand_identifier != "MAP" && simple_op->operand_identifier != "NOMAP"
				&& simple_op->operand_identifier != "NOWARN" && simple_op->operand_identifier != "NOLIMIT")
			{
				add_diagnostic(diagnostic_op::error_A227_USING_format(instr_name));
				return false;
			}
		}
		else if (is_operand_complex(operand.get()))
		{
			complex_operand* current_operand = (complex_operand*)operand.get();
			if (current_operand->operand_identifier != "WARN" && current_operand->operand_identifier != "LIMIT")
			{
				add_diagnostic(diagnostic_op::error_A227_USING_format(instr_name));
				return false;
			}
			if (current_operand->operand_parameters.size() != 1 || !is_operand_simple(current_operand->operand_parameters[0].get()))
			{
				add_diagnostic(diagnostic_op::error_A228_USING_complex_param_no(instr_name, current_operand->operand_identifier));
				return false;
			}
			auto param = get_simple_operand(current_operand->operand_parameters[0].get());
			if (current_operand->operand_identifier == "WARN")
			{
				int number = 0;
				try
				{
					number = std::stoi(param->operand_identifier);
				}
				catch (...)
				{
					add_diagnostic(diagnostic_op::error_A228_USING_complex_param_no(instr_name,current_operand->operand_identifier));
					return false;
				}
				if (!is_byte_value(number))
				{
					add_diagnostic(diagnostic_op::error_A229_USING_WARN_format(instr_name));
					return false;
				}
			}
			else if (current_operand->operand_identifier == "LIMIT")
			{
				auto ident = param->operand_identifier;
				if (ident.size() >= 3 && ident.front() == 'X' && ident[1] == '\'' && ident.back() == '\'' && is_value_hexa(ident.substr(2, ident.size() - 3)))
				{
					// the value is specified in hexa
					if (std::stoul(ident.substr(2, ident.size() - 3), nullptr, 16) > 0xFFF)
					{
						add_diagnostic(diagnostic_op::error_A232_USING_LIMIT_hexa(instr_name));
						return false;
					}
				}
				else
				{
					int number = 0;
					try
					{
						number = std::stoi(ident);
					}
					catch (...)
					{
						// the value is in incorrect format
						add_diagnostic(diagnostic_op::error_A230_USING_LIMIT_format(instr_name));
						return false;
					};
					if (number > 4095)
					{
						// the number is specified in decimal but is not lower than the allowed value
						add_diagnostic(diagnostic_op::error_A231_USING_LIMIT_decimal(instr_name));
						return false;
					}

				}
			}
		}
		else
		{
			add_diagnostic(diagnostic_op::error_A021_cannot_be_empty(name_of_instruction));
			return false;
		}
	}
	return true;
}

bool assembler_instruction::check_xref_parameters(const std::vector<std::unique_ptr<asm_operand>>& input, const std::string& instr_name)
{
	auto first = get_simple_operand(input[0].get());
	if (first == nullptr)
	{
		add_diagnostic(diagnostic_op::error_A224_XREF_par_format(instr_name));
		return false;
	}
	if (input.size() == 1 && first->operand_identifier == "FULL")
		return true;
	for (const auto& operand : input)
	{
		auto simple_op = get_simple_operand(operand.get());
		if (simple_op == nullptr || (simple_op->operand_identifier != "SHORT" && simple_op->operand_identifier != "UNREFS"))
		{
			add_diagnostic(diagnostic_op::error_A224_XREF_par_format(instr_name));
			return false;
		}
	}
	return true;
}

bool assembler_instruction::check_suprwarn_parameters(const std::vector<std::unique_ptr<asm_operand>>& input, const std::string instr_name, const std::string& op_name)
{
	std::string option;
	if (op_name == "NOSUP")
		option = "NOSUPRWARN";
	else if (op_name == "SUP")
		option = "SUPRWARN";
	else option = op_name;
	for (const auto& operand : input)
	{
		auto simple_op = get_simple_operand(operand.get());
		if (simple_op == nullptr || !has_all_digits(simple_op->operand_identifier))
		{
			add_diagnostic(diagnostic_op::error_A225_SUPRWARN_par_format(instr_name, option));
			return false;
		}
		if (simple_op->operand_identifier.size() > 4 || simple_op->operand_identifier.empty())
		{
			add_diagnostic(diagnostic_op::error_A226_SUPRWARN_par_size(instr_name, option));
			return false;
		}
	}
	return true;
}

bool assembler_instruction::check_assembler_process_operand(const asm_operand* input)
{
	if (is_operand_simple(input)) //operand is simple
	{
		auto simple_op = get_simple_operand(input);
		const static std::vector<std::string> assembler_pair_options = { "ALIGN", "BATCH", "DBCS", "DXREF", "DX", "ERASE", "ESD", "FOLD", "ILMA", "PROFILE", "PROF",
			"INFO", "LIBMAC", "LMAC", "PRINT", "PR", "RA2", "RENT", "RLD", "RXREF", "RX", "SEG", "TEST", "THREAD", "THR", "WORKFILE", "MXREF", "MX",
			"NOALIGN", "NOBATCH", "NODBCS", "NODXREF", "NODX", "NOERASE", "NOESD", "NOFOLD", "NOILMA", "NOPROFILE", "NOPROF",
			"NOINFO", "NOLIBMAC", "NOLMAC", "NOPRINT", "NOPR", "NORA2", "NORENT", "NORLD", "NORXREF", "NORX", "NOSEG", "NOTEST", "NOTHREAD", "NOTHR", "NOWORKFILE", "NOMXREF", "NOMX" };
		const static std::vector<std::string> other_simple_options = { "NOCOMPAT", "NOCPAT", "DISK", "DI", "NOFAIL", "NOPCONTROL", "NOPC",
			"NOTYPECHECK", "NOTC", "NOUSING", "NOUS", "NOXREF", "PESTOP=YES", "PESTOP=NO", "NOSUPRWARN", "NOSUP" };
		if (!is_param_in_vector(simple_op->operand_identifier, assembler_pair_options) && !is_param_in_vector(simple_op->operand_identifier, other_simple_options))
		{
			add_diagnostic(diagnostic_op::error_A162_PROCESS_uknown_option(simple_op->operand_identifier));
			return false;
		}
	}
	else if (is_operand_complex(input)) //operand is complex, check all possible complex operand names
	{
		complex_operand* current_operand = (complex_operand*)input;
		const static std::vector<std::string> one_simple_param = { "CODEPAGE", "CP", "INFO", "MXREF", "MX", "SECTALGN", "PROFILE", "PROF" };
		const static std::vector<std::string> two_simple_param = { "MACHINE", "MAC", "OPTABLE", "OP" };
		const static std::vector<std::string> one_plus_simple_params = { "COMPAT", "CPAT", "FLAG", "PCONTROL", "PC",
			"XREF", "SUPRWARN" , "SUP", "NOSUPRWARN", "NOSUP", "TYPECHECK", "TC" };
		const static std::vector<std::string> complex_params = { "US", "FAIL", "USING" };
		// check whether the identifier is ok
		if (!is_param_in_vector(current_operand->operand_identifier, one_simple_param) && !is_param_in_vector(current_operand->operand_identifier, one_plus_simple_params)
			&& !is_param_in_vector(current_operand->operand_identifier, two_simple_param) && !is_param_in_vector(current_operand->operand_identifier, complex_params))
		{
			add_diagnostic(diagnostic_op::error_A162_PROCESS_uknown_option(current_operand->operand_identifier));
			return false;
		}
		// checking for individual options
		if (current_operand->operand_identifier == "FAIL")
		{
			if (!check_fail_parameters(current_operand->operand_parameters, name_of_instruction))
				return false;
			else
				return true;
		}
		else if (current_operand->operand_identifier == "USING" || current_operand->operand_identifier == "US")
		{
			if (!check_using_parameters(current_operand->operand_parameters, name_of_instruction))
				return false;
			else
				return true;
		}
		// options that must have only simple operands
		else
		{
			// check for all operands to be simple
			if (!all_operands_simple(current_operand->operand_parameters))
			{
				add_diagnostic(diagnostic_op::error_A002_simple_par_expected(name_of_instruction, current_operand->operand_identifier));
				return false;
			}
			// check for operands with only one parameter
			if (is_param_in_vector(current_operand->operand_identifier, one_simple_param))
			{
				// check for the number of parameters
				if (current_operand->operand_parameters.size() != 1)
				{
					if (current_operand->operand_identifier == "INFO")
						add_diagnostic(diagnostic_op::error_A018_either(name_of_instruction, current_operand->operand_identifier, 0, 1));
					else add_diagnostic(diagnostic_op::error_A016_exact(name_of_instruction, current_operand->operand_identifier, 1));
					return false;
				}
				one_operand* param = (one_operand*)(current_operand->operand_parameters[0].get());
				// check codepage option
				if (current_operand->operand_identifier == "CODEPAGE" || current_operand->operand_identifier == "CP")
				{
					if (!check_codepage_parameter(param->operand_identifier))
						return false;
				}
				// check info option
				else if (current_operand->operand_identifier == "INFO")
				{
					if (param->is_default || !is_date(param->operand_identifier))
					{
						add_diagnostic(diagnostic_op::error_A217_INFO_value(name_of_instruction));
						return false;
					}
				}
				// check mxref option
				else if (current_operand->operand_identifier == "MXREF" || current_operand->operand_identifier == "MX")
				{
					if (param->operand_identifier != "FULL" && param->operand_identifier != "SOURCE" && param->operand_identifier != "XREF")
					{
						add_diagnostic(diagnostic_op::error_A218_MXREF_format(name_of_instruction));
						return false;
					}
				}
				else if (current_operand->operand_identifier == "SECTALGN")
				{
					if (!has_all_digits(param->operand_identifier) || param->is_default)
					{
						add_diagnostic(diagnostic_op::error_A219_SECTALGN_par_format(name_of_instruction));
						return false;
					}
					if (!is_power_of_two(param->value) || param->value < 8 || param->value > 4096)
					{
						add_diagnostic(diagnostic_op::error_A220_SECTALGN_par_value(name_of_instruction));
						return false;
					}
				}
				// TO DO - PROFILE
			}
			else if (is_param_in_vector(current_operand->operand_identifier, two_simple_param)) // check for operands with two parameters
			{
				if (current_operand->operand_parameters.empty() || current_operand->operand_parameters.size() > 2)
				{
					add_diagnostic(diagnostic_op::error_A018_either(name_of_instruction, current_operand->operand_identifier, 1, 2));
					return false;
				}
				if (current_operand->operand_identifier == "MACHINE" || current_operand->operand_identifier == "MAC")
				{
					one_operand* first_param = (one_operand*)(current_operand->operand_parameters[0].get());
					if (!check_first_machine_operand(first_param->operand_identifier))
					{
						add_diagnostic(diagnostic_op::error_A222_MACH_first_par_format(name_of_instruction));
						return false;
					}
					if (current_operand->operand_parameters.size() == 2)
					{
						one_operand* second_param = (one_operand*)(current_operand->operand_parameters[1].get());
						if (second_param->operand_identifier != "LIST" && second_param->operand_identifier != "NOLIST")
						{
							add_diagnostic(diagnostic_op::error_A221_MACH_second_par_format(name_of_instruction));
							return false;
						}
					}
				}
				else if (current_operand->operand_identifier == "OPTABLE" || current_operand->operand_identifier == "OP")
				{
					if (!check_optable_operands(current_operand->operand_parameters, name_of_instruction))
						return false;
				}
				else
				{
					assert(false);
					add_diagnostic(diagnostic_op::error_I999(name_of_instruction));
					return false;
				}	
			}
			else // any number of parameters
			{
				if (current_operand->operand_parameters.empty())
				{
					add_diagnostic(diagnostic_op::error_A015_minimum(name_of_instruction, current_operand->operand_identifier, 1));
					return false;
				}
				if (current_operand->operand_identifier == "COMPAT" || current_operand->operand_identifier == "CPAT")
				{
					if (!check_compat_operands(current_operand->operand_parameters, name_of_instruction, current_operand->operand_identifier))
						return false;
				}
				else if (current_operand->operand_identifier == "TYPECHECK" || current_operand->operand_identifier == "TC")
				{
					if (!check_typecheck_operands(current_operand->operand_parameters, name_of_instruction, current_operand->operand_identifier))
						return false;
				}
				else if (current_operand->operand_identifier == "PCONTROL" || current_operand->operand_identifier == "PC")
				{
					if (!check_pcontrol_parameters(current_operand->operand_parameters, name_of_instruction))
						return false;
				}
				else if (current_operand->operand_identifier == "XREF")
				{
					if (!check_xref_parameters(current_operand->operand_parameters, name_of_instruction))
						return false;
				}
				else if (current_operand->operand_identifier == "SUPRWARN" || current_operand->operand_identifier == "SUP"
					|| current_operand->operand_identifier == "NOSUPRWARN" || current_operand->operand_identifier == "NOSUP")
				{
					if (!check_suprwarn_parameters(current_operand->operand_parameters, name_of_instruction, current_operand->operand_identifier))
						return false;
				}
				else if (current_operand->operand_identifier == "FLAG")
				{
					if (!check_process_flag_parameters(current_operand->operand_parameters, name_of_instruction))
						return false;
				}
				else
					assert(false);
			}
		}
	}
	else
	{
		add_diagnostic(diagnostic_op::error_A021_cannot_be_empty(name_of_instruction));
		return false;
	}
	return true;
}