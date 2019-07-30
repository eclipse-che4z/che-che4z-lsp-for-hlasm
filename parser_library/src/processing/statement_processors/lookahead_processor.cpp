#include "lookahead_processor.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

processing_status lookahead_processor::get_processing_status(const semantics::instruction_si& instruction) const
{
	if (instruction.type == semantics::instruction_si_type::ORD)
	{
		auto opcode = hlasm_ctx.get_mnemonic_opcode(std::get<context::id_index>(instruction.value));
		if (opcode == copy_id || opcode == equ_id_)
			return std::make_pair(processing_format(processing_kind::LOOKAHEAD, processing_form::ASM), op_code(opcode,context::instruction_type::ASM));
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
		add_diagnostic(diagnostic_s::error_E047("", *start.target, start.target_range));

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

lookahead_processor::lookahead_processor(context::hlasm_context& hlasm_ctx, branching_provider& branch_provider, processing_state_listener& listener, const lookahead_start_data start)
	: statement_processor(processing_kind::LOOKAHEAD, hlasm_ctx),
	finished_flag_(false),result_(start.source), macro_nest_(0),provider_(branch_provider), listener_(listener), equ_id_(hlasm_ctx.ids().add("EQU")), start(std::move(start)) {}

void lookahead_processor::process_MACRO() { ++macro_nest_; }
void lookahead_processor::process_MEND() { --macro_nest_; }
void lookahead_processor::process_COPY() {  }

void lookahead_processor::process_statement(const context::hlasm_statement& statement)
{
	assert(statement.kind == context::statement_kind::RESOLVED);

	auto resolved = statement.access_resolved();

	if (macro_nest_ == 0)
		find_target(*resolved);

	if (resolved->opcode_ref().value == macro_id)
	{
		process_MACRO();
	}
	else if (resolved->opcode_ref().value == mend_id)
	{
		process_MEND();
	}
	else if (resolved->opcode_ref().value == copy_id)
	{
		process_COPY();
	}
}

void lookahead_processor::find_target(const semantics::complete_statement& statement)
{
	switch (start.action)
	{
	case lookahead_action::SEQ:
		find_seq(statement);
		break;
	case lookahead_action::ORD:
		//TODO
		break;
	default:
		assert(false);
		break;
	}
}

void lookahead_processor::find_seq(const semantics::complete_statement& statement)
{
	if (statement.label_ref().type == semantics::label_si_type::SEQ)
	{
		auto symbol = std::get<semantics::seq_sym>(statement.label_ref().value);
		
		if (!hlasm_ctx.get_sequence_symbol(symbol.name))
		{
			provider_.register_sequence_symbol(symbol.name,symbol.symbol_range);
		}

		if (symbol.name == start.target)
		{
			finished_flag_ = true;
			result_ = lookahead_processing_result(symbol.name, symbol.symbol_range);
		}
	}

}
