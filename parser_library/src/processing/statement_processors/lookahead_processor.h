#ifndef PROCESSING_LOOKAHEAD_PROCESSOR_H
#define PROCESSING_LOOKAHEAD_PROCESSOR_H

#include "../../parse_lib_provider.h"
#include "../processing_state_listener.h"
#include "../branching_provider.h"
#include "lookahead_processing_info.h"
#include "statement_processor.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//processor used for lookahead, hence finding desired symbol
class lookahead_processor : public statement_processor
{
	bool finished_flag_;
	lookahead_processing_result result_;
	size_t macro_nest_;
	branching_provider& branch_provider_;
	processing_state_listener& listener_;
	parse_lib_provider& lib_provider_;
	const context::id_index equ_id_;
public:
	const lookahead_start_data start;

	lookahead_processor(
		context::hlasm_context& hlasm_ctx,
		branching_provider& branch_provider, processing_state_listener& listener, parse_lib_provider& lib_provider, const lookahead_start_data start);

	virtual processing_status get_processing_status(const semantics::instruction_si& instruction) const override;
	virtual void process_statement(context::unique_stmt_ptr statement) override;
	virtual void process_statement(context::shared_stmt_ptr statement) override;
	virtual void end_processing() override;
	virtual bool terminal_condition(const statement_provider_kind kind) const override;
	virtual bool finished() override;

	virtual void collect_diags() const override;
private:
	void process_MACRO();
	void process_MEND();
	void process_COPY(const resolved_statement& statement);

	void process_statement(const context::hlasm_statement& statement);

	void find_target(const context::hlasm_statement& statement);
	void find_seq(const semantics::core_statement& statement);
};

}
}
}
#endif
