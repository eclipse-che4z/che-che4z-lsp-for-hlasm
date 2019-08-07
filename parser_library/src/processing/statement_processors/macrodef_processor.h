#ifndef PROCESSING_MACRODEF_PROCESSOR_H
#define PROCESSING_MACRODEF_PROCESSOR_H

#include "macrodef_processing_info.h"
#include "statement_processor.h"
#include "../processing_state_listener.h"
#include "../../context/hlasm_context.h"
#include "../../parse_lib_provider.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//processor that creates macro definition from provided statements
class macrodef_processor : public statement_processor
{
	processing_state_listener& listener_;
	parse_lib_provider& provider_;
	const macrodef_start_data start_;

	size_t macro_nest_;
	size_t curr_line_;
	position curr_outer_position_;
	bool expecting_prototype_;
	bool expecting_MACRO_;
	bool omit_next_;
	size_t initial_copy_nest_;

	macrodef_processing_result result_;
	bool finished_flag_;
public:
	macrodef_processor(context::hlasm_context& hlasm_ctx, processing_state_listener& listener, parse_lib_provider& provider, const macrodef_start_data start);

	static context::macro_data_ptr create_macro_data(const semantics::concat_chain& chain, context::hlasm_context& hlasm_ctx);

	virtual processing_status get_processing_status(const semantics::instruction_si& instruction) const override;
	virtual void process_statement(context::unique_stmt_ptr statement) override;
	virtual void process_statement(context::shared_stmt_ptr statement) override;
	virtual void end_processing() override;
	virtual bool terminal_condition(const statement_provider_kind kind) const override;
	virtual bool finished() override;

	static processing_status get_macro_processing_status(const semantics::instruction_si& instruction, context::hlasm_context& hlasm_ctx);

	virtual void collect_diags() const override;
private:
	void process_statement(const context::hlasm_statement& statement);

	void process_prototype(const resolved_statement& statement);
	void process_MACRO();
	void process_MEND();
	void process_COPY(const resolved_statement& statement);

	void process_sequence_symbol(const semantics::label_si& label);

	void add_correct_copy_nest();
};

}
}
}
#endif
