#ifndef PROCESSING_INSTRUCTION_PROCESSOR_H
#define PROCESSING_INSTRUCTION_PROCESSOR_H

#include "../../context/hlasm_context.h"
#include "../../diagnosable_ctx.h"
#include "../statement.h"

#include <unordered_map>
#include <functional>

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//interface for processing instructions
//processing is divided thrue classes for assembler, conditional assembly, machine, macro instrucion processing
class instruction_processor : public diagnosable_ctx
{
	virtual void process(context::unique_stmt_ptr stmt) = 0;
	virtual void process(context::shared_stmt_ptr stmt) = 0;

	virtual void collect_diags() const override {}
protected:
	context::hlasm_context& hlasm_ctx;

	instruction_processor(context::hlasm_context& hlasm_ctx)
		:diagnosable_ctx(hlasm_ctx), hlasm_ctx(hlasm_ctx) {}
};

}
}
}
#endif