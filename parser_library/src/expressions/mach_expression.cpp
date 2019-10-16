#include "mach_expression.h"
#include "mach_expr_term.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::expressions;

mach_expression::mach_expression( range rng) : expr_range_(rng) {}

mach_expr_ptr hlasm_plugin::parser_library::expressions::mach_expression::assign_expr(mach_expr_ptr expr,range expr_range)
{
	return expr ? std::move(expr) : std::make_unique<mach_expr_default>(expr_range);
}

range mach_expression::get_range() const
{
	return expr_range_;
}

context::symbol_value hlasm_plugin::parser_library::expressions::mach_expression::resolve(context::dependency_solver& solver) const
{
	auto tmp_val = evaluate(solver);
	if (tmp_val.value_kind() == context::symbol_value_kind::UNDEF)
		return 0;
	else
		return tmp_val;
}

