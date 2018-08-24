#include "common_types.h"

using namespace hlasm_plugin::parser_library::context;

std::string & hlasm_plugin::parser_library::context::to_upper(std::string & s)
{
	for (auto & c : s) c = static_cast<char>(std::toupper(c));
	return s;
}
