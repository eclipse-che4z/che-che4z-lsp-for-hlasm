#ifndef HLASMPLUGIN_PARSERLIBRARY_DIAGNOSABLE_CTX_H
#define HLASMPLUGIN_PARSERLIBRARY_DIAGNOSABLE_CTX_H

#include "diagnosable_impl.h"
#include "context/hlasm_context.h"

namespace hlasm_plugin::parser_library {

class diagnosable_ctx : public diagnosable_impl
{
protected:
	
	diagnosable_ctx(context::ctx_ptr ctx) : ctx_(ctx) {}

	virtual void add_diagnostic(diagnostic_s diagnostic) const override
	{
		const auto & stack = ctx_->get_scope_stack();
		if (stack.back().is_in_macro())
		{
			context::id_index id = stack.back().this_macro->id;
			diagnostic.file_name = ctx_->macros().at(id)->file_name;
			
			for(auto frame = ++stack.rbegin(); frame != stack.rend(); ++frame)
			{
				const std::string & file_name = frame->this_macro ? ctx_->macros().at(frame->this_macro->id)->file_name : ctx_->get_top_level_file_name();
				range r;
				semantics::symbol_range sr;
				sr = frame->current_stmt_range;
				r = { {sr.begin_ln, sr.begin_col}, {sr.end_ln, sr.end_col } };
				diagnostic_related_info_s s = diagnostic_related_info_s(range_uri_s(file_name, r), "While compiling " + file_name + '(' + std::to_string(frame->current_stmt_range.begin_ln) + ")");
				diagnostic.related.push_back(std::move(s));
			}
			
		}
		else
		{
			diagnostic.file_name = ctx_->get_top_level_file_name();
		}
		diagnosable_impl::add_diagnostic(std::move(diagnostic));
	}

	virtual ~diagnosable_ctx() {};
private:
	context::ctx_ptr ctx_;
};

}


#endif