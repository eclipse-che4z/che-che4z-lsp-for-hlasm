#ifndef CONTEXT_ID_GENERATOR_H
#define CONTEXT_ID_GENERATOR_H

#include <vector>

#include "id_storage.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

//class generating id_index-es
//recycles id_index-es that has beed returned via release_id method
class id_generator
{
	id_storage& storage_;
	size_t last_id;
	std::vector<id_index> released_;

public:
	id_generator(id_storage& storage);

	id_index get_id();

	void release_id(id_index id);

	static bool is_generated(id_index id);
};


}
}
}

#endif