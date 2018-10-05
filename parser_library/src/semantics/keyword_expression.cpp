#include "keyword_expression.h"
#include "arithmetic_expression.h"
#include <algorithm>
#include "../error_messages.h"

using namespace hlasm_plugin::parser_library::semantics;

std::map<std::string, keyword_expression::keyword_type, keyword_expression::upper_equal> keyword_expression::keywords_ = {
#define X(k) {#k, keyword_expression::keyword_type::k},
		KEYWORDS
#undef X
		{"*", keyword_expression::keyword_type::ASTERISK}
};

keyword_expression::keyword_expression(str_ref k)
{
	std::string kw = k;
	std::transform(kw.begin(), kw.end(), kw.begin(), [](char c) { return static_cast<char>(toupper(c)); });
	auto f = keywords_.find(kw);
	if (f != keywords_.cend())
		value_ = f->second;
	else
		throw std::runtime_error("symbol is not a keyword");
	s_val_ = std::move(kw);
}

bool hlasm_plugin::parser_library::semantics::keyword_expression::is_unary() const
{
	return value_ == keyword_type::NOT || value_ == keyword_type::BYTE || value_ == keyword_type::LOWER || value_ == keyword_type::SIGNED || value_ == keyword_type::UPPER || value_ == keyword_type::DOUBLE;
}

uint8_t keyword_expression::priority() const
{
	if (is_unary())
		return 0;

	switch (value_)
	{
	case keyword_type::AND:
		return 1;
	case keyword_type::OR:
		return 2;
	case keyword_type::XOR:
		return 3;
	case keyword_type::SLA:
	case keyword_type::SLL:
	case keyword_type::SRA:
	case keyword_type::SRL:
		return 4;
	default:
		return 5;
	}
}

bool hlasm_plugin::parser_library::semantics::keyword_expression::is_keyword() const { return true; }

std::string hlasm_plugin::parser_library::semantics::keyword_expression::get_str_val() const { return s_val_; }

expr_ptr hlasm_plugin::parser_library::semantics::keyword_expression::to_expression() const
{
	/* TODO: resolve expr */
	return default_expr_with_error<arithmetic_expression>(error_messages::not_implemented());
}

bool hlasm_plugin::parser_library::semantics::keyword_expression::is_keyword(str_ref k)
{
	return keywords_.find(k) != keywords_.cend();
}
