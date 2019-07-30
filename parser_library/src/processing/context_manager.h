#ifndef PROCESSING_CONTEXT_MANAGER_H
#define PROCESSING_CONTEXT_MANAGER_H

#include "../diagnosable_ctx.h"
#include "../context/hlasm_context.h"
#include "../parse_lib_provider.h"
#include "processing_format.h"


namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//class wrapping context providing ranges, checks and diagnostics to hlasm_context
class context_manager : public diagnosable_ctx
{
public:
	context::hlasm_context& hlasm_ctx;

	context_manager(context::hlasm_context& hlasm_ctx);

	context::SET_t get_var_sym_value(const semantics::var_sym& symbol) const;

	context::id_index get_symbol_name(const semantics::var_sym* symbol);
	context::id_index get_symbol_name(const std::string& symbol);

	context::id_index concatenate(const semantics::concat_chain& chain) const;
	std::string concatenate_str(const semantics::concat_chain& chain) const;

	expressions::expr_ptr evaluate_expression_tree(antlr4::ParserRuleContext* expr_context) const;

	virtual void collect_diags() const override;
private:
	std::string concat(semantics::char_str* str) const;
	std::string concat(semantics::var_sym* vs) const;
	std::string concat(semantics::dot*) const;
	std::string concat(semantics::equals*) const;
	std::string concat(semantics::sublist* sublist) const;

	bool test_var_sym(context::var_sym_ptr var, const expressions::expr_list& subscript, const range& symbol_range) const;
};

}
}
}
#endif
