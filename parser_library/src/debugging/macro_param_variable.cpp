#include <cstddef>
#include <stdexcept>

#include "macro_param_variable.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::debugging;

macro_param_variable::macro_param_variable(const context::macro_param_base& param, std::vector<size_t> index)
	: macro_param_(param), index_(std::move(index))
{
	if (!index_.empty())
		name_.emplace(std::to_string(index_.back()));
}

const std::string& macro_param_variable::get_string_value() const
{
	return macro_param_.get_value(index_);
};

set_type macro_param_variable::type() const
{
	return set_type::C_TYPE;
}

const std::string& macro_param_variable::get_string_name() const
{
	return *macro_param_.id;
}

bool macro_param_variable::is_scalar() const
{
	return macro_param_.size(index_) == 0;
}

std::vector<variable_ptr> macro_param_variable::values() const
{
	std::vector<std::unique_ptr<variable>> vals;

	std::vector<size_t> child_index = index_;
	child_index.push_back(0);

	if (macro_param_.access_syslist_param() && child_index.size() == 1)
	{
		for (size_t i = 0; i < size(); ++i)
		{
			child_index.back() = i;
			vals.push_back(std::make_unique<macro_param_variable>(macro_param_, child_index));
		}
	}
	else
	{
		for (int i = 1; i <= size(); ++i)
		{
			child_index.back() = i;
			vals.push_back(std::make_unique<macro_param_variable>(macro_param_, child_index));
		}
	}
	return vals;
	
}

size_t macro_param_variable::size() const
{
	return macro_param_.size(index_);
}
