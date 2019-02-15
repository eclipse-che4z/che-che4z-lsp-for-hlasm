#include "operand.h"

using namespace hlasm_plugin::parser_library::semantics;

model_operand * hlasm_plugin::parser_library::semantics::operand::access_model_op() { return dynamic_cast<model_operand*>(this); }

ca_operand * hlasm_plugin::parser_library::semantics::operand::access_ca_op() { return dynamic_cast<ca_operand*>(this); }

substituable_operand * hlasm_plugin::parser_library::semantics::operand::access_subs_op() { return dynamic_cast<substituable_operand*>(this); }

machine_operand* operand::access_mach_op()
{
	return dynamic_cast<machine_operand *>(this);
}
assembler_operand* operand::access_asm_op()
{
	return dynamic_cast<assembler_operand *>(this);
}

operand_type hlasm_plugin::parser_library::semantics::operand::type() const { return operand_type::UNDEF; }

hlasm_plugin::parser_library::semantics::model_operand::model_operand(std::vector<concat_point_ptr> conc_list) : conc_list(std::move(conc_list)) {}

operand_type hlasm_plugin::parser_library::semantics::model_operand::type() const { return operand_type::MODEL; }

hlasm_plugin::parser_library::semantics::substituable_operand::substituable_operand(std::function<std::string()> get_text) : get_text(std::move(get_text)) {}

hlasm_plugin::parser_library::semantics::machine_operand::machine_operand(std::function<std::string()> get_text, std::unique_ptr<checking::one_operand> op_value) : substituable_operand(std::move(get_text)), op_value(std::move(op_value)) {}

operand_type hlasm_plugin::parser_library::semantics::machine_operand::type() const { return operand_type::MACH; }

hlasm_plugin::parser_library::semantics::assembler_operand::assembler_operand(std::function<std::string()> get_text, std::unique_ptr<checking::one_operand> op_value)
	: substituable_operand(std::move(get_text)), op_value(std::move(op_value)) {}

operand_type hlasm_plugin::parser_library::semantics::assembler_operand::type() const { return operand_type::ASM; }

hlasm_plugin::parser_library::semantics::ca_operand::ca_operand(seq_sym seqence_symbol)
	: seqence_symbol(std::move(seqence_symbol)), kind(ca_operand_kind::BRANCH_SIMPLE) {}

hlasm_plugin::parser_library::semantics::ca_operand::ca_operand(var_sym vs)
	: vs(std::move(vs)), kind(ca_operand_kind::VAR) {}

hlasm_plugin::parser_library::semantics::ca_operand::ca_operand(seq_sym seqence_symbol, antlr4::ParserRuleContext* expression)
	: expression(std::move(expression)), seqence_symbol(std::move(seqence_symbol)), kind(ca_operand_kind::BRANCH_EXPR) {}

hlasm_plugin::parser_library::semantics::ca_operand::ca_operand(antlr4::ParserRuleContext* expression)
	: expression(std::move(expression)), kind(ca_operand_kind::EXPR) {}

operand_type hlasm_plugin::parser_library::semantics::ca_operand::type() const { return operand_type::CA; }

operand_type hlasm_plugin::parser_library::semantics::empty_operand::type() const
{
	return operand_type::EMPTY;
}

hlasm_plugin::parser_library::semantics::data_def_operand::data_def_operand(std::function<std::string()> get_text)
	:substituable_operand(std::move(get_text))
{
}

operand_type hlasm_plugin::parser_library::semantics::data_def_operand::type() const
{
	return operand_type::DAT;
}
