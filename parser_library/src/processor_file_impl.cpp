#include <string>
#include <memory>

#include "processor_file_impl.h"
#include "file.h"
namespace hlasm_plugin::parser_library {

processor_file_impl::processor_file_impl(std::string file_name, std::atomic<bool>* cancel) :
	file_impl(std::move(file_name)), cancel_(cancel)
{
}

processor_file_impl::processor_file_impl(file_impl && f_impl, std::atomic<bool>* cancel) :
	file_impl(std::move(f_impl)), cancel_(cancel)
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

parse_result processor_file_impl::parse(parse_lib_provider & lib_provider)
{
	analyzer_ = std::make_unique<analyzer>(get_text(), get_file_name(), lib_provider);

	dependencies_.clear();

	auto res = parse_inner(*analyzer_);

	for (auto& file : analyzer_->context().get_visited_files())
		if(file != get_file_name())
			dependencies_.insert(file);
	
	return res;
}


parse_result processor_file_impl::parse(parse_lib_provider & lib_provider, context::hlasm_context& hlasm_ctx, const library_data data)
{
	analyzer_ = std::make_unique<analyzer>(get_text(), get_file_name(), hlasm_ctx, lib_provider, data);

	return parse_inner(*analyzer_);
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

bool processor_file_impl::parse_inner(analyzer& new_analyzer)
{
	diags().clear();

	new_analyzer.analyze(cancel_);

	collect_diags_from_child(new_analyzer);

	//TO DO ICTL
	analyzer_->lsp_processor().get_hl_info().cont_info.continue_column = 15;
	analyzer_->lsp_processor().get_hl_info().cont_info.continuation_column = 71;

	//collect semantic info
	parse_info_updated_ = true;

	if (cancel_ && *cancel_)
		return false;
	return true;
}

}