#ifndef CONTEXT_COPY_MEMBER_H
#define CONTEXT_COPY_MEMBER_H

#include "shared/range.h"
#include "hlasm_statement.h"
#include "id_storage.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

//structure represents invocation of COPY member in HLASM macro library
struct copy_member_invocation
{
	const id_index name;
	const statement_block& definition;
	const location& definition_location;
	int current_statement;

	copy_member_invocation(const id_index name, const statement_block& definition, const location& definition_location)
		:name(name), definition(definition), definition_location(definition_location), current_statement(-1) {}
};

//structure represents COPY member in HLASM macro library
struct copy_member
{
	//member idenifier
	const id_index name;
	//block of statements defining the member
	const statement_block definition;
	//location of the definition
	const location definition_location;

	copy_member(id_index name, statement_block definition, location definition_location)
		:name(name), definition(std::move(definition)), definition_location(std::move(definition_location)) {}

	copy_member_invocation enter() { return copy_member_invocation(name, definition, definition_location); }
};

}
}
}
#endif
