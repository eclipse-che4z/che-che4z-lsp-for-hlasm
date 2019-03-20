#ifndef SEMANTICS_LOOKAHEADPROCESSOR_H
#define SEMANTICS_LOOKAHEADPROCESSOR_H

#include "statement_processor.h"
#include "../context/variable.h"
#include "context_manager.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

//struct represents state of lookahead
struct lookahead_info : public processor_start_info
{
	//describes whether we are looking for sequence symbol or ordinary symbol
	enum la_type { SEQ, ORD };

	lookahead_info(context::id_index target, parser_library::location location, la_type type, symbol_range target_range) : target(target), location(location), type(type),target_range(target_range) {}

	context::id_index target;
	parser_library::location location;

	symbol_range target_range;

	//describes whether we are looking for sequence symbol or ordinary symbol
	la_type type;
};

//processor of statements inside of lookahead mode
class lookahead_processor : public statement_processor
{
	size_t macro_nest_;
	std::unique_ptr<lookahead_info> start_info_;
	op_code_info curr_op_code_;
	label_semantic_info curr_label_;
	bool failed;
public:
	lookahead_processor(processing_manager& mngr);

	void set_start_info(start_info_ptr info) override;

	void process_instruction(instruction_semantic_info instruction) override;

	void process_statement(statement statement) override;

	virtual void collect_diags() const override;

protected:
	void finish() override;

private:
	void process_COPY();

	void process_MACRO();

	void process_MEND();

	void process_label_field();

	process_table_t init_table(context::hlasm_context& ctx);
};



}
}
}
#endif
