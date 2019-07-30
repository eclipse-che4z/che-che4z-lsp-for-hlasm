#include "dependable.h"
#include "dependable.h"

#include <algorithm>

using namespace hlasm_plugin::parser_library::context;

hlasm_plugin::parser_library::context::dependency_holder::dependency_holder()
	:has_error(false) {}

hlasm_plugin::parser_library::context::dependency_holder::dependency_holder(const std::string* undefined_symbol)
	: has_error(false)
{
	undefined_symbols.insert(undefined_symbol);
}

hlasm_plugin::parser_library::context::dependency_holder::dependency_holder(const address& unresolved_address)
	: has_error(false), unresolved_address(unresolved_address) {}

hlasm_plugin::parser_library::context::dependency_holder::dependency_holder(address&& unresolved_address)
	: has_error(false), unresolved_address(std::move(unresolved_address)) {}

dependency_holder& dependency_holder::operator+(const dependency_holder& holder)
{
	return add_sub(holder, true);
}

dependency_holder& dependency_holder::operator-(const dependency_holder& holder)
{
	return add_sub(holder, false);
}

dependency_holder& dependency_holder::operator*(const dependency_holder& holder)
{
	return div_mul(holder);
}

dependency_holder& dependency_holder::operator/(const dependency_holder& holder)
{
	return div_mul(holder);
}

bool hlasm_plugin::parser_library::context::dependency_holder::is_address() const
{
	return undefined_symbols.empty() && unresolved_address && !unresolved_address.value().bases.empty();
}

bool hlasm_plugin::parser_library::context::dependency_holder::contains_dependencies() const
{
	return !undefined_symbols.empty() || (unresolved_address && !unresolved_address->spaces.empty());
}

bool dependency_holder::merge_undef(const dependency_holder& holder)
{
	has_error = holder.has_error;

	if (has_error)
		return true;

	undefined_symbols.insert(holder.undefined_symbols.begin(), holder.undefined_symbols.end());

	return !undefined_symbols.empty();
}

dependency_holder& dependency_holder::add_sub(const dependency_holder& holder, bool add)
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
		if(add)
			unresolved_address = *holder.unresolved_address;
		else
			unresolved_address = -*holder.unresolved_address;
	}

	return *this;
}

dependency_holder& dependency_holder::div_mul(const dependency_holder& holder)
{
	bool finished = merge_undef(holder);

	if (finished)
		return *this;

	if (unresolved_address || holder.unresolved_address)
		has_error = true;

	return *this;
}
