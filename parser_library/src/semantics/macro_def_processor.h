#ifndef SEMANTICS_MACRODEFPROCESSOR_H
#define SEMANTICS_MACRODEFPROCESSOR_H

#include "statement_processor.h"
#include "context_manager.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

//struct representing state of macro definition processing
struct macro_def_info : public processor_start_info
{
	macro_def_info(symbol_range macro_range) : macro_range(macro_range) {}

	symbol_range macro_range;
};

//struct holding data for specifying prototype of the macro (its name and parameters)
struct macro_prototype
{
	context::id_index label;

	context::id_index name;

	std::vector<context::macro_arg> symbolic_params;

	macro_prototype() : label(nullptr), name(nullptr) {}
};

//processor for evaluating macro definitions
class macro_def_processor : public statement_processor
{
	std::vector<statement> macro_def_;

	macro_prototype prototype_;

	std::unique_ptr<macro_def_info> start_info_;

	size_t definition_begin_ln_;

	bool expecting_prototype_;

	op_code_info curr_op_code_;

	statement curr_statement_;
	instruction_semantic_info curr_instruction_;

	size_t macro_nest_;

public:
	macro_def_processor(processing_manager& mngr);

	void set_start_info(start_info_ptr info) override;

	void process_instruction(instruction_semantic_info instruction) override;

	void process_statement(statement statement) override;

	virtual void collect_diags() const override;

private:
	process_table_t init_table(context::hlasm_context& ctx);

	void process_prototype();

	void store_statement();

	void process_MACRO();

	void process_COPY();

	void process_MEND();
};

}
}
}
#endif
