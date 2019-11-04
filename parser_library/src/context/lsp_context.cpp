#include "lsp_context.h"

using namespace hlasm_plugin::parser_library::context;

bool definition::operator==(const definition& other) const
{
	if (check_scopes || other.check_scopes)
		return name == other.name && scope == other.scope && file_name == other.file_name && version == other.version;
	else
		return name == other.name && version == other.version;
}

bool definition::operator!=(const definition& other) const
{
	return !(other == *this);
}

bool definition::operator<(const definition& other) const
{
	if (check_scopes || other.check_scopes)
		return std::tie(name, scope, file_name,version) < std::tie(other.name, other.scope, other.file_name,other.version);
	else
		return std::tie(name,version) < std::tie(other.name, other.version);
}