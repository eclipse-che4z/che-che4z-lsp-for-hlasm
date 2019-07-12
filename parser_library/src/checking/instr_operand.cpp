#include "instr_operand.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::checking;

one_operand::one_operand(std::string operand_identifier) : operand_identifier(operand_identifier) {}

std::unique_ptr<asm_operand> one_operand::clone() const
{
	return std::unique_ptr<asm_operand>();
}

std::string one_operand::to_string() const
{
	return operand_identifier;
}

complex_operand::complex_operand()
{
}

complex_operand::complex_operand(std::string operand_identifier, std::vector<std::unique_ptr<asm_operand>> operand_params) :
	operand_identifier(operand_identifier), operand_parameters(std::move(operand_params)) {};

std::unique_ptr<asm_operand> complex_operand::clone() const
{
	return std::unique_ptr<asm_operand>();
}

std::string complex_operand::to_string() const 
{
	std::string res(operand_identifier);
	res.append("(");
	for (auto& op : operand_parameters)
	{
		res.append(op->to_string());
	}
	res.append(")");
	return res;
}

machine_operand_value::machine_operand_value()
{
}

bool machine_operand_value::check(diagnostic_op &, const machine_operand_format, const std::string &) const
{
	return true;
}

std::unique_ptr<machine_operand_value> machine_operand_value::clone() const
{
	return std::unique_ptr<machine_operand_value>();
}

std::string machine_operand_value::to_string() const
{
	return std::string("");
}


bool machine_operand_value::is_operand_corresponding(int operand, parameter param) const
{
	if (param.is_signed && is_size_corresponding_signed(operand, param.size))
		return true;
	else if (!param.is_signed && is_size_corresponding_unsigned(operand, param.size))
		return true;
	return false;
}

bool machine_operand_value::is_size_corresponding_signed(int operand, int size) const
{
	auto boundary = 1ll << (size - 1);
	return operand < boundary && operand >= -boundary;
}

bool machine_operand_value::is_size_corresponding_unsigned(int operand, int size) const
{
	return operand >= 0 && operand <= (1ll << size) - 1;
}

bool machine_operand_value::is_simple_operand(const machine_operand_format & operand) const
{
	return (operand.first.is_signed == false && operand.first.size == 0 && operand.second.is_signed == false && operand.second.size == 0
		&& operand.first.type == machine_operand_type::NONE && operand.second.type == machine_operand_type::NONE);
}

address_operand_value::address_operand_value(address_state state, int displacement, int first, int second) :
	state(state), displacement(displacement), first_op(first), second_op(second), first_state(operand_state::PRESENT) {};

address_operand_value::address_operand_value(address_state state, int displacement, int second, operand_state first_state) :
	state(state), displacement(displacement), first_op(0), second_op(second), first_state(first_state) {};


hlasm_plugin::parser_library::diagnostic_op machine_operand_value::get_address_operand_expected(const machine_operand_format & op_format, const std::string & instr_name) const
{
	if (op_format.first.is_empty()) // D(B)
		return diagnostic_op::error_M104(instr_name);
	switch (op_format.first.type)
	{
	case machine_operand_type::LENGTH: // D(L,B)
		return diagnostic_op::error_M101(instr_name);
	case machine_operand_type::DIS_REG: // D(X,B)
		return diagnostic_op::error_M100(instr_name);
	case machine_operand_type::REG: // D(R,B)
		return diagnostic_op::error_M102(instr_name);
	case machine_operand_type::VEC_REG: // D(V,B)
		return diagnostic_op::error_M103(instr_name);
	}
	assert(false);
	return diagnostic_op::error_I999(instr_name);
}

hlasm_plugin::parser_library::diagnostic_op address_operand_value::get_first_parameter_error(const machine_operand_type & op_type, const std::string & instr_name, long long from, long long to) const
{
	switch (op_type)
	{
	case machine_operand_type::LENGTH: // D(L,B)
		return diagnostic_op::error_M132(instr_name, from, to);
	case machine_operand_type::DIS_REG: // D(X,B)
		return diagnostic_op::error_M135(instr_name, from, to);
	case machine_operand_type::REG: // D(R,B)
		return diagnostic_op::error_M133(instr_name, from, to);
	case machine_operand_type::VEC_REG: // D(V,B)
		return diagnostic_op::error_M134(instr_name, from, to);
	}
	assert(false);
	return diagnostic_op::error_I999(instr_name);
}

bool address_operand_value::check(diagnostic_op & diag, const machine_operand_format to_check, const std::string & instr_name) const
{
	if (is_simple_operand(to_check))
	{
		// operand must be simple
		diag = get_simple_operand_expected(to_check, instr_name);
		return false;
	}
	if (state == address_state::RES_VALID)
		return true;
	else if (state == address_state::RES_INVALID)
	{
		diag = diagnostic_op::error_M010(instr_name);
		return false;
	}
	else
	{
		// check displacement
		if (!is_operand_corresponding(displacement, to_check.identifier))
		{
			if (to_check.identifier.is_signed)
			{
				auto boundary = 1ll << (to_check.identifier.size - 1);
				diag = diagnostic_op::error_M130(instr_name, -boundary, boundary - 1);
			}
			else
				diag = diagnostic_op::error_M130(instr_name, 0, (1ll << to_check.identifier.size)-1);
			return false;
		}
		// check first operand (in case of D(B), the operand is ommited)
		if (to_check.first.is_empty())
		{
			// therefore we have a D(B) format
			if (first_state == operand_state::EMPTY)
			{
				// error, should not be empty
				diag = diagnostic_op::error_M104(instr_name);
				return false;
			}
			else if (first_state == operand_state::PRESENT)
			{
				// error, cannot be present
				diag = diagnostic_op::error_M104(instr_name);
				return false;
			}
		}
		else
		{
			// therefore a D(X,B) format
			if (first_state != operand_state::PRESENT)
			{
				diag = get_address_operand_expected(to_check, instr_name);
				return false;
			}
			if (!is_operand_corresponding(first_op, to_check.first))
			{
				if (to_check.first.is_signed)
				{
					auto boundary = 1ll << (to_check.first.size - 1);
					diag = get_first_parameter_error(to_check.first.type, instr_name, -boundary, boundary - 1);
				}
				else
					diag = get_first_parameter_error(to_check.first.type, instr_name, 0, (1ll << to_check.first.size) - 1);
				return false;
			}
		}
		// check second parameter, in all cases this is the base parameter
		if (!is_operand_corresponding(second_op, to_check.second))
		{
			diag = diagnostic_op::error_M131(instr_name);
			return false;
		}
		return true;
	}
}
;

simple_operand_value::simple_operand_value(int value) : value(value) {};

hlasm_plugin::parser_library::diagnostic_op machine_operand_value::get_simple_operand_expected(const machine_operand_format & op_format, const std::string & instr_name) const
{
	switch (op_format.identifier.type)
	{
	case machine_operand_type::REG: // R
		return diagnostic_op::error_M110(instr_name);
	case machine_operand_type::MASK: // M
		return diagnostic_op::error_M111(instr_name);
	case machine_operand_type::IMM: // I
		return diagnostic_op::error_M112(instr_name);
	case machine_operand_type::REG_IMM: // RI
		return diagnostic_op::error_M113(instr_name);
	case machine_operand_type::VEC_REG: // V
		return diagnostic_op::error_M114(instr_name);
	}
	assert(false);
	return diagnostic_op::error_I999(instr_name);
}

bool simple_operand_value::check(diagnostic_op & diag, const machine_operand_format to_check, const std::string & instr_name) const
{
	if (!is_simple_operand(to_check))
	{
		// operand must be simple
		diag = get_address_operand_expected(to_check, instr_name);
		return false;
	}
	if (to_check.identifier.is_signed && !is_size_corresponding_signed(value, to_check.identifier.size))
	{
		auto boundary = 1ll << (to_check.identifier.size - 1);
		switch (to_check.identifier.type)
		{
		case machine_operand_type::IMM:
			diag = std::move(diagnostic_op::error_M122(instr_name, -boundary, boundary - 1));
			break;
		case machine_operand_type::REG_IMM:
			diag = std::move(diagnostic_op::error_M123(instr_name, -boundary, boundary - 1));
			break;
		default:
			assert(false);
		}
		return false;
	}
	if (!to_check.identifier.is_signed && !is_size_corresponding_unsigned(value, to_check.identifier.size))
	{
		auto boundary = (1ll << to_check.identifier.size) - 1;
		switch (to_check.identifier.type)
		{
		case machine_operand_type::REG:
			diag = std::move(diagnostic_op::error_M120(instr_name));
			break;
		case machine_operand_type::MASK:
			diag = std::move(diagnostic_op::error_M121(instr_name));
			break;
		case machine_operand_type::IMM:
			diag = std::move(diagnostic_op::error_M122(instr_name, 0, boundary));
			break;
		case machine_operand_type::VEC_REG:
			diag = std::move(diagnostic_op::error_M124(instr_name));
			break;
		default:
			assert(false);
		}
		return false;
	}
	return true;
}

operand::operand()
{
}

empty_operand_value::empty_operand_value()
{
}

bool empty_operand_value::check(diagnostic_op & diag, const machine_operand_format, const std::string & instr_name) const
{
	diag = std::move(diagnostic_op::error_M003(instr_name));
	return false;
}

bool parameter::is_empty() const
{
	return (!is_signed && type == machine_operand_type::NONE && size == 0);
}

std::string parameter::to_string() const
{
	std::string ret_val = "";
	switch (type) {
	case machine_operand_type::MASK :
		return "M";
	case machine_operand_type::REG:
		return "R";
	case machine_operand_type::REG_IMM:
	{
		ret_val = "RI";
		break;
	}
	case machine_operand_type::IMM:
	{
		ret_val = "I";
		break;
	}
	case machine_operand_type::NONE:
		return "";
	case machine_operand_type::DISPLC:
	{
		ret_val = "D";
		break;
	}
	case machine_operand_type::BASE:
		return "B";
	case machine_operand_type::LENGTH:
	{
		ret_val = "L";
		break;
	}
	case machine_operand_type::VEC_REG:
		return "V";
	case machine_operand_type::DIS_REG:
		return "X";
	}
	ret_val += std::to_string(size);
	if (is_signed)
		ret_val += "S";
	else
		ret_val += "U";
	return ret_val;
}

asm_operand::asm_operand()
{
}

std::unique_ptr<asm_operand> asm_operand::clone() const
{
	return std::unique_ptr<asm_operand>();
}

std::string asm_operand::to_string() const
{
	return std::string();
}

std::string machine_operand_format::to_string() const
{
	std::string ret_val = identifier.to_string();
	if (first.is_empty() && second.is_empty())
		return ret_val;
	if (first.is_empty())
		return (ret_val + "(" + second.to_string() + ")");
	// only second cannot be empty
	return (ret_val + "(" + first.to_string() + "," + second.to_string() + ")");
}
