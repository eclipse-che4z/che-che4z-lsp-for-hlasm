/*
 * Copyright (c) 2023 Broadcom.
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

#include "program_configuration_storage.h"

#include <string_view>

#include "workspaces/wildcard.h"

namespace hlasm_plugin::parser_library::workspaces {

namespace {
constexpr std::string_view NOPROC_GROUP_ID = "*NOPROC*";

struct
{
    std::string_view operator()(const basic_conf& c) const noexcept { return c.name; }
    std::string_view operator()(const b4g_conf& c) const noexcept { return c.name; }
    std::string_view operator()(const external_conf&) const noexcept { return {}; }
} constexpr proc_group_name;
} // namespace

program_configuration_storage::program_configuration_storage(const proc_groups_map& proc_grps)
    : m_proc_grps(proc_grps)
{}

void program_configuration_storage::add_exact_conf(
    program pgm, const void* tag, const utils::resource::resource_location& alternative_cfg_rl)
{
    using enum cfg_affiliation;
    auto affiliation = pgm.external ? exact_ext : alternative_cfg_rl.empty() ? exact_pgm : exact_b4g;

    if (auto pgroup_name = std::visit(proc_group_name, pgm.pgroup);
        !m_proc_grps.contains(pgm.pgroup) && pgroup_name != NOPROC_GROUP_ID)
        m_exact_match.try_emplace(std::move(pgm.prog_id),
            program_properties {
                new_missing_pgroup_helper(std::string(pgroup_name), alternative_cfg_rl),
                affiliation,
                tag,
            });
    else
    {
        utils::resource::resource_location pgm_rl = pgm.prog_id;
        m_exact_match.try_emplace(std::move(pgm_rl),
            program_properties {
                std::move(pgm),
                affiliation,
                tag,
            });
    }
}

void program_configuration_storage::update_exact_conf(
    program pgm, const void* tag, const utils::resource::resource_location& alternative_cfg_rl)
{
    using enum cfg_affiliation;
    auto affiliation = pgm.external ? exact_ext : alternative_cfg_rl.empty() ? exact_pgm : exact_b4g;

    if (auto pgroup_name = std::visit(proc_group_name, pgm.pgroup);
        !m_proc_grps.contains(pgm.pgroup) && pgroup_name != NOPROC_GROUP_ID)
        m_exact_match.insert_or_assign(std::move(pgm.prog_id),
            program_properties {
                new_missing_pgroup_helper(std::string(pgroup_name), alternative_cfg_rl),
                affiliation,
                tag,
            });
    else
    {
        utils::resource::resource_location pgm_rl = pgm.prog_id;
        m_exact_match.insert_or_assign(std::move(pgm_rl),
            program_properties {
                std::move(pgm),
                affiliation,
                tag,
            });
    }
}

void program_configuration_storage::add_regex_conf(
    program pgm, const void* tag, const utils::resource::resource_location& alternative_cfg_rl)
{
    assert(!pgm.external);

    using enum cfg_affiliation;
    auto affiliation = alternative_cfg_rl.empty() ? regex_pgm : regex_b4g;
    auto& container = alternative_cfg_rl.empty() ? m_regex_pgm_conf : m_regex_b4g_json;
    auto r = wildcard2regex(pgm.prog_id.get_uri());

    if (auto pgroup_name = std::visit(proc_group_name, pgm.pgroup);
        !m_proc_grps.contains(pgm.pgroup) && pgroup_name != NOPROC_GROUP_ID)
        container.emplace_back(
            program_properties {
                new_missing_pgroup_helper(std::string(pgroup_name), alternative_cfg_rl),
                affiliation,
                tag,
            },
            std::move(r));
    else
        container.emplace_back(
            program_properties {
                std::move(pgm),
                affiliation,
                tag,
            },
            std::move(r));
}

program_configuration_storage::get_pgm_result program_configuration_storage::get_program(
    const utils::resource::resource_location& file_location_normalized) const
{
    if (const auto* details = get_program_properties(file_location_normalized))
        return { std::get_if<program>(&details->pgm_details), details->affiliation };

    return { nullptr, cfg_affiliation::none };
}

std::unordered_map<std::string, bool, utils::hashers::string_hasher, std::equal_to<>>
program_configuration_storage::get_categorized_missing_pgroups(const utils::resource::resource_location& config_file_rl,
    const std::vector<utils::resource::resource_location>& opened_files) const
{
    auto missing_proc_grps_it = m_missing_proc_grps.find(config_file_rl);
    if (missing_proc_grps_it == m_missing_proc_grps.end())
        return {};

    std::unordered_map<std::string, bool, utils::hashers::string_hasher, std::equal_to<>> categorized_missing_pgroups;

    for (const auto& missing_pgroup : missing_proc_grps_it->second)
        categorized_missing_pgroups[missing_pgroup] = false;

    for (const auto& opened_file : opened_files)
    {
        if (const auto missing_details = get_missing_pgroup_details(opened_file))
            categorized_missing_pgroups[missing_details->pgroup_name] = true;
    }

    return categorized_missing_pgroups;
}

void program_configuration_storage::remove_conf(const void* tag)
{
    std::erase_if(m_exact_match, [&tag](const auto& e) { return e.second.tag == tag; });
    std::erase_if(m_regex_pgm_conf, [&tag](const auto& e) { return e.first.tag == tag; });
    std::erase_if(m_regex_b4g_json, [&tag](const auto& e) { return e.first.tag == tag; });
}

void program_configuration_storage::prune_external_processor_groups(const utils::resource::resource_location& location)
{
    static constexpr auto is_external = [](const program_properties& pgm_props) {
        const auto* pgm = std::get_if<program>(&pgm_props.pgm_details);
        return pgm && pgm->external;
    };

    if (!location.empty())
    {
        if (auto p = m_exact_match.find(location.lexically_normal());
            p != m_exact_match.end() && is_external(p->second))
            m_exact_match.erase(p);
    }
    else
        std::erase_if(m_exact_match, [](const auto& p) { return is_external(p.second); });
}

void program_configuration_storage::clear()
{
    m_exact_match.clear();
    m_regex_pgm_conf.clear();
    m_regex_b4g_json.clear();
    m_missing_proc_grps.clear();
}

program_configuration_storage::missing_pgroup_details program_configuration_storage::new_missing_pgroup_helper(
    std::string missing_pgroup_name, utils::resource::resource_location config_rl)
{
    m_missing_proc_grps.try_emplace(config_rl).first->second.insert(missing_pgroup_name);
    return missing_pgroup_details {
        std::move(missing_pgroup_name),
        std::move(config_rl),
    };
}

const program_configuration_storage::missing_pgroup_details* program_configuration_storage::get_missing_pgroup_details(
    const utils::resource::resource_location& file_location) const
{
    if (const auto* details = get_program_properties(file_location))
        return std::get_if<missing_pgroup_details>(&details->pgm_details);

    return nullptr;
}

const program_configuration_storage::program_properties* program_configuration_storage::get_program_properties(
    const utils::resource::resource_location& file_location) const
{
    using enum cfg_affiliation;

    const program_properties* pgm_props_exact_match = nullptr;

    // exact match
    if (auto pgm_props_it = m_exact_match.find(file_location); pgm_props_it != m_exact_match.cend())
    {
        pgm_props_exact_match = &pgm_props_it->second;
        if (pgm_props_exact_match->affiliation == exact_pgm || pgm_props_exact_match->affiliation == exact_ext)
            return pgm_props_exact_match;
    }

    for (const auto& [pgm_props, pattern] : m_regex_pgm_conf)
    {
        if (std::regex_match(file_location.get_uri(), pattern))
            return &pgm_props;
    }

    if (pgm_props_exact_match)
        return pgm_props_exact_match;

    for (const auto& [pgm_props, pattern] : m_regex_b4g_json)
    {
        if (std::regex_match(file_location.get_uri(), pattern))
            return &pgm_props;
    }

    return nullptr;
}

} // namespace hlasm_plugin::parser_library::workspaces
