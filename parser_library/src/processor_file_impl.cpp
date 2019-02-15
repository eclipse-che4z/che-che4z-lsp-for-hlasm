#include <string>
#include <memory>

#include "processor_file_impl.h"
#include "parser_error_listener.h"
#include "generated/hlasmparser.h"
#include "file.h"
#include "error_strategy.h"
namespace hlasm_plugin::parser_library {

processor_file_impl::processor_file_impl(std::string file_name) :
	file_impl(std::move(file_name)),
	sm_info_(get_file_name())
{
}

processor_file_impl::processor_file_impl(file_impl && f_impl) :
	file_impl(std::move(f_impl)), sm_info_(get_file_name())
{
}

void processor_file_impl::collect_diags() const
{
	file_impl::collect_diags();
}

bool processor_file_impl::is_once_only() const
{
	return false;
}

semantics::semantic_info & processor_file_impl::semantic_info()
{
	return sm_info_;
}

program_context * processor_file_impl::parse(parse_lib_provider & find_provider)
{
	return parse(find_provider, std::make_shared<hlasm_plugin::parser_library::context::hlasm_context>());
}


program_context * processor_file_impl::parse(parse_lib_provider &, std::shared_ptr<context::hlasm_context> ctx)
{
	diags().clear();
	antlr4::CommonTokenFactory factory;
	input_ = std::make_unique<input_source>(get_text());
	lexer_ = std::make_unique<lexer>(input_.get());
	tokens_ = std::make_unique<token_stream>(lexer_.get());
	parser_ = std::make_unique<generated::hlasmparser>(tokens_.get());

	

	parser_->setErrorHandler(std::make_shared<error_strategy>());
	parser_->initialize(ctx, lexer_.get());

	parser_->file_name = get_file_name();

	parser_->removeErrorListeners();
	parser_->addErrorListener(&listener_);

	

	auto tree = parser_->program();
	
	collect_diags_from_child(listener_);
	collect_diags_from_child(*parser_);
	for (auto & it : diags())
	{
		it.file_name = get_file_name();
	}

	sm_info_.clear();
	//TODO add ICTL support
	sm_info_.clear();
	sm_info_.continue_column = 15;
	sm_info_.continuation_column = 71;
	sm_info_.merge(lexer_->semantic_info);
	sm_info_.merge(parser_->semantic_info);

	sm_info_.hl_info.document.version = get_version();
	//collect semantic info
	parse_info_updated_ = true;
	return tree;
}

bool processor_file_impl::parse_info_updated()
{
	bool ret = parse_info_updated_;
	parse_info_updated_ = false;
	return ret;
}

}