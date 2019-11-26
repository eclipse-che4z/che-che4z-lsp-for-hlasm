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

#ifndef CONTEXT_ADDRESS_RESOLVER_H
#define CONTEXT_ADDRESS_RESOLVER_H

#include "dependable.h"
#include "address.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

//structure wrapping address providing resolvable interface to it
struct address_resolver : public resolvable
{
	address_resolver(address addr);

	dependency_collector get_dependencies(dependency_solver& solver) const override;

	symbol_value resolve(dependency_solver& solver) const override;

private:
	address address_;
};

}
}
}

#endif
