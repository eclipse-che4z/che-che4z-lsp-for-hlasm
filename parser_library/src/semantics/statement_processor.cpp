#include "semantic_analyzer.h"
#include "../context/instruction.h"
#include "../checking/instruction_checker.h"
#include "expression_visitor.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library::semantics;

ordinary_processor::ordinary_processor(semantic_analyzer& analyzer) : analyzer_(analyzer)
{
	process_table_.emplace(analyzer_.ctx_->ids.add("SETA"),
		std::bind(&ordinary_processor::process_SET<A_t>, this));
	process_table_.emplace(analyzer_.ctx_->ids.add("SETB"),
		std::bind(&ordinary_processor::process_SET<B_t>, this));
	process_table_.emplace(analyzer_.ctx_->ids.add("SETC"),
		std::bind(&ordinary_processor::process_SET<C_t>, this));
	process_table_.emplace(analyzer_.ctx_->ids.add("LCLA"),
		std::bind(&ordinary_processor::process_GBL_LCL<A_t, false>, this));
	process_table_.emplace(analyzer_.ctx_->ids.add("LCLB"),
		std::bind(&ordinary_processor::process_GBL_LCL<B_t, false>, this));
	process_table_.emplace(analyzer_.ctx_->ids.add("LCLC"),
		std::bind(&ordinary_processor::process_GBL_LCL<C_t, false>, this));
	process_table_.emplace(analyzer_.ctx_->ids.add("GBLA"),
		std::bind(&ordinary_processor::process_GBL_LCL<A_t, true>, this));
	process_table_.emplace(analyzer_.ctx_->ids.add("GBLB"),
		std::bind(&ordinary_processor::process_GBL_LCL<B_t, true>, this));
	process_table_.emplace(analyzer_.ctx_->ids.add("GBLC"),
		std::bind(&ordinary_processor::process_GBL_LCL<C_t, true>, this));
	process_table_.emplace(analyzer_.ctx_->ids.add("ANOP"),
		std::bind(&ordinary_processor::process_ANOP, this));
	process_table_.emplace(analyzer_.ctx_->ids.add("ACTR"),
		std::bind(&ordinary_processor::process_ACTR, this));
	process_table_.emplace(analyzer_.ctx_->ids.add("AGO"),
		std::bind(&ordinary_processor::process_AGO, this));
	process_table_.emplace(analyzer_.ctx_->ids.add("AIF"),
		std::bind(&ordinary_processor::process_AIF, this));

	for (size_t i = 0; i < instruction::machine_instructions.size(); i++)
	{
		process_table_.emplace(analyzer_.ctx_->ids.add(instruction::machine_instructions[i].name),
			std::bind(&ordinary_processor::check_machine_instr, this));
	}
	for (size_t i = 0; i < instruction::assembler_instructions.size(); i++)
	{
		process_table_.emplace(analyzer_.ctx_->ids.add(instruction::assembler_instructions[i].name),
			std::bind(&ordinary_processor::check_assembler_instr, this));
	}
	
}

checking::one_operand ordinary_processor::empty_one_operand("");



void ordinary_processor::check_assembler_instr()
{
	std::vector<operand_ptr> * operands;
	if (analyzer_.current_operands_and_remarks().substituted)
		operands = &analyzer_.current_operands_and_remarks().substituted_operands;
	else
		operands = &analyzer_.current_operands_and_remarks().operands;

 	std::vector<checking::one_operand*> operand_vector = {};
	for (size_t i = 0; i < operands->size(); i++)
	{
		auto & operand = (*operands)[i];
		if (!operand || operand->type() == operand_type::EMPTY || operand->type() == operand_type::UNDEF || operand->access_model_op())
		{
			operand_vector.push_back(&empty_one_operand);
			continue;
		}
		auto asm_op = operand->access_asm_op();
		assert(asm_op);
		operand_vector.push_back(asm_op->op_value.get());
	}

	assembler_checker.check(*analyzer_.current_instruction().id, operand_vector);

	auto diags = assembler_checker.get_diagnostics();
	auto range = analyzer_.curr_statement_.range;
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
	std::vector<operand_ptr> * operands;
	if (analyzer_.current_operands_and_remarks().substituted)
		operands = &analyzer_.current_operands_and_remarks().substituted_operands;
	else
		operands = &analyzer_.current_operands_and_remarks().operands;

	std::vector<checking::one_operand*> operand_vector = {};
	for (size_t i = 0; i < operands->size(); i++)
	{
		auto & operand = (*operands)[i];
		if (!operand || operand->type() == operand_type::EMPTY || operand->type() == operand_type::UNDEF || operand->access_model_op())
		{
			operand_vector.push_back(&empty_one_operand);
			continue;
		}
		auto mach_op = operand->access_mach_op();
		assert(mach_op);
		operand_vector.push_back(mach_op->op_value.get());
	}

	mach_checker.mach_instr_check(*analyzer_.current_instruction().id, operand_vector);

	const auto & diags = mach_checker.get_diagnostics();
	auto range = analyzer_.curr_statement_.range;
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

void hlasm_plugin::parser_library::semantics::ordinary_processor::process_current_statement()
{
	auto stmt_eval = process_table_.find(analyzer_.current_instruction().id);
	if (stmt_eval!= process_table_.end())
		stmt_eval->second();
	else
		;//throw std::exception("not supported operation code");

}

void ordinary_processor::collect_diags() const {}

void hlasm_plugin::parser_library::semantics::ordinary_processor::check_and_prepare_GBL_LCL(bool & recoverable, std::vector<context::id_index>& ids, std::vector<bool>& scalar_info)
{
	assert(!analyzer_.current_operands_and_remarks().substituted);
	auto& ops = analyzer_.current_operands_and_remarks().operands;
	recoverable = true;
	if (ops.size() == 0)
	{
		recoverable = false;
		return;
	}

	for (auto& op : ops)
	{
		if (!op || op->type() == operand_type::EMPTY)
			continue;

		auto tmp = op->access_ca_op();

		assert(tmp);

		if (tmp->kind == ca_operand_kind::VAR)
		{
			auto id = analyzer_.get_id(std::move(tmp->vs.name));
			bool scalar = tmp->vs.subscript.size() == 0;
			ids.push_back(id);
			scalar_info.push_back(scalar);
		}
		else
		{
			//ERR bad operand
			recoverable = false;
			return;
		}
	}

	if (analyzer_.current_label().type != label_type::EMPTY)
	{
		//ERR-WARN
	}
}

void hlasm_plugin::parser_library::semantics::ordinary_processor::process_ANOP()
{
	assert(analyzer_.current_operands_and_remarks().operands.size() == 0);

	process_label_field_seq_or_empty_();
}


void hlasm_plugin::parser_library::semantics::ordinary_processor::check_and_prepare_ACTR(bool& recoverable, A_t& ctr)
{
	recoverable = true;
	assert(!analyzer_.current_operands_and_remarks().substituted);
	auto& ops = analyzer_.current_operands_and_remarks().operands;
	if (ops.size() != 1)
	{
		//ERR bad nomber of operands
		if (ops.size() == 0)
		{
			recoverable = false;
			return;
		}
	}

	auto op = ops[0]->access_ca_op();
	assert(op);

	auto e = analyzer_.evaluate_expression_tree(op->expression);

	if (op->kind == ca_operand_kind::EXPR || op->kind == ca_operand_kind::VAR)
		ctr = e ? e->get_numeric_value() : object_traits<A_t>::default_v();
	else
	{
		recoverable = false;
		//ERR bad op
	}

}

void hlasm_plugin::parser_library::semantics::ordinary_processor::process_ACTR()
{
	process_label_field_seq_or_empty_();
	
	bool recoverable;
	A_t ctr;
	check_and_prepare_ACTR(recoverable, ctr);

	if (recoverable)
		analyzer_.ctx_->set_branch_counter(ctr);
}

void ordinary_processor::check_and_prepare_AGO(bool& recoverable, A_t& branch, std::vector<sequence_symbol>& targets)
{
	assert(!analyzer_.current_operands_and_remarks().substituted);
	auto& ops = analyzer_.current_operands_and_remarks().operands;
	recoverable = true;
	if (ops.size() == 0)
	{
		recoverable = false;
		return;
	}
	for (auto && op : ops)
	{
		if (!op)
		{
			recoverable = false;
			return;
		}
	}

	auto op = ops[0]->access_ca_op();
	assert(op);

	if (op->kind == ca_operand_kind::BRANCH_SIMPLE || op->kind == ca_operand_kind::BRANCH_EXPR)
	{
		auto id = analyzer_.get_id(std::move(op->seqence_symbol.name));
		targets.push_back({ id,op->seqence_symbol.loc });
		branch = 1;
	}

	if (op->kind == ca_operand_kind::BRANCH_SIMPLE && ops.size() != 1)
	{
		//ERR
	}
	else if (op->kind == ca_operand_kind::BRANCH_EXPR)
	{
		auto ev = std::make_shared<expression_visitor>();
		ev->set_semantic_analyzer(&analyzer_);
		auto e = analyzer_.evaluate_expression_tree(op->expression);

		branch = e ? e->get_numeric_value() : object_traits<A_t>::default_v();

		for (size_t i = 1; i < ops.size(); ++i)
		{
			auto tmp = ops[i]->access_ca_op();
			assert(tmp);

			if (tmp->kind == ca_operand_kind::BRANCH_SIMPLE)
			{
				auto id = analyzer_.get_id(std::move(tmp->seqence_symbol.name));
				targets.push_back({ id,tmp->seqence_symbol.loc });
			}
			else
			{
				//err
				recoverable = false;
			}

		}
	}
	else if (op->kind != ca_operand_kind::BRANCH_EXPR && op->kind != ca_operand_kind::BRANCH_SIMPLE)
	{
		//bad operands
		recoverable = false;
	}
}

void hlasm_plugin::parser_library::semantics::ordinary_processor::process_AGO()
{
	process_label_field_seq_or_empty_();

	bool recoverable;
	A_t branch;
	std::vector<sequence_symbol> targets;
	check_and_prepare_AGO(recoverable,branch,targets);

	if (recoverable)
	{
		if (branch - 1 >= 0 && branch - 1 < (int)targets.size())
			analyzer_.jump(targets[branch - 1]);
	}
}

void ordinary_processor::check_and_prepare_AIF(bool& recoverable, B_t& condition, sequence_symbol& target)
{
	assert(!analyzer_.current_operands_and_remarks().substituted);
	auto& ops = analyzer_.current_operands_and_remarks().operands;
	recoverable = true;
	condition = false;

	if (ops.size() == 0)
	{
		recoverable = false;
		return;
	}

	for (auto& op : ops)
	{
		auto tmp = op->access_ca_op();
		assert(tmp);

		if (tmp->kind == ca_operand_kind::BRANCH_EXPR)
		{
			if (!condition)
			{
				auto e = analyzer_.evaluate_expression_tree(tmp->expression);

				condition = e ? e->get_numeric_value() : object_traits<B_t>::default_v();
				auto id = analyzer_.get_id(std::move(tmp->seqence_symbol.name));
				target = { id,tmp->seqence_symbol.loc };
			}
		}
		else
		{
			//bad op
			if (!condition)
			{
				recoverable = false;
				return;
			}
		}
	}
}

void hlasm_plugin::parser_library::semantics::ordinary_processor::process_AIF()
{
	process_label_field_seq_or_empty_();

	bool recoverable;
	B_t condition;
	sequence_symbol target;
	check_and_prepare_AIF(recoverable,condition,target);

	if(recoverable && condition)
		analyzer_.jump(target);
}

inline void hlasm_plugin::parser_library::semantics::ordinary_processor::process_seq_sym_()
{
	auto id = analyzer_.get_id(std::move(analyzer_.current_label().sequence_symbol.name));
	sequence_symbol ss = { id,analyzer_.current_label().sequence_symbol.loc };
	analyzer_.ctx_->add_sequence_symbol(ss);
}

inline void hlasm_plugin::parser_library::semantics::ordinary_processor::process_label_field_seq_or_empty_()
{
	if (analyzer_.current_label().type == label_type::SEQ)
		process_seq_sym_();
	else if (analyzer_.current_label().type != label_type::EMPTY)
	{
		//WARN unexpected name field
	}
}

lookahead_processor::lookahead_processor(semantic_analyzer& analyzer) : analyzer_(analyzer), macro_nest_(0)
{
	process_table_.emplace(analyzer_.ctx_->ids.add("COPY"),
		std::bind(&lookahead_processor::process_COPY, this));
	process_table_.emplace(analyzer_.ctx_->ids.add("MACRO"),
		std::bind(&lookahead_processor::process_MACRO, this));
	process_table_.emplace(analyzer_.ctx_->ids.add("MEND"),
		std::bind(&lookahead_processor::process_MEND, this));
}

void hlasm_plugin::parser_library::semantics::lookahead_processor::process_current_statement()
{
	process_label_field();
	auto stmt_eval = process_table_[analyzer_.current_instruction().id];
	if (stmt_eval)
		stmt_eval();
}

void hlasm_plugin::parser_library::semantics::lookahead_processor::process_COPY()
{
	//TODO
}

void hlasm_plugin::parser_library::semantics::lookahead_processor::process_MACRO()
{
	++macro_nest_;
}

void hlasm_plugin::parser_library::semantics::lookahead_processor::process_MEND()
{
	if (macro_nest_ != 0)
		--macro_nest_;
}

void hlasm_plugin::parser_library::semantics::lookahead_processor::process_label_field()
{
	if (macro_nest_ != 0)
		return;

	if (analyzer_.current_label().type == label_type::SEQ && analyzer_.ctx_->ids.add(std::move(analyzer_.current_label().sequence_symbol.name)) == analyzer_.lookahead_.target.name)
	{
		analyzer_.lookahead_ = look_ahead();
		analyzer_.jump_in_statements(analyzer_.current_label().sequence_symbol.loc);
		return;
	}

	if (analyzer_.lexer_->is_last_line())
	{
		analyzer_.jump_in_statements(analyzer_.lookahead_.target.loc);
		analyzer_.lookahead_ = look_ahead();
		analyzer_.lookahead_failed_ = true;
		//ERR lookahead failed
	}
}
