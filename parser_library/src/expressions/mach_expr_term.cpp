#include "mach_expr_term.h"

#include "../checking/checker_helper.h"
#include "arithmetic_expression.h"

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library;

//***********  mach_expr_constant ************
mach_expr_constant::mach_expr_constant(std::string value_text, range rng) : mach_expression( rng)
{
	try
	{
		value_ = std::stoi(value_text);
	}
	catch (std::out_of_range&)
	{
		add_diagnostic(diagnostic_op::error_ME001(get_range()));
	}
}
mach_expr_constant::mach_expr_constant(int value, range rng) : mach_expression( rng), value_(value)
{}

context::dependency_holder mach_expr_constant::get_dependencies(context::dependency_solver& ) const { return context::dependency_holder(); }

mach_expr_constant::value_t mach_expr_constant::evaluate(mach_evaluate_info ) const
{
	return value_;
}

void mach_expr_constant::fill_location_counter(context::address)
{
}




//***********  mach_expr_symbol ************
mach_expr_symbol::mach_expr_symbol(context::id_index value, range rng) : mach_expression(rng),value(value) {}

mach_expr_ptr mach_expr_symbol::from_id(context::id_index id, range r)
{
	if (checking::is_number(*id, false))
		return std::make_unique<mach_expr_constant>(*id, r);
	else
		return std::make_unique<mach_expr_symbol>(id, r);
	
}

context::dependency_holder mach_expr_symbol::get_dependencies(context::dependency_solver& solver) const
{
	auto symbol = solver.get_symbol(value);

	if (symbol == nullptr || symbol->kind() == context::symbol_kind::UNDEF)
		return value;
	else if (symbol->kind() == context::symbol_kind::RELOC)
		return symbol->value().get_reloc();
	else
		return context::dependency_holder();
}

mach_expr_constant::value_t mach_expr_symbol::evaluate(mach_evaluate_info info) const
{
	auto symbol = info.get_symbol(value);

	if (symbol == nullptr || symbol->kind() == context::symbol_kind::UNDEF)
		return context::symbol_value();

	return symbol->value();
}
void mach_expr_symbol::fill_location_counter(context::address )
{
}
//***********  mach_expr_self_def ************
mach_expr_self_def::mach_expr_self_def(std::string option, std::string value, range rng) : mach_expression(rng)
{
	auto ae = arithmetic_expression::from_string(std::move(option), std::move(value), false); //could generate diagnostic + DBCS
	ae->diag->diag_range = rng;
	if (ae->has_error())
		add_diagnostic(*ae->diag);
	else
		value_ = ae->get_numeric_value();
}

context::dependency_holder mach_expr_self_def::get_dependencies(context::dependency_solver& ) const { return context::dependency_holder(); }

mach_expr_self_def::value_t mach_expr_self_def::evaluate(mach_evaluate_info ) const
{
	return value_;
}

void mach_expr_self_def::fill_location_counter(context::address addr)
{
}

mach_expr_location_counter::mach_expr_location_counter(range rng)
	: mach_expression(rng) {}

context::dependency_holder mach_expr_location_counter::get_dependencies(context::dependency_solver& ) const
{
	return context::dependency_holder(*location_counter);
}

mach_expression::value_t mach_expr_location_counter::evaluate(mach_evaluate_info ) const
{
	return *location_counter;
}

void mach_expr_location_counter::fill_location_counter(context::address addr)
{
	if (location_counter)
		throw std::runtime_error("location counter already set");

	location_counter = std::move(addr);
}

mach_expr_default::mach_expr_default(range rng)
	: mach_expression(rng) {}

context::dependency_holder mach_expr_default::get_dependencies(context::dependency_solver& ) const
{
	return context::dependency_holder();
}

mach_expression::value_t mach_expr_default::evaluate(mach_evaluate_info ) const
{
	return value_t();
}

void mach_expr_default::fill_location_counter(context::address addr)
{
}

void mach_expr_default::collect_diags() const
{
}
