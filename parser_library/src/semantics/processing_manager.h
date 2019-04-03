#ifndef SEMANTICS_PROCESSING_MANAGER_H
#define SEMANTICS_PROCESSING_MANAGER_H

#include "../parser_impl.h"
#include "context_manager.h"
#include "statement_processor.h"
#include "../parse_lib_provider.h"
#include "../diagnosable_ctx.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

class ordinary_processor;
class lookahead_processor;
class macro_def_processor;

using proc_ptr = std::unique_ptr<statement_processor>;

//class wrapping whole statement processing
class processing_manager : public diagnosable_ctx
{
	context_manager ctx_mngr_;
	parser_impl* parser_;

	//statement processors for ordinary assembly, lookahead assembly and macro definition processing
	proc_ptr ord_processor_;
	proc_ptr look_processor_;
	proc_ptr mac_def_processor_;

	statement_processor* current_processor_;


	std::string file_name_;
public:
	processing_manager(context::ctx_ptr ctx);
	processing_manager(std::string file_name, context::ctx_ptr ctx, parse_lib_provider & lib_provider);
	void initialize(parser_impl* parser);

	void process_instruction(instruction_semantic_info instr);
	void process_statement(statement stmt);

	void collect_diags() const override;

	const std::string & file_name();
private:
	void macro_loop();
	
	friend statement_processor;
};

}
}
}
#endif
