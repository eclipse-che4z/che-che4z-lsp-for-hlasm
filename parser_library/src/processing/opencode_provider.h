#ifndef PROCESSING_OPENCODE_PROVIDER_H
#define PROCESSING_OPENCODE_PROVIDER_H

#include "../context/source_snapshot.h"
#include "statement_provider.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//interface for hiding parser implementation
class opencode_provider : public statement_provider
{
public:
	//rewinds position in file
	virtual void rewind_input(context::source_position pos) = 0;

	//pushes the logical line ending to currently evaluated statement
	//so other statements can be processed within evaluation of current
	//ending is poped with first rewind_input
	virtual void push_line_end() = 0;

	opencode_provider()
		: statement_provider(processing::statement_provider_kind::OPEN) {}

	virtual ~opencode_provider() = default;
};

}
}
}
#endif
