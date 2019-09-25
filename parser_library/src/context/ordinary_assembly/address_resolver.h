#ifndef CONTEXT_ADDRESS_RESOLVER_H
#define CONTEXT_ADDRESS_RESOLVER_H

#include "dependable.h"
#include "address.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

//structure wrapping address providing resolvable interface to it
struct address_resolver : public resolvable
{
	address_resolver(address addr);

	dependency_collector get_dependencies(dependency_solver& solver) const override;

	symbol_value resolve(dependency_solver& solver) const override;

private:
	address address_;
};

}
}
}

#endif
