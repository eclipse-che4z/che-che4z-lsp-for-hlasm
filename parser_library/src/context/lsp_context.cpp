#include "lsp_context.h"

using namespace hlasm_plugin::parser_library::context;

bool definition::operator==(const definition& other) const
{
	if (check_scopes)
		return name == other.name && scope == other.scope && file_name == other.file_name;
	else
		return name == other.name;
}

bool definition::operator<(const definition& other) const
{
	if (check_scopes)
		return std::tie(name, scope, file_name) < std::tie(other.name, other.scope, other.file_name);
	else
		return name < other.name;
}