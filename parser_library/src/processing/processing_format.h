#ifndef PROCESSING_PROCESSING_FORMAT_H
#define PROCESSING_PROCESSING_FORMAT_H

#include "../context/instruction.h"

#include <utility>

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

enum class processing_kind
{
	ORDINARY,LOOKAHEAD,MACRO,COPY
};

enum class processing_form
{
	MACH, ASM, MAC, CA, DAT, IGNORED, DEFERRED, UNKNOWN
};

enum class ordinary_proc_type
{
	MACH, ASM, MAC, CA, DAT, UNKNOWN
};

enum class lookahead_proc_type
{
	COPY, EQU, IGNORED
};

enum class macro_proc_type
{
	PROTO, COPY, CA, DEFERRED
};

enum class operand_occurence
{
	PRESENT,ABSENT
};

//structure respresenting in which fashion should be statement processed
struct processing_format
{
	processing_format(processing_kind kind, processing_form form, operand_occurence occurence = operand_occurence::PRESENT)
		: kind(kind), form(form), occurence(occurence) {}

	processing_kind kind;
	processing_form form;
	operand_occurence occurence;
};

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
