#include "dependant.h"

#include <cassert>

using namespace hlasm_plugin::parser_library::context;

dependant::dependant(id_index symbol)
	:value(symbol) {}


dependant::dependant(space_ptr space_id)
	: value(std::move(space_id)) {}

dependant::dependant(attr_ref attribute_reference)
	: value(std::move(attribute_reference)) {}

bool dependant::operator==(const dependant& oth) const
{
	return value == oth.value;
}

dependant_kind dependant::kind() const
{
	return static_cast<dependant_kind>(value.index());
}

bool attr_ref::operator==(const attr_ref& oth) const
{
	return attribute == oth.attribute && symbol_id == oth.symbol_id;
}