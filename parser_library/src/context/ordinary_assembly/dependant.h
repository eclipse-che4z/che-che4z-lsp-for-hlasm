#ifndef SEMANTICS_SYMBOL_DEPENDANT_H
#define SEMANTICS_SYMBOL_DEPENDANT_H

#include <variant>

#include "address.h"
#include "symbol_attributes.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

struct attr_ref
{
	data_attr_kind attribute;
	id_index symbol_id;

	bool operator==(const attr_ref& oth) const;
};

enum class dependant_kind { SYMBOL = 0, SYMBOL_ATTR = 1, SPACE = 2 };
struct dependant
{
	using value_t = std::variant<id_index, attr_ref, space_ptr>;

	dependant(id_index symbol_id);
	dependant(attr_ref attribute_reference);
	dependant(space_ptr space_id);

	bool operator==(const dependant& oth) const;
	dependant_kind kind() const;

	value_t value;
};

}
}
}

namespace std
{
template<>
struct hash<hlasm_plugin::parser_library::context::attr_ref>
{
	std::size_t operator()(const hlasm_plugin::parser_library::context::attr_ref& k) const
	{
		return std::hash<hlasm_plugin::parser_library::context::id_index>()(k.symbol_id) + (size_t)k.attribute;
	}
};

template<>
struct hash<hlasm_plugin::parser_library::context::dependant>
{
	std::size_t operator()(const hlasm_plugin::parser_library::context::dependant& k) const
	{
		return hash<hlasm_plugin::parser_library::context::dependant::value_t>()(k.value);
	}
};
}

#endif
