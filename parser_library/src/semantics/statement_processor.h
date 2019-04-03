#ifndef SEMANTICS_STATEMENT_PROCESSOR_H
#define SEMANTICS_STATEMENT_PROCESSOR_H

#include <memory>
#include <unordered_map>
#include <functional>

#include "../diagnosable_ctx.h"
#include "statement_fields.h"
#include "../context/hlasm_context.h"


namespace hlasm_plugin {
namespace parser_library {
class parser_impl;
namespace semantics {

class processing_manager;
class context_manager;

//struct for passing processors information to specify processing
struct processor_start_info
{
	virtual ~processor_start_info() = default;
};

using start_info_ptr = std::unique_ptr<processor_start_info>;
using process_table_t = std::unordered_map<context::id_index, std::function<void()>>;

//base class for statement processors
//holds all methods that are important for derived processors to minimize friend relations in processing manager
class statement_processor : public diagnosable_ctx
{
public:
	virtual void set_start_info(start_info_ptr info) = 0;
	virtual void process_instruction(instruction_semantic_info instruction) = 0;
	virtual void process_statement(statement statement) = 0;

	virtual ~statement_processor() = default;
protected:
	statement_processor(processing_manager& mngr,std::function<process_table_t(context::hlasm_context&)> table_init);

	const process_table_t process_table;

	void jump_in_statements(hlasm_plugin::parser_library::location loc);
	bool is_last_line();

	virtual void finish();

	context_manager& ctx_mngr();
	parser_impl& parser();

	void start_lookahead(start_info_ptr start_info);
	void start_macro_definition(start_info_ptr start_info);

	const std::string & file_name();
private:
	processing_manager& mngr_;

	context::macro_invo_ptr current_macro();

};

}
}
}
#endif
