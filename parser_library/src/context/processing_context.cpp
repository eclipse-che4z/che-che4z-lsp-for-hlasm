#include "processing_context.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;

source_context::source_context(std::string source_name)
	: source_status(position(), std::move(source_name)), begin_index(0), end_index(0) {}

source_snapshot source_context::create_snapshot() const
{
	std::vector<copy_frame> copy_frames;

	for (auto& member : copy_stack)
		copy_frames.emplace_back(member.name, member.current_statement);

	if (!copy_frames.empty())
		--copy_frames.back().statement_offset;

	return source_snapshot{ (size_t)source_status.pos.line,begin_index,end_index,std::move(copy_frames) };
}

std::vector<location>& source_context::append_processing_stack(std::vector<location>& proc_stack) const
{
	proc_stack.push_back(source_status);

	for (auto& member : copy_stack)
	{
		auto& stmt = member.definition[member.current_statement];
		proc_stack.emplace_back(stmt->statement_position(), member.definition_location.file);
	}

	return proc_stack;
}

processing_context::processing_context(processing::processing_kind proc_kind, bool owns_source)
	: proc_kind(proc_kind), owns_source(owns_source) {}