#ifndef PROCESSING_COPY_PROCESSOR_H
#define PROCESSING_COPY_PROCESSOR_H

#include "copy_processing_info.h"
#include "statement_processor.h"
#include "../processing_state_listener.h"
#include "../../context/hlasm_context.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//processor that processes copy members
class copy_processor : public statement_processor
{
	processing_state_listener& listener_;
	const copy_start_data start_;

	int macro_nest_;

	copy_processing_result result_;
	bool first_statement_;
public:
	copy_processor(context::hlasm_context& hlasm_ctx, processing_state_listener& listener, const copy_start_data start);

	virtual processing_status get_processing_status(const semantics::instruction_si& instruction) const override;
	virtual void process_statement(context::unique_stmt_ptr statement) override;
	virtual void process_statement(context::shared_stmt_ptr statement) override;
	virtual void end_processing() override;
	virtual bool terminal_condition(const statement_provider_kind kind) const override;
	virtual bool finished() override;

	virtual void collect_diags() const override;
private:
	void process_statement(const context::hlasm_statement& statement);

	void process_MACRO();
	void process_MEND();
};

}
}
}
#endif
