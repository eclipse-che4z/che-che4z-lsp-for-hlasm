#ifndef CONTEXT_PROCESSING_CONTEXT_H
#define CONTEXT_PROCESSING_CONTEXT_H

#include  "../processing/processing_format.h"
#include "copy_member.h"
#include "source_snapshot.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

struct source_context
{
	location current_instruction;

	size_t begin_index;
	size_t end_index;
	size_t end_line;
	std::vector<copy_member_invocation> copy_stack;

	source_context(std::string source_name);

	source_snapshot create_snapshot() const;
};

struct processing_context
{
	processing::processing_kind proc_kind;

	bool owns_source;

	processing_context(processing::processing_kind proc_kind, bool owns_source);
};

enum class file_processing_type { OPENCODE, COPY, MACRO };

struct code_scope;

struct processing_frame
{
	processing_frame(location proc_location, const code_scope& scope, file_processing_type proc_type);

	location proc_location;
	const code_scope& scope;
	file_processing_type proc_type;
};

using processing_stack_t = std::vector<processing_frame>;

}
}
}
#endif
