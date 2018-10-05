#include "macro.h"

using namespace hlasm_plugin::parser_library::context;
using namespace antlr4;
using namespace std;

const std::unordered_map<id_index, macro_param_ptr>& hlasm_plugin::parser_library::context::macro_definition::named_params() const
{
	return named_params_;
}

macro_definition::macro_definition(id_index name, id_index label_param_name, vector<macro_arg> params, ParserRuleContext * derivation_tree) 
	: id(name),label_param_name_(label_param_name), derivation_tree(derivation_tree)
{
	if (label_param_name_)
	{
		auto tmp(std::make_shared<positional_param>(label_param_name, 0));
		named_params_.emplace(label_param_name, tmp);
		positional_params_.push_back(std::move(tmp));
	}
	size_t idx = 1;

	for (auto it = params.begin(); it != params.end(); ++it)
	{
		if (it->data)
		{
			//TODO errors of multiplied symbolic params
			if (!it->id)
				throw std::invalid_argument("keyword parameter without name used");

			named_params_.emplace(it->id, std::make_shared<keyword_param>(it->id, move(it->data)));
		}
		else 
		{
			if (it->id)
			{
				std::shared_ptr<positional_param> tmp(std::make_shared<positional_param>(it->id, idx));
				named_params_.insert({ it->id,tmp });
				positional_params_.push_back(std::move(tmp));
			}
			++idx;
		}
	}
}

macro_invo_ptr macro_definition::call(macro_data_ptr label_param_data, vector<macro_arg> actual_params) const
{
	std::unordered_map<id_index,macro_param_ptr> named_cpy;
	for (auto&& field : named_params_)
	{
		if (field.second->param_type() == macro_param_type::POS_PAR_KIND)
			named_cpy.emplace(field.first, std::make_shared<positional_param>(*field.second->access_positional_param()));
		else
			named_cpy.emplace(field.first, std::make_shared<keyword_param>(*field.second->access_keyword_param()));
	}

	std::vector<macro_data_shared_ptr> syslist;

	if (label_param_data)
		syslist.push_back(std::move(label_param_data));
	else
		syslist.push_back(macro_param_data_component::dummy);

	for (auto&& param : actual_params)
	{
		if (param.id)
		{
			auto tmp = named_cpy.find(param.id);
			if (tmp == named_cpy.end() || tmp->second->param_type() == macro_param_type::POS_PAR_KIND)
			{
				//ERROR no keyword param matched
				//TODO when NOMACROCASE, case sensitivity of not found keyword is retained
				syslist.push_back(std::make_shared<macro_param_data_single>(*param.id + "=" + param.data->get_value()));
			}
			else
			{
				if (tmp->second->data)
				{
					//ERROR keyword already specified
				}
				tmp->second->data = std::move(param.data);
			}
		}
		else
		{
			syslist.push_back(move(param.data));
		}
	}

	for (auto&& pos_par : positional_params_)
	{
		if (pos_par->position < syslist.size())
			named_cpy.find(pos_par->id)->second->data = syslist[pos_par->position];
	}

	return std::make_shared<macro_invocation>(this->id, this->derivation_tree, std::move(named_cpy), std::move(syslist));
}

bool macro_definition::operator=(const macro_definition& m) { return id == m.id; }

macro_invocation::macro_invocation(id_index name, const antlr4::ParserRuleContext * derivation_tree, std::unordered_map<id_index, macro_param_ptr> named_params, std::vector<macro_data_shared_ptr> syslist)
	:id(name), derivation_tree(derivation_tree),named_params(std::move(named_params)),syslist_(std::move(syslist))
{
}

const C_t & hlasm_plugin::parser_library::context::macro_invocation::SYSLIST(size_t idx) const
{
	if (idx < syslist_.size())
		return syslist_[idx]->get_value();
	else
		return object_traits<C_t>::default_v();
}

const C_t & hlasm_plugin::parser_library::context::macro_invocation::SYSLIST(const std::vector<size_t>& offset) const
{
	auto it = offset.begin();

	if (it == offset.end())
		return object_traits<C_t>::default_v();

	const macro_param_data_component* param;

	if (*it < syslist_.size())
		param = &*syslist_[*it];
	else
		return object_traits<C_t>::default_v();

	++it;

	for (; it != offset.end(); ++it)
		param = param->get_ith(*it);

	return param->get_value();
}
