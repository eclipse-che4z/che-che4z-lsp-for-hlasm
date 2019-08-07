#include "operand.h"

using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library;

//***************** operand *********************

operand::operand(const operand_type type, const range operand_range) 
	:type(type),operand_range(std::move(operand_range)) {}

model_operand * operand::access_model() 
{
	return type == operand_type::MODEL ? static_cast<model_operand*>(this) : nullptr;
}

ca_operand * operand::access_ca()
{
	return type == operand_type::CA ? static_cast<ca_operand*>(this) : nullptr;
}

macro_operand * operand::access_mac()
{
	return type == operand_type::MAC ? static_cast<macro_operand*>(this) : nullptr;
}

data_def_operand* operand::access_data_def()
{
	return type == operand_type::DAT ? static_cast<data_def_operand*>(this) : nullptr;
}

machine_operand* operand::access_mach()
{
	return dynamic_cast<machine_operand*>(this);
}

assembler_operand* operand::access_asm()
{
	return dynamic_cast<assembler_operand*>(this);
}

empty_operand::empty_operand(const range operand_range)
	:operand(operand_type::EMPTY,std::move(operand_range)) {}

model_operand::model_operand(concat_chain chain, const range operand_range) 
	: operand(operand_type::MODEL,std::move(operand_range)), chain(std::move(chain)) {}

evaluable_operand::evaluable_operand(const operand_type type, const range operand_range) 
	: operand(type,std::move(operand_range)) {}

machine_operand::machine_operand(const mach_kind kind)
	: kind(kind) {}

expr_machine_operand* machine_operand::access_expr()
{
	return kind == mach_kind::EXPR ? static_cast<expr_machine_operand*>(this) :nullptr;
}

address_machine_operand* machine_operand::access_address()
{
	return kind == mach_kind::EXPR ? static_cast<address_machine_operand*>(this) : nullptr;
}

std::unique_ptr<checking::operand> make_check_operand(expressions::mach_evaluate_info info, expressions::mach_expression & expr)
{
	//TODO get_abs get_reloc
	auto res = expr.evaluate(info);
	if(res.value_kind() == context::symbol_kind::ABS)
		return std::make_unique<checking::one_operand>(res.get_abs());
	else
		return std::make_unique<checking::address_operand>(checking::address_state::UNRES,0,0,0);
}

//***************** expr_machine_operand *********************

expr_machine_operand::expr_machine_operand(expressions::mach_expr_ptr expression, const range operand_range) 
	:  evaluable_operand(operand_type::MACH, std::move(operand_range)), machine_operand(mach_kind::EXPR), simple_expr_operand(std::move(expression)) {}

bool expr_machine_operand::has_dependencies(hlasm_plugin::parser_library::expressions::mach_evaluate_info info) const
{
	return expression->get_dependencies(info).contains_dependencies();
}

std::unique_ptr<checking::operand> expr_machine_operand::get_operand_value(expressions::mach_evaluate_info info) const
{
	return make_check_operand(info, *expression);
}

void expr_machine_operand::collect_diags() const
{
	collect_diags_from_child(*expression);
}

//***************** address_machine_operand *********************

address_machine_operand::address_machine_operand(
	expressions::mach_expr_ptr displacement,
	expressions::mach_expr_ptr first_par, 
	expressions::mach_expr_ptr second_par, 
	const range operand_range)
	: evaluable_operand(operand_type::MACH,std::move(operand_range)),
	machine_operand(mach_kind::ADDR),
	displacement(std::move(displacement)),
	first_par(std::move(first_par)),
	second_par(std::move(second_par)) {}

bool address_machine_operand::has_dependencies(hlasm_plugin::parser_library::expressions::mach_evaluate_info info) const
{
	return first_par ?
		(second_par ?
			displacement->get_dependencies(info).contains_dependencies() || first_par->get_dependencies(info).contains_dependencies() || second_par->get_dependencies(info).contains_dependencies() //D(B1,B2)
			: displacement->get_dependencies(info).contains_dependencies() ||  first_par->get_dependencies(info).contains_dependencies() //D(B)
			)
		: displacement->get_dependencies(info).contains_dependencies()|| second_par->get_dependencies(info).contains_dependencies(); //D(,B)
}

std::unique_ptr<checking::operand> address_machine_operand::get_operand_value(expressions::mach_evaluate_info info) const
{
	context::symbol_value displ, first, second;
	context::symbol_value::abs_value_t displ_v, first_v, second_v;

	displ = displacement->evaluate(info);
	displ_v = displ.value_kind() == context::symbol_kind::ABS ? displ.get_abs() : 0;
	if (first_par)
	{
		first = first_par->evaluate(info);
		first_v = first.value_kind() == context::symbol_kind::ABS ? first.get_abs() : 0;
	}
	if (second_par)
	{
		second = second_par->evaluate(info);
		second_v = second.value_kind() == context::symbol_kind::ABS ? second.get_abs() : 0;
	}

	return first_par ?
		(second_par ?
			std::make_unique<checking::address_operand>(checking::address_state::UNRES, displ_v, first_v, second_v) //D(B1,B2)
			: std::make_unique<checking::address_operand>(checking::address_state::UNRES, displ_v, 0, first_v) //D(B)
		)
		: std::make_unique<checking::address_operand>(checking::address_state::UNRES, displ_v, 0, second_v); //D(,B)
}

void address_machine_operand::collect_diags() const
{
	collect_diags_from_child(*displacement);
	if(first_par)
		collect_diags_from_child(*first_par);
	if(second_par)
		collect_diags_from_child(*second_par);
}

assembler_operand::assembler_operand(const asm_kind kind)
	: kind(kind) {}

expr_assembler_operand* assembler_operand::access_expr()
{
	return kind == asm_kind::EXPR ? static_cast<expr_assembler_operand*>(this) : nullptr;
}

end_instr_assembler_operand* assembler_operand::access_base_end()
{
	return kind == asm_kind::BASE_END ? static_cast<end_instr_assembler_operand*>(this) : nullptr;
}

complex_assembler_operand* assembler_operand::access_complex()
{
	return kind == asm_kind::COMPLEX ? static_cast<complex_assembler_operand*>(this) : nullptr;
}

string_assembler_operand* assembler_operand::access_string()
{
	return kind == asm_kind::STRING ? static_cast<string_assembler_operand*>(this) : nullptr;
}

//***************** expr_assembler_operand *********************

expr_assembler_operand::expr_assembler_operand(expressions::mach_expr_ptr expression, std::string string_value, const range operand_range)
	: evaluable_operand(operand_type::ASM, std::move(operand_range)), assembler_operand(asm_kind::EXPR), simple_expr_operand(std::move(expression)),value_(std::move(string_value)) {}

bool expr_assembler_operand::has_dependencies(hlasm_plugin::parser_library::expressions::mach_evaluate_info info) const
{
	return expression->get_dependencies(info).contains_dependencies();
}

std::unique_ptr<checking::operand> expr_assembler_operand::get_operand_value(expressions::mach_evaluate_info info) const
{
	auto res = expression->evaluate(info);
	switch (res.value_kind())
	{
	case context::symbol_kind::UNDEF:
		return std::make_unique<checking::one_operand>(value_);
	case context::symbol_kind::ABS:
		return std::make_unique<checking::one_operand>(value_, res.get_abs());
	case context::symbol_kind::RELOC:
		return std::make_unique<checking::address_operand>(checking::address_state::UNRES, 0, 0, 0);
	default:
		assert(false);
		return std::make_unique<checking::empty_operand>();
	}
}

void expr_assembler_operand::collect_diags() const
{
	collect_diags_from_child(*expression);
}

//***************** end_instr_machine_operand *********************

end_instr_assembler_operand::end_instr_assembler_operand(expressions::mach_expr_ptr base, expressions::mach_expr_ptr end,const range operand_range) 
	: evaluable_operand(operand_type::ASM, std::move(operand_range)), assembler_operand(asm_kind::BASE_END),base(std::move(base)), end(std::move(end)) {}

bool end_instr_assembler_operand::has_dependencies(hlasm_plugin::parser_library::expressions::mach_evaluate_info info) const
{
	return base->get_dependencies(info).contains_dependencies() || end->get_dependencies(info).contains_dependencies();
}

std::unique_ptr<checking::operand> end_instr_assembler_operand::get_operand_value(expressions::mach_evaluate_info info) const
{
	std::vector<std::unique_ptr<checking::asm_operand>> pair;
	//pair.push_back(make_check_operand(info, *base));
	//pair.push_back(make_check_operand(info, *end));
	return std::make_unique<checking::complex_operand>("", std::move(pair));
}

void end_instr_assembler_operand::collect_diags() const
{
	collect_diags_from_child(*base);
	collect_diags_from_child(*end);
}

//***************** complex_assempler_operand *********************
complex_assembler_operand::complex_assembler_operand(std::string identifier, std::vector<std::unique_ptr<component_value_t>> values, const range operand_range)
	:  evaluable_operand(operand_type::ASM, std::move(operand_range)), assembler_operand(asm_kind::COMPLEX), value(identifier,std::move(values)) {}

bool complex_assembler_operand::has_dependencies(hlasm_plugin::parser_library::expressions::mach_evaluate_info) const
{
	return false;
}

std::unique_ptr<checking::operand> complex_assembler_operand::get_operand_value(expressions::mach_evaluate_info) const
{
	return value.create_operand();
}

void complex_assembler_operand::collect_diags() const {}

//***************** ca_operand *********************
ca_operand::ca_operand(const ca_kind kind, const range operand_range)
	: operand(operand_type::CA,std::move(operand_range)), kind(kind) {}

var_ca_operand* ca_operand::access_var()
{
	return kind == ca_kind::VAR ? static_cast<var_ca_operand*>(this) : nullptr;
}

const var_ca_operand* ca_operand::access_var() const
{
	return kind == ca_kind::VAR ? static_cast<const var_ca_operand*>(this) : nullptr;
}

expr_ca_operand* ca_operand::access_expr()
{
	return kind == ca_kind::EXPR ? static_cast<expr_ca_operand*>(this) : nullptr;
}

const expr_ca_operand* ca_operand::access_expr() const
{
	return kind == ca_kind::EXPR ? static_cast<const expr_ca_operand*>(this) : nullptr;
}

seq_ca_operand* ca_operand::access_seq()
{
	return kind == ca_kind::SEQ ? static_cast<seq_ca_operand*>(this) : nullptr;
}

const seq_ca_operand* ca_operand::access_seq() const
{
	return kind == ca_kind::SEQ ? static_cast<const seq_ca_operand*>(this) : nullptr;
}

branch_ca_operand* ca_operand::access_branch()
{
	return kind == ca_kind::BRANCH ? static_cast<branch_ca_operand*>(this) : nullptr;
}

const branch_ca_operand* ca_operand::access_branch() const
{
	return kind == ca_kind::BRANCH ? static_cast<const branch_ca_operand*>(this) : nullptr;
}

simple_expr_operand::simple_expr_operand(expressions::mach_expr_ptr expression)
	: expression(std::move(expression)) {}

var_ca_operand::var_ca_operand(vs_ptr variable_symbol, const range operand_range)
	: ca_operand(ca_kind::VAR, std::move(operand_range)), variable_symbol(std::move(variable_symbol)) {}

expr_ca_operand::expr_ca_operand(antlr4::ParserRuleContext* expression, const range operand_range)
	: ca_operand(ca_kind::EXPR, std::move(operand_range)), expression(expression) {}

seq_ca_operand::seq_ca_operand(seq_sym sequence_symbol, const range operand_range)
	: ca_operand(ca_kind::SEQ, std::move(operand_range)), sequence_symbol(std::move(sequence_symbol)) {}

branch_ca_operand::branch_ca_operand(seq_sym sequence_symbol, antlr4::ParserRuleContext* expression, const range operand_range)
	: ca_operand(ca_kind::BRANCH, std::move(operand_range)), sequence_symbol(std::move(sequence_symbol)), expression(expression) {}



macro_operand::macro_operand(concat_chain chain, const range operand_range)
	: operand(operand_type::MAC,std::move(operand_range)),chain(std::move(chain)) {}



data_def_operand::data_def_operand(expressions::data_definition val, const range operand_range)
	: evaluable_operand(operand_type::DAT,std::move(operand_range)), value(std::make_shared<expressions::data_definition>(std::move(val))) {}


bool data_def_operand::has_dependencies(expressions::mach_evaluate_info info) const
{
	return value->get_dependencies(info).contains_dependencies();
}

checking::data_def_field<checking::data_definition_operand::num_t> set_data_def_field(const expressions::mach_expression * e, expressions::mach_evaluate_info info)
{
	using namespace checking;
	data_def_field<data_definition_operand::num_t> field;
	field.present = e != nullptr;
	if (e)
	{
		field.rng = e->get_range();
		//TODO get_reloc get_abs
		auto ret(e->evaluate(info));
		if(ret.value_kind() == context::symbol_kind::ABS)
			field.value = e->evaluate(info).get_abs();
	}
	return field;
}

std::unique_ptr<checking::operand> data_def_operand::get_operand_value(expressions::mach_evaluate_info info) const
{
	auto op = std::make_unique<checking::data_definition_operand>();
	
	op->dupl_factor = set_data_def_field(value->dupl_factor.get(), info);
	op->type.value = value->type;
	op->type.rng = value->type_range;
	op->extension.present = value->extension != '\0';
	op->extension.value = value->extension;
	op->extension.rng = value->extension_range;
	op->length = set_data_def_field(value->length.get(), info);
	op->len_type = value->length_type == expressions::data_definition::length_type::BIT ? checking::data_definition_operand::length_type::BIT : checking::data_definition_operand::length_type::BYTE;
	op->scale = set_data_def_field(value->scale.get(), info);
	op->exponent = set_data_def_field(value->exponent.get(), info);

	return op;
}

void data_def_operand::collect_diags() const
{
	collect_diags_from_child(*value);
}

undefined_operand::undefined_operand(const range operand_range)
	:operand(operand_type::UNDEF,std::move(operand_range)) {}

string_assembler_operand::string_assembler_operand(std::string value, const range operand_range)
	: evaluable_operand(operand_type::ASM, std::move(operand_range)), assembler_operand(asm_kind::STRING), value(std::move(value)) {}

bool hlasm_plugin::parser_library::semantics::string_assembler_operand::has_dependencies(expressions::mach_evaluate_info ) const
{
	return false;
}

std::unique_ptr<checking::operand> string_assembler_operand::get_operand_value(expressions::mach_evaluate_info ) const
{
	return std::make_unique<checking::one_operand>("'" + value + "'");
}

void hlasm_plugin::parser_library::semantics::string_assembler_operand::collect_diags() const { }
