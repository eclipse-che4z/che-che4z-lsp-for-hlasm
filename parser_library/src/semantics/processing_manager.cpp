#include "processing_manager.h"
#include "ordinary_processor.h"
#include "lookahead_processor.h"
#include "macro_def_processor.h"
#include "../parser_impl.h"

namespace hlasm_plugin::parser_library::semantics
{

processing_manager::processing_manager(std::string file_name, context::ctx_ptr ctx, parse_lib_provider & lib_provider)
	: diagnosable_ctx(ctx),
	ctx_mngr_(std::move(ctx), lib_provider),
	ord_processor_(std::make_unique<ordinary_processor>(*this)),
	look_processor_(std::make_unique<lookahead_processor>(*this)),
	mac_def_processor_(std::make_unique<macro_def_processor>(*this)),
	current_processor_(ord_processor_.get()),
	file_name_(std::move(file_name)) {}

void processing_manager::initialize(parser_impl * parser)
{
	parser_ = parser;
}

void processing_manager::process_instruction(instruction_semantic_info instr)
{
	current_processor_->process_instruction(std::move(instr));
}

void processing_manager::process_statement(statement stmt)
{
	ctx_mngr_.ctx().set_current_statement_range(stmt.instr_info.range);

	//currently assigned processor processes the statement
	current_processor_->process_statement(std::move(stmt));

	//clears parser's parsing format
	parser_->format.reset();

	//if there is macro to be called, perform the action
	if (ctx_mngr_.ctx().is_in_macro())
		macro_loop();
}

void processing_manager::collect_diags() const
{
	collect_diags_from_child(ctx_mngr_);
	collect_diags_from_child(*ord_processor_);
	collect_diags_from_child(*look_processor_);
	collect_diags_from_child(*mac_def_processor_);
}

const std::string & processing_manager::file_name()
{
	return file_name_;
}

void processing_manager::macro_loop()
{
	while (ctx_mngr_.ctx().is_in_macro())
	{
		//take most inner macro frame
		auto & frame = ctx_mngr_.ctx().get_scope_stack().back().this_macro;

		assert(frame->current_statement <= frame->definition->size());

		//check if last statement was processed
		if (frame->current_statement == frame->definition->size())
		{
			ctx_mngr_.ctx().leave_macro();
			continue;
		}

		//copies next statement to process
		statement stmt((*frame->definition)[frame->current_statement++]);

		ctx_mngr_.ctx().set_current_statement_range(stmt.instr_info.range);
		//proceses statement
		current_processor_->process_instruction(std::move(stmt.instr_info));
		current_processor_->process_statement(std::move(stmt));
		parser_->format.reset();
	}
}

}