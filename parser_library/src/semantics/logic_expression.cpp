#include "logic_expression.h"
#include "../error_messages.h"
#include "numeric_wrapper.h"
#include <algorithm>

using namespace hlasm_plugin;
using namespace parser_library;
using namespace semantics;

#define make_arith(val) std::make_unique<arithmetic_expression>(val)
#define make_logic(val) std::make_unique<logic_expression>(val)

logic_expression::logic_expression(bool v)
	: value_(v)
{
}

logic_expression::logic_expression(int32_t v)
	: value_(v != 0)
{
}

expr_ptr hlasm_plugin::parser_library::semantics::logic_expression::to_arith() const
{
	return make_arith(static_cast<int32_t>(value_));
}

expr_ptr logic_expression::binary_operation(str_ref o, expr_ref arg2) const
{
	std::string operation_name = o;
	std::transform(operation_name.begin(), operation_name.end(), operation_name.begin(), [](char c) { return static_cast<char>(toupper(c)); });

	bool val = false;
	auto e = arg2->retype<logic_expression>();
	if (e == nullptr)
	{
		auto ae = arg2->retype<arithmetic_expression>();
		if (ae == nullptr)
			return default_expr_with_error<logic_expression>
			(error_messages::el01());
		val = ae->get_value() != 0;
	}
	else
		val = e->get_value();

	copy_return_on_error_binary(arg2.get(), logic_expression);

	if (operation_name == "OR")
		return make_logic(value_ || val);

	if (operation_name == "AND")
		return make_logic(value_ && val);

	if (operation_name == "XOR")
		return make_logic(value_ ^ val);

	return default_expr_with_error<logic_expression>
		(error_messages::el02());
}

int32_t hlasm_plugin::parser_library::semantics::logic_expression::get_numeric_value() const
{
	return static_cast<int32_t>(value_);
}

std::string hlasm_plugin::parser_library::semantics::logic_expression::get_str_val() const
{
	return std::to_string(get_numeric_value());
}

bool hlasm_plugin::parser_library::semantics::logic_expression::get_value() const
{
	return value_;
}

expr_ptr hlasm_plugin::parser_library::semantics::logic_expression::operator+(expression_ref e) const
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

expr_ptr hlasm_plugin::parser_library::semantics::logic_expression::operator-(expression_ref e) const
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

expr_ptr hlasm_plugin::parser_library::semantics::logic_expression::operator*(expression_ref e) const
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

expr_ptr hlasm_plugin::parser_library::semantics::logic_expression::operator/(expression_ref e) const
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

expr_ptr hlasm_plugin::parser_library::semantics::logic_expression::operator|(expression_ref e) const
{
	copy_return_on_error_binary(e, arithmetic_expression);

	auto w = al_wrap(e);
	if (!w)
		return default_expr_with_error<arithmetic_expression>(error_messages::ea09());
	auto value = w.value() != 0;

	return make_logic(value_ || value);
}

expr_ptr hlasm_plugin::parser_library::semantics::logic_expression::operator&(expression_ref e) const
{
	copy_return_on_error_binary(e, arithmetic_expression);

	auto w = al_wrap(e);
	if (!w)
		return default_expr_with_error<arithmetic_expression>(error_messages::ea09());
	auto value = w.value() != 0;

	return make_logic(value_ && value);
}

expr_ptr hlasm_plugin::parser_library::semantics::logic_expression::operator+() const
{
	copy_return_on_error(this, arithmetic_expression);

	return make_arith(static_cast<int32_t>(value_));
}

expr_ptr hlasm_plugin::parser_library::semantics::logic_expression::operator-() const
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
