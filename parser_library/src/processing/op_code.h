#ifndef PROCESSING_OP_CODE_H
#define PROCESSING_OP_CODE_H

#include "../context/instruction.h"
#include "processing_format.h"

#include <utility>

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//structure holding resolved operation code of the instruction (solving OPSYNs and so on)
struct op_code
{
	op_code()
		:value(context::id_storage::empty_id), type(context::instruction_type::UNDEF) {}
	op_code(context::id_index value, context::instruction_type type)
		:value(value), type(type) {}

	context::id_index value;
	context::instruction_type type;
};

using processing_status = std::pair<processing_format, op_code>;

}
}
}
#endif
