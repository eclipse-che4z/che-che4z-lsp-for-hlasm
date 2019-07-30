#ifndef PROCESSING_MACRO_STATEMENT_PROVIDER_H
#define PROCESSING_MACRO_STATEMENT_PROVIDER_H

#include "statement_provider.h"
#include "deferred_parser.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

class processing_manager;

//statement provider providing statements of macro defintion
class macro_statement_provider : public statement_provider
{
	context::hlasm_context& hlasm_ctx_;
	statement_field_reparser& parser_;
public:
	macro_statement_provider(context::hlasm_context& hlasm_ctx, statement_field_reparser& parser);

	virtual void process_next(statement_processor& processor) override;

	virtual bool finished() const override;

	void preprocess(statement_processor& processor, context::shared_stmt_ptr stmt);
};

}
}
}
#endif
