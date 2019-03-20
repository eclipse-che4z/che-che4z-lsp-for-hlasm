#include "instr_operand.h"

using namespace hlasm_plugin::parser_library::checking;

one_operand::one_operand(std::string operand_identifier) : operand_identifier(operand_identifier) {}

const one_operand one_operand::one_operand::empty_one_operand("");

std::unique_ptr<one_operand> one_operand::clone() const
{
	return std::make_unique<one_operand>(operand_identifier);
}

 std::string one_operand::to_string() const
{
	return operand_identifier;
}

address_operand::address_operand(address_state state, int displacement, int first_par, int second_par) :
	one_operand(""), state(state), displacement(displacement), first_par(first_par), second_par(second_par) {}

std::unique_ptr<one_operand> address_operand::clone() const 
{
	return std::make_unique<address_operand>(state, displacement, first_par, second_par);
}

std::string address_operand::to_string() const 
{
	return std::to_string(displacement) + "(" + std::to_string(first_par) + (second_par == -1 ? "" : "," + std::to_string(second_par)) + ")";
}

complex_operand::complex_operand(std::string operand_identifier, std::vector<std::unique_ptr<one_operand>> operand_params) :
	one_operand(std::move(operand_identifier)), operand_parameters_uniq(std::move(operand_params))
{
	std::transform(operand_parameters_uniq.cbegin(), operand_parameters_uniq.cend(), std::back_inserter(operand_parameters), [](const auto& p) {return p.get(); });
}

std::unique_ptr<one_operand> complex_operand::clone() const 
{
	std::vector<std::unique_ptr<one_operand>> new_operands;

	new_operands.insert(new_operands.end(), semantics::make_clone_iterator(operand_parameters_uniq.begin()), semantics::make_clone_iterator(operand_parameters_uniq.end()));

	return std::make_unique<complex_operand>(operand_identifier, std::move(new_operands));
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