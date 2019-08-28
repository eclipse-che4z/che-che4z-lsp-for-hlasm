#ifndef CONTEXT_COMMON_TYPES_H
#define CONTEXT_COMMON_TYPES_H

#include <string>

namespace hlasm_plugin {
namespace parser_library {
namespace context {

//type for SETA symbol
using A_t = int32_t;
//type for SETB symbol
using B_t = bool;
//type for SETC symbol
using C_t = std::string;

//enum of SET symbols
enum class SET_t_enum
{
	A_TYPE, B_TYPE, C_TYPE, UNDEF_TYPE
};

//enum of variable symbols
enum class variable_kind
{
	SET_VAR_KIND, MACRO_VAR_KIND
};

//enum of macro symbolic parameters
enum class macro_param_type
{
	POS_PAR_TYPE, KEY_PAR_TYPE, SYSLIST_TYPE
};

template <typename T>
struct object_traits
{
	static constexpr SET_t_enum type_enum = SET_t_enum::UNDEF_TYPE;
};

template <> struct object_traits<A_t>
{
	static constexpr SET_t_enum type_enum = SET_t_enum::A_TYPE;
	static const A_t& default_v()
	{
		static A_t def = 0;
		return def;
	}
};

template <> struct object_traits<B_t>
{
	static constexpr SET_t_enum type_enum = SET_t_enum::B_TYPE;
	static const B_t& default_v()
	{
		static B_t def = false;
		return def;
	}
};

template <> struct object_traits<C_t>
{
	static constexpr SET_t_enum type_enum = SET_t_enum::C_TYPE;
	static const C_t& default_v()
	{
		static C_t def("");
		return def;
	}
};

//struct agregating SET types for easier usage
struct SET_t
{
private:
	A_t a_value;
	B_t b_value;
	C_t c_value;
public:
	SET_t(A_t value);
	SET_t(B_t value);
	SET_t(C_t value);
	SET_t();

	const SET_t_enum type;

	A_t& access_a();
	B_t& access_b();
	C_t& access_c();

	A_t C2A(const C_t& value) const;

	template<typename T>
	T to();
};

template<>
inline A_t SET_t::to()
{
	switch (type)
	{
	case SET_t_enum::A_TYPE:
		return a_value;
	case SET_t_enum::B_TYPE:
		return b_value;
	case SET_t_enum::C_TYPE:
		return C2A(c_value); //TODO
	default:
		return a_value;
	}
}

template<>
inline B_t SET_t::to()
{
	switch (type)
	{
	case SET_t_enum::A_TYPE:
		return a_value;
	case SET_t_enum::B_TYPE:
		return b_value;
	case SET_t_enum::C_TYPE:

		return b_value; //TODO

	default:
		return b_value;
	}
}

template<>
inline C_t SET_t::to()
{
	switch (type)
	{
	case SET_t_enum::A_TYPE:
		return std::to_string(std::abs(a_value));
	case SET_t_enum::B_TYPE:
		return b_value ? "1" : "0";
	case SET_t_enum::C_TYPE:
		return c_value;
	default:
		return c_value;
	}
}

//just mock method for now, will be implemented later with respect to UTF/EBCDIC
std::string& to_upper(std::string& s);

}
}
}
#endif
