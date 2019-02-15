#ifndef SEMANTICS_SEMANTICANALYZER_H
#define SEMANTICS_SEMANTICANALYZER_H

#include <unordered_map>
#include <functional>

#include "../include/shared/lexer.h"
#include "semantic_objects.h"
#include "statement_fields.h"
#include "concatenation.h"
#include "expression.h"
#include "../context/hlasm_context.h"
#include "../diagnosable_impl.h"
#include "../checking/instruction_checker.h"
#include "../checking/instr_operand.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context
{
	class expression_visitor;
}
namespace semantics {

//struct represents state of lookahead
struct look_ahead
{
	//describes whether we are looking for sequence symbol or ordinary symbol
	enum la_type { SEQ, ORD };

	look_ahead() : active(false) {}

	look_ahead(context::sequence_symbol target, la_type type): active(true), target(target),type(type) {}

	context::sequence_symbol target;
	bool active;

	la_type type;
};

struct instruction_map_info
{
	context::instruction_type type;
	bool no_ops;
	size_t source;
};

class ordinary_processor;
class lookahead_processor;


//class that does semantic analysis over parsing tree
//collects info about current statement and processes it
//collects info regarding LSP requests  
class semantic_analyzer : public diagnosable_impl
{
	static_assert(sizeof(size_t) >= sizeof(int32_t));

	statement curr_statement_;

	//map of agregated instructions in instruction.h, holds only important info for semantic analysis and index to source arrays
	std::unordered_map<context::id_index, instruction_map_info> instructions_;

	//statement processors for ordinary assembly and lookahead assembly
	std::unique_ptr<ordinary_processor> ord_processor_;
	std::unique_ptr<lookahead_processor> look_processor_;

	look_ahead lookahead_;
	bool lookahead_failed_;

	std::shared_ptr<context::hlasm_context> ctx_;
	lexer* lexer_;
	std::string file_name_;
public:
	context::hlasm_context& context();
	lexer* get_lexer();

	void initialize(std::string file_name, std::shared_ptr<context::hlasm_context> ctx_init, hlasm_plugin::parser_library::lexer* lexer_init);
	void initialize(const semantic_analyzer& analyzer);

	const label_semantic_info& current_label();
	const instruction_semantic_info& current_instruction();
	operand_remark_semantic_info& current_operands_and_remarks();

	void set_label_field();
	void set_label_field(std::string label);
	void set_label_field(seq_sym sequence_symbol);
	void set_label_field(std::string label, antlr4::ParserRuleContext* ctx);
	void set_label_field(std::vector<concat_point_ptr> label);

	void set_instruction_field(std::string instr);
	void set_instruction_field(std::vector<concat_point_ptr> instr);

	void set_operand_remark_field(std::string name);
	void set_operand_remark_field(std::vector<operand_ptr> operands, std::vector<symbol_range> remarks);

	void set_statement_field(symbol_range range);

	void set_statement_range(symbol_range range);

	void process_statement();

	bool in_lookahead();

	set_type get_var_sym_value(context::id_index id, const std::vector<expr_ptr>& subscript, symbol_range range) const;

	static range convert_range(const symbol_range & range);

	template <typename T>
	void set_var_sym_value(context::id_index id, const std::vector<expr_ptr>& subscript, T value, symbol_range range, size_t offset = 0)
	{
		auto var = ctx_->get_var_sym(id);
		if (!var)
			var = ctx_->create_local_variable<T>(id, subscript.size() == 0);


		if (auto set_sym = var->access_set_symbol_base())
		{
			if (subscript.size() > 1)
			{
				add_diagnostic(diagnostic_s::error_E020("", "variable symbol subscript", convert_range(range) )); //error - too many operands
			}
			else if ((set_sym->is_scalar && subscript.size() == 1) || (!set_sym->is_scalar && subscript.size() == 0))
			{
				add_diagnostic(diagnostic_s::error_E013("", "subscript error", convert_range(range))); //error - inconsistent format of subcript
			}

			if (!set_sym->is_scalar && subscript.size() != 0 && subscript[0] && subscript[0]->get_numeric_value() < 1)
			{
				add_diagnostic(diagnostic_s::error_E012("", "subscript value has to be 1 or more", convert_range(range))); //error - subscript is less than 1
			}

			if (set_sym->type() != context::object_traits<T>::type_enum)
			{
				add_diagnostic(diagnostic_s::error_E013("", "wrong type of variable symbol", convert_range(range))); //error - wrong type of variable symbol
				return;
			}

			if (subscript.empty())
				set_sym->access_set_symbol<T>()->set_value(value);
			else
			{
				size_t tmp = subscript[0] ? subscript[0]->get_numeric_value() - 1 + offset : 0;
				set_sym->access_set_symbol<T>()->set_value(value, tmp);
			}
		}
		else
		{
			add_diagnostic(diagnostic_s::error_E030("", "symbolic parameter", convert_range(range))); //error - can't write to symbolic param
		}
	}

	context::id_index get_id(std::string name);

	void jump(context::sequence_symbol target);

	//std::string concatenate(std::vector<concat_point_ptr>&& conc_list);
	std::string concatenate(const std::vector<concat_point_ptr>& conc_list) const;

	void collect_diags() const override;

	expr_ptr evaluate_expression_tree(antlr4::ParserRuleContext* expr_context) const;

	

private:
	std::string to_string(const char_str* str) const;
	std::string to_string(const var_sym* vs) const;
	std::string to_string(const dot*) const;
	std::string to_string(const sublist* sublist) const;
	void init_instr();

	void jump_in_statements(context::location loc);

	friend ordinary_processor;
	friend lookahead_processor;
};

using process_table_t = std::unordered_map<context::id_index, std::function<void()>>;

//statement processor for assembling outside of lookahead mode
//changes inner state of semantic analyzer, therefore friend relation is used
//helps to move processing of statements outside of semantic analyzer, making its functions more separated
class ordinary_processor : public diagnosable_impl
{
	semantic_analyzer& analyzer_;
	process_table_t process_table_;

	static checking::one_operand empty_one_operand;
public:
	ordinary_processor(semantic_analyzer& analyzer);

	void process_current_statement();

	virtual void collect_diags() const override;
	size_t get_length_size(data_def_operand & DS_operand);
	size_t get_length_from_type(data_def_operand & DS_operand);
	size_t get_alignment_from_type(data_def_operand & DS_operand);

	void register_current_label_section();
private:
	template<typename T>
	void check_and_prepare_SET(bool& recoverable, context::id_index& id, std::vector<antlr4::ParserRuleContext*>& subscript, symbol_range& range, std::vector<T>& values)
	{
		assert(!analyzer_.current_operands_and_remarks().substituted);
		auto& ops = analyzer_.current_operands_and_remarks().operands;
		recoverable = true;

		if (ops.size() == 0)
		{
			auto r = analyzer_.curr_statement_.range;
			analyzer_.add_diagnostic(diagnostic_s::error_E010("", "label", { {r.begin_ln, r.begin_col}, {r.end_ln, r.end_col} })); //error - uknown label
			recoverable = false;
			return;
		}

		if (analyzer_.current_label().type != label_type::VAR)
		{
			//ERR
			recoverable = false;
			return;
		}

		for (auto& op : ops)
		{
			
			auto tmp = op->access_ca_op();
			if (!tmp)
				continue;
			assert(tmp);

			if (tmp->kind == ca_operand_kind::EXPR || tmp->kind == ca_operand_kind::VAR)
			{
				auto e = analyzer_.evaluate_expression_tree(tmp->expression);

				auto&& value = e ? e->get_value<T>() : context::object_traits<T>::default_v();
				values.push_back(std::move(value));
			}
			else
			{
				analyzer_.add_diagnostic(diagnostic_s::error_E010("", "operand", { {op->range.begin_ln, op->range.begin_col},{op->range.end_ln, op->range.end_col} })); //error - uknown operand
				recoverable = false;
				return;
			}
		}

		auto& var = analyzer_.curr_statement_.label_info.variable_symbol;
		id = analyzer_.get_id(std::move(var.name));
		subscript = std::move(var.subscript);
		range = std::move(var.range);
	}
	template<typename T>
	void process_SET()
	{
		bool recoverable;
		context::id_index id;
		std::vector<antlr4::ParserRuleContext*> subscripts_ctx;
		symbol_range range;
		std::vector<T> values;
		check_and_prepare_SET<T>(recoverable, id, subscripts_ctx, range, values);

		if (recoverable)
		{
			std::vector<expr_ptr> subscripts_expr;

			for (auto s : subscripts_ctx)
				subscripts_expr.push_back(analyzer_.evaluate_expression_tree(s));

			for (size_t i = 0; i < values.size(); i++)
			{
				analyzer_.set_var_sym_value<T>(id, subscripts_expr, std::move(values[i]), range, i);
			}
		}

	}

	//
			//analyzer.add_diagnostic(diagnostic_s::warning_W010("", "Field", {})); //warning - field not expected


	void check_and_prepare_GBL_LCL(bool& recoverable, std::vector<context::id_index>& ids, std::vector<bool>& scalar_info);
	template<typename T, bool global>
	void process_GBL_LCL()
	{
		bool recoverable;
		std::vector<context::id_index> ids;
		std::vector<bool> scalar_info;
		check_and_prepare_GBL_LCL(recoverable, ids, scalar_info);
		
		if (recoverable)
		{
			for (size_t i = 0; i < ids.size(); ++i)
			{
				if (global)
					analyzer_.ctx_->create_global_variable<T>(ids[i], scalar_info[i]);
				else
					analyzer_.ctx_->create_local_variable<T>(ids[i], scalar_info[i]);
			}
		}
	}

	void process_ANOP();

	void check_and_prepare_ACTR(bool& recoverable, context::A_t& ctr);
	void process_ACTR();

	void check_and_prepare_AGO(bool& recoverable, context::A_t& branch, std::vector<context::sequence_symbol>& targets);
	void process_AGO();

	void check_and_prepare_AIF(bool& recoverable, context::B_t& condition, context::sequence_symbol& target);
	void process_AIF();

	inline void process_seq_sym_();
	inline void process_label_field_seq_or_empty_();

	void check_assembler_instr();
	void check_machine_instr();
	checking::machine_instruction_checker mach_checker;
	checking::assembler_instruction_checker assembler_checker;

	

};

//processor of statements inside of lookahead mode
//processes almost no statement but COPY, MACRO and MEND
class lookahead_processor
{
	semantic_analyzer& analyzer_;
	process_table_t process_table_;

	size_t macro_nest_;
public:
	lookahead_processor(semantic_analyzer& analyzer);

	void process_current_statement();

private:
	void process_COPY();

	void process_MACRO();

	void process_MEND();

	void process_label_field();
};


}
}
}
#endif
