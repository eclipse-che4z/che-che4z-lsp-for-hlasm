#ifndef HLASMPARSER_PARSERLIBRARY_DIAGNOSTICABLE_H
#define HLASMPARSER_PARSERLIBRARY_DIAGNOSTICABLE_H

#include <string>
#include <vector>

#include "shared/protocol.h"
#include "diagnostic.h"

namespace hlasm_plugin::parser_library {

template <typename T>
class collectable
{
public:
	using diagnostic_container = std::vector<T>;

	virtual void collect_diags() const = 0;
	virtual diagnostic_container & diags() const = 0;
	virtual void add_diagnostic(T diagnostic) const = 0;
	virtual bool is_once_only() const = 0;

	virtual ~collectable() = 0;
};

template <typename T>
inline collectable<T>::~collectable() {};

using diagnosable = collectable<diagnostic_s>;

}

#endif

