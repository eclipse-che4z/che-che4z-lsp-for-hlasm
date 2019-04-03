#include <string>
#include <memory>

#include "processor_file_impl.h"
#include "file.h"
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
	
	

	sm_info_.clear();
	//TODO add ICTL support
	sm_info_.clear();
	sm_info_.continue_column = 15;
	sm_info_.continuation_column = 71;
	sm_info_.merge(dynamic_cast<lexer*>( analyzer_->parser().getTokenStream()->getTokenSource())->semantic_info);
	sm_info_.merge(analyzer_->parser().semantic_info);

	sm_info_.hl_info.document.version = get_version();
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

}