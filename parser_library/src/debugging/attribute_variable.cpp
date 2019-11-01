#include <stdexcept>
#include <cassert>

#include "attribute_variable.h"

using namespace hlasm_plugin::parser_library;
using namespace debugging;

attribute_variable::attribute_variable(std::string name, std::string value)
{
	name_ = std::move(name);
	value_ = std::move(value);
}

set_type attribute_variable::type() const
{
	return set_type::UNDEF_TYPE;
}

bool attribute_variable::is_scalar() const
{
	return true;
}

std::vector<variable_ptr> attribute_variable::values() const
{
	return std::vector<variable_ptr>();
}

size_t attribute_variable::size() const
{
	return 0;
}

const std::string& attribute_variable::get_string_value() const
{
	throw std::runtime_error("Function ord_sym_attribute::get_string_value should never be called!");
}

const std::string& attribute_variable::get_string_name() const
{
	throw std::runtime_error("Function ord_sym_attribute::get_string_name should never be called!");
}
