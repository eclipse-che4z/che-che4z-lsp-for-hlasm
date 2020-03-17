/*
 * Copyright (c) 2019 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

#include "dependency_collector.h"

#include <algorithm>

using namespace hlasm_plugin::parser_library::context;

dependency_collector::dependency_collector()
	:has_error(false) {}

dependency_collector::dependency_collector(id_index undefined_symbol)
	: has_error(false)
{
	undefined_symbols.insert(undefined_symbol);
}

dependency_collector::dependency_collector(address unresolved_address)
	: has_error(false), unresolved_address(std::move(unresolved_address)) {}

dependency_collector::dependency_collector(attr_ref attribute_reference)
	:has_error(false)
{
	undefined_attr_refs.push_back(std::move(attribute_reference));
}

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

bool dependency_collector::is_address() const
{
	return undefined_symbols.empty() && unresolved_address && !unresolved_address.value().bases.empty();
}

bool dependency_collector::contains_dependencies() const
{
	return !undefined_symbols.empty() || !undefined_attr_refs.empty() || (unresolved_address && !unresolved_address->spaces.empty());
}

bool dependency_collector::merge_undef(const dependency_collector& holder)
{
	has_error = holder.has_error;

	if (has_error)
		return true;

	undefined_symbols.insert(holder.undefined_symbols.begin(), holder.undefined_symbols.end());

	undefined_attr_refs.insert(undefined_attr_refs.end(), holder.undefined_attr_refs.begin(), holder.undefined_attr_refs.end());

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
		adjust_address(*unresolved_address);
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

void dependency_collector::adjust_address(address& addr)
{
	auto unknown_space = std::find_if(addr.spaces.begin(), addr.spaces.end(), [](auto& entry) { return entry.first->kind == context::space_kind::LOCTR_UNKNOWN; });
	if (unknown_space != addr.spaces.end())
	{
		for (int i = addr.spaces.size() - 1; i > 0; --i)
		{
			if (addr.spaces[i].first->kind != context::space_kind::LOCTR_UNKNOWN)
			{
				addr.spaces[i].first->remove_listener(&addr);
				addr.spaces.erase(addr.spaces.begin() + i);
			}
		}
	}
}
