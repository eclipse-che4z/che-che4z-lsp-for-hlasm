#ifndef HLASMPLUGIN_PARSERLIBRARY_DIAGNOSTIC_COLLECTOR_H
#define HLASMPLUGIN_PARSERLIBRARY_DIAGNOSTIC_COLLECTOR_H

#include <vector>
#include "shared/protocol.h"
#include "diagnostic.h"

namespace hlasm_plugin::parser_library {

class diagnosable_ctx;

class diagnostic_collector
{
	diagnosable_ctx* diagnoser_;
	std::vector<location> location_stack_;
public:
	diagnostic_collector(diagnosable_ctx* diagnoser, std::vector<location> location_stack);

	diagnostic_collector(diagnosable_ctx* diagnoser);

	diagnostic_collector();

	void operator()(diagnostic_op diagnostic) const;
};

}
#endif