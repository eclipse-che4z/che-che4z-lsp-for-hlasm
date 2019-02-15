#include "semantic_objects.h"
#include "operand.h"
#include "semantic_highlighting_info.h"

using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library::context;

hlasm_plugin::parser_library::semantics::symbol_range::symbol_range()
	: begin_ln(0), begin_col(0), end_ln(0), end_col(0) {}

hlasm_plugin::parser_library::semantics::symbol_range::symbol_range(size_t begin_ln, size_t begin_col, size_t end_ln, size_t end_col)
	: begin_ln(begin_ln), begin_col(begin_col), end_ln(end_ln), end_col(end_col) {}

bool hlasm_plugin::parser_library::semantics::symbol_range::operator==(const symbol_range& oth) const
{
	return begin_col == oth.begin_col && begin_ln == oth.begin_ln && end_col == oth.end_col && end_ln == oth.end_ln;
}

symbol_range hlasm_plugin::parser_library::semantics::symbol_range::union_range(const symbol_range & lhs, const symbol_range & rhs)
{
	symbol_range ret;
	ret.begin_ln = std::min(lhs.begin_ln, rhs.begin_ln);
	ret.begin_col = std::min(lhs.begin_col, rhs.begin_col);
	ret.end_ln = std::max(lhs.end_ln, rhs.end_ln);
	ret.end_col = std::max(lhs.end_col, rhs.end_col);
	return ret;
}

symbol_range hlasm_plugin::parser_library::semantics::symbol_range::get_range(antlr4::Token * start, antlr4::Token * stop)
{
	symbol_range ret;

	ret.begin_ln = start->getLine();
	ret.begin_col = start->getCharPositionInLine();

	if (stop)
	{
		ret.end_ln = stop->getLine();
		ret.end_col = stop->getCharPositionInLine() + stop->getStopIndex() - stop->getStartIndex() + 1;
	}
	else //empty rule
	{
		ret.end_ln = ret.begin_ln;
		ret.end_col = ret.begin_col;
	}

	return ret;
}

symbol_range hlasm_plugin::parser_library::semantics::symbol_range::get_range(antlr4::Token * token)
{
	return get_range(token, token);
}

symbol_range hlasm_plugin::parser_library::semantics::symbol_range::get_range(antlr4::ParserRuleContext * rule_ctx)
{
	return get_range(rule_ctx->getStart(), rule_ctx->getStop());
}

symbol_range hlasm_plugin::parser_library::semantics::symbol_range::get_empty_range(antlr4::Token * start)
{
	symbol_range ret;
	ret.begin_ln = start->getLine();
	ret.end_ln = ret.begin_ln;
	ret.begin_col = start->getCharPositionInLine();
	ret.end_col = ret.begin_col;
	return ret;
}

hlasm_plugin::parser_library::semantics::set_type::set_type(const context::A_t & value)
	:a_value(value), b_value(object_traits<B_t>::default_v()), c_value(object_traits<C_t>::default_v()), type(set_type_enum::A_TYPE) {}

hlasm_plugin::parser_library::semantics::set_type::set_type(const context::B_t & value)
	: a_value(object_traits<A_t>::default_v()), b_value(value), c_value(object_traits<C_t>::default_v()), type(set_type_enum::B_TYPE) {}

hlasm_plugin::parser_library::semantics::set_type::set_type(const context::C_t & value)
	: a_value(object_traits<A_t>::default_v()), b_value(object_traits<B_t>::default_v()), c_value(value), type(set_type_enum::C_TYPE) {}

hlasm_plugin::parser_library::semantics::set_type::set_type()
	: a_value(object_traits<A_t>::default_v()), b_value(object_traits<B_t>::default_v()), c_value(object_traits<C_t>::default_v()), type(set_type_enum::UNDEF_TYPE) {}

const A_t & hlasm_plugin::parser_library::semantics::set_type::access_a() { return a_value; }

const B_t & hlasm_plugin::parser_library::semantics::set_type::access_b() { return b_value; }

const C_t & hlasm_plugin::parser_library::semantics::set_type::access_c() { return c_value; }
