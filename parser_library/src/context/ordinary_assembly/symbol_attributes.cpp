#include <stdexcept>
#include <cctype>

#include "symbol_attributes.h"
#include "../../ebcdic_encoding.h"

using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library;

const symbol_attributes::type_attr symbol_attributes::undef_type = ebcdic_encoding::to_ebcdic('U');

const symbol_attributes::len_attr symbol_attributes::undef_length = static_cast<len_attr>(-1);

const symbol_attributes::scale_attr symbol_attributes::undef_scale = static_cast<scale_attr>(-1);

symbol_attributes symbol_attributes::make_section_attrs() { return symbol_attributes(ebcdic_encoding::to_ebcdic('J'), 1); }
symbol_attributes symbol_attributes::make_machine_attrs(symbol_attributes::len_attr length) { return symbol_attributes(ebcdic_encoding::to_ebcdic('I'), length); }

data_attr_kind symbol_attributes::transform_attr(char c)
{
	c = (char)std::toupper(c);
	switch (c)
	{
	case 'D':
		return data_attr_kind::D;
	case 'O':
		return data_attr_kind::O;
	case 'N':
		return data_attr_kind::N;
	case 'S':
		return data_attr_kind::S;
	case 'K':
		return data_attr_kind::K;
	case 'I':
		return data_attr_kind::I;
	case 'L':
		return data_attr_kind::L;
	case 'T':
		return data_attr_kind::T;
	default:
		return data_attr_kind::UNKNOWN;
	}
}

bool hlasm_plugin::parser_library::context::symbol_attributes::needs_ordinary(data_attr_kind attribute)
{
	return attribute == data_attr_kind::D;
}

symbol_attributes::symbol_attributes()
	: type_(undef_type), length_(undef_length), scale_(undef_scale), undefined_(true) {}

symbol_attributes::symbol_attributes(type_attr type, len_attr length, scale_attr scale)
	: type_(type), length_(length), scale_(scale), undefined_(false) {}

void symbol_attributes::set_attributes(const symbol_attributes& attrs)
{
	if (!undefined_)
		throw std::runtime_error("value can be assigned only once");

	undefined_ = false;
	type_ = attrs.type_;
	length_ = attrs.length_;
	scale_ = attrs.scale_;
}

symbol_attributes::type_attr symbol_attributes::type() const { return type_; }

symbol_attributes::len_attr symbol_attributes::length() const { return length_; }

void symbol_attributes::length(len_attr new_length)
{
	length_ == undef_length ? length_ = new_length : throw std::runtime_error("value can be assigned only once");
}

symbol_attributes::scale_attr symbol_attributes::scale() const { return scale_; }

bool symbol_attributes::is_defined(data_attr_kind attribute) const
{
	switch (attribute)
	{
	case hlasm_plugin::parser_library::context::data_attr_kind::T:
		return type_ == undef_type;
	case hlasm_plugin::parser_library::context::data_attr_kind::L:
		return length_ == undef_length;
	case hlasm_plugin::parser_library::context::data_attr_kind::S:
	case hlasm_plugin::parser_library::context::data_attr_kind::I:
		return scale_ == undef_scale;
	default:
		return false;
	}
}

symbol_attributes::value_t symbol_attributes::get_attribute_value(data_attr_kind attribute) const
{
	switch (attribute)
	{
	case hlasm_plugin::parser_library::context::data_attr_kind::T:
		return type_ == undef_type ? 0 : type_;
	case hlasm_plugin::parser_library::context::data_attr_kind::L:
		return length_ == undef_length ? 0 : length_;
	case hlasm_plugin::parser_library::context::data_attr_kind::S:
	case hlasm_plugin::parser_library::context::data_attr_kind::I:
		//TODO
	default:
		return 0;
	}
}
