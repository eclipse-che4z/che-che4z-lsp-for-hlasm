#ifndef PROCESSING_PROCESSING_STATE_LISTENER_H
#define PROCESSING_PROCESSING_STATE_LISTENER_H

#include "statement_processors/macrodef_processing_info.h"
#include "statement_processors/lookahead_processing_info.h"
#include "../context/variable.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//interface for listening that a statement processor needs to be started or has finished
class processing_state_listener
{
public:
	virtual void start_macro_definition(const macrodef_start_data start) = 0;
	virtual void finish_macro_definition(macrodef_processing_result result) = 0;

	virtual void start_lookahead(const lookahead_start_data start) = 0;
	virtual void finish_lookahead(lookahead_processing_result result) = 0;

	virtual ~processing_state_listener() = default;
};

}
}
}
#endif
