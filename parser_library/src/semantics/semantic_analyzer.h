#ifndef SEMANTICS_SEMANTICANALYZER_H
#define SEMANTICS_SEMANTICANALYZER_H
#include "../context/hlasm_context.h"
#include <unordered_map>
#include "../include/shared/lexer.h"
#include "semantic_objects.h"
#include "statement_fields.h"
#include "concatenation.h"
#include "expression.h"
#include <functional>

namespace hlasm_plugin {
namespace parser_library {
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
class semantic_analyzer
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
public:
	context::hlasm_context& context();
	lexer* lexer();

	void initialize(std::shared_ptr<context::hlasm_context> ctx_init, hlasm_plugin::parser_library::lexer* lexer_init);
	void initialize(const semantic_analyzer& analyzer);

	const label_semantic_info& current_label();
	const instruction_semantic_info& current_instruction();
	const operand_remark_semantic_info& current_operands_and_remarks();

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

	void process_statement();

	bool in_lookahead();

	set_type get_var_sym_value(context::id_index id, const std::vector<expr_ptr>& subscript, symbol_range range);

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
				//error too much operands
			}
			else if ((set_sym->is_scalar && subscript.size() == 1) || (!set_sym->is_scalar && subscript.size() == 0))
			{
				//error inconsistent subscript
			}

			if (!set_sym->is_scalar && subscript[0] && subscript[0]->get_numeric_value() < 1)
			{
				//error subscript lesser than 1
			}

			if (set_sym->type() != context::object_traits<T>::type_enum)
			{
				//ERROR
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
			//ERR
		}
	}

	context::id_index get_id(std::string name);

	void jump(context::sequence_symbol target);

	std::string concatenate(std::vector<concat_point_ptr>&& conc_list);

private:
	std::string to_string(char_str* str);
	std::string to_string(var_sym* vs);
	std::string to_string(dot*);
	std::string to_string(sublist* sublist);
	void init_instr();

	void jump_in_statements(context::location location);

	friend ordinary_processor;
	friend lookahead_processor;
};

using process_table_t = std::unordered_map<context::id_index, std::function<void()>>;

//statement processor for assembling outside of lookahead mode
//changes inner state of semantic analyzer, therefore friend relation is used
//helps to move processing of statements outside of semantic analyzer, making its functions more separated
class ordinary_processor
{
	semantic_analyzer& analyzer_;
	process_table_t process_table_;

public:
	ordinary_processor(semantic_analyzer& analyzer);

	void process_current_statement();

private:
	template<typename T>
	void check_and_prepare_SET(bool& recoverable, context::id_index& id, std::vector<expr_ptr>& subscript, symbol_range& range, std::vector<T>& values)
	{
		assert(!analyzer_.current_operands_and_remarks().substituted);
		auto& ops = analyzer_.current_operands_and_remarks().operands;
		recoverable = true;

		if (ops.size() == 0)
		{
			//ERR
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
			assert(tmp);

			if (tmp->kind == ca_operand_kind::EXPR || tmp->kind == ca_operand_kind::VAR)
			{
				auto&& value = tmp->expression ? tmp->expression->get_value<T>() : context::object_traits<T>::default_v();
				values.push_back(std::move(value));
			}
			else
			{
				//ERR bad operand
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
		std::vector<expr_ptr> subscript;
		symbol_range range;
		std::vector<T> values;
		check_and_prepare_SET<T>(recoverable, id, subscript, range, values);

		if (recoverable)
		{
			for (size_t i = 0; i < values.size(); i++)
			{
				analyzer_.set_var_sym_value<T>(id, subscript, std::move(values[i]), range, i);
			}
		}

	}

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
