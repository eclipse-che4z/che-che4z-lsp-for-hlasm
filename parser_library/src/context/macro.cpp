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

#include "macro.h"
#include "variables/system_variable.h"

#include <stdexcept>
#include <cassert>

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;



const std::unordered_map<id_index, const macro_param_base*>& macro_definition::named_params() const
{
	return named_params_;
}

macro_definition::macro_definition(
	id_index name, 
	id_index label_param_name, std::vector<macro_arg> params,
	statement_block definition, copy_nest_storage copy_nests, label_storage labels,
	location definition_location)
	: label_param_name_(label_param_name), 
	id(name), copy_nests(std::move(copy_nests)), labels(std::move(labels)), definition_location(std::move(definition_location))
{
	for (auto&& stmt : definition)
		cached_definition.emplace_back(std::move(stmt));

	if (label_param_name_)
	{
		auto tmp = std::make_unique<positional_param>(label_param_name, 0, *macro_param_data_component::dummy);
		named_params_.emplace(label_param_name, &*tmp);
		positional_params_.push_back(std::move(tmp));
	}
	else
	{
		positional_params_.push_back(nullptr);
	}
	size_t idx = 1;

	for (auto it = params.begin(); it != params.end(); ++it)
	{
		if (it->data)
		{
			if (!it->id)
				throw std::invalid_argument("keyword parameter without name used");
			
			auto tmp = std::make_unique<keyword_param>(it->id, move(it->data), nullptr);
			named_params_.emplace(it->id, &*tmp);
			keyword_params_.push_back(std::move(tmp));
		}
		else
		{
			if (it->id)
			{
				auto tmp = std::make_unique<positional_param>(it->id, idx, *macro_param_data_component::dummy);
				named_params_.emplace(it->id, &*tmp);
				positional_params_.push_back(std::move(tmp));
			}
			else
			{
				positional_params_.push_back(nullptr);
			}
			++idx;
		}
	}
}

macro_invo_ptr macro_definition::call(macro_data_ptr label_param_data, std::vector<macro_arg> actual_params, id_index syslist_name)
{
	std::vector<macro_data_ptr> syslist;
	std::unordered_map<id_index, macro_param_ptr> named_cpy;

	if (label_param_data)
		syslist.push_back(std::move(label_param_data));
	else
		syslist.push_back(std::make_unique<macro_param_data_dummy>());

	if (positional_params_[0])
	{
		named_cpy.emplace(positional_params_[0]->id,
			std::make_unique<positional_param>(positional_params_[0]->id, 0, *syslist.back()));
	}

	for (auto&& param : actual_params)
	{
		if (param.id)
		{
			auto tmp = named_params_.find(param.id);

			if(tmp == named_params_.end() || tmp->second->param_type == macro_param_type::POS_PAR_TYPE)
				throw std::invalid_argument("use of undefined keyword parameter");

			auto key_par = dynamic_cast<const keyword_param*>(tmp->second);
			assert(key_par);
			named_cpy.emplace(param.id, std::make_unique<keyword_param>(param.id, key_par->default_data, std::move(param.data)));
		}
		else
		{
			if (positional_params_.size() > syslist.size() && positional_params_[syslist.size()])
			{
				named_cpy.emplace(positional_params_[syslist.size()]->id,
					std::make_unique<positional_param>(positional_params_[syslist.size()]->id, positional_params_[syslist.size()]->position, *param.data));
			}
			syslist.push_back(move(param.data));
		}
	}

	for (size_t i = syslist.size(); i < positional_params_.size(); ++i)
	{
		if (positional_params_[i])
		{
			named_cpy.emplace(positional_params_[i]->id,
				std::make_unique<positional_param>(positional_params_[i]->id, positional_params_[i]->position, *macro_param_data_component::dummy));
		}
	}
	for (auto&& key_par : keyword_params_)
	{
		if (named_cpy.find(key_par->id) == named_cpy.end())
		{
			named_cpy.emplace(key_par->id, std::make_unique<keyword_param>(key_par->id, key_par->default_data, nullptr));
		}
	}

	named_cpy.emplace(syslist_name, std::make_unique<system_variable>(syslist_name, std::make_unique<macro_param_data_composite>(std::move(syslist)), false));

	return std::make_shared<macro_invocation>(id, cached_definition, copy_nests, labels, std::move(named_cpy), definition_location);
}

bool macro_definition::operator=(const macro_definition& m) { return id == m.id; }

macro_invocation::macro_invocation(
	id_index name,
	cached_block& cached_definition, const copy_nest_storage& copy_nests, const label_storage& labels,
	std::unordered_map<id_index, macro_param_ptr> named_params, 
	const location& definition_location)
	: id(name), named_params(std::move(named_params)),
	cached_definition(cached_definition), copy_nests(copy_nests), labels(labels), definition_location(definition_location), current_statement(-1)
{
}

