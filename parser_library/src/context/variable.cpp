#include "variable.h"

using namespace std;

namespace hlasm_plugin::parser_library::context
{


variable_kind variable_symbol::var_kind() const
{
	return variable_kind::UNDEF_VAR_KIND;
}

set_symbol_base * variable_symbol::access_set_symbol_base()
{
	return dynamic_cast<set_symbol_base*>(this);
}

macro_param_base * variable_symbol::access_macro_param_base()
{
	return dynamic_cast<macro_param_base*>(this);
}

variable_symbol::variable_symbol(id_index name) :id(name) {}

variable_symbol::~variable_symbol() {}

SET_t_enum set_symbol_base::type() const
{
	return SET_t_enum::UNDEF_TYPE;
}

variable_kind set_symbol_base::var_kind() const
{
	return variable_kind::SET_VAR_KIND;
}

set_symbol_base::set_symbol_base(id_index name, bool is_scalar) : variable_symbol(name), is_scalar(is_scalar) {}


variable_kind macro_param_base::var_kind() const
{
	return variable_kind::MACRO_VAR_KIND;
}

macro_param_type macro_param_base::param_type() const
{
	return macro_param_type::UNDEF_PAR_TYPE;
}

keyword_param * macro_param_base::access_keyword_param()
{
	return dynamic_cast<keyword_param*>(this);
}

positional_param * macro_param_base::access_positional_param()
{
	return dynamic_cast<positional_param*>(this);
}

macro_param_base::macro_param_base(id_index name) :variable_symbol(name) {}

macro_param_type keyword_param::param_type() const
{
	return macro_param_type::KEY_PAR_TYPE;
}

const C_t & keyword_param::get_value(const std::vector<size_t>& offset) const
{
	const macro_param_data_component* tmp = data ? data.get() : default_data_.get();

	for (auto idx : offset)
	{
		tmp = tmp->get_ith(idx);
	}
	return tmp->get_value();
}

const C_t & keyword_param::get_value(size_t idx) const
{
	return (data ? data.get() : default_data_.get())->get_ith(idx)->get_value();
}

const C_t & keyword_param::get_value() const
{
	return (data ? data.get() : default_data_.get())->get_value();
}

keyword_param::keyword_param(id_index name, macro_data_ptr default_value) : macro_param_base(name), default_data_(std::move(default_value)) {}

macro_param_type positional_param::param_type() const
{
	return macro_param_type::POS_PAR_TYPE;
}

const C_t & positional_param::get_value(const std::vector<size_t>& offset) const
{
	const macro_param_data_component* tmp = (data ? data.get() : &*macro_param_data_component::dummy);

	for (auto idx : offset)
	{
		tmp = tmp->get_ith(idx);
	}
	return tmp->get_value();
}

const C_t & positional_param::get_value(size_t idx) const
{
	return (data ? data : macro_param_data_component::dummy)->get_ith(idx)->get_value();
}

const C_t & positional_param::get_value() const
{
	return (data ? data->get_value() : object_traits<C_t>::default_v());
}

positional_param::positional_param(id_index name, size_t position) : macro_param_base(name), position(position) {}

const macro_sequence_symbol* sequence_symbol::access_macro_symbol() const
{
	return kind == sequence_symbol_kind::MACRO ? static_cast<const macro_sequence_symbol*>(this) : nullptr;
}

const opencode_sequence_symbol* sequence_symbol::access_opencode_symbol() const
{
	return kind == sequence_symbol_kind::OPENCODE ? static_cast<const opencode_sequence_symbol*>(this) : nullptr;
}

sequence_symbol::sequence_symbol(id_index name, const sequence_symbol_kind kind, location symbol_location)
	: name(name), symbol_location(std::move(symbol_location)), kind(kind) {}

opencode_sequence_symbol::opencode_sequence_symbol(id_index name, location loc, opencode_position statement_position,std::vector<copy_frame> copy_stack)
	: sequence_symbol(name, sequence_symbol_kind::OPENCODE, std::move(loc)), statement_position(statement_position), copy_stack(std::move(copy_stack)) {}

bool opencode_sequence_symbol::operator==(const opencode_sequence_symbol& oth) const
{
	if (!(statement_position == oth.statement_position && copy_stack.size() == oth.copy_stack.size()))
		return false;

	for (size_t i = 0; i < copy_stack.size(); i++)
	{
		if (!(copy_stack[i] == oth.copy_stack[i]))
			return false;
	}
	
	return true;
}

macro_sequence_symbol::macro_sequence_symbol(id_index name, location loc, size_t statement_offset)
	: sequence_symbol(name, sequence_symbol_kind::MACRO, std::move(loc)), statement_offset(statement_offset) {}


}
