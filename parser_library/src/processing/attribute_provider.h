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

#ifndef PROCESSING_ATTRIBUTE_PROVIDER_H
#define PROCESSING_ATTRIBUTE_PROVIDER_H

#include "shared/range.h"
#include "../context/ordinary_assembly/symbol_attributes.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

class attribute_provider
{
public:
	using forward_reference_storage = std::set<context::id_index>;
	using resolved_reference_storage = std::unordered_map<context::id_index, context::symbol>;

	virtual const resolved_reference_storage& lookup_forward_attribute_references(forward_reference_storage references) = 0;

protected:
	resolved_reference_storage resolved_symbols;

};

}
}
}

#endif
