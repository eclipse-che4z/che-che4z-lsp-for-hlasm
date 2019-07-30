#include "mach_expression.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::expressions;

mach_expression::mach_expression( range rng) : expr_range_(rng) {}

range mach_expression::get_range() const
{
	return expr_range_;
}

context::symbol_value hlasm_plugin::parser_library::expressions::mach_expression::resolve(context::dependency_solver& solver) const
{
	return evaluate(solver);
}

