#ifndef HLASMPLUGIN_PARSERLIBRARY_INSTR_OPERAND_H
#define HLASMPLUGIN_PARSERLIBRARY_INSTR_OPERAND_H

#include <vector>
#include "../semantics/concatenation.h"

namespace hlasm_plugin {
namespace parser_library {
namespace checking {

	enum class address_state { RES_VALID, RES_INVALID, UNRES };

	// class that represents one operand
	// used alone for simple operands (operands that have only string value with no futher meaning) - LIBMAC, RA2, AFPR...
	class one_operand
	{
	public:
		static const one_operand empty_one_operand;

		//the string value of the operand
		std::string operand_identifier;

		one_operand(std::string operand_identifier);

		virtual std::unique_ptr<one_operand> clone() const;

		virtual std::string to_string() const;

		virtual ~one_operand() = default;
	};

	class address_operand : public one_operand
	{
	public:
		address_state state;
		int displacement;
		int first_par;
		int second_par;

		address_operand(address_state state, int displacement, int first_par, int second_par);

		std::unique_ptr<one_operand> clone() const override;

		std::string to_string() const override;
	};

	// extended class representing complex operands
	// contains vector of all parameters of operand - for example FLAG, COMPAT, OPTABLE...
	class complex_operand : public one_operand
	{
	public:
		std::vector<const one_operand *> operand_parameters;
		std::vector<std::unique_ptr<one_operand>> operand_parameters_uniq;

		complex_operand(std::string operand_identifier, std::vector<std::unique_ptr<one_operand>> operand_params);

		std::unique_ptr<one_operand> clone() const override;

		std::string to_string() const override;
	};
}
}
}

#endif
