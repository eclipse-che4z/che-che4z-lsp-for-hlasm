#include "copy_processor.h"
#include "macrodef_processor.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

copy_processor::copy_processor(context::hlasm_context& hlasm_ctx, processing_state_listener& listener, const copy_start_data start)
	:statement_processor(processing_kind::COPY, hlasm_ctx), listener_(listener), start_(std::move(start)), macro_nest_(0), first_statement_(true) 
{
	result_.member_name = start_.member_name;
	result_.invalid_member = false;
}

processing_status copy_processor::get_processing_status(const semantics::instruction_si& instruction) const
{
	auto status = macrodef_processor::get_macro_processing_status(instruction, hlasm_ctx);
	status.first.kind = processing_kind::COPY;
	return status;
}

void copy_processor::process_statement(context::shared_stmt_ptr statement)
{
	process_statement(*statement);

	result_.definition.push_back(statement);
}

void copy_processor::process_statement(context::unique_stmt_ptr statement)
{
	process_statement(*statement);

	result_.definition.push_back(std::move(statement));
}

void copy_processor::end_processing()
{
	if (macro_nest_ > 0)
	{
		range r(hlasm_ctx.processing_stack().back().pos);
		add_diagnostic(diagnostic_s::error_E061("", *start_.member_name, r));
		result_.invalid_member = true;
	}

	listener_.finish_copy_member(std::move(result_));

	hlasm_ctx.pop_processing_file();
}

bool copy_processor::terminal_condition(const statement_provider_kind prov_kind) const
{
	return prov_kind == statement_provider_kind::OPEN;
}

bool copy_processor::finished()
{
	return false;
}

void copy_processor::collect_diags() const {}

void copy_processor::process_statement(const context::hlasm_statement& statement)
{
	if (first_statement_)
	{
		result_.definition_location = hlasm_ctx.processing_stack().back();
		first_statement_ = false;
	}

	if (auto res_stmt = statement.access_resolved())
	{
		if (res_stmt->opcode_ref().value == macro_id)
			process_MACRO();
		else if (res_stmt->opcode_ref().value == mend_id)
			process_MEND();
	}
}

void copy_processor::process_MACRO() { ++macro_nest_; }

void copy_processor::process_MEND() 
{
	--macro_nest_;
	if (macro_nest_ < 0)
	{
		range r(hlasm_ctx.processing_stack().back().pos);
		add_diagnostic(diagnostic_s::error_E061("", *start_.member_name, r));
		result_.invalid_member = true;
	}
}