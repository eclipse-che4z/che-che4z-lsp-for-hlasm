#ifndef CONTEXT_ADDRESS_H
#define CONTEXT_ADDRESS_H

#include "id_storage.h"

#include <utility>
#include <memory>
#include <vector>

namespace hlasm_plugin {
namespace parser_library {
namespace context {

class location_counter;
class section;
struct space;
struct address;

using space_ptr = std::shared_ptr<space>;
using space_storage = std::vector<space_ptr>;

//stucture representing space of unknown lenght in an adress structure (these are created when there are location counter dependencies in a code)
struct space
{
	const id_index name;

	space(location_counter& owner, id_index name);

	space(const space&) = delete;

	static void resolve(space_ptr this_space, size_t length);

	void add_listener(address* addr);

	void remove_listener(address* addr);

private:
	bool resolved_;
	location_counter& owner_;
	std::vector<address*> listeners_;
};

//structure representing relative address in a section
struct address
{
	struct base
	{
		const section* owner;
	};

	using space_entry = std::pair<space_ptr, int>;
	using base_entry = std::pair<base, int>;

	std::vector<base_entry> bases;
	int offset;
	std::vector<space_entry> spaces;

	address(base address_base, size_t offset, const space_storage& spaces);

	address(const address& addr);
	address& operator=(const address& addr);
	address(address&& addr);
	address& operator=(address&& addr);

	address operator+(const address& addr) const;
	address operator+(int offs) const;
	address operator-(const address& addr) const;
	address operator-(int offs) const;
	address operator-() const;

	~address();

private:
	address(std::vector<base_entry> bases, int offset, std::vector<space_entry> spaces);
};


}
}
}

#endif
