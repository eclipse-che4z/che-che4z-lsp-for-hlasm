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

#ifndef CONTEXT_ADDRESS_H
#define CONTEXT_ADDRESS_H

#include "../id_storage.h"
#include "alignment.h"

#include <utility>
#include <memory>
#include <vector>
#include <variant>

namespace hlasm_plugin {
namespace parser_library {
namespace context {

class location_counter;
class section;
struct space;
struct address;

using space_ptr = std::shared_ptr<space>;
using space_storage = std::vector<space_ptr>;
using aligned_addr = std::pair<address, space_ptr>;


//structure representing relative address in a section
struct address
{
	struct base
	{
		const section* owner;
	};

	std::string to_string() const;

	using space_entry = std::pair<space_ptr, int>;
	using base_entry = std::pair<base, int>;

	//list of bases and their counts to which is the address relative
	std::vector<base_entry> bases;
	//offset relative to bases
	int offset;
	//list of spaces with their counts this address contains
	std::vector<space_entry> spaces;

	address(base address_base, int offset, const space_storage& spaces);

	address(const address& addr);
	address& operator=(const address& addr);
	address(address&& addr);
	address& operator=(address&& addr);

	address operator+(const address& addr) const;
	address operator+(int offs) const;
	address operator-(const address& addr) const;
	address operator-(int offs) const;
	address operator-() const;

	bool is_complex() const;
	bool in_same_loctr(const address& addr) const;
	bool is_simple() const;
	bool has_dependant_space() const;

	~address();

private:
	address(std::vector<base_entry> bases, int offset, std::vector<space_entry> spaces);
};

enum class space_kind
{
	ORDINARY = 'O', LOCTR_BEGIN = 'B', ALIGNMENT = 'A', LOCTR_SET = 'S', LOCTR_MAX = 'M', LOCTR_UNKNOWN = 'U'
};

//stucture representing space of unknown lenght in an adress structure (these are created when there are location counter dependencies in a code)
struct space
{
	const space_kind kind;
	//alignment of space end address
	alignment align;

	//previous address values
	//for LOCTR_UNKNOWN space kind
	address previous_loctr_value;
	size_t previous_boundary;
	int previous_offset;

	location_counter& owner;

	space(location_counter& owner, alignment align, space_kind kind);
	space(location_counter& owner, alignment align, address previous_loctr_value, size_t boundary, int offset);

	space(const space&) = delete;

	//fill space with a length
	static void resolve(space_ptr this_space, int length);
	//replace space with another
	static void resolve(space_ptr this_space, space_ptr value);
	//fill space with the whole address
	static void resolve(space_ptr this_space, address value);
	//common resolver for 2 methods above
	static void resolve(space_ptr this_space, std::variant<space_ptr, address> value);

	void add_listener(address* addr);
	void remove_listener(address* addr);

private:
	bool resolved_;
	//loctr to witch the space belong
	//addresses that contain the space
	std::vector<address*> listeners_;
};


}
}
}

#endif
