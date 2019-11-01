#include "variable.h"

using namespace hlasm_plugin::parser_library::debugging;

const std::string& variable::get_value() const
{
	if (value_)
		return *value_;
	else
		return get_string_value();
}

const std::string& variable::get_name() const
{
	if (name_)
		return *name_;
	else
		return get_string_name();
}
