#ifndef PROCESSING_DEFERRED_PARSER_H
#define PROCESSING_DEFERRED_PARSER_H

#include "../context/hlasm_context.h"
#include "../semantics/range_provider.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

class statement_provider;
using provider_ptr = std::unique_ptr<statement_provider>;

//interface for class parsing deferred statement fields
class statement_field_reparser
{
public:
	using parse_result = std::pair<semantics::operands_si, semantics::remarks_si>;

	virtual parse_result reparse_operand_field(context::hlasm_context* hlasm_ctx, std::string field, semantics::range_provider field_range, processing::processing_status status) = 0;

	virtual ~statement_field_reparser() = default;
};

}
}
}
#endif
