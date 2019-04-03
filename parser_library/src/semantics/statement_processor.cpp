#include "statement_processor.h"
#include "processing_manager.h"

namespace hlasm_plugin::parser_library::semantics
{

statement_processor::statement_processor(processing_manager & mngr, std::function<process_table_t(context::hlasm_context&)> table_init)
	: diagnosable_ctx(mngr), process_table(table_init(mngr.ctx_mngr_.ctx())), mngr_(mngr)  {}

void statement_processor::jump_in_statements(hlasm_plugin::parser_library::location loc)
{
	if (!ctx_mngr().ctx().is_in_macro())
		mngr_.parser_->rewind_input(loc);
	else
		current_macro()->current_statement = loc.line;
}

context::macro_invo_ptr statement_processor::current_macro()
{
	return ctx_mngr().ctx().get_scope_stack().back().this_macro;
}

bool statement_processor::is_last_line()
{
	
	if (!ctx_mngr().ctx().is_in_macro())
		return mngr_.parser_->is_last_line();
	else
		return current_macro()->current_statement == current_macro()->definition->size();
}

void statement_processor::finish()
{
	mngr_.current_processor_ = mngr_.ord_processor_.get();
}

context_manager & statement_processor::ctx_mngr()
{
	return mngr_.ctx_mngr_;
}

hlasm_plugin::parser_library::parser_impl & statement_processor::parser()
{
	return *mngr_.parser_;
}

void statement_processor::start_lookahead(start_info_ptr start_info)
{
	mngr_.current_processor_ = mngr_.look_processor_.get();
	mngr_.current_processor_->set_start_info(std::move(start_info));
	parser().format.in_lookahead = true;
}

void statement_processor::start_macro_definition(start_info_ptr start_info)
{
	mngr_.current_processor_ = mngr_.mac_def_processor_.get();
	mngr_.current_processor_->set_start_info(std::move(start_info));
}

const std::string & statement_processor::file_name()
{
	return mngr_.file_name();
}

}