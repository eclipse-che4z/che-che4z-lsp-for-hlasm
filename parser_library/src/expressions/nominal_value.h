#ifndef HLASMPLUGIN_PARSERLIBRARY_EXPRESSIONS_NOMINAL_VALUE_H
#define HLASMPLUGIN_PARSERLIBRARY_EXPRESSIONS_NOMINAL_VALUE_H

#include "mach_expression.h"

namespace hlasm_plugin::parser_library::expressions
{
struct nominal_value_string;
struct nominal_value_exprs;
struct nominal_value_address;

struct nominal_value_t : public context::dependable
{
	nominal_value_string* access_string();
	nominal_value_exprs* access_exprs();
	nominal_value_address* access_address();

	virtual ~nominal_value_t() = default;
};

using nominal_value_ptr = std::unique_ptr<nominal_value_t>;

struct nominal_value_string final : public nominal_value_t
{
	virtual context::dependency_holder get_dependencies(context::dependency_solver& solver) const override;

	nominal_value_string(std::string value);
	std::string value;
	range value_range;
};

struct nominal_value_exprs final : public nominal_value_t
{
	virtual context::dependency_holder get_dependencies(context::dependency_solver& solver) const override;

	nominal_value_exprs(mach_expr_list exprs);
	mach_expr_list exprs;
};

struct nominal_value_address final : public nominal_value_t
{
	virtual context::dependency_holder get_dependencies(context::dependency_solver& solver) const override;

	nominal_value_address(mach_expr_ptr displacement, mach_expr_ptr base);
	mach_expr_ptr displacement;
	mach_expr_ptr base;
};

}
#endif