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
	location source_status;
	size_t begin_index;
	size_t end_index;
	std::vector<copy_member_invocation> copy_stack;

	source_context(std::string source_name);

	source_snapshot create_snapshot() const;

	std::vector<location>& append_processing_stack(std::vector<location>& proc_stack) const;
};

struct processing_context
{
	processing::processing_kind proc_kind;

	bool owns_source;

	processing_context(processing::processing_kind proc_kind, bool owns_source);
};

}
}
}
#endif
