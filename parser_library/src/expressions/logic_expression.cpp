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

#include "logic_expression.h"
#include "logic_expression.h"
#include "logic_expression.h"
#include "../error_messages.h"
#include "numeric_wrapper.h"
#include <algorithm>

using namespace hlasm_plugin;
using namespace parser_library;
using namespace expressions;

logic_expression::logic_expression(bool v)
	: value_(v)
{
}

logic_expression::logic_expression(const logic_expression & expr):value_(expr.value_)
{
	if (expr.diag)
		diag = std::make_unique<diagnostic_op>(*expr.diag);
}

logic_expression::logic_expression(int32_t v)
	: value_(v != 0)
{
}

expr_ptr logic_expression::to_arith() const
{
	return make_arith(static_cast<int32_t>(value_));
}

expr_ptr logic_expression::binary_operation(str_ref o, expr_ref arg2) const
{
	std::string operation_name = o;
	//normalize operation name
	//all text is in HLASM capitalized by default, but we expect lowercase too (for easier work on PC)
	std::transform(operation_name.begin(), operation_name.end(), operation_name.begin(), [](char c) { return static_cast<char>(toupper(c)); });

	bool val = false;
	//second operand can be either logical or arithmetic
	auto e = arg2->retype<logic_expression>();
	if (e == nullptr)
	{
		auto ae = arg2->retype<arithmetic_expression>();
		if (ae == nullptr)
			return default_expr_with_error<logic_expression>
			(error_messages::el01());
		//arithmetic value is converted into logical
		val = ae->get_value() != 0;
	}
	else
		val = e->get_value();

	//check if the second operand contains error
	//is so, copy it and return
	copy_return_on_error_binary(arg2.get(), logic_expression);

	/**
	 * standard logical operations
	 * */

	if (operation_name == "EQ")
		return make_logic(value_ == val);

	if (operation_name == "NE")
		return make_logic(value_ != val);

	if (operation_name == "OR")
		return make_logic(value_ || val);

	if (operation_name == "OR NOT")
		return make_logic(value_ || !val);

	if (operation_name == "AND")
		return make_logic(value_ && val);

	if (operation_name == "AND NOT")
		return make_logic(value_ && !val);

	if (operation_name == "XOR")
		return make_logic(value_ ^ val);

	if (operation_name == "XOR NOT")
		return make_logic(value_ ^ !val);

	return default_expr_with_error<logic_expression>
		(error_messages::el02());
}

int32_t logic_expression::get_numeric_value() const
{
	return static_cast<int32_t>(value_);
}

std::string logic_expression::get_str_val() const
{
	return std::to_string(get_numeric_value());
}

context::SET_t logic_expression::get_set_value() const
{
	return value_;
}

bool logic_expression::get_value() const
{
	return value_;
}

expr_ptr logic_expression::operator+(expression_ref e) const
{
	copy_return_on_error_binary(e, arithmetic_expression);

	auto w = al_wrap(e);
	if (!w)
		return default_expr_with_error<arithmetic_expression>(error_messages::ea09());
	auto value = w.value();

	auto res = static_cast<int64_t>(value_) + static_cast<int64_t>(value);
	if (res > INT32_MAX || res < INT32_MIN)
		return default_expr_with_error<arithmetic_expression>
		(error_messages::ea10());

	return make_arith(static_cast<int32_t>(res));
}

expr_ptr logic_expression::operator-(expression_ref e) const
{
	copy_return_on_error_binary(e, arithmetic_expression);

	auto w = al_wrap(e);
	if (!w)
		return default_expr_with_error<arithmetic_expression>(error_messages::ea09());
	auto value = w.value();

	auto res = static_cast<int64_t>(value_) - static_cast<int64_t>(value);
	if (res > INT32_MAX || res < INT32_MIN)
		return default_expr_with_error<arithmetic_expression>
		(error_messages::ea10());

	return make_arith(static_cast<int32_t>(res));
}

expr_ptr logic_expression::operator*(expression_ref e) const
{
	copy_return_on_error_binary(e, arithmetic_expression);

	auto w = al_wrap(e);
	if (!w)
		return default_expr_with_error<arithmetic_expression>(error_messages::ea09());
	auto value = w.value();

	auto res = static_cast<int64_t>(value_) * static_cast<int64_t>(value);
	if (res > INT32_MAX || res < INT32_MIN)
		return default_expr_with_error<arithmetic_expression>
		(error_messages::ea10());

	return make_arith(static_cast<int32_t>(res));
}

expr_ptr logic_expression::operator/(expression_ref e) const
{
	copy_return_on_error_binary(e, arithmetic_expression);

	auto w = al_wrap(e);
	if (!w)
		return default_expr_with_error<arithmetic_expression>(error_messages::ea09());
	auto value = w.value();

	if (value == 0)
		return make_arith(0);

	return make_arith(get_numeric_value() / value);
}

expr_ptr logic_expression::operator|(expression_ref e) const
{
	copy_return_on_error_binary(e, arithmetic_expression);

	auto w = al_wrap(e);
	if (!w)
		return default_expr_with_error<arithmetic_expression>(error_messages::ea09());
	auto value = w.value() != 0;

	return make_logic(value_ || value);
}

expr_ptr logic_expression::operator&(expression_ref e) const
{
	copy_return_on_error_binary(e, arithmetic_expression);

	auto w = al_wrap(e);
	if (!w)
		return default_expr_with_error<arithmetic_expression>(error_messages::ea09());
	auto value = w.value() != 0;

	return make_logic(value_ && value);
}

expr_ptr logic_expression::operator+() const
{
	copy_return_on_error(this, arithmetic_expression);

	return make_arith(static_cast<int32_t>(value_));
}

expr_ptr logic_expression::operator-() const
{
	copy_return_on_error(this, arithmetic_expression);

	return make_arith(-static_cast<int32_t>(value_));
}

expr_ptr logic_expression::unary_operation(str_ref o) const
{
	std::string operation_name = o;
	std::transform(operation_name.begin(), operation_name.end(), operation_name.begin(), [](char c) { return static_cast<char>(toupper(c)); });
	if (operation_name == "NOT")
	{
		copy_return_on_error(this, logic_expression);
		return make_logic(!value_);
	}

	auto ax = to_arith();
	return ax->unary_operation(operation_name);
}
