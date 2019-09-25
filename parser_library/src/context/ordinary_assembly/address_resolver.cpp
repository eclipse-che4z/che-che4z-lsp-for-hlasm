#include "address_resolver.h"

#include <assert.h>
#include <algorithm>

using namespace hlasm_plugin::parser_library::context;

address_resolver::address_resolver(address addr)
	:address_(std::move(addr)) {}

dependency_collector address_resolver::get_dependencies(dependency_solver& ) const
{
	return dependency_collector(address_);
}

symbol_value address_resolver::resolve(dependency_solver& ) const
{
	if (!address_.bases.empty())
		return address_;
	else
		return address_.offset;
}