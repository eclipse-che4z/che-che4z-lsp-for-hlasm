#ifndef PROCESSING_LOW_LANGUAGE_PROCESSOR_H
#define PROCESSING_LOW_LANGUAGE_PROCESSOR_H

#include "instruction_processor.h"
#include "../branching_provider.h"
#include "../statement_fields_parser.h"
#include "../../checking/instruction_checker.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//common ancestor for ASM and MACH processing containing useful methods
class low_language_processor : public instruction_processor
{
public:
	static void check(const resolved_statement& stmt, context::hlasm_context& hlasm_ctx, checking::instruction_checker& checker, diagnosable_ctx& diagnoser);

protected:
	branching_provider& provider;
	statement_fields_parser& parser;

	low_language_processor(context::hlasm_context& hlasm_ctx, branching_provider& provider, statement_fields_parser& parser);

	rebuilt_statement preprocess(context::unique_stmt_ptr stmt);
	rebuilt_statement preprocess(context::shared_stmt_ptr stmt);

	//adds dependency and also check for cyclic dependency and adds diagnostics if so
	template <typename... Args>
	bool add_dependency(range err_range, Args&&... args)
	{
		bool cycle_ok = hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(std::forward<Args>(args)...);
		if (!cycle_ok)
			add_diagnostic(diagnostic_op::error_E033(err_range));
		return cycle_ok;
	}

	void create_symbol(range err_range, context::id_index symbol_name, context::symbol_value value, context::symbol_attributes attributes);

private:
	using preprocessed_part = std::pair<std::optional<semantics::label_si>, std::optional<semantics::operands_si>>;
	preprocessed_part preprocess_inner(const resolved_statement_impl& stmt);

	using transform_result = std::optional<std::vector<checking::check_op_ptr>>;
	//transform semantic operands to checking operands - machine mnemonics instructions
	static transform_result transform_mnemonic(const resolved_statement& stmt, context::hlasm_context& hlasm_ctx, diagnostic_collector collector);
	//transform semantic operands to checking operands - default machine instructions
	static transform_result transform_default(const resolved_statement& stmt, context::hlasm_context& hlasm_ctx, diagnostic_collector collector);

	static checking::check_op_ptr get_check_op(
		const semantics::operand* op, context::hlasm_context& hlasm_ctx, diagnostic_collector collector, const resolved_statement& stmt,
		size_t op_position, const std::string* mnemonic = nullptr);
};

}
}
}
#endif