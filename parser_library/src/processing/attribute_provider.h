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
	using resolved_reference_storage = std::vector<context::symbol>;

	virtual resolved_reference_storage lookup_forward_attribute_references(forward_reference_storage references) = 0;
};

}
}
}

#endif
