#ifndef SEMANTICS_CONTEXT_MANAGER_H
#define SEMANTICS_CONTEXT_MANAGER_H

#include <unordered_map>

#include "../context/instruction.h"
#include "../context/hlasm_context.h"
#include "../parse_lib_provider.h"
#include "../diagnosable_ctx.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

//struct caching important info about instructions to be accessed easily 
struct instruction_map_info
{
	context::instruction_type type;
	bool no_ops;
};

//stuct representing decoded instruction
struct op_code_info
{
	op_code_info() : op_code(nullptr), type(), has_no_ops(false), unknown(true) {}

	op_code_info(context::id_index op_code, context::instruction_type type, bool has_no_ops) :op_code(op_code), type(type), has_no_ops(has_no_ops), unknown(false) {}

	context::id_index op_code;
	context::instruction_type type;
	bool has_no_ops;
	bool unknown;
};

//class wrapping context providing ranges, checks and diagnostics to hlasm_context
class context_manager : public diagnosable_ctx
{
	context::ctx_ptr ctx_;

	parse_lib_provider & lib_provider_;
public:
	context::hlasm_context& ctx();

	//map of agregated instructions in instruction.h, holds only important info for semantic analysis and index to source arrays
	std::unordered_map<context::id_index, instruction_map_info> instructions;
	
	context_manager(context::ctx_ptr ctx);
	context_manager(context::ctx_ptr ctx, parse_lib_provider & lib_provider);

	SET_t get_var_sym_value(var_sym symbol) const;

	void set_var_sym_value_base(var_sym symbol, context::set_symbol_base* set_sym, std::vector<expr_ptr>& subscript, context::SET_t_enum type);
	template <typename T>
	void set_var_sym_value(var_sym symbol, std::vector<T> values)
	{
		auto id = symbol.created ? get_id(concatenate(std::move(symbol.created_name))) : get_id(std::move(symbol.name));

		std::vector<expr_ptr> subscript;

		auto var = ctx().get_var_sym(id);
		if (!var)
			var = ctx().create_local_variable<T>(id, symbol.subscript.size() == 0);

		set_var_sym_value_base(std::move(symbol), var->access_set_symbol_base(), subscript, context::object_traits<T>::type_enum);

		if (auto set_sym = var->access_set_symbol_base())
		{
			if (set_sym->type() != context::object_traits<T>::type_enum)
				return;

			if (subscript.empty())
				set_sym->access_set_symbol<T>()->set_value(values[0]);
			else
			{
				for (size_t i = 0; i < values.size(); ++i)
				{
					set_sym->access_set_symbol<T>()->set_value(values[i], subscript[0]->get_numeric_value() - 1 + i);
				}
			}
		}
	}

	context::id_index get_id(std::string name) const;

	//translates concat_chain to its string representation
	std::string to_string(concat_chain chain);
	//substitutes variable symbols, edit dots and concatenates chain
	std::string concatenate(concat_chain chain) const;

	expr_ptr evaluate_expression_tree(antlr4::ParserRuleContext* expr_context) const;

	context::macro_invo_ptr enter_macro(context::id_index opcode, label_semantic_info label, operand_remark_semantic_info operands);

	//translates concat_chain to macro data
	context::macro_data_ptr create_macro_data(concat_chain chain);

	op_code_info get_opcode_info(std::string name);
	op_code_info get_opcode_info(concat_chain model_name);
	op_code_info get_opcode_info(instruction_semantic_info info);

	static range convert_range(symbol_range sr);

	void collect_diags() const override;

private:
	std::string concat(char_str* str) const;
	std::string concat(var_sym* vs) const;
	std::string concat(dot*) const;
	std::string concat(equals*) const;
	std::string concat(sublist* sublist) const;

	std::vector<context::macro_arg> process_macro_args(context::id_index opcode, label_semantic_info label, operand_remark_semantic_info operands);

	void init_instr();
};

}
}
}
#endif
