#ifndef SEMANTICS_ORDINARYPROCESSOR_H
#define SEMANTICS_ORDINARYPROCESSOR_H

#include "statement_processor.h"
#include "../checking/instruction_checker.h"
#include "lookahead_processor.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {


//statement processor for assembling outside of lookahead mode
class ordinary_processor : public statement_processor
{
public:
	ordinary_processor(processing_manager& mngr);

	void set_start_info(start_info_ptr info) override;

	void process_instruction(instruction_semantic_info instruction) override;

	void process_statement(statement statement) override;

	virtual void collect_diags() const override;

private:
	op_code_info curr_op_code_;
	statement curr_statement_;

	void decode_statement(statement&& statement);

	void check_and_prepare_SET_base(bool& recoverable, var_sym& symbol);
	template<typename T>
	void check_and_prepare_SET(bool& recoverable, var_sym& symbol, std::vector<T>& values)
	{
		check_and_prepare_SET_base(recoverable, symbol);
		if (!recoverable)
			return;

		for (auto& op : curr_statement_.op_rem_info.operands)
		{
			if (!op || op->type == operand_type::EMPTY)
				continue;

			auto e = ctx_mngr().evaluate_expression_tree(op->access_ca_op()->expression);

			auto&& value = e ? e->get_set_value().to<T>() : context::object_traits<T>::default_v();//todo check if expr is apropriate for set
			values.push_back(std::move(value));
		}
		
	}
	template<typename T>
	void process_SET()
	{
		bool recoverable;
		var_sym symbol;
		std::vector<T> values;
		check_and_prepare_SET<T>(recoverable, symbol, values);

		if (recoverable)
			ctx_mngr().set_var_sym_value<T>(std::move(symbol), std::move(values));

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
					ctx_mngr().ctx().create_global_variable<T>(ids[i], scalar_info[i]);
				else
					ctx_mngr().ctx().create_local_variable<T>(ids[i], scalar_info[i]);
			}
		}
	}

	void process_ANOP();

	void check_and_prepare_ACTR(bool& recoverable, context::A_t& ctr);
	void process_ACTR();

	void check_and_prepare_AGO(bool& recoverable, context::A_t& branch, std::vector<seq_sym>& targets);
	void process_AGO();

	void check_and_prepare_AIF(bool& recoverable, context::B_t& condition, seq_sym& target);
	void process_AIF();

	void process_MACRO();
	void process_MEND();
	void process_MEXIT();

	void process_empty();

	process_table_t init_table(context::hlasm_context& ctx);

	inline void process_seq_sym_();
	inline void process_label_field_seq_or_empty_();
	
	void jump(seq_sym target);

	void check_assembler_instr();
	void check_machine_instr();
	checking::machine_instruction_checker mach_checker;
	checking::assembler_instruction_checker assembler_checker;
};


}
}
}
#endif
