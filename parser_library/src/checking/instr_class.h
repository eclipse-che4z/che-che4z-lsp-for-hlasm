#ifndef HLASMPLUGIN_PARSERLIBRARY_INSTR_CLASS_H
#define HLASMPLUGIN_PARSERLIBRARY_INSTR_CLASS_H

#include <iomanip>
#include <string>
#include <algorithm>

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
	diagnostic_op diagnostic;
	size_t min_operands;
	size_t max_operands; // maximum number of operands, if not specified, value is -1
	virtual bool check(const std::vector<const one_operand*> & to_check) { (void)to_check; return true; };
	assembler_instruction(std::vector<label_types> allowed_types, std::string name_of_instruction, int min_operands, int max_operands) :
		allowed_types(allowed_types), name_of_instruction(name_of_instruction), diagnostic(diagnostic_op::error_NOERR()),
		min_operands(min_operands), max_operands(max_operands) {};
	assembler_instruction() {};
	virtual ~assembler_instruction() {};

protected:

	static bool is_param_in_vector(const std::string& parameter, const std::vector<std::string>& options)
	{
		return std::find(options.cbegin(), options.cend(), parameter) != options.cend();
	}

	bool check_vector_size(const std::vector<const one_operand*> & to_check)
	{
		if (to_check.size() >= min_operands && (to_check.size() <= max_operands || max_operands == -1))
			return true;

		if (max_operands == -1)
			diagnostic = diagnostic_op::error_I030_minimum(name_of_instruction, true, min_operands);
		else if (min_operands == max_operands)
			diagnostic = diagnostic_op::error_I031_exact(name_of_instruction, true, min_operands);
		else
			diagnostic = diagnostic_op::error_I032_from_to(name_of_instruction, true, min_operands, max_operands);
		return false;
	}

	// functions for checking complex operands
	bool check_compat_operands(const std::vector<const one_operand*> & input)
	{
		//vector of keywords that can be used in compat option
		const static std::vector<std::string> keywords = { "NOCASE", "NOLITTYPE", "NOMACROCASE", "NOSYSLIST", "NOTRANSDT", "NOLIT", "NOMC", "NOSYSL", "NOTRS", "CASE", "LITTYPE", "MACROCASE", "SYSLIST", "TRANSDT", "LIT", "MC", "SYSL", "TRS" };
		for (const auto& operand : input)
		{
			if (!is_param_in_vector(operand->operand_identifier, keywords))
			{
				diagnostic = diagnostic_op::error_I020_format("COMPAT", false);
				return false;
			}
		}
		return true;
	}

	bool check_flag_operand(const one_operand* input)
	{
		if (is_operand_complex(input))
		{
			diagnostic = diagnostic_op::error_I010_simple_expected();
			return false;
		}
		//vector of keywords that can be used in flag option
		const static std::vector<std::string> flag_operands = { "NOALIGN", "NOCONT", "NOEXLITW", "NOIMPLEN", "NOPAGE0", "NOSUBSTR", "NOUSING0", "NOAL", "NOSUB", "NOUS0", "ALIGN", "CONT", "EXLITW", "IMPLEN", "PAGE0", "SUBSTR", "USING0", "AL", "SUB", "US0" };
		//either param can be in flag_operands vector or is an integer
		return is_param_in_vector(input->operand_identifier, flag_operands) || 
			(is_number(input->operand_identifier) && std::stoi(input->operand_identifier) >= 0 && std::stoi(input->operand_identifier) <= 255);
	}

	bool check_process_flag_parameters(const std::vector<const one_operand*>& input)
	{
		const static std::vector<std::string> other_pair_options = { "NOPUSH", "NORECORD", "NOPU", "NORC", "PUSH", "RECORD", "PU", "RC" };
		return std::all_of(input.begin(), input.end(), [this](const auto& operand) { return check_flag_operand(operand) || is_param_in_vector(operand->operand_identifier, other_pair_options); });
	}

	bool check_optable_operands(const std::vector<const one_operand*>& input)
	{
		const static std::array<std::string,13> optable_array = { "DOS", "ESA", "XA", "370", "YOP", "ZOP", "ZS3", "ZS4", "ZS5", "ZS6", "ZS7", "ZS8" }; //declare array of options for first parameter
		if (std::find(optable_array.cbegin(), optable_array.cend(), input[0]->operand_identifier) != optable_array.end()) //check first parameter
		{
			if (is_operand_complex(input[0]) || is_operand_complex(input[1]))
			{
				diagnostic = diagnostic_op::error_I010_simple_expected();
				return false;
			}
			if ((input[1]->operand_identifier == "LIST" || input[1]->operand_identifier == "NOLIST") && (input.size()) == 2)
				return true;
		}
		diagnostic = diagnostic_op::error_I020_format("OPTABLE", false);
		return false;
	}

	bool check_typecheck_operands(const std::vector<const one_operand*>& input)
	{
		const static std::vector<std::string> typecheck_operands = { "MAGNITUDE", "REGISTER", "MAG", "REG","NOMAGNITUDE", "NOREGISTER", "NOMAG", "NOREG" };
		for (const auto& operand : input)
		{
			if (!is_param_in_vector(operand->operand_identifier, typecheck_operands))
			{
				diagnostic = diagnostic_op::error_I020_format("TYPECHECK", false);
				return false;
			}
		}
		return true;
	}

	static bool check_assembler_type_value(const std::string& input_operand)
	{
		const static std::vector<std::string> assembler_type_operands = { "AR", "CR", "CR32", "CR64", "FPR", "GR", "VR", "GR32", "GR64", "" };
		return is_param_in_vector(input_operand, assembler_type_operands);
	}

	static bool check_ictl_parameters(int begin, int end, int continuation)
	{
		return (begin >= 1 && begin <= 40 && end >= 41 && end <= 80 && end >= begin + 5
			&& ((end > continuation && continuation > begin  && continuation >= 2 && continuation <= 40) || continuation == -1));
	}

	// process instruction functions
	static bool check_codepage_parameters(const std::string& input_str)
	{
		//hexa value
		if (input_str.front() == 'X')
		{
			return input_str.size() >= 3 && input_str[1] == '\'' && input_str.back() == '\'' && std::all_of(input_str.cbegin() + 2, input_str.cend() - 1, ::isxdigit);
		}
		//decimal value
		else
		{
			return std::all_of(input_str.cbegin(), input_str.cend(), ::isdigit) && std::stoi(input_str) >= 1140 && std::stoi(input_str) <= 1148;
		}
	}

	bool check_fail_parameters(const std::vector<const one_operand*>& input)
	{
		for (const auto& operand : input)
		{
			// check simple parameters 
			if (is_operand_simple(operand) && operand->operand_identifier != "NOMSG" && operand->operand_identifier != "NOMNOTE")
				return false;
			else
			{
				complex_operand* current_operand = (complex_operand*)operand;
				if (current_operand->operand_parameters.size() == 0 || current_operand->operand_parameters.size() > 1 || !check_all_operands_simple(input))
					return false;
				auto ident = current_operand->operand_parameters[0]->operand_identifier;
				if (current_operand->operand_identifier == "MNOTE" || current_operand->operand_identifier == "MSG")
				{
					if (!is_number(ident) || std::stoi(ident) < 0 || std::stoi(ident) > 7)
						return false;
				}
				else if (current_operand->operand_identifier == "MAXERRS" || current_operand->operand_identifier == "NOMAXERRS")
				{
					if (!is_number(ident) || std::stoi(ident) < 32 || std::stoi(ident) > 65535)
						return false;
				}
				else
					return false;
			}
		}
		return true;
	}

	static bool check_first_machine_operand(const std::string& input_str)
	{
		const static std::vector<std::string> machine_operand_options = { "S370", "S370XA", "S370ESA", "S390", "S390E", "ZSERIES", "ZS", "ZSERIES-2",
			"ZS-2", "ZSERIES-3", "ZS-3", "ZSERIES-4", "ZS-4", "ZSERIES-5", "ZS-5", "ZSERIES-6", "ZS-6", "ZSERIES-7", "ZS-7", "ZSERIES-8", "ZS-8" };
		return is_param_in_vector(input_str, machine_operand_options);
	}

	bool check_pcontrol_parameters(const std::vector<const one_operand*>& input)
	{
		const static std::vector<std::string> pcontrol_pair_options = { "DATA", "GEN", "MCALL", "MSOURCE", "UHEAD", "MC", "MS", "UHD", "NODATA", "NOGEN", "NOMCALL", "NOMSOURCE", "NOUHEAD", "NOMC", "NOMS", "NOUHD" };
		for (const auto& operand : input)
		{
			if (!is_param_in_vector(operand->operand_identifier, pcontrol_pair_options) && operand->operand_identifier != "ON" && operand->operand_identifier != "OFF")
			{
				diagnostic = diagnostic_op::error_I020_format("PCONTROL", false);
				return false;
			}
		}
		return true;
	}

	static bool check_using_parameters(const std::vector<const one_operand*>& input)
	{
		for (const auto& operand : input)
		{
			//check simple operands
			if (is_operand_simple(operand))
			{
				if (operand->operand_identifier != "MAP"  && operand->operand_identifier != "NOMAP"
					&& operand->operand_identifier != "NOWARN"  && operand->operand_identifier != "NOLIMIT")
					return false;
			}
			else
			{
				complex_operand* current_operand = (complex_operand*) operand;
				if (current_operand->operand_parameters.size() != 1 || is_operand_complex(current_operand->operand_parameters[0]))
					return false;
				if (current_operand->operand_identifier == "WARN" && is_base_register(current_operand->operand_parameters[0]->operand_identifier))
					return true;
				else if (auto ident = current_operand->operand_parameters[0]->operand_identifier; 
						current_operand->operand_identifier == "LIMIT"
						&& ((is_positive_number(ident) && std::stoi(ident) <= 4095) 
						|| (ident.front() == 'X' && ident[1] == '\'' && ident.back() == '\''
							&& is_value_hexa(ident.substr(2, ident.size() - 3))
							&& std::stoul(ident.substr(2, ident.size() - 3)) <= 0xFFF)))
					return true;
				else
					return false;
			}
		}
		return false;
	}

	bool check_xref_parameters(const std::vector<const one_operand*>& input)
	{
		if (input.size() == 1 && (input[0]->operand_identifier == "FULL" || input[0]->operand_identifier == "SHORT" || input[0]->operand_identifier == "UNREFS"))
			return true;
		else if(input.size() == 2 
				&& (input[0]->operand_identifier == "SHORT" && input[1]->operand_identifier == "UNREFS") || (input[1]->operand_identifier == "SHORT" && input[0]->operand_identifier == "UNREFS"))
			return true;
		diagnostic = diagnostic_op::error_I020_format("XREF", false);
		return false;
	}

	static bool check_suprwarn_parameters(const std::vector<const one_operand*>& input)
	{
		return std::all_of(input.cbegin(), input.cend(), [](const auto& operand) 
			{ return operand->operand_identifier.size() <= 4 && operand->operand_identifier.size() != 0 && is_positive_number(operand->operand_identifier); });
	}

	bool check_all_operands_simple(const std::vector<const one_operand*>& input)
	{
		for (const auto& operand : input)
		{
			if (is_operand_complex(operand))
			{
				diagnostic = diagnostic_op::error_I010_simple_expected();
				return false;
			}
		}
		return true;
	}

	bool check_assembler_process_operand(const one_operand* input)
	{
		if (is_operand_simple(input)) //operand is simple
		{
			const static std::vector<std::string> assembler_pair_options = { "ALIGN", "BATCH", "DBCS", "DXREF", "DX", "ERASE", "ESD", "FOLD", "ILMA", "PROFILE", "PROF",
				"INFO", "LIBMAC", "LMAC", "PRINT", "PR", "RA2", "RENT", "RLD", "RXREF", "RX", "SEG", "TEST", "THREAD", "THR", "WORKFILE", "MXREF", "MX",
				"NOALIGN", "NOBATCH", "NODBCS", "NODXREF", "NODX", "NOERASE", "NOESD", "NOFOLD", "NOILMA", "NOPROFILE", "NOPROF",
				"NOINFO", "NOLIBMAC", "NOLMAC", "NOPRINT", "NOPR", "NORA2", "NORENT", "NORLD", "NORXREF", "NORX", "NOSEG", "NOTEST", "NOTHREAD", "NOTHR", "NOWORKFILE", "NOMXREF", "NOMX" };
			const static std::vector<std::string> other_simple_options = { "NOCOMPAT", "NOCPAT", "DISK", "DI", "NOFAIL", "NOPCONTROL", "NOPC",
				"NOTYPECHECK", "NOTC", "NOUSING", "NOUS", "NOXREF", "PESTOP=YES", "PESTOP=NO", "NOSUPRWARN", "NOSUP" };
			if (!is_param_in_vector(input->operand_identifier, assembler_pair_options) && !is_param_in_vector(input->operand_identifier, other_simple_options))
			{
				diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
				return false;
			}
		}
		else //operand is complex, check all possible complex operand names
		{
			complex_operand* current_operand = (complex_operand*) input;
			const static std::vector<std::string> one_simple_param = { "CODEPAGE", "CP", "INFO", "MXREF", "MX", "SECTALGN", "PROFILE", "PROF" };
			const static std::vector<std::string> two_simple_param = { "MACHINE", "MAC", "OPTABLE", "OP" };
			const static std::vector<std::string> one_plus_simple_params = { "COMPAT", "CPAT", "FLAG", "PCONTROL", "PC",
				"XREF", "SUPRWARN" , "SUP", "NOSUPRWARN", "NOSUP", "TYPECHECK", "TC" };
			//check operands with only simple parameters
			if (is_param_in_vector(current_operand->operand_identifier, one_simple_param) || is_param_in_vector(current_operand->operand_identifier, one_plus_simple_params)
				|| is_param_in_vector(current_operand->operand_identifier, two_simple_param))
			{
				if (!check_all_operands_simple(current_operand->operand_parameters)) // check whether all operands are simple
					return false;
				if (is_param_in_vector(current_operand->operand_identifier, one_simple_param)) // check for operands with only one parameter
				{
					if (current_operand->operand_parameters.size() != 1)
					{
						if (current_operand->operand_identifier == "INFO")
							diagnostic = diagnostic_op::error_I033_either(name_of_instruction, false, 0, 1);
						else diagnostic = diagnostic_op::error_I031_exact(name_of_instruction, false, 1);
						return false;
					}
					if (((current_operand->operand_identifier == "CODEPAGE" || current_operand->operand_identifier == "CP") // check codepage option
						&& !check_codepage_parameters(current_operand->operand_parameters[0]->operand_identifier))
						||
						((current_operand->operand_identifier == "INFO") // check info option
							&& (!is_positive_number(current_operand->operand_parameters[0]->operand_identifier) || !is_date(current_operand->operand_parameters[0]->operand_identifier)))
						||
						((current_operand->operand_identifier == "MXREF" || current_operand->operand_identifier == "MX") // check mxref option
							&& (current_operand->operand_parameters[0]->operand_identifier != "FULL" && current_operand->operand_parameters[0]->operand_identifier != "SOURCE"
								&& current_operand->operand_parameters[0]->operand_identifier != "XREF"))
						||
						((current_operand->operand_identifier == "SECTALGN")
							&& (!is_positive_number(current_operand->operand_parameters[0]->operand_identifier)
								|| !is_power_of_two(std::stoi(current_operand->operand_parameters[0]->operand_identifier))
								|| std::stoi(current_operand->operand_parameters[0]->operand_identifier) < 8
								|| std::stoi(current_operand->operand_parameters[0]->operand_identifier) > 4096))
						)
					{
						diagnostic = diagnostic_op::error_I020_format(current_operand->operand_identifier, false);
						return false;
					}
				}
				else if (is_param_in_vector(current_operand->operand_identifier, two_simple_param)) // check for operands with two parameters
				{
					if (current_operand->operand_parameters.size() != 2)
					{
						diagnostic = diagnostic_op::error_I031_exact(name_of_instruction, false, 2);
						return false;
					}
					if (((current_operand->operand_identifier == "MACHINE" || current_operand->operand_identifier == "MAC")
						&& ((current_operand->operand_parameters[1]->operand_identifier != "LIST" && current_operand->operand_parameters[1]->operand_identifier != "NOLIST")
							|| !check_first_machine_operand(current_operand->operand_parameters[0]->operand_identifier)))
						||
						((current_operand->operand_identifier == "OPTABLE" || current_operand->operand_identifier == "OP")
							&& (!check_optable_operands(current_operand->operand_parameters)))
						)
					{
						diagnostic = diagnostic_op::error_I020_format(current_operand->operand_identifier, false);
						return false;
					}

				}
				else // any number of parameters
				{
					if (current_operand->operand_parameters.size() == 0)
					{
						diagnostic = diagnostic_op::error_I030_minimum(name_of_instruction, false, 1);
						return false;
					}
					if (((current_operand->operand_identifier == "COMPAT" || current_operand->operand_identifier == "CPAT")
						&& (!check_compat_operands(current_operand->operand_parameters)))
						||
						((current_operand->operand_identifier == "TYPECHECK" || current_operand->operand_identifier == "TC")
							&& (!check_typecheck_operands(current_operand->operand_parameters)))
						||
						((current_operand->operand_identifier == "PCONTROL" || current_operand->operand_identifier == "PC")
							&& !check_pcontrol_parameters(current_operand->operand_parameters))
						|| (current_operand->operand_identifier == "XREF" && !check_xref_parameters(current_operand->operand_parameters))
						|| ((current_operand->operand_identifier == "SUPRWARN" || current_operand->operand_identifier == "SUP"
							|| current_operand->operand_identifier == "NOSUPRWARN" || current_operand->operand_identifier == "NOSUP")
							&& !check_suprwarn_parameters(current_operand->operand_parameters))
						|| (current_operand->operand_identifier == "FLAG" && !check_process_flag_parameters(current_operand->operand_parameters))
						)
						return false;
				}
			}
			// operands also contain complex parameters
			else if (current_operand->operand_identifier == "FAIL")
			{
				if (!check_fail_parameters(current_operand->operand_parameters))
				{
					diagnostic = diagnostic_op::error_I020_format(current_operand->operand_identifier, false);
					return false;
				}
				else
					return true;
			}
			else if (current_operand->operand_identifier == "USING" || current_operand->operand_identifier == "US")
			{
				if (!check_using_parameters(current_operand->operand_parameters))
				{
					diagnostic = diagnostic_op::error_I020_format(current_operand->operand_identifier, false);
					return false;
				}
				else
					return true;
			}
			else
			{
				diagnostic = diagnostic_op::error_I020_format(name_of_instruction, true);
				return false;
			}
		}
		return true;
	}


};


}
}
}

#endif
