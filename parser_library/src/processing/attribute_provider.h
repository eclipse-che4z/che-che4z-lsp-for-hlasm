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
