/*
 * Copyright (c) 2019 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

#ifndef HLASMPLUGIN_PARSERLIBRARY_DEBUG_LIB_PROVIDER_H
#define HLASMPLUGIN_PARSERLIBRARY_DEBUG_LIB_PROVIDER_H

#include "../workspace.h"

namespace hlasm_plugin::parser_library::debugging
{

//Implements dependency (macro and COPY files) fetcher for macro tracer.
//Takes the information from a workspace, but calls special methods for
//parsing that do not collide with LSP.
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
