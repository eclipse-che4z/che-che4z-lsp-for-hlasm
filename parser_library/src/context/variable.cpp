#include "variable.h"

using namespace std;

namespace hlasm_plugin::parser_library::context
{

set_symbol_base * variable_symbol::access_set_symbol_base()
{
	return dynamic_cast<set_symbol_base*>(this);
}

macro_param_base * variable_symbol::access_macro_param_base()
{
	return dynamic_cast<macro_param_base*>(this);
}

variable_symbol::variable_symbol(variable_kind var_kind, id_index name)
	:id(name), var_kind(var_kind) {}

set_symbol_base::set_symbol_base(id_index name, bool is_scalar, SET_t_enum type)
	: variable_symbol(variable_kind::SET_VAR_KIND, name), is_scalar(is_scalar),type(type) {}

const keyword_param * macro_param_base::access_keyword_param() const
{
	return (param_type == macro_param_type::KEY_PAR_TYPE) ? static_cast<const keyword_param*>(this) : nullptr;
}

const positional_param * macro_param_base::access_positional_param() const
{
	return (param_type == macro_param_type::POS_PAR_TYPE) ? static_cast<const positional_param*>(this) : nullptr;
}

const syslist_param* macro_param_base::access_syslist_param() const
{
	return (param_type == macro_param_type::SYSLIST_TYPE) ? static_cast<const syslist_param*>(this) : nullptr;
}

macro_param_base::macro_param_base(macro_param_type param_type, id_index name)
	:variable_symbol(variable_kind::MACRO_VAR_KIND, name), param_type(param_type) {}

const C_t & macro_param_base::get_value(std::vector<int> offset) const
{
	const macro_param_data_component* tmp = real_data();

	for (auto idx : offset)
	{
		tmp = tmp->get_ith(idx);
	}
	return tmp->get_value();
}

const C_t & macro_param_base::get_value(int idx) const
{
	return real_data()->get_ith(idx)->get_value();
}

const C_t & macro_param_base::get_value() const
{
	return real_data()->get_value();
}

A_t macro_param_base::number(std::vector<size_t> offset) const
{
	const macro_param_data_component* tmp = real_data();

	for (auto idx : offset)
	{
		tmp = tmp->get_ith(idx);
	}
	return (A_t)tmp->number;
}

A_t macro_param_base::count(std::vector<size_t> offset) const
{
	const macro_param_data_component* tmp = real_data();

	for (auto idx : offset)
	{
		tmp = tmp->get_ith(idx);
	}
	return (A_t)tmp->get_value().size();
}

keyword_param::keyword_param(id_index name, macro_data_shared_ptr default_value, macro_data_ptr assigned_value)
	: macro_param_base(macro_param_type::KEY_PAR_TYPE, name), assigned_data_(std::move(assigned_value)), default_data(std::move(default_value)) {}

const macro_param_data_component* keyword_param::real_data() const
{
	return assigned_data_ ? assigned_data_.get() : default_data.get();
}

positional_param::positional_param(id_index name, size_t position, const macro_param_data_component& assigned_value)
	: macro_param_base(macro_param_type::POS_PAR_TYPE, name), data_(assigned_value), position(position) {}

const macro_param_data_component* positional_param::real_data() const
{
	return &data_;
}

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


syslist_param::syslist_param(id_index name, macro_data_ptr value)
	: macro_param_base(macro_param_type::SYSLIST_TYPE, name), data_(std::move(value)) {}

const C_t& syslist_param::get_value(std::vector<int> offset) const
{
	if (!offset.empty()) ++offset.front();

	return macro_param_base::get_value(std::move(offset));
}

const C_t& syslist_param::get_value(int idx) const
{
	return macro_param_base::get_value(idx+1);
}

const C_t& syslist_param::get_value() const
{
	return macro_param_base::get_value(1);
}

A_t syslist_param::number(std::vector<size_t> offset) const
{
	if (offset.empty())
		return (A_t)data_->number - 1;
	else
		return (A_t)macro_param_base::number(std::move(offset));
}

A_t syslist_param::count(std::vector<size_t> offset) const
{
	if (offset.empty())
		return (A_t)data_->get_ith(1)->get_value().size();
	else
		return (A_t)macro_param_base::count(std::move(offset));
}

const macro_param_data_component* syslist_param::real_data() const
{
	return &*data_;
}

}
