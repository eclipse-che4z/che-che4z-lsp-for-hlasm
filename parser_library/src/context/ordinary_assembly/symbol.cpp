#include "symbol.h"

#include <stdexcept>
#include <assert.h>




using namespace hlasm_plugin::parser_library::context;

symbol_value::symbol_value(abs_value_t value)
	: value_(value) {}

symbol_value::symbol_value(reloc_value_t value)
	:  value_(value) {}

symbol_value::symbol_value() {}

symbol_value symbol_value::operator+(const symbol_value& value) const
{
	if (value_kind() == symbol_kind::UNDEF || value.value_kind() == symbol_kind::UNDEF)
		return symbol_value();

	if (value_kind() == symbol_kind::ABS)
	{
		if (value.value_kind() == symbol_kind::ABS)
			return get_abs() + value.get_abs();
		else
			return value.get_reloc() + get_abs();
	}
	else if(value_kind() == symbol_kind::RELOC)
	{
		if (value.value_kind() == symbol_kind::ABS)
			return get_reloc() + value.get_abs();
		else
		{
			auto tmp_val(get_reloc() + value.get_reloc());
			if (tmp_val.bases.empty() && tmp_val.spaces.empty())
				return tmp_val.offset;
			else 
				return tmp_val;
		}
	}
	else
	{
		assert(false);
		return symbol_value();
	}
}

symbol_value symbol_value::operator-(const symbol_value& value) const
{
	if (value_kind() == symbol_kind::UNDEF || value.value_kind() == symbol_kind::UNDEF)
		return symbol_value();

	if (value_kind() == symbol_kind::ABS)
	{
		if (value.value_kind() == symbol_kind::ABS)
			return get_abs() - value.get_abs();
		else
			return value.get_reloc() - get_abs();
	}
	else if (value_kind() == symbol_kind::RELOC)
	{
		if (value.value_kind() == symbol_kind::ABS)
			return get_reloc() - value.get_abs();
		else
		{
			auto tmp_val(get_reloc() - value.get_reloc());
			if (tmp_val.bases.empty() && tmp_val.spaces.empty())
				return tmp_val.offset;
			else
				return tmp_val;
		}
	}
	else
	{
		assert(false);
		return symbol_value();
	}
}

symbol_value symbol_value::operator*(const symbol_value& value) const
{
	if (value_kind() == symbol_kind::ABS && value.value_kind() == symbol_kind::ABS)
		return get_abs() * value.get_abs();

	return symbol_value();
}

symbol_value symbol_value::operator/(const symbol_value& value) const
{
	if (value_kind() == symbol_kind::ABS && value.value_kind() == symbol_kind::ABS)
		return value.get_abs() == 0 ? 0 : get_abs() / value.get_abs();

	return symbol_value();
}

symbol_value symbol_value::operator-() const
{
	switch (value_kind())
	{
	case symbol_kind::ABS:
		return -get_abs();
	case symbol_kind::RELOC:
		return -get_reloc();
	case symbol_kind::UNDEF:
		return symbol_value();
	default:
		assert(false);
		return symbol_value();
	}
}

symbol_value& symbol_value::operator=(const symbol_value& value)
{
	if (value_kind() != symbol_kind::UNDEF)
		throw std::runtime_error("assigning to defined symbol");
	
	value_ = value.value_;
	return *this;
}

const symbol_value::abs_value_t& symbol_value::get_abs() const { return std::get<abs_value_t>(value_); }

const symbol_value::reloc_value_t& symbol_value::get_reloc() const { return std::get<reloc_value_t>(value_); }

symbol_kind symbol_value::value_kind() const
{
	return static_cast<symbol_kind>(value_.index());
}

symbol::symbol(id_index name, symbol_value value, symbol_attributes attributes)
	: name(name), value_(std::move(value)), attributes_(attributes) {}

symbol::symbol() 
	: name(id_storage::empty_id) {}

const symbol_value& symbol::value() const { return value_; }

const symbol_attributes& symbol::attributes() const
{
	return attributes_;
}

symbol_kind symbol::kind() const
{
	return value_.value_kind();
}

void symbol::set_value(symbol_value value)
{
	if (value.value_kind() == symbol_kind::UNDEF)
		throw std::runtime_error("can not assign undefined value");

	if(kind() != symbol_kind::UNDEF)
		throw std::runtime_error("can not assign value to already defined symbol");

	value_ = value;
}
