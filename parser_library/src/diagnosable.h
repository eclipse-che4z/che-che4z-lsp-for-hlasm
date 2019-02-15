#ifndef HLASMPARSER_PARSERLIBRARY_DIAGNOSTICABLE_H
#define HLASMPARSER_PARSERLIBRARY_DIAGNOSTICABLE_H

#include <string>
#include <vector>

#include "shared/protocol.h"
#include "diagnostic.h"

namespace hlasm_plugin::parser_library {


using diagnostic_container = std::vector<diagnostic_s>;

class diagnosable
{
public:
	virtual void collect_diags() const = 0;
	virtual diagnostic_container & diags() const = 0;
	virtual void add_diagnostic(diagnostic_s diagnostic) const = 0;
	virtual bool is_once_only() const = 0;
};



}
#endif
