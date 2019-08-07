#ifndef PROCESSING_LOW_LANGUAGE_PROCESSOR_H
#define PROCESSING_LOW_LANGUAGE_PROCESSOR_H

#include "instruction_processor.h"
#include "../branching_provider.h"
#include "../deferred_parser.h"
#include "../../checking/instruction_checker.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//common ancestor for ASM and MACH processing containing useful methods
class low_language_processor : public instruction_processor
{
public:
	static void check(const resolved_statement& stmt, context::hlasm_context& hlasm_ctx, checking::instruction_checker& checker, diagnosable& diagnoser);

protected:
	branching_provider& provider;
	statement_field_reparser& parser;

	low_language_processor(context::hlasm_context& hlasm_ctx, branching_provider& provider,statement_field_reparser& parser);

	rebuilt_statement preprocess(context::unique_stmt_ptr stmt);
	rebuilt_statement preprocess(context::shared_stmt_ptr stmt);

	template <typename... Args>
	void add_dependency(range err_range, Args&&... args)
	{
		bool cycle_ok = hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(std::forward<Args>(args)...);
		if (!cycle_ok)
			add_diagnostic(diagnostic_s::error_E033("", "", err_range));
	}

private:
	using preprocessed_part = std::pair<std::optional<semantics::label_si>, std::optional<semantics::operands_si>>;
	preprocessed_part preprocess_inner(const resolved_statement_impl& stmt);

	using transform_result = std::optional<std::vector<checking::check_op_ptr>>;
	static transform_result transform_mnemonic(const resolved_statement& stmt, context::hlasm_context& hlasm_ctx, diagnosable& diagnoser);
	static transform_result transform_default(const resolved_statement& stmt, context::hlasm_context& hlasm_ctx, diagnosable& diagnoser);

	static checking::check_op_ptr get_check_op(const semantics::operand* op, context::hlasm_context& hlasm_ctx, diagnosable& diagnoser, const resolved_statement& stmt);
};

}
}
}
#endif