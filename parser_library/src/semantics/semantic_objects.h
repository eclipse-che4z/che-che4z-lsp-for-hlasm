#ifndef SEMANTICS_SEMANTICOBJECTS_H
#define SEMANTICS_SEMANTICOBJECTS_H
#include <cmath>
#include "antlr4-runtime.h"
#include "../context/common_types.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

//struct representing interaval of symbol
struct symbol_range
{
	size_t begin_ln;
	size_t begin_col;

	size_t end_ln;
	size_t end_col;

	symbol_range();

	symbol_range(size_t begin_ln, size_t begin_col, size_t end_ln, size_t end_col);


	bool operator==(const symbol_range& oth) const;

	static symbol_range union_range(const symbol_range& lhs, const symbol_range& rhs);

	static symbol_range get_range(antlr4::Token * start, antlr4::Token * stop);

	static symbol_range get_range(antlr4::Token * token);

	static symbol_range get_range(antlr4::ParserRuleContext * rule_ctx);

	static symbol_range get_empty_range(antlr4::Token * start);
};

//struct agregating SET types for easier usage
struct set_type
{
private:
	const context::A_t& a_value;
	const context::B_t& b_value;
	const context::C_t& c_value;
public:
	set_type(const context::A_t& value);
	set_type(const context::B_t& value);
	set_type(const context::C_t& value);
	set_type();

	const context::set_type_enum type;

	const context::A_t& access_a();
	const context::B_t& access_b();
	const context::C_t& access_c();

	template<typename T>
	T to();
};

template<>
inline context::A_t set_type::to()
{
	switch (type)
	{
	case context::set_type_enum::A_TYPE:
		return a_value;
	case context::set_type_enum::B_TYPE:
		return b_value;
	case context::set_type_enum::C_TYPE:
		return b_value; //TODO
	default:
		return a_value;
	}
}

template<>
inline context::B_t set_type::to()
{
	switch (type)
	{
	case context::set_type_enum::A_TYPE:
		return a_value;
	case context::set_type_enum::B_TYPE:
		return b_value;
	case context::set_type_enum::C_TYPE:
		return b_value; //TODO
	default:
		return b_value;
	}
}

template<>
inline context::C_t set_type::to()
{
	switch (type)
	{
	case context::set_type_enum::A_TYPE:
		return std::to_string(std::abs(a_value));
	case context::set_type_enum::B_TYPE:
		if (b_value) return "1";
		else return "0";
	case context::set_type_enum::C_TYPE:
		return c_value;
	default:
		return c_value;
	}
}

//struct guarding other symbol in propagation over parsing tree
template <typename T>
struct symbol_guard
{
	symbol_guard(T&& value) : value(std::move(value)), valid(true) {}

	symbol_guard() : valid(false) {}

	operator bool() const { return valid; }

	T value;
	bool valid;
};

//representation of sequence symbol in parsed tree
struct seq_sym
{
	std::string name;
	context::location loc;
};

}
}
}
#endif
