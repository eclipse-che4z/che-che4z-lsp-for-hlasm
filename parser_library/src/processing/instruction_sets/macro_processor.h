#ifndef PROCESSING_MACRO_PROCESSOR_H
#define PROCESSING_MACRO_PROCESSOR_H

#include "instruction_processor.h"
#include "../../context/macro.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

struct macro_arguments 
{
	context::macro_data_ptr name_param;
	std::vector<context::macro_arg> symbolic_params;
};

//processor of macro instructions
class macro_processor : public instruction_processor
{
public:
	macro_processor(context::hlasm_context& hlasm_ctx);
	virtual void process(context::unique_stmt_ptr stmt) override;
	virtual void process(context::shared_stmt_ptr stmt) override;

	static context::macro_data_ptr string_to_macrodata(std::string data);

private:
	macro_arguments get_args(const resolved_statement& statement) const;
};

}
}
}
#endif