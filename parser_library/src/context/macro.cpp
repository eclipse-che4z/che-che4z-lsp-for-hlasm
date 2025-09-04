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

#include <cassert>
#include <numeric>
#include <stdexcept>

#include "copy_member.h"
#include "variables/system_variable.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;



const std::unordered_map<id_index, const macro_param_base*>& macro_definition::named_params() const
{
    return named_params_;
}

macro_definition::macro_definition(id_index name,
    id_index label_param_name,
    std::vector<macro_arg> params,
    statement_block definition,
    copy_nest_storage copy_nests,
    macro_label_storage labels,
    location definition_location,
    std::unordered_set<copy_member_ptr> used_copy_members)
    : label_param_name_(label_param_name)
    , id(name)
    , copy_nests(std::move(copy_nests))
    , labels(std::move(labels))
    , definition_location(std::move(definition_location))
    , used_copy_members(std::move(used_copy_members))
{
    cached_definition.reserve(definition.size());
    for (auto&& stmt : definition)
        cached_definition.emplace_back(std::move(stmt));

    auto r = std::accumulate(params.begin(), params.end(), std::pair<size_t, size_t>(1, 0), [](auto a, const auto& e) {
        if (e.data)
            ++a.second;
        else
            ++a.first;
        return a;
    });
    positional_params_.reserve(r.first);
    keyword_params_.reserve(r.second);

    named_params_.emplace(label_param_name,
        positional_params_
            .emplace_back(std::make_unique<positional_param>(label_param_name, 0, *macro_param_data_component::dummy))
            .get());

    for (size_t idx = 1; auto& [p_id, p_data] : params)
    {
        if (p_data)
        {
            named_params_.emplace(p_id,
                keyword_params_.emplace_back(std::make_unique<keyword_param>(p_id, std::move(p_data), nullptr)).get());
        }
        else
        {
            if (!p_id.empty())
            {
                named_params_.emplace(p_id,
                    positional_params_
                        .emplace_back(std::make_unique<positional_param>(p_id, idx, *macro_param_data_component::dummy))
                        .get());
            }
            else
            {
                positional_params_.push_back(nullptr);
            }
            ++idx;
        }
    }
}

std::pair<std::unique_ptr<macro_invocation>, bool> macro_definition::call(
    macro_data_ptr label_param_data, std::vector<macro_arg> actual_params, id_index syslist_name)
{
    std::vector<macro_data_ptr> syslist;
    std::unordered_map<id_index, std::unique_ptr<macro_param_base>> named_cpy;

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
        if (!param.id.empty())
        {
            auto tmp = named_params_.find(param.id);

            if (tmp == named_params_.end() || tmp->second->param_type == macro_param_type::POS_PAR_TYPE)
                throw std::invalid_argument("use of undefined keyword parameter");

            const auto& key_par = *tmp->second->access_keyword_param();
            named_cpy.emplace(
                param.id, std::make_unique<keyword_param>(param.id, key_par.default_data, std::move(param.data)));
        }
        else
        {
            if (positional_params_.size() > syslist.size() && positional_params_[syslist.size()])
            {
                named_cpy.emplace(positional_params_[syslist.size()]->id,
                    std::make_unique<positional_param>(positional_params_[syslist.size()]->id,
                        positional_params_[syslist.size()]->position,
                        *param.data));
            }
            syslist.push_back(std::move(param.data));
        }
    }

    for (size_t i = syslist.size(); i < positional_params_.size(); ++i)
    {
        if (positional_params_[i])
        {
            named_cpy.emplace(positional_params_[i]->id,
                std::make_unique<positional_param>(
                    positional_params_[i]->id, positional_params_[i]->position, *macro_param_data_component::dummy));
        }
    }
    for (auto&& key_par : keyword_params_)
    {
        if (named_cpy.find(key_par->id) == named_cpy.end())
        {
            named_cpy.emplace(
                key_par->id, std::make_unique<keyword_param>(key_par->id, key_par->default_data, nullptr));
        }
    }

    bool truncated_syslist = syslist.size() - 1 > std::numeric_limits<A_t>::max();
    if (truncated_syslist)
        syslist.erase(syslist.begin() + std::numeric_limits<A_t>::max() + 1, syslist.end());

    named_cpy.emplace(syslist_name,
        std::make_unique<system_variable_syslist>(
            syslist_name, std::make_unique<macro_param_data_zero_based>(std::move(syslist))));

    return { std::make_unique<macro_invocation>(
                 id, cached_definition, copy_nests, labels, std::move(named_cpy), definition_location),
        truncated_syslist };
}

const std::vector<std::unique_ptr<positional_param>>& macro_definition::get_positional_params() const
{
    return positional_params_;
}

const std::vector<std::unique_ptr<keyword_param>>& macro_definition::get_keyword_params() const
{
    return keyword_params_;
}

const id_index& macro_definition::get_label_param_name() const { return label_param_name_; }

macro_invocation::macro_invocation(id_index name,
    cached_block& cached_definition,
    const copy_nest_storage& copy_nests,
    const macro_label_storage& labels,
    std::unordered_map<id_index, std::unique_ptr<macro_param_base>> named_params,
    const location& definition_location)
    : id(name)
    , named_params(std::move(named_params))
    , cached_definition(cached_definition)
    , copy_nests(copy_nests)
    , labels(labels)
    , definition_location(definition_location)
{}
