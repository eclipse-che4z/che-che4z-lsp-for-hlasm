#ifndef SEMANTICS_PROCESSING_MANAGER_H
#define SEMANTICS_PROCESSING_MANAGER_H

#include "../parser_impl.h"
#include "context_manager.h"
#include "statement_processor.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

class ordinary_processor;
class lookahead_processor;
class macro_def_processor;

using proc_ptr = std::unique_ptr<statement_processor>;

//class wrapping whole statement processing
class processing_manager : public diagnosable_impl
{
	//struct holding macro call context
	struct macro_frame
	{
		context::macro_invo_ptr macro_ctx;
		size_t current_statement;
	};

	context_manager ctx_;
	parser_impl* parser_;

	//statement processors for ordinary assembly, lookahead assembly and macro definition processing
	proc_ptr ord_processor_;
	proc_ptr look_processor_;
	proc_ptr mac_def_processor_;

	statement_processor* current_processor_;

	std::stack<macro_frame> macro_call_stack_;

public:
	processing_manager(context::ctx_ptr ctx);
	void initialize(parser_impl* parser);

	void process_instruction(instruction_semantic_info instr);
	void process_statement(statement stmt);

	void collect_diags() const override;

private:
	void macro_loop();
	
	friend statement_processor;
};

}
}
}
#endif
