#include "dependency_collector.h"

#include <algorithm>

using namespace hlasm_plugin::parser_library::context;

hlasm_plugin::parser_library::context::dependency_collector::dependency_collector()
	:has_error(false) {}

hlasm_plugin::parser_library::context::dependency_collector::dependency_collector(const std::string* undefined_symbol)
	: has_error(false)
{
	undefined_symbols.insert(undefined_symbol);
}

hlasm_plugin::parser_library::context::dependency_collector::dependency_collector(const address& unresolved_address)
	: has_error(false), unresolved_address(unresolved_address) {}

hlasm_plugin::parser_library::context::dependency_collector::dependency_collector(address&& unresolved_address)
	: has_error(false), unresolved_address(std::move(unresolved_address)) {}

dependency_collector& dependency_collector::operator+(const dependency_collector& holder)
{
	return add_sub(holder, true);
}

dependency_collector& dependency_collector::operator-(const dependency_collector& holder)
{
	return add_sub(holder, false);
}

dependency_collector& dependency_collector::operator*(const dependency_collector& holder)
{
	return div_mul(holder);
}

dependency_collector& dependency_collector::operator/(const dependency_collector& holder)
{
	return div_mul(holder);
}

bool hlasm_plugin::parser_library::context::dependency_collector::is_address() const
{
	return undefined_symbols.empty() && unresolved_address && !unresolved_address.value().bases.empty();
}

bool hlasm_plugin::parser_library::context::dependency_collector::contains_dependencies() const
{
	return !undefined_symbols.empty() || (unresolved_address && !unresolved_address->spaces.empty());
}

bool dependency_collector::merge_undef(const dependency_collector& holder)
{
	has_error = holder.has_error;

	if (has_error)
		return true;

	undefined_symbols.insert(holder.undefined_symbols.begin(), holder.undefined_symbols.end());

	return !undefined_symbols.empty();
}

dependency_collector& dependency_collector::add_sub(const dependency_collector& holder, bool add)
{
	bool finished = merge_undef(holder);

	if (finished)
		return *this;

	if (unresolved_address && holder.unresolved_address)
	{
		if (add)
			unresolved_address = *unresolved_address + (*holder.unresolved_address);
		else
			unresolved_address = *unresolved_address - (*holder.unresolved_address);
	}
	else if (!unresolved_address && holder.unresolved_address)
	{
		if (add)
			unresolved_address = *holder.unresolved_address;
		else
			unresolved_address = -*holder.unresolved_address;
	}

	return *this;
}

dependency_collector& dependency_collector::div_mul(const dependency_collector& holder)
{
	bool finished = merge_undef(holder);

	if (finished)
		return *this;

	if (unresolved_address || holder.unresolved_address)
		has_error = true;

	return *this;
}
