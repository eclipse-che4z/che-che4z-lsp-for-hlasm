#ifndef HLASMPARSER_PARSERLIBRARY_DIAGNOSTICABLE_H
#define HLASMPARSER_PARSERLIBRARY_DIAGNOSTICABLE_H

#include <string>
#include <vector>

#include "../include/shared/protocol.h"

namespace hlasm_plugin::parser_library {

	enum class diagnostic_severity
	{
		error = 1,
		warning = 2,
		info = 3,
		hint = 4,
		unspecified = 5
	};

	struct diagnostic_op
	{
		diagnostic_severity severity;
		std::string code;
		std::string message;
		diagnostic_op() {}
		diagnostic_op(const diagnostic_op&d) :
			severity(d.severity), code((d.code)), message((d.message)) {}
		diagnostic_op(diagnostic_severity severity, std::string code, std::string message) :
			severity(severity), code(std::move(code)), message(std::move(message)) {}
	};

}
#endif
