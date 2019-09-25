#ifndef CONTEXT_DEPENDENCY_COLLECTOR_H
#define CONTEXT_DEPENDENCY_COLLECTOR_H

#include <optional>
#include <set>

#include "address.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

//helper structure that holds dependencies throughout whole process of getting dependencies
struct dependency_collector
{
	//errorous holder
	bool has_error;
	//dependent address
	std::optional<address> unresolved_address;
	//dependent symbol
	std::set<id_index> undefined_symbols;

	dependency_collector();
	dependency_collector(const std::string* undefined_symbol);
	dependency_collector(const address& unresolved_address);
	dependency_collector(address&& unresolved_address);

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