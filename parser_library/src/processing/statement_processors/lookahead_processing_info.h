#ifndef PROCESSING_LOOKAHEAD_PROCESSING_INFO_H
#define PROCESSING_LOOKAHEAD_PROCESSING_INFO_H

#include "../../context/id_storage.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

enum class lookahead_action { SEQ, ORD };

struct lookahead_start_data
{
	lookahead_action action;
	context::id_index target;
	range target_range;

	context::opencode_sequence_symbol::opencode_position source;
};

struct lookahead_processing_result
{
	bool success;

	context::opencode_sequence_symbol::opencode_position source;
	context::id_index target;
	range target_range;

	lookahead_processing_result(context::opencode_sequence_symbol::opencode_position source)
		:success(false), source(source),target(context::id_storage::empty_id) {}

	lookahead_processing_result(context::id_index target, range target_range)
		:success(true), target(target), target_range(target_range) {}
};

}
}
}
#endif
