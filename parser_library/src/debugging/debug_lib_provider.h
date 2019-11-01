#ifndef HLASMPLUGIN_PARSERLIBRARY_DEBUG_LIB_PROVIDER_H
#define HLASMPLUGIN_PARSERLIBRARY_DEBUG_LIB_PROVIDER_H

#include "../workspace.h"

namespace hlasm_plugin::parser_library::debugging
{

class debug_lib_provider : public parse_lib_provider
{
	const workspace& ws_;
public:
	debug_lib_provider(const workspace& ws) : ws_(ws) {}

	virtual parse_result parse_library(const std::string& library, context::hlasm_context& hlasm_ctx, const library_data data) override
	{
		auto& proc_grp = ws_.get_proc_grp_by_program(hlasm_ctx.opencode_file_name());
		for (auto&& lib : proc_grp.libraries())
		{
			std::shared_ptr<processor> found = lib->find_file(library);
			if (found)
				return found->parse_no_lsp_update(*this, hlasm_ctx, data);
		}

		return false;
	}

	virtual bool has_library(const std::string& library, context::hlasm_context& hlasm_ctx) const override
	{
		auto& proc_grp = ws_.get_proc_grp_by_program(hlasm_ctx.opencode_file_name());
		for (auto&& lib : proc_grp.libraries())
		{
			std::shared_ptr<processor> found = lib->find_file(library);
			if (found)
				return true;
		}

		return false;
	}

};

}

#endif
