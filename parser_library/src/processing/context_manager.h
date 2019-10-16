#ifndef PROCESSING_CONTEXT_MANAGER_H
#define PROCESSING_CONTEXT_MANAGER_H

#include "../diagnosable_ctx.h"
#include "../context/hlasm_context.h"
#include "../parse_lib_provider.h"
#include "../expressions/expression.h"
#include "../expressions/evaluation_context.h"
#include "processing_format.h"
#include "antlr4-runtime.h"


namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//class wrapping context providing ranges, checks and diagnostics to hlasm_context
class context_manager : public diagnosable_ctx
{
public:
	using name_result = std::pair<bool, context::id_index>;

	//wrapped context
	context::hlasm_context& hlasm_ctx;

	context_manager(context::hlasm_context& hlasm_ctx);

	expressions::expr_ptr evaluate_expression(antlr4::ParserRuleContext* expr_context, expressions::evaluation_context eval_ctx) const;

	context::SET_t get_var_sym_value(const semantics::var_sym& symbol, expressions::evaluation_context eval_ctx) const;
	context::SET_t get_var_sym_value(context::id_index name, const expressions::expr_list& subscript, const range& symbol_range) const;

	name_result try_get_symbol_name(const semantics::var_sym* symbol, expressions::evaluation_context eval_ctx) const;
	name_result try_get_symbol_name(const std::string& symbol,range symbol_range) const;

	context::id_index concatenate(const semantics::concat_chain& chain, expressions::evaluation_context eval_ctx) const;
	std::string concatenate_str(const semantics::concat_chain& chain, expressions::evaluation_context eval_ctx) const;

	context::macro_data_ptr create_macro_data(const semantics::concat_chain& chain) const;
	context::macro_data_ptr create_macro_data(const semantics::concat_chain& chain, expressions::evaluation_context eval_ctx) const;

	bool test_symbol_for_read(context::var_sym_ptr var, const expressions::expr_list& subscript, const range& symbol_range) const;

	virtual void collect_diags() const override;

private:
	context::macro_data_ptr create_macro_data(const semantics::concat_chain& chain, 
		const std::function<std::string(const semantics::concat_chain& chain)>& to_string) const;
};

}
}
}
#endif
