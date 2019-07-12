#include "operand.h"

using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library;

operand::operand(operand_type type) :type(type) {}

model_operand * operand::access_model_op() 
{
	return type == operand_type::MODEL ? static_cast<model_operand*>(this) : nullptr;
}

ca_operand * operand::access_ca_op()
{
	return type == operand_type::CA ? static_cast<ca_operand*>(this) : nullptr;
}

substituable_operand * operand::access_subs_op()
{
	return type == operand_type::ASM || type == operand_type::MACH || type == operand_type::DAT ? static_cast<substituable_operand*>(this) : nullptr;
}

macro_operand * operand::access_mac_op()
{
	return type == operand_type::MAC ? static_cast<macro_operand*>(this) : nullptr;
}

machine_operand* operand::access_mach_op()
{
	return type == operand_type::MACH ? static_cast<machine_operand*>(this) : nullptr;
}

assembler_operand* operand::access_asm_op()
{
	return type == operand_type::ASM ? static_cast<assembler_operand*>(this) : nullptr;
}

empty_operand::empty_operand() :operand(operand_type::EMPTY) {}

model_operand::model_operand(concat_chain chain) : operand(operand_type::MODEL), chain(std::move(chain)) {}

operand_ptr model_operand::clone() const
{
	concat_chain new_chain;

	new_chain.insert(new_chain.end(), make_clone_iterator(chain.begin()), make_clone_iterator(chain.end()));

	return std::make_unique<model_operand>(std::move(new_chain));
}

machine_operand::machine_operand(std::unique_ptr<checking::machine_operand_value> op_value) : substituable_operand(operand_type::MACH), op_value(std::move(op_value)) {}

std::string machine_operand::to_string() const
{
	return op_value->to_string();
}

operand_ptr machine_operand::clone() const
{
	return std::make_unique<machine_operand>(op_value->clone());
}

hlasm_plugin::parser_library::semantics::assembler_operand::assembler_operand(std::unique_ptr<checking::asm_operand> op_value)
	: substituable_operand(operand_type::ASM),op_value(std::move(op_value)) {}

std::string assembler_operand::to_string() const
{
	return op_value->to_string();
}

operand_ptr assembler_operand::clone() const
{
	return std::make_unique<assembler_operand>(op_value->clone());
}

ca_operand::ca_operand(seq_sym seqence_symbol)
	: operand(operand_type::CA),sequence_symbol(std::move(seqence_symbol)), kind(ca_operand_kind::BRANCH_SIMPLE) {}

ca_operand::ca_operand(var_sym vs)
	: operand(operand_type::CA), vs(std::move(vs)), kind(ca_operand_kind::VAR) {}

ca_operand::ca_operand(seq_sym seqence_symbol, antlr4::ParserRuleContext* expression)
	: operand(operand_type::CA), expression(std::move(expression)), sequence_symbol(std::move(seqence_symbol)), kind(ca_operand_kind::BRANCH_EXPR) {}

ca_operand::ca_operand(antlr4::ParserRuleContext* expression)
	: operand(operand_type::CA), expression(std::move(expression)), kind(ca_operand_kind::EXPR) {}

ca_operand::ca_operand(const ca_operand & ca_op) : operand(operand_type::CA), expression(ca_op.expression), sequence_symbol(ca_op.sequence_symbol), vs(ca_op.vs), kind(ca_op.kind) {}

operand_ptr ca_operand::clone() const
{
	return std::make_unique<ca_operand>(*this);
}

operand_ptr empty_operand::clone() const
{
	return std::make_unique<empty_operand>();
}

/*
hlasm_plugin::parser_library::checking::empty_asm_operand::empty_asm_operand()
{
}

std::unique_ptr<hlasm_plugin::parser_library::checking::asm_operand> hlasm_plugin::parser_library::checking::empty_asm_operand::clone() const
{
	return std::unique_ptr<asm_operand>();
}

std::string hlasm_plugin::parser_library::checking::empty_asm_operand::to_string() const
{
	return std::string();
}*/

macro_operand::macro_operand(concat_chain chain) : operand(operand_type::MAC),chain(std::move(chain)) {}

operand_ptr macro_operand::clone() const
{
	concat_chain new_chain;

	new_chain.insert(new_chain.end(), make_clone_iterator(chain.begin()), make_clone_iterator(chain.end()));

	return std::make_unique<macro_operand>(std::move(new_chain));
}

substituable_operand::substituable_operand(operand_type type) : operand(type) {}

data_def_operand::data_def_operand() : substituable_operand(operand_type::DAT) {}