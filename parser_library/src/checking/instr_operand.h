#ifndef HLASMPLUGIN_PARSERLIBRARY_INSTR_OPERAND_H
#define HLASMPLUGIN_PARSERLIBRARY_INSTR_OPERAND_H

#include <vector>
#include "../semantics/concatenation.h"

namespace hlasm_plugin {
namespace parser_library {
namespace checking {

	enum class address_state { RES_VALID, RES_INVALID, UNRES };

	/*
	PRESENT - D(X,B)
	EMPTY - D(B,)
	OMITTED - D(B)
	*/
	enum class operand_state { OMITTED, EMPTY, PRESENT };

	class operand
	{
	public:
		operand();

		semantics::symbol_range range;

		virtual ~operand() = default;
	};

	class asm_operand : public operand
	{
	public:
		asm_operand();
		virtual ~asm_operand() = default;

		virtual std::unique_ptr<asm_operand> clone() const;
		virtual std::string to_string() const;
	};

	// class that represents one operand
	// used alone for simple operands (operands that have only string value with no futher meaning) - LIBMAC, RA2, AFPR...
	class one_operand final : public asm_operand
	{
	public:
		//static const one_operand empty_one_operand;

		//the string value of the operand
		std::string operand_identifier;

		one_operand() : one_operand("") {};

		one_operand(std::string operand_identifier);

		virtual std::unique_ptr<asm_operand> clone() const;

		virtual std::string to_string() const;
	};

	// extended class representing complex operands
	// contains vector of all parameters of operand - for example FLAG, COMPAT, OPTABLE...
	class complex_operand final : public asm_operand
	{
	public:
		std::string operand_identifier;
		std::vector<std::unique_ptr<asm_operand>> operand_parameters;

		complex_operand();
		complex_operand(std::string operand_identifier, std::vector<std::unique_ptr<asm_operand>> operand_params);

		std::unique_ptr<asm_operand> clone() const override;

		std::string to_string() const override;
	};

	enum class machine_operand_type : uint8_t { MASK, REG, REG_IMM, IMM, NONE, DISPLC, BASE, LENGTH, VEC_REG, DIS_REG };

	struct parameter
	{
		bool is_signed;
		uint8_t size;
		machine_operand_type type;

		bool is_empty() const;

		std::string to_string() const;
	};

	struct machine_operand_format
	{
		parameter identifier; // used as displacement operand in adress operand
		parameter first; // empty when simple operand
		parameter second; // empty when simple operand

		machine_operand_format(parameter id, parameter first, parameter second) : identifier(id), first(first), second(second)
		{
			if (second.is_empty() && !first.is_empty())
				assert(false);
		};

		std::string to_string() const;
	};


	class machine_operand_value : public operand
	{
	public:
		machine_operand_value();

		virtual bool check(diagnostic_op & diag, const machine_operand_format to_check, const std::string & instr_name) const;

		std::unique_ptr<machine_operand_value> clone() const;

		std::string to_string() const;

		diagnostic_op get_address_operand_expected(const machine_operand_format & op_format, const std::string & instr_name) const;
		diagnostic_op get_simple_operand_expected(const machine_operand_format & op_format, const std::string & instr_name) const;

	protected:
		bool is_operand_corresponding(int operand, parameter param) const;
		bool is_simple_operand(const machine_operand_format & operand) const;
		bool is_size_corresponding_signed(int operand, int size) const;
		bool is_size_corresponding_unsigned(int operand, int size) const;
	};

	class address_operand_value final : public machine_operand_value
	{
	public:
		address_state state;
		int displacement;
		int first_op;
		int second_op;
		operand_state first_state;

		address_operand_value(address_state state, int displacement, int first, int second);
		address_operand_value(address_state state, int displacement, int second, operand_state first_state);

		diagnostic_op get_first_parameter_error(const machine_operand_type & op_type, const std::string & instr_name, long long from, long long to) const;

		bool check(diagnostic_op & diag, const machine_operand_format to_check, const std::string & instr_name) const override;
	};

	class simple_operand_value final : public machine_operand_value
	{
	public:
		int value;

		simple_operand_value(int value);

		bool check(diagnostic_op & diag, const machine_operand_format to_check, const std::string & instr_name) const override;

	};

	class empty_operand_value final : public machine_operand_value
	{
	public:
		empty_operand_value();

		bool check(diagnostic_op & diag, const machine_operand_format to_check, const std::string & instr_name) const override;
	};
}
}
}

#endif
