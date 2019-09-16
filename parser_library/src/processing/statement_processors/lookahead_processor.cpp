#include "lookahead_processor.h"
#include "../instruction_sets/asm_processor.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

processing_status lookahead_processor::get_processing_status(const semantics::instruction_si& instruction) const
{
	if (instruction.type == semantics::instruction_si_type::ORD)
	{
		auto opcode = hlasm_ctx.get_mnemonic_opcode(std::get<context::id_index>(instruction.value));
		if (opcode == copy_id || opcode == equ_id_)
			return std::make_pair(
				processing_format(processing_kind::LOOKAHEAD, processing_form::ASM, operand_occurence::PRESENT),
				op_code(opcode,context::instruction_type::ASM));

		if (opcode == macro_id || opcode == mend_id)
			return std::make_pair(
				processing_format(processing_kind::LOOKAHEAD, processing_form::CA,operand_occurence::ABSENT),
				op_code(opcode, context::instruction_type::CA));
	}

	return std::make_pair(processing_format(processing_kind::LOOKAHEAD, processing_form::IGNORED), op_code());
}

void lookahead_processor::process_statement(context::shared_stmt_ptr statement)
{
	process_statement(*statement);
}

void lookahead_processor::process_statement(context::unique_stmt_ptr statement)
{
	process_statement(*statement);
}

void lookahead_processor::end_processing()
{
	if (!finished_flag_)
		add_diagnostic(diagnostic_op::error_E047(*start.target, start.target_range));

	listener_.finish_lookahead(std::move(result_));
	finished_flag_ = true;
}

bool hlasm_plugin::parser_library::processing::lookahead_processor::terminal_condition(const statement_provider_kind prov_kind) const
{
	return prov_kind == statement_provider_kind::MACRO || prov_kind == statement_provider_kind::OPEN;
}

bool lookahead_processor::finished()
{
	return finished_flag_;
}

void lookahead_processor::collect_diags() const {}

lookahead_processor::lookahead_processor(
	context::hlasm_context& hlasm_ctx,
	branching_provider& branch_provider, processing_state_listener& listener, parse_lib_provider& lib_provider, const lookahead_start_data start)
	: statement_processor(processing_kind::LOOKAHEAD, hlasm_ctx),
	finished_flag_(false), result_(start.source), macro_nest_(0), branch_provider_(branch_provider),
	listener_(listener), lib_provider_(lib_provider), equ_id_(hlasm_ctx.ids().add("EQU")), start(std::move(start)) {}

void lookahead_processor::process_MACRO() { ++macro_nest_; }
void lookahead_processor::process_MEND() { macro_nest_ -= macro_nest_ == 0 ? 0 : 1; }
void lookahead_processor::process_COPY(const resolved_statement& statement)
{
	if (statement.operands_ref().value.size() == 1 && statement.operands_ref().value.front()->access_asm())
	{
		asm_processor::process_copy(statement, hlasm_ctx, lib_provider_, nullptr);
	}
}

void lookahead_processor::process_statement(const context::hlasm_statement& statement)
{
	if (macro_nest_ == 0)
		find_target(statement);

	auto resolved = statement.access_resolved();

	if (!resolved)
		return;

	if (resolved->opcode_ref().value == macro_id)
	{
		process_MACRO();
	}
	else if (resolved->opcode_ref().value == mend_id)
	{
		process_MEND();
	}
	else if (macro_nest_ == 0 && resolved->opcode_ref().value == copy_id)
	{
		process_COPY(*resolved);
	}
}

void lookahead_processor::find_target(const context::hlasm_statement& statement)
{
	switch (start.action)
	{
	case lookahead_action::SEQ:
		find_seq(dynamic_cast<const semantics::core_statement&>(statement));
		break;
	case lookahead_action::ORD:
		//TODO
		break;
	default:
		assert(false);
		break;
	}
}

void lookahead_processor::find_seq(const semantics::core_statement& statement)
{
	if (statement.label_ref().type == semantics::label_si_type::SEQ)
	{
		auto symbol = std::get<semantics::seq_sym>(statement.label_ref().value);
		
		branch_provider_.register_sequence_symbol(symbol.name,symbol.symbol_range);

		if (symbol.name == start.target)
		{
			finished_flag_ = true;
			result_ = lookahead_processing_result(symbol.name, symbol.symbol_range);
		}
	}

}
