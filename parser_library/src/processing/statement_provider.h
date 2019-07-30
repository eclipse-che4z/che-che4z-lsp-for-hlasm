#ifndef PROCESSING_STATEMENT_PROVIDER_H
#define PROCESSING_STATEMENT_PROVIDER_H

#include "statement_processors/statement_processor.h"
#include "statement_provider_kind.h"
#include "../context/hlasm_context.h"


namespace hlasm_plugin {
namespace parser_library {
namespace processing {

class statement_provider;
using provider_ptr = std::unique_ptr<statement_provider>;

//interface for statement providers
//till they are finished they provide statements to statement processors
class statement_provider
{
public:
	statement_provider(const statement_provider_kind kind) : kind(kind) {}

	virtual void process_next(statement_processor& processor) = 0;

	virtual bool finished() const = 0;

	const statement_provider_kind kind;

	virtual ~statement_provider() = default;
};

}
}
}
#endif
