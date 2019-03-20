#include "processing_manager.h"
#include "ordinary_processor.h"
#include "lookahead_processor.h"
#include "macro_def_processor.h"
#include "../parser_impl.h"

hlasm_plugin::parser_library::semantics::processing_manager::processing_manager(context::ctx_ptr ctx)
	: ctx_(std::move(ctx)), ord_processor_(std::make_unique<ordinary_processor>(*this)), look_processor_(std::make_unique<lookahead_processor>(*this)), mac_def_processor_(std::make_unique<macro_def_processor>(*this)), current_processor_(ord_processor_.get()) {}

void hlasm_plugin::parser_library::semantics::processing_manager::initialize(parser_impl * parser)
{
	parser_ = parser;
}

void hlasm_plugin::parser_library::semantics::processing_manager::process_instruction(instruction_semantic_info instr)
{
	current_processor_->process_instruction(std::move(instr));
}

void hlasm_plugin::parser_library::semantics::processing_manager::process_statement(statement stmt)
{
	//currently assigned processor processes the statement
	current_processor_->process_statement(std::move(stmt));

	//clears parser's parsing format
	parser_->format.reset();

	//if there is macro to be called, perform the action
	if (!macro_call_stack_.empty())
		macro_loop();
}

void hlasm_plugin::parser_library::semantics::processing_manager::collect_diags() const
{
	collect_diags_from_child(ctx_);
	collect_diags_from_child(*ord_processor_);
	collect_diags_from_child(*look_processor_);
	collect_diags_from_child(*mac_def_processor_);
}

void hlasm_plugin::parser_library::semantics::processing_manager::macro_loop()
{
	while (!macro_call_stack_.empty())
	{
		//take most inner macro frame
		auto& frame = macro_call_stack_.top();

		assert(frame.current_statement <= frame.macro_ctx->definition->size());

		//check if last statement was processed
		if (frame.current_statement == frame.macro_ctx->definition->size())
		{
			macro_call_stack_.pop();
			ctx_.ctx().leave_macro();
			continue;
		}

		//copies next statement to process
		statement stmt((*frame.macro_ctx->definition)[frame.current_statement++]);

		//proceses statement
		current_processor_->process_instruction(std::move(stmt.instr_info));
		current_processor_->process_statement(std::move(stmt));
		parser_->format.reset();
	}
}
