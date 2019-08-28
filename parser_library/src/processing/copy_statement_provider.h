#ifndef PROCESSING_COPY_STATEMENT_PROVIDER_H
#define PROCESSING_COPY_STATEMENT_PROVIDER_H

#include "common_statement_provider.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//statement provider providing statements of macro defintion
class copy_statement_provider : public common_statement_provider
{
	size_t initial_nest_;
public:
	copy_statement_provider(context::hlasm_context& hlasm_ctx, statement_fields_parser& parser);

	virtual void process_next(statement_processor& processor) override;

	virtual bool finished() const override;
};

}
}
}
#endif
