#include "ordinary_processor.h"
#include "../context/macro.h"
#include "macro_def_processor.h"
#include "processing_manager.h"

using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library::semantics;

ordinary_processor::ordinary_processor(processing_manager& mngr) : statement_processor(mngr,std::bind(&ordinary_processor::init_table,this,std::placeholders::_1)) {}

void ordinary_processor::check_assembler_instr()
{
	auto empty_op = checking::one_operand();
	auto operands = &curr_statement_.op_rem_info.operands;
	std::vector<const checking::asm_operand*> operand_vector = {};
	for (size_t i = 0; i < operands->size(); i++)
	{
		auto & operand = (*operands)[i];
		if (!operand || operand->type == operand_type::EMPTY || operand->type == operand_type::UNDEF || operand->access_model_op())
		{
			operand_vector.push_back(&empty_op);
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
	auto operands = &curr_statement_.op_rem_info.operands;
	std::string instr_name = *curr_op_code_.op_code;
	auto empty_op = checking::empty_operand_value();
	std::vector<checking::machine_operand_value*> operand_vector = {};
	for (size_t i = 0; i < operands->size(); i++)
	{
		auto & operand = (*operands)[i];
		if (!operand || operand->type == operand_type::EMPTY || operand->type == operand_type::UNDEF || operand->access_model_op())
		{
			operand_vector.push_back(&empty_op);
			if (!operand)
			{
				operand_vector.at(operand_vector.size() - 1)->range = curr_statement_.range;
				continue;
			}
			operand_vector.at(operand_vector.size() - 1)->range = operand->range;
			continue;
		}
		auto mach_op = operand->access_mach_op();
		assert(mach_op);
		operand_vector.push_back(mach_op->op_value.get());
	}
	auto curr_instr = &hlasm_plugin::parser_library::context::instruction::machine_instructions.at(instr_name);
	curr_instr->get()->check(instr_name, operand_vector);
	const auto & diags = curr_instr->get()->diagnostics;
	for (auto & diag : diags)
	{
		auto range = diag.range;
		if (range.begin_col == 0 && range.begin_ln == 0 && range.end_col == 0 && range.end_ln == 0)
			range = curr_statement_.range;
		if (diagnostic_op::is_error(diag.diag))
		{
			add_diagnostic(diagnostic_s{ "",{{range.begin_ln, range.begin_col},{range.end_ln, range.end_col}},
				diag.diag.severity, std::move(diag.diag.code),
				"HLASM Plugin", std::move(diag.diag.message), {} });
		}
	}
	curr_instr->get()->clear_diagnostics();
	curr_instr->get()->clear_diagnostics();
}

void ordinary_processor::check_mnemonic_code_instr()
{
	// operands obtained from the user
	auto operands = &curr_statement_.op_rem_info.operands;
	// the name of the instruction (mnemonic) obtained from the user
	auto instr_name = *curr_op_code_.op_code;
	// the associated mnemonic structure with the given name
	auto mnemonic = instruction::mnemonic_codes.at(*curr_op_code_.op_code);
	// the machine instruction structure associated with the given instruction name
	auto curr_instr = & hlasm_plugin::parser_library::context::instruction::machine_instructions.at(mnemonic.instruction);
	curr_instr->get()->instr_name = instr_name;

	// check whether substituted mnemonic values are ok

	// check size of mnemonic operands
	int diff = curr_instr->get()->operands.size() - operands->size() - mnemonic.replaced.size();
	if (std::abs(diff) > curr_instr->get()->no_optional)
	{
		auto curr_diag = diagnostic_op::error_optional_number_of_operands(curr_instr->get()->instr_name, curr_instr->get()->no_optional, curr_instr->get()->operands.size() - mnemonic.replaced.size());
		auto range = curr_statement_.range;
		add_diagnostic(diagnostic_s{ "",{{range.begin_ln, range.begin_col},{range.end_ln, range.end_col}},
		curr_diag.severity, std::move(curr_diag.code),
		"HLASM Plugin", std::move(curr_diag.message), {} });
		return;
	}

	std::vector<checking::simple_operand_value> substituted_mnems = {};
	for (auto mnem : mnemonic.replaced)
		substituted_mnems.push_back( checking::simple_operand_value(mnem.second));

	auto empty_op = checking::empty_operand_value();
	std::vector<checking::machine_operand_value*> operand_vector = {};
	// create vector of empty operands
	for (size_t i = 0; i < curr_instr->get()->operands.size() + curr_instr->get()->no_optional; i++)
		operand_vector.push_back(nullptr);
	// add substituted
	for (size_t i = 0; i < mnemonic.replaced.size(); i++)
		operand_vector[mnemonic.replaced[i].first] = &substituted_mnems[i];
	// add other
	for (size_t i = 0; i < operands->size(); i++)
	{
		auto& operand = (*operands)[i];
		for (size_t j = 0; j < operand_vector.size(); j++)
		{
			if (operand_vector[j] == nullptr)
			{
				// if operand is empty
				if (!operand || operand->type == operand_type::EMPTY || operand->type == operand_type::UNDEF || operand->access_model_op())
				{
					operand_vector[j] = &empty_op;
					// hot fix as eg C 2,2(2,) makes server crash
					// TO DO range once the bug is fixed
					if (!operand)
					{
						operand_vector.at(operand_vector.size() - 1)->range = curr_statement_.range;
						continue;
					}
					operand_vector.at(operand_vector.size() - 1)->range = operand->range;
					continue;
				}
				auto mach_op = operand->access_mach_op();
				//mach_op->range = operand->range;
				assert(mach_op);
				operand_vector[j] = mach_op->op_value.get();
			}
		}
	}

	// check
	curr_instr->get()->check(instr_name, operand_vector);
	const auto & diags = curr_instr->get()->diagnostics;
	for (auto & diag : diags)
	{
		auto range = diag.range;
		if (range.begin_col == 0 && range.begin_ln == 0 && range.end_col == 0 && range.end_ln == 0)
			range = curr_statement_.range;
		if (diagnostic_op::is_error(diag.diag))
		{
			add_diagnostic(diagnostic_s{ "",{{range.begin_ln, range.begin_col},{range.end_ln, range.end_col}},
				diag.diag.severity, std::move(diag.diag.code),
				"HLASM Plugin", std::move(diag.diag.message), {} });
		}
	}
	curr_instr->get()->clear_diagnostics();
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
		ctx_mngr().enter_macro(curr_op_code_.op_code, std::move(curr_statement_.label_info), std::move(curr_statement_.op_rem_info));
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

void hlasm_plugin::parser_library::semantics::ordinary_processor::process_MEXIT()
{
	//TO DO
	if (!ctx_mngr().ctx().is_in_macro())
		;//err MEXIT not expected outside of macro definition
}

void hlasm_plugin::parser_library::semantics::ordinary_processor::process_ASPACE()
{
	// TO DO
}

void hlasm_plugin::parser_library::semantics::ordinary_processor::process_AREAD()
{
	// TO DO
}

void hlasm_plugin::parser_library::semantics::ordinary_processor::process_AEJECT()
{
	// TO DO
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
	table.emplace(ctx.ids.add("MEXIT"),
		std::bind(&ordinary_processor::process_MEXIT, this));
	table.emplace(ctx.ids.add("ASPACE"),
		std::bind(&ordinary_processor::process_ASPACE, this));
	table.emplace(ctx.ids.add("AREAD"),
		std::bind(&ordinary_processor::process_AREAD, this));
	table.emplace(ctx.ids.add("AEJECT"),
		std::bind(&ordinary_processor::process_AEJECT, this));
	table.emplace(nullptr,
		std::bind(&ordinary_processor::process_empty, this));

	for (const auto & mach_instr : instruction::machine_instructions)
	{
		table.emplace(ctx.ids.add(mach_instr.first),
			std::bind(&ordinary_processor::check_machine_instr, this));
	};
	for (const auto & asm_instr : instruction::assembler_instructions)
	{
		table.emplace(ctx.ids.add(asm_instr.first),
			std::bind(&ordinary_processor::check_assembler_instr, this));
	}
	for (const auto & instr : instruction::mnemonic_codes)
	{
		table.emplace(ctx.ids.add(instr.first),
			std::bind(&ordinary_processor::check_mnemonic_code_instr, this));
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
