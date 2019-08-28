#ifndef HLASMPLUGIN_PARSERLIBRARY_DIAGNOSABLE_CTX_H
#define HLASMPLUGIN_PARSERLIBRARY_DIAGNOSABLE_CTX_H

#include "diagnosable_impl.h"
#include "context/hlasm_context.h"
#include "diagnostic_collector.h"

namespace hlasm_plugin::parser_library {

class diagnosable_ctx : public diagnosable_impl
{
protected:
	
	diagnosable_ctx(context::hlasm_context& ctx) : ctx_(ctx) {}

	virtual void add_diagnostic(diagnostic_s diagnostic) const override
	{
		add_diagnostic_inner(std::move(diagnostic), ctx_.processing_stack());
	}

	virtual ~diagnosable_ctx() {};
private:
	void add_diagnostic_inner(diagnostic_s diagnostic, const std::vector<location>& stack) const
	{
		diagnostic.file_name = stack.back().file;

		for (auto frame = ++stack.rbegin(); frame != stack.rend(); ++frame)
		{
			auto& file_name = frame->file;
			range r = range(frame->pos, frame->pos);
			diagnostic_related_info_s s = diagnostic_related_info_s(range_uri_s(file_name, r), "While compiling " + file_name + '(' + std::to_string(frame->pos.line + 1) + ")");
			diagnostic.related.push_back(std::move(s));
		}

		diagnosable_impl::add_diagnostic(std::move(diagnostic));
	}

	context::hlasm_context& ctx_;

	friend diagnostic_collector;
};

}


#endif