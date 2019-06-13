#include <string>
#include <memory>

#include "processor_file_impl.h"
#include "file.h"
namespace hlasm_plugin::parser_library {

processor_file_impl::processor_file_impl(std::string file_name) :
	file_impl(std::move(file_name))
{
}

processor_file_impl::processor_file_impl(file_impl && f_impl) :
	file_impl(std::move(f_impl))
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

parse_result processor_file_impl::parse(parse_lib_provider & find_provider)
{
	auto ctx = std::make_shared<hlasm_plugin::parser_library::context::hlasm_context>(get_file_name());

	auto res = parse(find_provider, ctx);

	for (auto m : ctx->get_called_macros())
		if(m->file_name != get_file_name())
			dependencies_.insert(std::move(m->file_name));
	

	return res;
}


parse_result processor_file_impl::parse(parse_lib_provider & lib_provider, context::ctx_ptr ctx)
{
	diags().clear();

	analyzer_ = std::make_unique<analyzer>(get_text(), ctx, lib_provider, get_file_name());

	analyzer_->analyze();
	
	collect_diags_from_child(*analyzer_);
	
	//TO DO ICTL
	analyzer_->lsp_processor().get_hl_info().cont_info.continue_column = 15;
	analyzer_->lsp_processor().get_hl_info().cont_info.continuation_column = 71;

	//collect semantic info
	parse_info_updated_ = true;
	
	return true;
}

bool processor_file_impl::parse_info_updated()
{
	bool ret = parse_info_updated_;
	parse_info_updated_ = false;
	return ret;
}

const std::set<std::string> & processor_file_impl::dependencies()
{
	return dependencies_;
}

const file_highlighting_info processor_file_impl::get_hl_info()
{
	return analyzer_->lsp_processor().get_hl_info();
}

const semantics::lsp_info_processor processor_file_impl::get_lsp_info()
{
	return analyzer_->lsp_processor();
}

}