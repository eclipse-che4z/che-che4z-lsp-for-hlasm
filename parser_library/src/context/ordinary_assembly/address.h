#ifndef CONTEXT_ADDRESS_H
#define CONTEXT_ADDRESS_H

#include "../id_storage.h"

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
	//unique identifier
	const id_index name;

	space(location_counter& owner, id_index name);

	space(const space&) = delete;

	//fill space with a length
	static void resolve(space_ptr this_space, int length);

	void add_listener(address* addr);

	void remove_listener(address* addr);

private:
	bool resolved_;
	//loctr to witch the space belong
	location_counter& owner_;
	//addresses that contain the space
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

	~address();

private:
	address(std::vector<base_entry> bases, int offset, std::vector<space_entry> spaces);
};


}
}
}

#endif
