#include "statement_processor.h"
#include "processing_manager.h"


hlasm_plugin::parser_library::semantics::statement_processor::statement_processor(processing_manager & mngr, std::function<process_table_t(context::hlasm_context&)> table_init) : mngr_(mngr),process_table(table_init(mngr.ctx_.ctx())) {}

void hlasm_plugin::parser_library::semantics::statement_processor::jump_in_statements(hlasm_plugin::parser_library::location loc)
{
	if (mngr_.macro_call_stack_.empty())
		mngr_.parser_->rewind_input(loc);
	else
		mngr_.macro_call_stack_.top().current_statement = loc.line - mngr_.macro_call_stack_.top().macro_ctx->location.offset;
}

bool hlasm_plugin::parser_library::semantics::statement_processor::is_last_line()
{
	if (mngr_.macro_call_stack_.empty())
		return mngr_.parser_->is_last_line();
	else
		return mngr_.macro_call_stack_.top().current_statement == mngr_.macro_call_stack_.top().macro_ctx->definition->size();
}

void hlasm_plugin::parser_library::semantics::statement_processor::finish()
{
	mngr_.current_processor_ = mngr_.ord_processor_.get();
}

void hlasm_plugin::parser_library::semantics::statement_processor::enter_macro(context::macro_invo_ptr macro)
{
	mngr_.macro_call_stack_.push(
		{std::move(macro), (size_t)0}
	);
}

hlasm_plugin::parser_library::semantics::context_manager & hlasm_plugin::parser_library::semantics::statement_processor::ctx_mngr()
{
	return mngr_.ctx_;
}

hlasm_plugin::parser_library::parser_impl & hlasm_plugin::parser_library::semantics::statement_processor::parser()
{
	return *mngr_.parser_;
}

void hlasm_plugin::parser_library::semantics::statement_processor::start_lookahead(start_info_ptr start_info)
{
	mngr_.current_processor_ = mngr_.look_processor_.get();
	mngr_.current_processor_->set_start_info(std::move(start_info));
	parser().format.in_lookahead = true;
}

void hlasm_plugin::parser_library::semantics::statement_processor::start_macro_definition(start_info_ptr start_info)
{
	mngr_.current_processor_ = mngr_.mac_def_processor_.get();
	mngr_.current_processor_->set_start_info(std::move(start_info));
}
