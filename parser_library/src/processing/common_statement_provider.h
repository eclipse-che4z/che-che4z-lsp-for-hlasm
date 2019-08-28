#ifndef PROCESSING_COMMON_STATEMENT_PROVIDER_H
#define PROCESSING_COMMON_STATEMENT_PROVIDER_H

#include "statement_provider.h"
#include "statement_fields_parser.h"
#include "../context/hlasm_context.h"


namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//common class for more complicated statement providers
class common_statement_provider : public statement_provider
{
public:
	common_statement_provider(const statement_provider_kind kind, context::hlasm_context& hlasm_ctx, statement_fields_parser& parser);

protected:
	context::hlasm_context& hlasm_ctx;
	statement_fields_parser& parser;

	void preprocess_deferred(statement_processor& processor, context::shared_stmt_ptr stmt);
};

}
}
}
#endif
