#ifndef PROCESSING_BRANCHING_PROVIDER_H
#define PROCESSING_BRANCHING_PROVIDER_H

#include "../context/id_storage.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//interface for registering and using sequence symbols
class branching_provider
{
public:
	virtual void jump_in_statements(context::id_index target, range symbol_range) = 0;
	virtual void register_sequence_symbol(context::id_index target, range symbol_range) = 0;

	virtual ~branching_provider() = default;
};

}
}
}
#endif
