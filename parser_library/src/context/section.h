#ifndef CONTEXT_SECTION_H
#define CONTEXT_SECTION_H
#include "id_storage.h"
#include "location_counter.h"

#include <vector>

namespace hlasm_plugin {
namespace parser_library {
namespace context {

enum class section_kind
{
	DUMMY, COMMON, EXTERNAL, EXECUTABLE, READONLY
};


//class representing section (CSECT/DSECT ...)
class section
{
	std::vector<loctr_ptr> loctrs_;
	location_counter* curr_loctr_;

	id_storage& ids_;
public:
	const id_index name;
	const section_kind kind;

	const std::vector<loctr_ptr>& location_counters() const;

	section(id_index name, const section_kind kind, id_storage& ids);

	void set_location_counter(id_index loctr_name);

	bool counter_defined(id_index name);

	location_counter& current_location_counter() const;

	void finish_layout();
};

}
}
}
#endif
