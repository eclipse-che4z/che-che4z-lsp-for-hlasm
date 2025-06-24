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

#ifndef HLASMPLUGIN_PARSERLIBRARY_PROGRAM_CONFIGURATION_STORAGE_H
#define HLASMPLUGIN_PARSERLIBRARY_PROGRAM_CONFIGURATION_STORAGE_H

#include <map>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "utils/general_hashers.h"
#include "utils/resource_location.h"
#include "workspaces/configuration_datatypes.h"

namespace hlasm_plugin::parser_library::workspaces {

class program_configuration_storage
{
public:
    enum class cfg_affiliation
    {
        none,
        exact_pgm,
        regex_pgm,
        exact_b4g,
        regex_b4g,
        exact_ext
    };

    struct get_pgm_result
    {
        const program* pgm;
        cfg_affiliation affiliation;
    };

    explicit program_configuration_storage(const proc_groups_map& proc_grps);

    void add_exact_conf(program pgm, const void* tag, const utils::resource::resource_location& alternative_cfg_rl);

    void update_exact_conf(program pgm, const void* tag, const utils::resource::resource_location& alternative_cfg_rl);

    void add_regex_conf(program pgm, const void* tag, const utils::resource::resource_location& alternative_cfg_rl);

    get_pgm_result get_program(const utils::resource::resource_location& file_location_normalized) const;

    std::unordered_map<std::string, bool, utils::hashers::string_hasher, std::equal_to<>>
    get_categorized_missing_pgroups(const utils::resource::resource_location& config_file_rl,
        const std::vector<utils::resource::resource_location>& opened_files) const;

    void remove_conf(const void* tag);

    void prune_external_processor_groups(const utils::resource::resource_location& location);

    void clear();

private:
    struct missing_pgroup_details
    {
        std::string pgroup_name;
        utils::resource::resource_location config_rl;
    };

    using name_set = std::unordered_set<std::string, utils::hashers::string_hasher, std::equal_to<>>;

    using program_details = std::variant<program, missing_pgroup_details>;

    struct program_properties
    {
        program_details pgm_details;
        cfg_affiliation affiliation;
        const void* tag = nullptr;
    };

    const proc_groups_map& m_proc_grps;
    std::map<utils::resource::resource_location, program_properties> m_exact_match;
    std::vector<std::pair<program_properties, std::regex>> m_regex_pgm_conf;
    std::vector<std::pair<program_properties, std::regex>> m_regex_b4g_json;
    std::unordered_map<utils::resource::resource_location, name_set> m_missing_proc_grps;

    missing_pgroup_details new_missing_pgroup_helper(
        std::string missing_pgroup_name, utils::resource::resource_location config_rl);

    const missing_pgroup_details* get_missing_pgroup_details(
        const utils::resource::resource_location& file_location) const;

    const program_properties* get_program_properties(const utils::resource::resource_location& file_location) const;
};

} // namespace hlasm_plugin::parser_library::workspaces

#endif
