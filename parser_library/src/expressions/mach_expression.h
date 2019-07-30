#ifndef HLASMPLUGIN_PARSERLIBRARY_MACH_EXPRESSION_H
#define HLASMPLUGIN_PARSERLIBRARY_MACH_EXPRESSION_H

#include <string>
#include <memory>

#include "../diagnosable_impl.h"
#include "../context/dependable.h"


namespace hlasm_plugin::parser_library::expressions
{

using mach_evaluate_info = context::dependency_solver&;


class mach_expression : public diagnosable_impl, public context::resolvable
{
	
public:
	using value_t = context::symbol_value;

	context::symbol_value resolve(context::dependency_solver& solver) const override;

	virtual value_t evaluate(mach_evaluate_info info) const = 0;

	virtual void fill_location_counter(context::address addr) = 0;

	range get_range() const;
	virtual ~mach_expression() {}

protected:

	mach_expression(range rng);

private:
	range expr_range_;
};

using mach_expr_ptr = std::unique_ptr<mach_expression>;
using mach_expr_list = std::vector<mach_expr_ptr>;


}

#endif
