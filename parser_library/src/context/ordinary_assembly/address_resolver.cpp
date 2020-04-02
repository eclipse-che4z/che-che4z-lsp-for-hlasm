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

#include "address_resolver.h"

#include <assert.h>
#include <algorithm>

using namespace hlasm_plugin::parser_library::context;

address_resolver::address_resolver(address dependency_address_)
	: dependency_address(std::move(dependency_address_))
{
	cached_deps_.unresolved_address = extract_dep_address(dependency_address);
}

dependency_collector address_resolver::get_dependencies(dependency_solver& ) const
{
	return cached_deps_;
}

symbol_value address_resolver::resolve(dependency_solver& ) const
{
	if (!dependency_address.bases.empty())
		return dependency_address;
	else
		return dependency_address.offset;
}

address address_resolver::extract_dep_address(const address& addr)
{
	address tmp(address::base{}, 0, {});
	for (auto it = addr.spaces.rbegin(); it != addr.spaces.rend(); ++it)
	{
		tmp.spaces.push_back(*it);
		it->first->add_listener(&tmp);
		if (it->first->kind == space_kind::ALIGNMENT
			|| it->first->kind == space_kind::LOCTR_SET
			|| it->first->kind == space_kind::LOCTR_MAX)
			break;
	}
	return tmp;
}

alignable_address_resolver::alignable_address_resolver(address dependency_address, std::vector<address> base_addrs, size_t boundary, int offset)
	: address_resolver(std::move(dependency_address)), base_addrs(std::move(base_addrs)), boundary(boundary), offset(offset) {}

symbol_value alignable_address_resolver::resolve(dependency_solver& ) const
{
	return resolve(base_addrs.front());
}

symbol_value alignable_address_resolver::resolve(const address& addr) const
{
	auto al = boundary ? (boundary - addr.offset % boundary) % boundary : 0;
	return addr.offset + al + offset;
}

alignable_address_resolver::alignable_address_resolver(address dependency_address_, std::vector<address>& base_addrs_, size_t boundary_, int offset_, bool)
	: address_resolver(std::move(dependency_address_)), base_addrs(std::move(base_addrs_)), boundary(boundary_), offset(offset_) {}


alignable_address_abs_part_resolver::alignable_address_abs_part_resolver(const resolvable* dependency_source)
	: dependency_source_(dependency_source) {}

dependency_collector alignable_address_abs_part_resolver::get_dependencies(dependency_solver& solver) const
{
	auto deps = dependency_source_->get_dependencies(solver);
	deps.unresolved_address = std::nullopt;
	return deps;
}

symbol_value alignable_address_abs_part_resolver::resolve(dependency_solver& solver) const
{
	return dependency_source_->resolve(solver);
}

aggregate_address_resolver::aggregate_address_resolver(std::vector<address> base_addrs, size_t boundary, int offset)
	: alignable_address_resolver(base_addrs.back(), base_addrs, boundary, offset, false) {}

symbol_value aggregate_address_resolver::resolve(dependency_solver& ) const
{
	int max = -1;
	size_t idx = 0;
	for (size_t i = 0; i < base_addrs.size(); ++i)
	{
		if (base_addrs[i].offset > max)
		{
			max = base_addrs[i].offset;
			idx = i;
		}
	}

	return alignable_address_resolver::resolve(base_addrs[idx]);

}