#ifndef PROCESSING_OPENCODE_PROVIDER_H
#define PROCESSING_OPENCODE_PROVIDER_H

#include "../context/variable.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//interface for hiding parser implementation
class opencode_provider
{
public:
	virtual void rewind_input(context::opencode_sequence_symbol::opencode_position loc) = 0;
	virtual context::opencode_sequence_symbol::opencode_position statement_start() const = 0;
	virtual context::opencode_sequence_symbol::opencode_position statement_end() const = 0;

	virtual ~opencode_provider() = default;
};

}
}
}
#endif
