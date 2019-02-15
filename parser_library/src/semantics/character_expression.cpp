#include "character_expression.h"
#include "../ebcdic_encoding.h"
#include <algorithm>
#include <bitset>
#include <locale>
#include "arithmetic_expression.h"
#include "../error_messages.h"

using namespace hlasm_plugin;
using namespace parser_library;
using namespace semantics;

std::string hlasm_plugin::parser_library::semantics::character_expression::get_str_val() const { return value_; }

char_ptr character_expression::append(const char_ptr& arg) const
{
	copy_return_on_error_binary(arg.get(), character_expression);
	return make_char(value_ + arg->value_);
}

char_ptr hlasm_plugin::parser_library::semantics::character_expression::append(const character_expression * arg) const
{
	copy_return_on_error_binary(arg, character_expression);
	return make_char(value_ + arg->value_);
}

const std::string & hlasm_plugin::parser_library::semantics::character_expression::get_value() const
{
	return value_;
}

hlasm_plugin::parser_library::semantics::character_expression::character_expression(std::string val)
	: value_(std::move(val))
{
}

void hlasm_plugin::parser_library::semantics::character_expression::append(std::string v)
{
	value_.append(v);
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::binary_operation(str_ref operation, expr_ref arg2) const
{
	std::string o = operation;
	std::transform(o.begin(), o.end(), o.begin(), [](char c) { return static_cast<char>(toupper(c)); });
	auto a2 = arg2->retype<character_expression>();
	if (a2 == nullptr)
		return default_expr_with_error<character_expression>(error_messages::ec03());
	auto& b = a2->value_;

	if (o == "EQ")
	{
		copy_return_on_error_binary(arg2.get(), logic_expression);
		return make_logic(value_ == b);
	}

	if (o == "NE")
	{
		copy_return_on_error_binary(arg2.get(), logic_expression);
		return make_logic(value_ != b);
	}

	if (o == "LE")
	{
		copy_return_on_error_binary(arg2.get(), logic_expression);
		return make_logic(value_ <= b);
	}

	if (o == "LT")
	{
		copy_return_on_error_binary(arg2.get(), logic_expression);
		return make_logic(value_ < b);
	}

	if (o == "GT")
	{
		copy_return_on_error_binary(arg2.get(), logic_expression);
		return make_logic(value_ > b);
	}

	if (o == "GE")
	{
		copy_return_on_error_binary(arg2.get(), logic_expression);
		return make_logic(value_ >= b);
	}

	if (value_.empty() || b.empty())
		return default_expr_with_error<character_expression>(error_messages::ec04());

	copy_return_on_error_binary(arg2.get(), arithmetic_expression);

	if (o == "FIND")
		return make_arith(static_cast<int32_t>(value_.find_first_of(b)) + 1);

	if (o == "INDEX")
	{
		auto i = value_.find(b);
		if (i == std::string::npos)
			return make_arith(0);
		else
			return make_arith(static_cast<int32_t>(i) + 1);
	}

	return default_expr_with_error<character_expression>(error_messages::ec05());
}

bool character_expression::isalpha_hlasm(char c)
{
	return (isalpha(c) || c == '$' || c == '_' || c == '#' || c == '@');
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::unary_operation(str_ref operation) const
{
	std::string o = operation;
	std::transform(o.begin(), o.end(), o.begin(), [](char c) { return static_cast<char>(toupper(c)); });

	if (o == "B2A")
	{
		copy_return_on_error(this, arithmetic_expression);
		if (value_.empty())
			return make_arith(0);
		return arithmetic_expression::from_string(value_, 2);
	}

	if (o == "C2A")
	{
		copy_return_on_error(this, arithmetic_expression);
		if (value_.empty())
			return make_arith(0);
		return arithmetic_expression::c2arith(value_);
	}

	if (o == "D2A")
	{
		copy_return_on_error(this, arithmetic_expression);
		if (value_.empty())
			return default_expr_with_error<arithmetic_expression>(error_messages::ec04());
		return arithmetic_expression::from_string(value_, 10);
	}

	if (o == "DCLEN")
		return dclen();

	if (o == "ISBIN")
		return isbin();

	if (o == "ISDEC")
		return isdec();

	if (o == "ISHEX")
		return ishex();

	if (o == "ISSYM")
		return issym();

	if (o == "X2A")
	{
		copy_return_on_error(this, arithmetic_expression);
		return arithmetic_expression::from_string(value_, 16);
	}

	if (o == "B2C")
		return b2c();

	if (o == "B2D")
		return b2d();

	if (o == "B2X")
		return b2x();

	if (o == "C2B")
		return c2b();

	if (o == "C2D")
		return c2d();

	if (o == "C2X")
		return c2x();

	if (o == "D2B")
		return d2b();

	if (o == "D2C")
		return d2c();

	if (o == "D2X")
		return d2x();

	if (o == "DCVAL")
		return dcval();

	if (o == "DEQUOTE")
		return dequote();

	if (o == "DOUBLE")
		return double_quote();

	if (o == "ESYM")
	{
		/*TODO*/
		return default_expr_with_error<logic_expression>
			(error_messages::not_implemented());
	}

	if (o == "LOWER")
	{
		copy_return_on_error(this, character_expression);
		std::string rv = value_;
		std::transform(rv.begin(), rv.end(), rv.begin(), [](char c) { return static_cast<char>(tolower(c)); });
		return make_char(std::move(rv));
	}

	if (o == "SYSATTRA")
	{
		/*TODO*/
		return default_expr_with_error<logic_expression>
			(error_messages::not_implemented());
	}

	if (o == "SYSATTRP")
	{
		/*TODO*/
		return default_expr_with_error<logic_expression>
			(error_messages::not_implemented());
	}

	if (o == "UPPER")
	{
		copy_return_on_error(this, character_expression);
		std::string rv = value_;
		std::transform(rv.begin(), rv.end(), rv.begin(), [](char c) { return static_cast<char>(toupper(c)); });
		return make_char(std::move(rv));
	}

	if (o == "X2B")
		return x2b();

	if (o == "X2C")
		return x2c();

	if (o == "X2D")
		return x2d();

	return default_expr_with_error<character_expression>(error_messages::ec05());
}

expr_ptr character_expression::dclen() const {
	copy_return_on_error(this, arithmetic_expression);
	int32_t v = 0;
	for (auto i = this->value_.cbegin(); i != this->value_.cend(); ++i)
	{
		//escaping double apostrophe '' and double ampersand &&
		if ((*i == '\'' && i + 1 != this->value_.end() && *(i + 1) == '\'')|| (*i == '&' && i + 1 != this->value_.end() && *(i + 1) == '&'))
			++i;
		++v;
	}
	return make_arith(v);
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::isbin() const
{
	copy_return_on_error(this, logic_expression);
	if (value_.empty())
		return default_expr_with_error<logic_expression>
		(error_messages::ec04());

	return make_logic(!value_.empty()
		&& value_.size() < 33
		&& std::all_of(value_.cbegin(), value_.cend(), [](char c) {return c == '0' || c == '1'; }));
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::isdec() const
{
	copy_return_on_error(this, logic_expression);
	size_t t = 0;
	if (value_.empty())
		return default_expr_with_error<logic_expression>
		(error_messages::ec04());
	try
	{
		std::stoi(value_, &t);
	}
	catch (...)
	{
		t = 0;
	}
	return make_logic(t == value_.length() && value_.length() <= 10 && isdigit(value_[0]));
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::ishex() const
{
	copy_return_on_error(this, logic_expression);
	size_t t = 0;
	if (value_.empty())
		return default_expr_with_error<logic_expression>
		(error_messages::ec04());

	return make_logic(
		t == value_.length()
		&& value_.length() <= 8
		&& std::all_of(value_.cbegin(), value_.cend(),
			[](char c) {return isdigit(c) || (toupper(c) >= 'A' && toupper(c) <= 'F'); }));
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::issym() const
{
	copy_return_on_error(this, logic_expression);
	if (value_.empty())
		return default_expr_with_error<logic_expression>(error_messages::ec04());

	return make_logic(
		value_.length() < 64
		&& isalpha_hlasm(value_[0])
		&& std::all_of(value_.cbegin(), value_.cend(),
			[](char c) {return isdigit(c) || isalpha_hlasm(c); })
	);
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::b2c() const
{
	copy_return_on_error(this, character_expression);
	if (value_.empty())
		return make_char("");

	int32_t offset = value_.length() % 8 > 0 ? -8 + value_.length() % 8 : 0;
	std::string res;
	for (int32_t i = offset; i < static_cast<int32_t>(value_.length());)
	{
		unsigned char val = 0;
		auto e = i + 8;
		for (; i < e; ++i)
		{
			unsigned char num = i >= 0 ? value_[i] - '0' : 0;
			if (num > 1 || num < 0)
				return default_expr_with_error<character_expression>
				(error_messages::ec06());
			val = (val << 1) + num;
		}
		res.append(ebcdic_encoding::to_ascii(val));
	}

	return make_char(std::move(res));
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::b2d() const
{
	copy_return_on_error(this, character_expression);
	if (value_.empty())
		return make_char("+0");

	auto val = arithmetic_expression::from_string(value_, 2);

	auto rv = std::to_string(val->get_numeric_value());
	if (rv[0] == '-')
		return make_char(std::move(rv));
	else
		return make_char("+" + rv);
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::b2x() const
{
	copy_return_on_error(this, character_expression);
	if (value_.empty())
		return make_char("");

	if (!std::all_of(value_.cbegin(), value_.cend(), [](char c) {return c == '1' || c == '0'; }))
		return default_expr_with_error<character_expression>
		(error_messages::ec06());

	auto inp = value_;
	if (inp.length() % 4 > 0)
		inp = std::string(4 - inp.length() % 4, '0') + inp;

	std::string v;
	for (size_t i = 0; i < inp.length(); i += 4)
	{
		int32_t off = ((inp[i] - '0') << 3)
			+ ((inp[i + 1] - '0') << 2)
			+ ((inp[i + 2] - '0') << 1)
			+ (inp[i + 3] - '0');

		v.push_back(num_to_hex_char(off));
	}
	return make_char(std::move(v));
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::c2b() const
{
	copy_return_on_error(this, character_expression);
	if (value_.empty())
		return make_char("");

	std::string rv;
	rv.reserve(32);
	for (const char *j = value_.c_str(); *j != 0; ++j)
	{
		int32_t v = ebcdic_encoding::to_ebcdic(ebcdic_encoding::to_pseudoascii(j));
		for (size_t i = 0; i < 8; i++)
		{
			rv.push_back('0' + ((v >> (7 - i)) & 1));
		}
	}
	return make_char(std::move(rv));
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::c2d() const
{
	copy_return_on_error(this, character_expression);
	if (value_.empty())
		return make_char("+0");
	int32_t val = 0;
	for (const char *j = value_.c_str(); *j != 0; ++j)
		val = (val << 8) + ebcdic_encoding::to_ebcdic(ebcdic_encoding::to_pseudoascii(j));
	std::string rv = std::to_string(val);
	if (rv[0] == '-')
		return make_char(std::move(rv));
	else
		return make_char("+" + rv);
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::c2x() const
{
	copy_return_on_error(this, character_expression);
	if (value_.empty())
		return make_char("");

	std::string rv;
	rv.reserve(value_.size() * 2);
	for (const char *j = value_.c_str(); *j != 0; ++j)
	{
		auto v = ebcdic_encoding::to_ebcdic(ebcdic_encoding::to_pseudoascii(j));
		rv.push_back(num_to_hex_char(v >> 4));
		rv.push_back(num_to_hex_char(v & 15));
	}
	return make_char(std::move(rv));
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::d2b() const
{
	copy_return_on_error(this, character_expression);
	size_t t = 0;
	if (value_.empty())
		return make_char("");

	int32_t val = 0;
	try {
		val = std::stoi(value_, &t, 10);
	}
	catch (...) { t = 0; }

	if (t != value_.length())
		return default_expr_with_error<character_expression>
		(error_messages::ec06());

	return make_char(std::bitset<32>(val).to_string());
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::d2c() const
{
	copy_return_on_error(this, character_expression);
	size_t t = 0;
	if (value_.empty())
		return default_expr_with_error<character_expression>
		(error_messages::ec04());

	try {
		int32_t val = std::stoi(value_, &t, 10);
		if (t != value_.length())
			return default_expr_with_error<character_expression>
			(error_messages::ec06());
		return make_char(num_to_ebcdic(val));
	}
	catch (...) {
		return default_expr_with_error<character_expression>
			(error_messages::ec08());
	}
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::d2x() const
{
	copy_return_on_error(this, character_expression);
	size_t t = 0;
	if (value_.empty())
		return default_expr_with_error<character_expression>
		(error_messages::ec04());

	try {
		int32_t val = std::stoi(value_, &t, 10);
		if (t != value_.length())
			return default_expr_with_error<character_expression>
			(error_messages::ec07());

		return make_char(num_to_hex(val));
	}
	catch (...) {
		return default_expr_with_error<character_expression>
			(error_messages::ec08());
	}
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::dcval() const
{
	copy_return_on_error(this, character_expression);
	std::string v;
	v.reserve(value_.length());
	for (auto i = value_.cbegin(); i != value_.end(); ++i)
	{
		//escaping double apostrophe '' and double ampersand &&
		if ((*i == '\'' && i + 1 != this->value_.end() && *(i + 1) == '\'') || (*i == '&' && i + 1 != this->value_.end() && *(i + 1) == '&'))
			++i;
		v.push_back(*i);
	}
	return make_char(v);
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::dequote() const
{
	copy_return_on_error(this, character_expression);
	if (value_.empty())
		return make_char("");
	auto begin = value_.cbegin();
	auto end = value_.cend();
	if (value_[0] == '\'')
		begin = std::next(begin);
	if (value_[value_.length() - 1] == '\'')
		end = std::prev(end);
	if (begin <= end)
		return make_char(std::string(begin, end));
	else
		return make_char("");
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::double_quote() const
{
	copy_return_on_error(this, character_expression);
	std::string v;
	for (char c : value_)
	{
		v.push_back(c);
		if (c == '\'' || c == '&')
			v.push_back(c);
	}
	return make_char(std::move(v));
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::x2b() const
{
	copy_return_on_error(this, character_expression);
	if (value_.empty())
		return make_char("");
	size_t tt = 0;
	int32_t val = 0;
	try {
		val = std::stoul(value_.c_str(), &tt, 16);
	}
	catch (...) { tt = 0; }
	if (tt != value_.length())
		return default_expr_with_error<character_expression>
		(error_messages::ec09());

	auto rv = std::bitset<32>(val).to_string();
	return make_char(
		rv.substr(32 - value_.length() * 4, value_.length() * 4)
	);
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::x2c() const
{
	copy_return_on_error(this, character_expression);
	size_t t = 0;
	if (value_.empty())
		return make_char("");
	auto val = value_;
	std::string rv;
	auto it = val.cbegin();

	if (val.length() % 2 == 1)
		rv.append(ebcdic_encoding::to_ascii(hex_to_num(*(it++), &t)));

	for (; it != val.cend(); ++it)
	{
		char c = (char)hex_to_num(*it, &t);
		if (t == 0)
			break;

		c = (c << 4) + (char)hex_to_num(*(++it), &t);
		if (t == 0)
			break;

		rv.append(ebcdic_encoding::to_ascii(c));
	}

	if (t == 0)
		return default_expr_with_error<character_expression>
		(error_messages::ec09());

	return make_char(std::move(rv));
}

expr_ptr hlasm_plugin::parser_library::semantics::character_expression::x2d() const
{
	copy_return_on_error(this, character_expression);
	if (value_.empty())
		return make_char("+0");

	size_t tt = 0;
	int32_t val = 0;
	try {
		val = std::stoul(value_.c_str(), &tt, 16);
	}
	catch (...) { tt = 0; }
	if (tt != value_.length())
		return default_expr_with_error<character_expression>
		(error_messages::ec09());

	return make_char((val >= 0 ? "+" : "") + std::to_string(val));
}

std::string hlasm_plugin::parser_library::semantics::character_expression::num_to_ebcdic(int32_t val)
{
	std::string c;
	c.append(ebcdic_encoding::to_ascii((val >> 24) & 255));
	c.append(ebcdic_encoding::to_ascii((val >> 16) & 255));
	c.append(ebcdic_encoding::to_ascii((val >> 8) & 255));
	c.append(ebcdic_encoding::to_ascii((val >> 0) & 255));
	return c;
}

char hlasm_plugin::parser_library::semantics::character_expression::num_to_hex_char(int32_t val)
{
	if (val < 0 || val >= 16)
		throw std::runtime_error("value must be less than 16");
	return "0123456789ABCDEF"[val];
}

char hlasm_plugin::parser_library::semantics::character_expression::hex_to_num(char c, size_t* t)
{
	c = static_cast<char>(toupper(c));
	*t = 1;
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'A' && c <= 'F')
		return 10 + c - 'A';
	*t = 0;
	return 0;
}

std::string hlasm_plugin::parser_library::semantics::character_expression::num_to_hex(int32_t val)
{
	std::string v;
	for (size_t i = 0; i < 8; i++)
	{
		v.push_back(num_to_hex_char(15 & (val >> (28 - 4 * i))));
	}
	return v;
}
