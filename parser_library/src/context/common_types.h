#ifndef CONTEXT_COMMON_TYPES_H
#define CONTEXT_COMMON_TYPES_H

#include <string>
#include <cctype>

namespace hlasm_plugin {
namespace parser_library {
namespace context {

//type for SETA symbol
using A_t = int;
//type for SETB symbol
using B_t = bool;
//type for SETC symbol
using C_t = std::string;

//enum of SET symbols
enum class set_type_enum
{
	A_TYPE, B_TYPE, C_TYPE, UNDEF_TYPE
};

//enum of variable symbols
enum class variable_kind
{
	SET_VAR_KIND, MACRO_VAR_KIND, UNDEF_VAR_KIND
};

//enum of macro symbolic parameters
enum class macro_param_type
{
	POS_PAR_KIND, KEY_PAR_KIND, UNDEF_PAR_KIND
};

template <typename T>
struct object_traits
{
	static constexpr set_type_enum type_enum = set_type_enum::UNDEF_TYPE;
};

template <> struct object_traits<A_t>
{
	static constexpr set_type_enum type_enum = set_type_enum::A_TYPE;
	static const A_t& default_v()
	{
		static A_t def = 0;
		return def;
	}
};

template <> struct object_traits<B_t>
{
	static constexpr set_type_enum type_enum = set_type_enum::B_TYPE;
	static const B_t& default_v()
	{
		static B_t def = false;
		return def;
	}
};

template <> struct object_traits<C_t>
{
	static constexpr set_type_enum type_enum = set_type_enum::C_TYPE;
	static const C_t& default_v()
	{
		static C_t def("");
		return def;
	}
};

//just mock method for now, will be implemented later with respect to UTF/EBCDIC
std::string& to_upper(std::string& s);

}
}
}
#endif
