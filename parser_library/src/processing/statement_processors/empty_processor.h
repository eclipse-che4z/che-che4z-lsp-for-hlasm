#ifndef PROCESSING_EMPTY_PROCESSOR_H
#define PROCESSING_EMPTY_PROCESSOR_H

#include "statement_processor.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//processor ignoring provided statements
class empty_processor : public statement_processor
{
public:
	empty_processor(context::hlasm_context& hlasm_ctx);
	virtual processing_status get_processing_status(const semantics::instruction_si& instruction) const override;
	virtual void process_statement(context::unique_stmt_ptr statement) override;
	virtual void process_statement(context::shared_stmt_ptr statement) override;
	virtual void end_processing() override;
	virtual bool terminal_condition(const statement_provider_kind kind) const override;
	virtual bool finished() override;

	virtual void collect_diags() const override;
};

}
}
}
#endif
