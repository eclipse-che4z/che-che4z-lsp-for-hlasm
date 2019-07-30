#ifndef CONTEXT_FILE_PROCESSING_STATUS_H
#define CONTEXT_FILE_PROCESSING_STATUS_H

#include "variable.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

enum class file_processing_type { OPENCODE, COPY, MACRO };

//structure grouping information about current position in file and what it was opened for
struct file_processing_status
{
	file_processing_type type;
	location processing_location;
};

using file_proc_stack = std::vector<file_processing_status>;

}
}
}

#endif
