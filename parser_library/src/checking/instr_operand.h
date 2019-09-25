#ifndef HLASMPLUGIN_PARSERLIBRARY_INSTR_OPERAND_H
#define HLASMPLUGIN_PARSERLIBRARY_INSTR_OPERAND_H

#include <vector>
#include <variant>
#include <string>
#include <memory>
#include <functional>
#include <assert.h>
#include "shared/range.h"
#include "../diagnostic.h"
#include "../diagnostic_collector.h"

namespace hlasm_plugin::parser_library::checking
{
	enum class address_state { RES_VALID, RES_INVALID, UNRES };

	/*
	FIRST_OMITTED = D(,B)
	SECOND_OMMITED = D(X,)
	PRESENT - D(X,B)
	ONE_OP - D(B)
	*/
	enum class operand_state { FIRST_OMITTED, SECOND_OMITTED, PRESENT, ONE_OP };

	class operand
	{
	public:
		operand();
		operand(range operand_range);

		range operand_range;

		virtual ~operand() = default;
	};

	using check_op_ptr = std::unique_ptr<operand>;

	class asm_operand : public virtual operand
	{
	public:
		asm_operand();
		virtual ~asm_operand() = default;
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
		parameter identifier; // used as displacement operand in address operand
		parameter first; // empty when simple operand
		parameter second; // empty when simple operand

		machine_operand_format(parameter id, parameter first, parameter second) : identifier(id), first(first), second(second)
		{
			if (second.is_empty() && !first.is_empty())
				assert(false);
		};

		std::string to_string() const;
	};

	class machine_operand : public virtual operand
	{
	public:
		machine_operand();

		virtual bool check(diagnostic_op & diag, const machine_operand_format to_check, const std::string & instr_name, const range& stmt_range) const = 0;

		diagnostic_op get_simple_operand_expected(const machine_operand_format & op_format, const std::string & instr_name, const range& stmt_range) const;

	protected:
		bool is_operand_corresponding(int operand, parameter param) const;
		bool is_simple_operand(const machine_operand_format & operand) const;
		bool is_size_corresponding_signed(int operand, int size) const;
		bool is_size_corresponding_unsigned(int operand, int size) const;
	};

	class address_operand final : public machine_operand
	{
	public:
		address_state state;
		int displacement;
		int first_op;
		int second_op;
		operand_state op_state;

		address_operand(address_state state, int displacement, int first, int second);
		address_operand(address_state state, int displacement, int first, int second, operand_state op_state);

		diagnostic_op get_first_parameter_error(const machine_operand_type & op_type, const std::string & instr_name, long long from, long long to, const range& stmt_range) const;

		bool check(diagnostic_op & diag, const machine_operand_format to_check, const std::string & instr_name, const range& range) const override;

		bool is_length_corresponding(int param_value, int length_size) const;
	};

	// class that represents both one_operand and value operands
	class one_operand final : public asm_operand, public machine_operand
	{
	public:

		//the string value of the operand
		std::string operand_identifier;
		int value;
		bool is_default;

		one_operand();

		one_operand(std::string operand_identifier, int value);

		one_operand(std::string operand_identifier);

		one_operand(int value);

		one_operand(std::string operand_identifier, range range);

		one_operand(int value, range range);

		one_operand(const one_operand& op);

		bool check(diagnostic_op& diag, const machine_operand_format to_check, const std::string& instr_name, const range& stmt_range) const override;
	};

	class empty_operand final : public machine_operand, public asm_operand
	{
	public:
		empty_operand();

		bool check(diagnostic_op & diag, const machine_operand_format to_check, const std::string & instr_name, const range& stmt_range) const override;
	};

	template<typename T>
	struct data_def_field
	{
		bool present;
		T value;
		range rng;
	};

	struct data_def_address
	{
		int base;
		int displacement;
	};

	class data_definition_operand : public asm_operand
	{
	public:
		using num_t = int32_t;

		enum class length_type
		{
			BYTE,
			BIT
		};

		data_def_field<num_t> dupl_factor;
		data_def_field<char> type;
		data_def_field<char> extension;
		data_def_field<num_t> length;
		length_type len_type;
		data_def_field<num_t> exponent;
		data_def_field<num_t> scale;
		
		data_def_field < std::variant<std::string, std::vector<data_def_field<int>>, data_def_address> > nominal_value;
	};

}

#endif
