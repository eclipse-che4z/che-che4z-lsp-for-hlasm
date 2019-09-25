#ifndef PROCESSING_STATEMENT_FIELDS_PARSER_H
#define PROCESSING_STATEMENT_FIELDS_PARSER_H

#include "../context/hlasm_context.h"
#include "../semantics/range_provider.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

class statement_provider;
using provider_ptr = std::unique_ptr<statement_provider>;

//interface for objects parsing deferred statement fields
class statement_fields_parser
{
public:
	using parse_result = std::pair<semantics::operands_si, semantics::remarks_si>;

	virtual parse_result parse_operand_field(
		context::hlasm_context* hlasm_ctx, std::string field, bool after_substitution, semantics::range_provider field_range, processing::processing_status status) = 0;

	virtual ~statement_fields_parser() = default;
};

}
}
}
#endif
