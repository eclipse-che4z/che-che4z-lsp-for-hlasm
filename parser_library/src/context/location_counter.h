#ifndef CONTEXT_LOCATION_COUNTER_H
#define CONTEXT_LOCATION_COUNTER_H

#include "id_storage.h"
#include "alignment.h"
#include "address.h"

#include <memory>

namespace hlasm_plugin {
namespace parser_library {
namespace context {

class section;
class location_counter;

using loctr_ptr = std::unique_ptr<location_counter>;

enum class loctr_kind { STARTING, NONSTARTING };

//class representing section's location counter
class location_counter
{
	size_t storage_;

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
	template<size_t byte, size_t boundary>
	address reserve_storage_area(size_t length, alignment<byte, boundary>)
	{
		if (storage_ % boundary != byte)
			storage_ += (boundary - (storage_ % boundary)) + byte;

		storage_ += length;

		return address({&owner}, (int)storage_, spaces_);
	}

	//aligns storage
	template<size_t byte, size_t boundary>
	address align(alignment<byte, boundary> align)
	{
		return reserve_storage_area(0, align);
	}

	space_ptr register_space();

	bool has_undefined_layout() const;

	void finish_layout(size_t offset);

	friend space;
};


}
}
}
#endif
