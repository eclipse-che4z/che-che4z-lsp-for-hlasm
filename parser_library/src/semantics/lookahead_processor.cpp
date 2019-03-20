#include "lookahead_processor.h"
#include "processing_manager.h"

using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library::semantics;

lookahead_processor::lookahead_processor(processing_manager& mngr) : statement_processor(mngr, std::bind(&lookahead_processor::init_table, this, std::placeholders::_1)), macro_nest_(0) {}

void hlasm_plugin::parser_library::semantics::lookahead_processor::set_start_info(start_info_ptr info)
{
	failed = false;
	auto look_info = dynamic_cast<lookahead_info*>(info.get());
	
	if (!look_info)
		throw std::invalid_argument("bad type of start_info");

	start_info_ = std::make_unique<lookahead_info>(std::move(*look_info));
}

void hlasm_plugin::parser_library::semantics::lookahead_processor::process_instruction(instruction_semantic_info instruction)
{
	curr_op_code_ = ctx_mngr().get_opcode_info(std::move(instruction));
}

void hlasm_plugin::parser_library::semantics::lookahead_processor::process_statement(statement stmt)
{
	curr_label_ = std::move(stmt.label_info);

	if (failed)
	{
		finish();
		//ERR lookahead failed TODO diagnostics, sequence symbol not defined --- use start_info.target_range
		return;
	}

	process_label_field();

	//if current instruction is macro do nothing
	if (curr_op_code_.type == instruction_type::MAC)
		return;

	//else process current instruction
	auto it = process_table.find(curr_op_code_.op_code);
	if (it!=process_table.end())
		it->second();
}

void hlasm_plugin::parser_library::semantics::lookahead_processor::collect_diags() const {}

void hlasm_plugin::parser_library::semantics::lookahead_processor::finish()
{
	parser().format.in_lookahead = false;
	statement_processor::finish();
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
	//if in macro, continue
	if (macro_nest_ != 0)
		return;
	
	if (curr_label_.type == label_type::SEQ)
	{
		auto found = ctx_mngr().get_id(std::move(curr_label_.sequence_symbol.name));
		auto loc = curr_label_.sequence_symbol.location;
		if (found == start_info_->target)
		{
			jump_in_statements(loc);
			finish();
			return;
		}
	}

	if (is_last_line())
	{
		jump_in_statements(start_info_->location);
		failed = true;
	}
}

process_table_t hlasm_plugin::parser_library::semantics::lookahead_processor::init_table(hlasm_context& ctx)
{
	process_table_t table;
	table.emplace(ctx.ids.add("COPY"),
		std::bind(&lookahead_processor::process_COPY, this));
	table.emplace(ctx.ids.add("MACRO"),
		std::bind(&lookahead_processor::process_MACRO, this));
	table.emplace(ctx.ids.add("MEND"),
		std::bind(&lookahead_processor::process_MEND, this));

	return table;
}
