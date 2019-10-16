#ifndef CONTEXT_DEPENDENCY_COLLECTOR_H
#define CONTEXT_DEPENDENCY_COLLECTOR_H

#include <optional>
#include <set>

#include "address.h"
#include "symbol_attributes.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

//helper structure that holds dependencies throughout whole process of getting dependencies
struct dependency_collector
{
	using attr_ref = std::pair<data_attr_kind, id_index>;
	//errorous holder
	bool has_error;
	//dependent address
	std::optional<address> unresolved_address;
	//dependent symbol
	std::set<id_index> undefined_symbols;
	std::vector<attr_ref> undefined_attr_refs;

	dependency_collector();
	dependency_collector(id_index undefined_symbol);
	dependency_collector(const address& unresolved_address);
	dependency_collector(address&& unresolved_address);
	dependency_collector(attr_ref attribute_reference);

	dependency_collector& operator+(const dependency_collector& holder);
	dependency_collector& operator-(const dependency_collector& holder);
	dependency_collector& operator*(const dependency_collector& holder);
	dependency_collector& operator/(const dependency_collector& holder);

	bool is_address() const;

	bool contains_dependencies() const;

private:
	bool merge_undef(const dependency_collector& holder);

	dependency_collector& add_sub(const dependency_collector& holder, bool add);

	dependency_collector& div_mul(const dependency_collector& holder);
};

}
}
}
#endif