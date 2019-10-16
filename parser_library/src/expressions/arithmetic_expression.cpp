#include "arithmetic_expression.h"
#include "../ebcdic_encoding.h"
#include <algorithm>
#include "expression.h"
#include <bitset>
#include "character_expression.h"
#include "../error_messages.h"
#include "numeric_wrapper.h"
#include <stdexcept>
#include <charconv>

using namespace hlasm_plugin;
using namespace parser_library;
using namespace expressions;

const int32_t numeric_part_mask = (1 << 31) ^ static_cast<int32_t>(-1);

arithmetic_expression::arithmetic_expression(int32_t val)
	: value_(val)
{
}

arithmetic_expression::arithmetic_expression(const arithmetic_expression & expr):value_(expr.value_)
{
	if (expr.diag)
		diag = std::make_unique<diagnostic_op>(*expr.diag);
}

context::SET_t arithmetic_expression::get_set_value() const
{
	return value_;
}

int32_t arithmetic_expression::get_value() const
{
	return value_;
}

expr_ptr arithmetic_expression::from_string(const std::string_view& s, int base)
{
	if (s.empty())
		return make_arith(0);

	bool may_have_sign = base == 10;
	constexpr auto max = static_cast<long long>(INT32_MAX);
	constexpr auto min = static_cast<long long>(INT32_MIN);
	constexpr auto umax = static_cast<long long>(UINT32_MAX);
	
	size_t sign_off = 0;

	if (*s.begin() == '+' && base == 10)
		sign_off = 1;

	long long lval;
	auto conversion_result = std::from_chars(s.data() + sign_off, s.data() + sign_off + s.size(), lval, base);

	if ((may_have_sign && lval > max && lval < min) || !may_have_sign && lval > umax 
		|| conversion_result.ec == std::errc::result_out_of_range)
		return default_expr_with_error<arithmetic_expression>
		(error_messages::ea01());

	if (conversion_result.ec == std::errc::invalid_argument)
		return default_expr_with_error<arithmetic_expression>
		(error_messages::ea02(std::string(s)));

	return make_arith(static_cast<int32_t>(lval));
}

std::string arithmetic_expression::get_str_val() const
{
	return std::to_string(value_);
}

expr_ptr arithmetic_expression::from_string(const std::string &option, const std::string_view&value, bool dbcs)
{
	if (option.empty() || toupper(option[0]) == 'D')
		return from_string(value, 10);

	if (toupper(option[0]) == 'B')
		return from_string(value, 2);

	if (toupper(option[0]) == 'X')
		return from_string(value, 16);

	if (toupper(option[0]) == 'C')
		return c2arith(std::string(value));

	if (toupper(option[0]) == 'G')
		return g2arith(std::string(value), dbcs);

	return default_expr_with_error<arithmetic_expression>(error_messages::ea03());
}

expr_ptr arithmetic_expression::from_string(const std::string_view& value, bool dbcs)
{
	if(value.empty())
		return default_expr_with_error<arithmetic_expression>(error_messages::ea03());

	if (isdigit(value.front()))
		return from_string("", value, dbcs);

	if (value.size() >= 3 && value[1] == '\'' && value.back() == '\'')
		return from_string({ value.front() }, value.substr(2, value.size() - 3), dbcs);
	else
		return default_expr_with_error<arithmetic_expression>(error_messages::ea03());
}

expr_ptr arithmetic_expression::c2arith(const std::string& value)
{
	/*
		first escaping is enforced in grammar (for ampersands and apostrophes)
		second escaping escapes only apostrophes
	*/
	int32_t val = 0;
	size_t escaped = 0;
	for (const char *i = value.c_str(); *i != 0; ++i)
	{
		if (*i == '\''
			&& *(i + 1) == '\'')
		{
			++i;
			++escaped;
		}

		val <<= 8;
		val += ebcdic_encoding::to_ebcdic(ebcdic_encoding::to_pseudoascii(i));
	}

	if (value.length() - escaped > 4)
		return default_expr_with_error<arithmetic_expression>(error_messages::ea04());

	return make_arith(val);
}

enum class G2C_STATES
{
	EMPTY,
	CLOSED,
	DOUBLE_BYTE_EMPTY,
	DOUBLE_BYTE_ODD,
	DOUBLE_BYTE_EVEN,
	INVALID
};

expr_ptr arithmetic_expression::g2arith(const std::string& value, bool dbcs)
{
	if (!dbcs)
		return default_expr_with_error<arithmetic_expression>
		(error_messages::ea05());

	int32_t val = 0;
	G2C_STATES state = G2C_STATES::EMPTY;
	for (const char *i = value.c_str(); *i != 0; ++i)
	{
		if (*i == '<')
		{
			if (state == G2C_STATES::EMPTY
				|| state == G2C_STATES::CLOSED)
			{
				state = G2C_STATES::DOUBLE_BYTE_EMPTY;
				continue;
			}
			else
				break;
		}
		else if (*i == '>')
		{
			if (state == G2C_STATES::DOUBLE_BYTE_EVEN)
			{
				state = G2C_STATES::CLOSED;
				continue;
			}
			else
				break;
		}
		else if (state != G2C_STATES::CLOSED)
		{
			if (state == G2C_STATES::DOUBLE_BYTE_EVEN)
				state = G2C_STATES::DOUBLE_BYTE_ODD;
			else if (state == G2C_STATES::DOUBLE_BYTE_ODD)
				state = G2C_STATES::DOUBLE_BYTE_EVEN;
		}
		else
		{
			state = G2C_STATES::INVALID;
			break;
		}

		val <<= 8;
		val += ebcdic_encoding::to_ebcdic(ebcdic_encoding::to_pseudoascii(i));
	}

	if (state != G2C_STATES::CLOSED)
		return default_expr_with_error<arithmetic_expression>
		(error_messages::ea06());

	return make_arith(val);
}

expr_ptr arithmetic_expression::binary_operation(str_ref operation, expr_ref arg2) const
{
	std::string o = operation;
	std::transform(o.begin(), o.end(), o.begin(), [](char c) { return static_cast<char>(toupper(c)); });
	int32_t val = 0;
	auto e = arg2->retype<arithmetic_expression>();
	if (e == nullptr)
	{
		auto ae = arg2->retype<logic_expression>();
		if (ae == nullptr)
		{
			return default_expr_with_error<arithmetic_expression>
				(error_messages::ea07());
		}
		val = ae->get_value();
	}
	else
		val = e->get_value();

	if (o == "OR")
	{
		copy_return_on_error_binary(arg2.get(), arithmetic_expression);
		return make_arith(value_ | val);
	}

	if (o == "AND")
	{
		copy_return_on_error_binary(arg2.get(), arithmetic_expression);
		return make_arith(value_ & val);
	}

	if (o == "SLA")
	{
		copy_return_on_error_binary(arg2.get(), arithmetic_expression);

		uint32_t value = static_cast<uint32_t>(value_);

		return make_arith(
			(value & (1 << 31))
			| ((value & (static_cast<uint32_t>(numeric_part_mask))) << static_cast<uint32_t>(val)));
	}

	if (o == "SLL")
	{
		copy_return_on_error_binary(arg2.get(), arithmetic_expression);
		uint32_t value = static_cast<uint32_t>(value_);
		return make_arith(static_cast<uint32_t>(value << static_cast<uint32_t>(val)));
	}

	if (o == "SRA")
	{
		copy_return_on_error_binary(arg2.get(), arithmetic_expression);
		uint64_t value = static_cast<uint64_t>(static_cast<int64_t>(value_));
		if ((63 & val) > 31)
			return make_arith(static_cast<int32_t>((value & (1 << 31)) >> 31));
		return make_arith(static_cast<int32_t>(value
			>> (static_cast<uint64_t>(val) & (63))
			));
	}

	if (o == "SRL")
	{
		copy_return_on_error_binary(arg2.get(), arithmetic_expression);
		return make_arith(
			static_cast<int32_t>(
				static_cast<uint64_t>(static_cast<uint32_t>(value_))
				>> static_cast<uint64_t>(val & (63))
				));
	}

	if (o == "XOR")
	{
		copy_return_on_error_binary(arg2.get(), arithmetic_expression);
		return make_arith(value_ ^ val);
	}

	if (o == "EQ")
	{
		copy_return_on_error_binary(arg2.get(), logic_expression);
		return make_logic(value_ == val);
	}

	if (o == "LE")
	{
		copy_return_on_error_binary(arg2.get(), logic_expression);
		return make_logic(value_ <= val);
	}

	if (o == "LT")
	{
		copy_return_on_error_binary(arg2.get(), logic_expression);
		return make_logic(value_ < val);
	}

	if (o == "GE")
	{
		copy_return_on_error_binary(arg2.get(), logic_expression);
		return make_logic(value_ >= val);
	}

	if (o == "GT")
	{
		copy_return_on_error_binary(arg2.get(), logic_expression);
		return make_logic(value_ > val);
	}

	if (o == "NE")
	{
		copy_return_on_error_binary(arg2.get(), logic_expression);
		return make_logic(value_ != val);
	}

	return default_expr_with_error<arithmetic_expression>(error_messages::ea08());
}

expr_ptr arithmetic_expression::operator+(expression_ref e) const
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

expr_ptr arithmetic_expression::operator-(expression_ref e) const
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

expr_ptr arithmetic_expression::operator*(expression_ref e) const
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

expr_ptr arithmetic_expression::operator/(expression_ref e) const
{
	copy_return_on_error_binary(e, arithmetic_expression);

	auto w = al_wrap(e);
	if (!w)
		return default_expr_with_error<arithmetic_expression>(error_messages::ea09());
	auto value = w.value();

	if (value == 0)
		return make_arith(0);

	return make_arith(value_ / value);
}

expr_ptr arithmetic_expression::operator+() const
{
	copy_return_on_error(this, arithmetic_expression);

	return make_arith(value_);
}

expr_ptr arithmetic_expression::operator-() const
{
	copy_return_on_error(this, arithmetic_expression);

	if (value_ == INT32_MIN)
		return default_expr_with_error<arithmetic_expression>
		(error_messages::ea10());

	return make_arith(-value_);
}

int32_t arithmetic_expression::get_numeric_value() const
{
	return value_;
}

expr_ptr arithmetic_expression::unary_operation(str_ref o) const
{
	std::string operation_name = o;
	std::transform(operation_name.begin(), operation_name.end(), operation_name.begin(), [](char c) { return static_cast<char>(toupper(c)); });
	if (operation_name == "NOT")
	{
		copy_return_on_error(this, arithmetic_expression);
		return make_arith(value_ ^ static_cast<uint32_t>(-1));
	}

	if (operation_name == "A2B")
	{
		copy_return_on_error(this, character_expression);
		return make_char(std::bitset<32>(get_value()).to_string());
	}

	if (operation_name == "A2C")
	{
		copy_return_on_error(this, character_expression);
		return make_char(character_expression::num_to_ebcdic(get_value()));
	}

	if (operation_name == "A2D")
	{
		copy_return_on_error(this, character_expression);
		auto val = std::to_string(get_value());
		if (val[0] == '-')
			return make_char(std::move(val));
		else
			return make_char("+" + val);
	}

	if (operation_name == "A2X")
	{
		copy_return_on_error(this, character_expression);
		return make_char(character_expression::num_to_hex(value_));
	}

	if (operation_name == "BYTE")
	{
		copy_return_on_error(this, character_expression);
		if (value_ > 255 || value_ < 0)
			return default_expr_with_error<character_expression>(error_messages::ea11());
		return make_char(ebcdic_encoding::to_ascii(static_cast<unsigned char>(value_)));
	}

	if (operation_name == "SIGNED")
	{
		copy_return_on_error(this, character_expression);
		return make_char(std::to_string(get_value()));
	}

	return default_expr_with_error<arithmetic_expression>
		(error_messages::ea08());
}
