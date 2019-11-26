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