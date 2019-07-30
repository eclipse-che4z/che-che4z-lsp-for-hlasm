#include "nominal_value.h"
#include <unordered_set>

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library::context;

nominal_value_string* nominal_value_t::access_string()
{
	return dynamic_cast<nominal_value_string*>(this);
}

nominal_value_exprs* nominal_value_t::access_exprs()
{
	return dynamic_cast<nominal_value_exprs*>(this);
}

nominal_value_address* nominal_value_t::access_address()
{
	return dynamic_cast<nominal_value_address*>(this);
}

//*********** nominal_value_string ***************
dependency_holder nominal_value_string::get_dependencies(dependency_solver&) const
{
	return dependency_holder();
}

nominal_value_string::nominal_value_string(std::string value)
	: value(std::move(value)) {}



//*********** nominal_value_exprs ***************
dependency_holder nominal_value_exprs::get_dependencies(dependency_solver& solver) const
{
	dependency_holder conjunction;
	for (auto& e : exprs)
	{
		auto list = e->get_dependencies(solver);
		conjunction.undefined_symbols.insert(list.undefined_symbols.begin(), list.undefined_symbols.end());
	}
	return conjunction;
}

nominal_value_exprs::nominal_value_exprs(mach_expr_list exprs)
	: exprs(std::move(exprs)) {}




//*********** nominal_value_list ***************
dependency_holder nominal_value_address::get_dependencies(dependency_solver& solver) const
{
	auto conjunction = base->get_dependencies(solver);

	auto list2 = displacement->get_dependencies(solver);
	conjunction.undefined_symbols.insert(list2.undefined_symbols.begin(), list2.undefined_symbols.end());
	
	return conjunction;
}

nominal_value_address::nominal_value_address(mach_expr_ptr displacement, mach_expr_ptr base)
	: displacement(std::move(displacement)), base(std::move(base)) {}


