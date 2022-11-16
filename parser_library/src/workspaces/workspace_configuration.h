/*
 * Copyright (c) 2022 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_CONFIGURATION_H
#define HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_CONFIGURATION_H

#include <atomic>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "config/b4g_config.h"
#include "config/pgm_conf.h"
#include "config/proc_grps.h"
#include "diagnosable.h"
#include "diagnostic.h"
#include "lib_config.h"
#include "processor_group.h"
#include "utils/general_hashers.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::parser_library::workspaces {
using program_id = utils::resource::resource_location;
using global_settings_map =
    std::unordered_map<std::string, std::optional<std::string>, utils::hashers::string_hasher, std::equal_to<>>;
using proc_grp_id = std::pair<std::string, utils::resource::resource_location>;
class file_manager;
struct library_local_options;
// represents pair program => processor group - saves
// information that a program uses certain processor group
struct program
{
    program(program_id prog_id, proc_grp_id pgroup, config::assembler_options asm_opts)
        : prog_id(std::move(prog_id))
        , pgroup(std::move(pgroup))
        , asm_opts(std::move(asm_opts))
    {}

    program_id prog_id;
    proc_grp_id pgroup;
    config::assembler_options asm_opts;
};

enum class parse_config_file_result
{
    parsed,
    not_found,
    error,
};

#if __cpp_lib_atomic_shared_ptr >= 201711L
using shared_json = std::atomic<std::shared_ptr<const nlohmann::json>>;
#else
class shared_json
{
    std::shared_ptr<const nlohmann::json> m_data;

public:
    shared_json(std::shared_ptr<const nlohmann::json> data)
        : m_data(std::move(data))
    {}

    auto load() const { return std::atomic_load(&m_data); }
    void store(std::shared_ptr<const nlohmann::json> data) { std::atomic_store(&m_data, std::move(data)); }
};
#endif

class workspace_configuration
{
    static constexpr const char FILENAME_PROC_GRPS[] = "proc_grps.json";
    static constexpr const char FILENAME_PGM_CONF[] = "pgm_conf.json";
    static constexpr const char HLASM_PLUGIN_FOLDER[] = ".hlasmplugin";
    static constexpr const char B4G_CONF_FILE[] = ".bridge.json";

    file_manager& m_file_manager;
    utils::resource::resource_location m_location;
    const shared_json& m_global_settings;

    utils::resource::resource_location m_proc_grps_loc;
    utils::resource::resource_location m_pgm_conf_loc;

    struct proc_grp_id_hasher
    {
        size_t operator()(const proc_grp_id& pgid) const
        {
            return std::hash<std::string>()(pgid.first) ^ utils::resource::resource_location_hasher()(pgid.second);
        }
    };

    config::proc_grps m_proc_grps_source;
    std::unordered_map<proc_grp_id, processor_group, proc_grp_id_hasher> m_proc_grps;

    struct tagged_program
    {
        program pgm;
        const void* tag = nullptr;
    };

    std::map<utils::resource::resource_location, tagged_program> m_exact_pgm_conf;
    std::vector<std::pair<tagged_program, std::regex>> m_regex_pgm_conf;

    struct b4g_config
    {
        std::optional<config::b4g_map> config;
        std::vector<diagnostic_s> diags;
    };

    std::unordered_map<utils::resource::resource_location, b4g_config, utils::resource::resource_location_hasher>
        m_b4g_config_cache;

    global_settings_map m_utilized_settings_values;

    lib_config m_local_config;

    std::vector<diagnostic_s> m_config_diags;

    void process_processor_group(const config::processor_group& pg,
        std::span<const std::string> fallback_macro_extensions,
        std::span<const std::string> always_recognize,
        const utils::resource::resource_location& alternative_root,
        std::vector<diagnostic_s>& diags);

    bool process_program(const config::program_mapping& pgm, std::vector<diagnostic_s>& diags);

    bool is_config_file(const utils::resource::resource_location& file_location) const;
    bool is_b4g_config_file(const utils::resource::resource_location& file) const;
    const program* get_program_normalized(const utils::resource::resource_location& file_location_normalized) const;

    parse_config_file_result parse_b4g_config_file(const utils::resource::resource_location& file_location);

    bool try_loading_alternative_configuration(const utils::resource::resource_location& file_location);

    parse_config_file_result load_and_process_config(std::vector<diagnostic_s>& diags);

    parse_config_file_result load_proc_config(config::proc_grps& proc_groups,
        file_ptr& proc_grps_file,
        global_settings_map& utilized_settings_values,
        std::vector<diagnostic_s>& diags);
    parse_config_file_result load_pgm_config(config::pgm_conf& pgm_config,
        file_ptr& pgm_conf_file,
        global_settings_map& utilized_settings_values,
        std::vector<diagnostic_s>& diags);

    void find_and_add_libs(const utils::resource::resource_location& root,
        const utils::resource::resource_location& path_pattern,
        processor_group& prc_grp,
        const library_local_options& opts,
        std::vector<diagnostic_s>& diags);

public:
    workspace_configuration(
        file_manager& fm, utils::resource::resource_location location, const shared_json& global_settings);

    bool is_configuration_file(const utils::resource::resource_location& file) const;
    parse_config_file_result parse_configuration_file(
        std::optional<utils::resource::resource_location> file = std::nullopt);
    void load_alternative_config_if_needed(const utils::resource::resource_location& file_location);

    const program* get_program(const utils::resource::resource_location& program) const;
    const processor_group& get_proc_grp_by_program(const program& p) const;
    processor_group& get_proc_grp_by_program(const program& p);
    const lib_config& get_config() const { return m_local_config; }

    bool settings_updated() const;
    void refresh_libraries();

    void copy_diagnostics(const diagnosable& target) const;

    const processor_group& get_proc_grp(const proc_grp_id& p) const; // test only
};

} // namespace hlasm_plugin::parser_library::workspaces

#endif
