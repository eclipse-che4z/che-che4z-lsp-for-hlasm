#ifndef CONTEXT_DEPENDABLE_H
#define CONTEXT_DEPENDABLE_H

#include "dependency_collector.h"
#include "symbol.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

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
	virtual dependency_collector get_dependencies(dependency_solver& solver) const = 0;

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
