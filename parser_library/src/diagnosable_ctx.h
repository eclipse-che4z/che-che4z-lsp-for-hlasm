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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DIAGNOSABLE_CTX_H
#define HLASMPLUGIN_PARSERLIBRARY_DIAGNOSABLE_CTX_H

#include "diagnosable_impl.h"
#include "context/hlasm_context.h"
#include "diagnostic_collector.h"

namespace hlasm_plugin::parser_library {

class diagnosable_ctx : public diagnosable_impl
{
public:
	virtual void add_diagnostic(diagnostic_op diagnostic) const
	{
		add_diagnostic_inner(std::move(diagnostic), ctx_.processing_stack());
	}

protected:
	diagnosable_ctx(context::hlasm_context& ctx) : ctx_(ctx) {}

	virtual ~diagnosable_ctx() {};
private:
	void add_diagnostic_inner(diagnostic_op diagnostic, const context::processing_stack_t& stack) const
	{
		diagnostic_s diag (stack.back().proc_location.file, diagnostic);

		for (auto frame = ++stack.rbegin(); frame != stack.rend(); ++frame)
		{
			auto& file_name = frame->proc_location.file;
			range r = range(frame->proc_location.pos, frame->proc_location.pos);
			diagnostic_related_info_s s = diagnostic_related_info_s(range_uri_s(file_name, r), "While compiling " + file_name + '(' + std::to_string(frame->proc_location.pos.line + 1) + ")");
			diag.related.push_back(std::move(s));
		}
		diagnosable_impl::add_diagnostic(std::move(diag));
	}
	context::hlasm_context& ctx_;

	friend diagnostic_collector;
};

}


#endif