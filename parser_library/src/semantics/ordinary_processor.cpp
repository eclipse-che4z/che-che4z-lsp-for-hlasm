#include "ordinary_processor.h"
#include "../context/macro.h"
#include "macro_def_processor.h"
#include "processing_manager.h"

using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library::semantics;

ordinary_processor::ordinary_processor(processing_manager& mngr) : statement_processor(mngr,std::bind(&ordinary_processor::init_table,this,std::placeholders::_1)) {}

void ordinary_processor::check_assembler_instr()
{
	auto operands = &curr_statement_.op_rem_info.operands;

	std::vector<const checking::one_operand*> operand_vector = {};
	for (size_t i = 0; i < operands->size(); i++)
	{
		auto & operand = (*operands)[i];
		if (!operand || operand->type == operand_type::EMPTY || operand->type == operand_type::UNDEF || operand->access_model_op())
		{
			operand_vector.push_back(&checking::one_operand::empty_one_operand);
			continue;
		}
		auto asm_op = operand->access_asm_op();
		assert(asm_op);
		operand_vector.push_back(asm_op->op_value.get());
	}

	assembler_checker.check(*curr_op_code_.op_code, operand_vector);

	auto diags = assembler_checker.get_diagnostics();
	auto range = curr_statement_.range;
	for (auto diag : diags)
	{
		if (!diagnostic_op::is_error(*diag))
			continue;
		add_diagnostic(diagnostic_s{ "",{{range.begin_ln, range.begin_col},{range.end_ln, range.end_col}},
			diag->severity, std::move(diag->code),
			"HLASM Plugin", std::move(diag->message), {} });
	}

	assembler_checker.clear_diagnostics();
}

void ordinary_processor::check_machine_instr()
{
	auto operands= &curr_statement_.op_rem_info.operands;

	std::vector<const checking::one_operand*> operand_vector = {};
	for (size_t i = 0; i < operands->size(); i++)
	{
		auto & operand = (*operands)[i];
		if (!operand || operand->type == operand_type::EMPTY || operand->type == operand_type::UNDEF || operand->access_model_op())
		{
			operand_vector.push_back(&checking::one_operand::empty_one_operand);
			continue;
		}
		auto mach_op = operand->access_mach_op();
		assert(mach_op);
		operand_vector.push_back(mach_op->op_value.get());
	}

	mach_checker.mach_instr_check(*curr_op_code_.op_code, operand_vector);

	const auto & diags = mach_checker.get_diagnostics();
	auto range = curr_statement_.range;
	for (auto diag : diags)
	{
		if (diagnostic_op::is_error(diag))
		{
			add_diagnostic(diagnostic_s{ "",{{range.begin_ln, range.begin_col},{range.end_ln, range.end_col}},
				diag.severity, std::move(diag.code),
				"HLASM Plugin", std::move(diag.message), {} });
		}
	}
	mach_checker.clear_diagnostic();
}

void hlasm_plugin::parser_library::semantics::ordinary_processor::process_statement(statement  statement)
{
	auto stmt_range = statement.range;
	decode_statement(std::move(statement));
	
	if (curr_op_code_.unknown)
	{
		add_diagnostic(diagnostic_s::error_E010("", "operation code", context_manager::convert_range(stmt_range))); //error - no operation code
	}
	else if (curr_op_code_.type != instruction_type::MAC)
	{
		assert( process_table.find(curr_op_code_.op_code) != process_table.end());
		process_table.at(curr_op_code_.op_code)();
	}
	else if (ctx_mngr().ctx().macros().find(curr_op_code_.op_code) != ctx_mngr().ctx().macros().end())
	{
		enter_macro(
			ctx_mngr().enter_macro(curr_op_code_.op_code, std::move(curr_statement_.label_info), std::move(curr_statement_.op_rem_info))
		);
	}
}

void ordinary_processor::collect_diags() const {}

void hlasm_plugin::parser_library::semantics::ordinary_processor::set_start_info(start_info_ptr info)
{
}

void hlasm_plugin::parser_library::semantics::ordinary_processor::process_instruction(instruction_semantic_info  instruction)
{
	curr_op_code_ = ctx_mngr().get_opcode_info(std::move(instruction));

	if (curr_op_code_.unknown)
	{
		parser().format.expect_macro();//when unnown instruction, operands are parsed as if instruction was macro
	}
	else
	{
		parser().format.no_operands = curr_op_code_.has_no_ops;
		parser().format.operand_type = curr_op_code_.type;
		parser().format.alt_format = curr_op_code_.type == instruction_type::CA || curr_op_code_.type == instruction_type::MAC;
	}
}

void hlasm_plugin::parser_library::semantics::ordinary_processor::decode_statement(statement && statement)
{
	//label
	if (statement.label_info.type == label_type::CONC)
	{
		statement.label_info.name = ctx_mngr().concatenate(std::move(statement.label_info.concatenation));
	}

	//oprem
	if (statement.op_rem_info.is_defered)
	{
		std::string op_rem_field;

		op_rem_field.append(ctx_mngr().concatenate(std::move(statement.op_rem_info.defered_field)));

		statement.op_rem_info = parser().reparse_operand_remark_field(std::move(op_rem_field));;

		statement.op_rem_info.is_defered = false;
	}

	auto& operands = statement.op_rem_info.operands;
	//if statement is machine or assembler instruction and operands contain variable symbol
	if (curr_op_code_.type != instruction_type::CA && curr_op_code_.type != instruction_type::MAC &&
		std::find_if(operands.begin(), operands.end(), [](auto& op) {return op && op->type == operand_type::MODEL; }) != operands.end())
	{
		//substituting
		std::string op_rem_field;
		for (size_t i = 0; i < operands.size(); ++i)
		{
			if (auto model = operands[i]->access_model_op())
				op_rem_field.append(ctx_mngr().concatenate(std::move(model->chain)));
			else if (auto subs = operands[i]->access_subs_op())
				op_rem_field.append(subs->to_string());

			if (i != operands.size() - 1)
				op_rem_field.push_back(',');
		}
		//reparsing
		statement.op_rem_info = parser().reparse_operand_remark_field(std::move(op_rem_field));
	}

	curr_statement_ = std::move(statement);
}

void hlasm_plugin::parser_library::semantics::ordinary_processor::check_and_prepare_GBL_LCL(bool & recoverable, std::vector<context::id_index>& ids, std::vector<bool>& scalar_info)
{
	auto& ops = curr_statement_.op_rem_info.operands;
	recoverable = true;

	bool has_operand = false;
	for (auto& op : ops)
	{
		if (!op || op->type == operand_type::EMPTY)
			continue;

		has_operand = true;

		auto tmp = op->access_ca_op();
		assert(tmp);

		if (tmp->kind == ca_operand_kind::VAR)
		{
			auto id = ctx_mngr().get_id(std::move(tmp->vs.name));
			bool scalar = tmp->vs.subscript.size() == 0;
			ids.push_back(id);
			scalar_info.push_back(scalar);
		}
		else
		{
			add_diagnostic(diagnostic_s::error_E010("", "operand", context_manager::convert_range(op->range))); //error - uknown operand
			recoverable = false;
			return;
		}
	}

	if (!has_operand)
	{
		add_diagnostic(diagnostic_s::error_E022("", "variable symbol definition", context_manager::convert_range(curr_statement_.op_rem_info.range)));
		recoverable = false;
		return;
	}

	if (curr_statement_.label_info.type != label_type::EMPTY)
	{
		add_diagnostic(diagnostic_s::warning_W010("", "Field", context_manager::convert_range(curr_statement_.label_info.range))); //warning - field not expected
	}
}

void hlasm_plugin::parser_library::semantics::ordinary_processor::process_ANOP()
{
	assert(curr_statement_.op_rem_info.operands.size() == 0);

	process_label_field_seq_or_empty_();
}

void hlasm_plugin::parser_library::semantics::ordinary_processor::check_and_prepare_ACTR(bool& recoverable, A_t& ctr)
{
	recoverable = true;
	auto& ops = curr_statement_.op_rem_info.operands;

	if (ops.size() != 1)
	{
		add_diagnostic(diagnostic_s::error_E020("", "operand", context_manager::convert_range(curr_statement_.op_rem_info.range)));
		recoverable = false;
		return;
	}

	auto op = ops[0]->access_ca_op();
	assert(op);


	if (op->kind == ca_operand_kind::EXPR || op->kind == ca_operand_kind::VAR)
	{
		auto e = ctx_mngr().evaluate_expression_tree(op->expression);
		ctr = e ? e->get_numeric_value() : object_traits<A_t>::default_v();
	}
	else
	{
		add_diagnostic(diagnostic_s::error_E010("", "operand", context_manager::convert_range(op->range)));
		recoverable = false;
	}

}

void hlasm_plugin::parser_library::semantics::ordinary_processor::process_ACTR()
{
	process_label_field_seq_or_empty_();
	
	bool recoverable;
	A_t ctr;
	check_and_prepare_ACTR(recoverable, ctr);

	if (recoverable)
		ctx_mngr().ctx().set_branch_counter(ctr);
}

void ordinary_processor::check_and_prepare_AGO(bool& recoverable, A_t& branch, std::vector<seq_sym>& targets)
{
	auto& ops = curr_statement_.op_rem_info.operands;
	recoverable = true;
	if (ops.size() == 0)
	{
		add_diagnostic(diagnostic_s::error_E022("", "AGO", context_manager::convert_range(curr_statement_.op_rem_info.range)));
		recoverable = false;
		return;
	}
	for (auto && op : ops)
	{
		if (!op || op->type == operand_type::EMPTY)
		{
			add_diagnostic(diagnostic_s::error_E010("", "operand", context_manager::convert_range(op->range)));
			recoverable = false;
			return;
		}
	}

	auto op = ops[0]->access_ca_op();
	assert(op);

	if (op->kind == ca_operand_kind::BRANCH_SIMPLE || op->kind == ca_operand_kind::BRANCH_EXPR)
	{
		targets.push_back(std::move(op->sequence_symbol));
		branch = 1;
	}

	if (op->kind == ca_operand_kind::BRANCH_SIMPLE && ops.size() != 1)
	{
		add_diagnostic(diagnostic_s::error_E010("", "operand", context_manager::convert_range(op->range)));
	}
	else if (op->kind == ca_operand_kind::BRANCH_EXPR)
	{
		auto e = ctx_mngr().evaluate_expression_tree(op->expression);

		branch = e ? e->get_numeric_value() : object_traits<A_t>::default_v();

		for (size_t i = 1; i < ops.size(); ++i)
		{
			auto tmp = ops[i]->access_ca_op();
			assert(tmp);

			if (tmp->kind == ca_operand_kind::BRANCH_SIMPLE)
			{
				targets.push_back(std::move(tmp->sequence_symbol));
			}
			else
			{
				add_diagnostic(diagnostic_s::error_E010("", "operand", context_manager::convert_range(op->range)));
				recoverable = false;
			}

		}
	}
	else if (op->kind != ca_operand_kind::BRANCH_EXPR && op->kind != ca_operand_kind::BRANCH_SIMPLE)
	{
		add_diagnostic(diagnostic_s::error_E010("", "operand", context_manager::convert_range(op->range)));
		recoverable = false;
	}
}

void hlasm_plugin::parser_library::semantics::ordinary_processor::process_AGO()
{
	process_label_field_seq_or_empty_();

	bool recoverable;
	A_t branch;
	std::vector<seq_sym> targets;
	check_and_prepare_AGO(recoverable,branch,targets);

	if (recoverable)
	{
		if (branch > 0 && branch  <= (int)targets.size())
			jump(targets[branch - 1]);
	}
}

void ordinary_processor::check_and_prepare_AIF(bool& recoverable, B_t& condition, seq_sym& target)
{
	auto& ops = curr_statement_.op_rem_info.operands;
	recoverable = true;
	condition = false;

	if (ops.size() == 0)
	{
		add_diagnostic(diagnostic_s::error_E022("", "AIF", context_manager::convert_range(curr_statement_.op_rem_info.range)));
		recoverable = false;
		return;
	}

	for (auto& op : ops)
	{
		if (!op || op->type == operand_type::EMPTY)
		{
			add_diagnostic(diagnostic_s::error_E010("", "operand", context_manager::convert_range(op->range)));
			recoverable = false;
			return;
		}

		auto tmp = op->access_ca_op();
		assert(tmp);

		if (tmp->kind == ca_operand_kind::BRANCH_EXPR)
		{
			if (!condition)
			{
				auto e = ctx_mngr().evaluate_expression_tree(tmp->expression);

				condition = e ? e->get_numeric_value() : object_traits<B_t>::default_v();
				target = std::move(tmp->sequence_symbol);
			}
		}
		else
		{
			add_diagnostic(diagnostic_s::error_E010("", "operand", context_manager::convert_range(op->range)));
			recoverable = false;
			return;
		}
	}
}

void hlasm_plugin::parser_library::semantics::ordinary_processor::process_AIF()
{
	process_label_field_seq_or_empty_();

	bool recoverable;
	B_t condition;
	seq_sym target;
	check_and_prepare_AIF(recoverable,condition,target);

	if(recoverable && condition)
		jump(target);
}

void hlasm_plugin::parser_library::semantics::ordinary_processor::process_MACRO()
{
	if (curr_statement_.label_info.type != label_type::EMPTY)
		add_diagnostic(diagnostic_s::warning_W010("", "Field", context_manager::convert_range(curr_statement_.label_info.range)));

	assert(curr_statement_.op_rem_info.operands.size() == 0);

	start_macro_definition(std::make_unique<macro_def_info>(curr_statement_.instr_info.range));
}

void hlasm_plugin::parser_library::semantics::ordinary_processor::process_MEND()
{
	if (!ctx_mngr().ctx().is_in_macro())
		;//err MEND not expected outside of macro definition
}

void hlasm_plugin::parser_library::semantics::ordinary_processor::process_empty()
{
	if (curr_statement_.label_info.type != label_type::EMPTY)
		;//err operation code not complete
}

process_table_t hlasm_plugin::parser_library::semantics::ordinary_processor::init_table(hlasm_context& ctx)
{
	process_table_t table;
	table.emplace(ctx.ids.add("SETA"),
		std::bind(&ordinary_processor::process_SET<A_t>, this));
	table.emplace(ctx.ids.add("SETB"),
		std::bind(&ordinary_processor::process_SET<B_t>, this));
	table.emplace(ctx.ids.add("SETC"),
		std::bind(&ordinary_processor::process_SET<C_t>, this));
	table.emplace(ctx.ids.add("LCLA"),
		std::bind(&ordinary_processor::process_GBL_LCL<A_t, false>, this));
	table.emplace(ctx.ids.add("LCLB"),
		std::bind(&ordinary_processor::process_GBL_LCL<B_t, false>, this));
	table.emplace(ctx.ids.add("LCLC"),
		std::bind(&ordinary_processor::process_GBL_LCL<C_t, false>, this));
	table.emplace(ctx.ids.add("GBLA"),
		std::bind(&ordinary_processor::process_GBL_LCL<A_t, true>, this));
	table.emplace(ctx.ids.add("GBLB"),
		std::bind(&ordinary_processor::process_GBL_LCL<B_t, true>, this));
	table.emplace(ctx.ids.add("GBLC"),
		std::bind(&ordinary_processor::process_GBL_LCL<C_t, true>, this));
	table.emplace(ctx.ids.add("ANOP"),
		std::bind(&ordinary_processor::process_ANOP, this));
	table.emplace(ctx.ids.add("ACTR"),
		std::bind(&ordinary_processor::process_ACTR, this));
	table.emplace(ctx.ids.add("AGO"),
		std::bind(&ordinary_processor::process_AGO, this));
	table.emplace(ctx.ids.add("AIF"),
		std::bind(&ordinary_processor::process_AIF, this));
	table.emplace(ctx.ids.add("MACRO"),
		std::bind(&ordinary_processor::process_MACRO, this));
	table.emplace(ctx.ids.add("MEND"),
		std::bind(&ordinary_processor::process_MEND, this));
	table.emplace(nullptr,
		std::bind(&ordinary_processor::process_empty, this));

	for (size_t i = 0; i < instruction::machine_instructions.size(); i++)
	{
		table.emplace(ctx.ids.add(instruction::machine_instructions[i].name),
			std::bind(&ordinary_processor::check_machine_instr, this));
	}
	for (size_t i = 0; i < instruction::assembler_instructions.size(); i++)
	{
		table.emplace(ctx.ids.add(instruction::assembler_instructions[i].name),
			std::bind(&ordinary_processor::check_assembler_instr, this));
	}

	return table;
}

inline void hlasm_plugin::parser_library::semantics::ordinary_processor::process_seq_sym_()
{
	auto id = ctx_mngr().get_id(std::move(curr_statement_.label_info.sequence_symbol.name));
	if (ctx_mngr().ctx().get_sequence_symbol(id) != sequence_symbol::EMPTY)
	{
		add_diagnostic(diagnostic_s::error_E011("", "Sequence symbol", context_manager::convert_range(curr_statement_.label_info.sequence_symbol.range))); 
	}
	else
	{
		sequence_symbol ss = { id,curr_statement_.label_info.sequence_symbol.location };
		ctx_mngr().ctx().add_sequence_symbol(ss);
	}
}

inline void hlasm_plugin::parser_library::semantics::ordinary_processor::process_label_field_seq_or_empty_()
{
	if (curr_statement_.label_info.type == label_type::SEQ)
		process_seq_sym_();
	else if (curr_statement_.label_info.type != label_type::EMPTY)
	{
		add_diagnostic(diagnostic_s::warning_W010("", "Field", context_manager::convert_range(curr_statement_.label_info.range))); //warning - field not expected
	}
}

void hlasm_plugin::parser_library::semantics::ordinary_processor::jump(seq_sym target)
{
	auto target_id = ctx_mngr().get_id(std::move(target.name));
	auto label = ctx_mngr().ctx().get_sequence_symbol(target_id);
	if (!label.name)
	{
		start_lookahead(std::make_unique<lookahead_info>(target_id,target.location, lookahead_info::la_type::SEQ,target.range));
	}
	else
	{

		if (ctx_mngr().ctx().get_branch_counter() <= 0)
		{
			//TODO evacuate macro or quit open code process
			return;
		}

		ctx_mngr().ctx().decrement_branch_counter();

		jump_in_statements(label.location);
	}
}

void hlasm_plugin::parser_library::semantics::ordinary_processor::check_and_prepare_SET_base(bool& recoverable, var_sym& symbol)
{
	auto& ops = curr_statement_.op_rem_info.operands;
	recoverable = true;

	if (curr_statement_.label_info.type != label_type::VAR)
	{
		add_diagnostic(diagnostic_s::error_E010("", "label", context_manager::convert_range(curr_statement_.label_info.range))); //todo illegal set symbol <- better error
		recoverable = false;
		return;
	}

	bool has_operand = false;
	for (auto& op : ops)
	{
		if (!op || op->type == operand_type::EMPTY)
			continue;

		has_operand = true;

		auto tmp = op->access_ca_op();
		assert(tmp);

		if (!(tmp->kind == ca_operand_kind::EXPR || tmp->kind == ca_operand_kind::VAR))
		{
			add_diagnostic(diagnostic_s::error_E010("", "operand", context_manager::convert_range(op->range))); //error - uknown operand
			recoverable = false;
			return;
		}
	}

	if (!has_operand)
	{
		add_diagnostic(diagnostic_s::error_E022("", "SET instruction", context_manager::convert_range(curr_statement_.op_rem_info.range)));
		recoverable = false;
		return;
	}

	symbol = std::move(curr_statement_.label_info.variable_symbol);
}
