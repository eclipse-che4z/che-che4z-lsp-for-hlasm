#ifndef HLASMPLUGIN_PARSERLIBRARY_PROCESSING_DATA_DEF_POSTPONED_STATEMENT_H
#define HLASMPLUGIN_PARSERLIBRARY_PROCESSING_DATA_DEF_POSTPONED_STATEMENT_H

#include "postponed_statement_impl.h"
#include "../../checking/data_definition/data_definition_operand.h"

#include <variant>

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

template<checking::data_instr_type instr_type>
struct data_def_postponed_statement : public postponed_statement_impl, public context::resolvable
{
	data_def_postponed_statement(rebuilt_statement stmt, std::vector<location> stmt_location_stack) :
		postponed_statement_impl(std::move(stmt), std::move(stmt_location_stack)) {}


	// Inherited via resolvable
	virtual context::dependency_collector get_dependencies(context::dependency_solver& solver) const override
	{
		context::dependency_collector conjunction;
		for (const auto & op : operands_ref().value)
		{
			if (op->type == semantics::operand_type::EMPTY)
				continue;
			conjunction = conjunction + op->access_data_def()->get_length_dependencies(solver);
		}
		return conjunction;
	}

	static int32_t get_operands_length(const semantics::operand_list& operands, context::dependency_solver& solver)
	{
		std::vector<checking::check_op_ptr> checking_ops;
		std::vector<const checking::data_definition_operand *> get_len_ops;
		for (const auto& op : operands)
		{
			if (op->type == semantics::operand_type::EMPTY)
				continue;
			auto o = op->access_data_def()->get_operand_value(solver);
			checking::data_definition_operand* dd_op = dynamic_cast<checking::data_definition_operand*>(o.get());
			checking_ops.push_back(std::move(o));
			get_len_ops.push_back(dd_op);
		}
		uint64_t len = checking::data_definition_operand::get_operands_length<instr_type>(get_len_ops);
		if (len > INT32_MAX)
			return 0;
		else
			return (int32_t) len;
	}

	virtual context::symbol_value resolve(context::dependency_solver& solver) const override
	{
		return get_operands_length(operands_ref().value, solver);
	}

};



}
}
}
#endif