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

#ifndef CONTEXT_LOCATION_COUNTER_H
#define CONTEXT_LOCATION_COUNTER_H

#include "alignment.h"
#include "address.h"

#include <memory>

namespace hlasm_plugin {
namespace parser_library {
namespace context {

class section;
class location_counter;

using loctr_ptr = std::unique_ptr<location_counter>;

//enum stating whether the location counter is the first in section or not
enum class loctr_kind { STARTING, NONSTARTING };

//class representing section's location counter
class location_counter
{
	//count of virtually allocated bytes pior to this location counter
	size_t storage_;

	//spaces assigned to the counter
	space_storage spaces_;
	size_t last_space_;

	id_storage& ids_;

	bool layuot_created_;
public:
	const id_index name;
	const section& owner;
	const loctr_kind kind;

	const space_storage& spaces() const;
	size_t storage() const;

	location_counter(id_index name, const section& owner,const loctr_kind kind, id_storage& ids);

	//reserves storage area of specified length and alignment
	address reserve_storage_area(size_t length, alignment a);

	//aligns storage
	address align(alignment align);

	//adds space to the top of the storage
	space_ptr register_space();

	//check method whether layout can be created
	bool has_undefined_layout() const;

	//creates layout
	void finish_layout(size_t offset);

	friend space;
};


}
}
}
#endif
