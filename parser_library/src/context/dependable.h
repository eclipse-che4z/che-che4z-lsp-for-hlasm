#ifndef CONTEXT_DEPENDABLE_H
#define CONTEXT_DEPENDABLE_H

#include <unordered_set>
#include <string>
#include <optional>
#include <set>

#include "symbol.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

struct dependency_holder
{
	bool has_error;
	std::optional<address> unresolved_address;
	std::set<id_index> undefined_symbols;

	dependency_holder();
	dependency_holder(const std::string* undefined_symbol);
	dependency_holder(const address& unresolved_address);
	dependency_holder(address&& unresolved_address);

	dependency_holder& operator+(const dependency_holder& holder);
	dependency_holder& operator-(const dependency_holder& holder);
	dependency_holder& operator*(const dependency_holder& holder);
	dependency_holder& operator/(const dependency_holder& holder);

	bool is_address() const;

	bool contains_dependencies() const;

private:
	bool merge_undef(const dependency_holder& holder);

	dependency_holder& add_sub(const dependency_holder& holder,bool add);

	dependency_holder& div_mul(const dependency_holder& holder);
};

//interface for obtaining symbol from its name
class dependency_solver
{
public:
	virtual symbol* get_symbol(id_index name) = 0;
};

//interface of an object that depends on another objects (addresses or symbols)
class dependable
{
public:
	virtual dependency_holder get_dependencies(dependency_solver& solver) const = 0;

	virtual ~dependable() = default;
};

//interface for obtaining symbol value from the object
class resolvable : public dependable
{
public:
	virtual symbol_value resolve(dependency_solver& solver) const = 0;
};


}
}
}
#endif
