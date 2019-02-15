#ifndef HLASMPLUGIN_PARSERLIBRARY_INSTR_OPERAND_H
#define HLASMPLUGIN_PARSERLIBRARY_INSTR_OPERAND_H

#include <vector>


namespace hlasm_plugin
{
namespace parser_library
{
namespace checking
{
	enum class address_state { RES_VALID, RES_INVALID, UNRES };

	// class that represents one operand
	// used alone for simple operands (operands that have only string value with no futher meaning) - LIBMAC, RA2, AFPR...
	class one_operand
	{
	public:
		//the string value of the operand
		std::string operand_identifier;
		//one_operand() {};
		one_operand(std::string operand_identifier) : operand_identifier(operand_identifier) {};
		virtual ~one_operand() {};
	};

	class address_operand : public one_operand
	{
	public:
		address_state state;
		int displacement;
		int first_par;
		int second_par;
		//address_operand() {};
		address_operand(address_state state, int displacement, int first_par, int second_par) :
			one_operand(""), state(state), displacement(displacement), first_par(first_par), second_par(second_par) {};
	};

	// extended class representing complex operands
	// contains vector of all parameters of operand - for example FLAG, COMPAT, OPTABLE...
	class complex_operand : public one_operand
	{
	public:
		std::vector<one_operand *> operand_parameters;
		std::vector<std::unique_ptr<one_operand>> operand_parameters_uniq;
		//complex_operand() {};
		complex_operand(std::string operand_identifier, std::vector<std::unique_ptr<one_operand>> operand_params) :
			one_operand(std::move(operand_identifier)), operand_parameters_uniq(std::move(operand_params))
		{
			std::transform(operand_parameters_uniq.cbegin(), operand_parameters_uniq.cend(), std::back_inserter(operand_parameters), [](const auto& p) {return p.get(); });
		}
	};
}
}
}

#endif
