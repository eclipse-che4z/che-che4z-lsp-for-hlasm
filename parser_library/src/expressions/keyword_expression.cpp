/*
 * Copyright (c) 2019 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

#include "keyword_expression.h"
#include "arithmetic_expression.h"
#include <algorithm>
#include "../error_messages.h"
#include <stdexcept>

using namespace hlasm_plugin::parser_library::expressions;

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

keyword_expression::keyword_expression(const keyword_expression & expr) : s_val_(expr.s_val_), value_(expr.value_)
{
	if (expr.diag)
		diag = std::make_unique<diagnostic_op>(*expr.diag);
}

bool keyword_expression::is_unary() const
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
	case keyword_type::AND_NOT:
		return 2;
	case keyword_type::OR:
	case keyword_type::OR_NOT:
		return 3;
	case keyword_type::XOR:
	case keyword_type::XOR_NOT:
		return 4;
	case keyword_type::SLA:
	case keyword_type::SLL:
	case keyword_type::SRA:
	case keyword_type::SRL:
		return 5;
	default:
		return 1;
	}
}

bool keyword_expression::is_keyword() const { return true; }

bool keyword_expression::is_complex_keyword() const 
{ 
	return value_ == keyword_type::AND || value_ == keyword_type::OR || value_ == keyword_type::XOR;
}

std::string keyword_expression::get_str_val() const { return s_val_; }

expr_ptr keyword_expression::to_expression() const
{
	return default_expr_with_error<arithmetic_expression>(error_messages::not_implemented());
}

bool keyword_expression::is_keyword(str_ref k)
{
	return keywords_.find(k) != keywords_.cend();
}
