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
#include <compare>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <version>

#include "compiler_options.h"
#include "config/b4g_config.h"
#include "config/pgm_conf.h"
#include "config/proc_grps.h"
#include "diagnostic.h"
#include "lib_config.h"
#include "processor_group.h"
#include "utils/general_hashers.h"
#include "utils/resource_location.h"
#include "utils/task.h"
#include "watcher_registration_provider.h"
#include "workspaces/configuration_datatypes.h"
#include "workspaces/configuration_provider.h"

namespace hlasm_plugin::parser_library {
struct asm_option;
class external_configuration_requests;
} // namespace hlasm_plugin::parser_library
namespace hlasm_plugin::parser_library::workspaces {
using global_settings_map =
    std::unordered_map<std::string, std::optional<nlohmann::json>, utils::hashers::string_hasher, std::equal_to<>>;

class file_manager;
class program_configuration_storage;
struct library_local_options;

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


class library_options
{
    struct impl
    {
        void (*const deleter)(const void* p) noexcept;
        bool (*const comparer_lt)(const void* l, const void* r) noexcept;
    };
    template<typename T>
    struct impl_t
    {
        static void deleter(const void* p) noexcept { delete static_cast<const T*>(p); }
        static bool comparer_lt(const void* l, const void* r) noexcept
        {
            return *static_cast<const T*>(l) < *static_cast<const T*>(r);
        }
    };
    template<typename T>
    static const impl* get_impl(const T&)
    {
        static
#ifdef _MSC_VER
            // This prevents COMDAT folding
            constinit
#else
            constexpr
#endif //  _MSC_VER
            impl i { &impl_t<T>::deleter, &impl_t<T>::comparer_lt };
        return &i;
    }
    const impl* m_impl;
    const void* m_data;

public:
    template<typename T>
    explicit library_options(T value)
        : m_impl(get_impl(value))
        , m_data(new T(std::move(value)))
    {}
    library_options(library_options&& o) noexcept
        : m_impl(std::exchange(o.m_impl, nullptr))
        , m_data(std::exchange(o.m_data, nullptr))
    {}
    library_options& operator=(library_options&& o) noexcept
    {
        library_options tmp(std::move(o));
        swap(tmp);
        return *this;
    }
    void swap(library_options& o) noexcept
    {
        std::swap(m_impl, o.m_impl);
        std::swap(m_data, o.m_data);
    }
    ~library_options()
    {
        if (m_impl)
            m_impl->deleter(m_data);
    }

    friend bool operator<(const library_options& l, const library_options& r) noexcept
    {
        if (auto c = l.m_impl <=> r.m_impl; c != 0)
            return c < 0;

        return l.m_impl && l.m_impl->comparer_lt(l.m_data, r.m_data);
    }

    template<typename T>
    friend bool operator<(const library_options& l, const T& r) noexcept
    {
        if (auto c = l.m_impl <=> get_impl(r); c != 0)
            return c < 0;
        return impl_t<T>::comparer_lt(l.m_data, &r);
    }
    template<typename T>
    friend bool operator<(const T& l, const library_options& r) noexcept
    {
        if (auto c = get_impl(l) <=> r.m_impl; c != 0)
            return c < 0;
        return impl_t<T>::comparer_lt(&l, r.m_data);
    }
};

class workspace_configuration : public configuration_provider
{
    static constexpr char FILENAME_PROC_GRPS[] = "proc_grps.json";
    static constexpr char FILENAME_PGM_CONF[] = "pgm_conf.json";
    static constexpr char HLASM_PLUGIN_FOLDER[] = ".hlasmplugin";
    static constexpr char B4G_CONF_FILE[] = ".bridge.json";

    file_manager& m_file_manager;
    utils::resource::resource_location m_location;
    utils::resource::resource_location m_proc_base;
    const shared_json& m_global_settings;
    const lib_config& m_global_config;

    utils::resource::resource_location m_proc_grps_loc;
    utils::resource::resource_location m_pgm_conf_loc;

    utils::resource::resource_location m_proc_grps_current_loc;
    utils::resource::resource_location m_pgm_conf_current_loc;

    config::proc_grps m_proc_grps_source;
    proc_groups_map m_proc_grps;

    struct b4g_config
    {
        std::optional<config::b4g_map> config;
        std::vector<diagnostic> diags;
    };

    std::unordered_map<utils::resource::resource_location, b4g_config> m_b4g_config_cache;

    global_settings_map m_utilized_settings_values;

    lib_config m_local_config;

    std::vector<diagnostic> m_config_diags;

    struct watcher_registration_handle
    {
        watcher_registration_provider* provider = nullptr;
        watcher_registration_id id = watcher_registration_id::INVALID;

        constexpr watcher_registration_handle() noexcept = default;
        constexpr watcher_registration_handle(
            watcher_registration_provider* provider, watcher_registration_id id) noexcept
            : provider(provider)
            , id(id)
        {}

        watcher_registration_handle(const watcher_registration_handle&) = delete;
        constexpr watcher_registration_handle(watcher_registration_handle&& o) noexcept;
        watcher_registration_handle& operator=(const watcher_registration_handle&) = delete;
        watcher_registration_handle& operator=(watcher_registration_handle&&) noexcept;
        ~watcher_registration_handle();
    };

    struct library_entry
    {
        std::shared_ptr<library> lib;
        watcher_registration_handle handle;
        bool used; // transient
    };

    struct library_prefix_entry
    {
        watcher_registration_handle handle;
        bool used; // transient
    };

    std::map<std::pair<utils::resource::resource_location, library_options>, library_entry, std::less<>> m_libraries;
    std::map<utils::resource::resource_location, library_prefix_entry, std::less<>> m_library_prefixes;

    external_configuration_requests* m_external_configuration_requests;
    watcher_registration_provider* m_watch_provider;
    std::unique_ptr<program_configuration_storage> m_pgm_conf_store;

    std::shared_ptr<library> get_local_library(
        const utils::resource::resource_location& url, const library_local_options& opts);

    [[nodiscard]] utils::task process_processor_group(const config::processor_group& pg,
        std::span<const std::string> fallback_macro_extensions,
        const utils::resource::resource_location& alternative_root,
        std::vector<diagnostic>& diags);

    [[nodiscard]] utils::task process_processor_group_library(const config::library& lib,
        const utils::resource::resource_location& alternative_root,
        std::vector<diagnostic>& diags,
        std::span<const std::string> fallback_macro_extensions,
        processor_group& prc_grp);
    [[nodiscard]] utils::task process_processor_group_library(const config::dataset& dsn,
        const utils::resource::resource_location& alternative_root,
        std::vector<diagnostic>& diags,
        std::span<const std::string> fallback_macro_extensions,
        processor_group& prc_grp);
    [[nodiscard]] utils::task process_processor_group_library(const config::endevor& end,
        const utils::resource::resource_location& alternative_root,
        std::vector<diagnostic>& diags,
        std::span<const std::string> fallback_macro_extensions,
        processor_group& prc_grp);
    [[nodiscard]] utils::task process_processor_group_library(const config::endevor_dataset& end,
        const utils::resource::resource_location& alternative_root,
        std::vector<diagnostic>& diags,
        std::span<const std::string> fallback_macro_extensions,
        processor_group& prc_grp);

    [[nodiscard]] utils::task process_processor_group_and_cleanup_libraries(
        std::span<const config::processor_group> pgs,
        std::span<const std::string> fallback_macro_extensions,
        const utils::resource::resource_location& alternative_root,
        std::vector<diagnostic>& diags);

    void process_program(const config::program_mapping& pgm, std::vector<diagnostic>& diags);

    bool is_config_file(const utils::resource::resource_location& file_location) const;
    bool is_b4g_config_file(const utils::resource::resource_location& file) const;

    [[nodiscard]] utils::value_task<parse_config_file_result> parse_b4g_config_file(
        utils::resource::resource_location cfg_file_rl);

    [[nodiscard]] utils::value_task<parse_config_file_result> load_and_process_config(std::vector<diagnostic>& diags);

    [[nodiscard]] utils::value_task<std::pair<parse_config_file_result, utils::resource::resource_location>>
    load_proc_config(
        config::proc_grps& proc_groups, global_settings_map& utilized_settings_values, std::vector<diagnostic>& diags);
    [[nodiscard]] utils::value_task<std::pair<parse_config_file_result, utils::resource::resource_location>>
    load_pgm_config(
        config::pgm_conf& pgm_config, global_settings_map& utilized_settings_values, std::vector<diagnostic>& diags);

    [[nodiscard]] utils::task find_and_add_libs(utils::resource::resource_location root,
        std::string path_pattern,
        processor_group& prc_grp,
        library_local_options opts,
        std::vector<diagnostic>& diags);

    void add_missing_diags(std::vector<diagnostic>& target,
        const utils::resource::resource_location& config_file_rl,
        const std::vector<utils::resource::resource_location>& opened_files,
        bool include_advisory_cfg_diags) const;

    watcher_registration_handle add_watcher(std::string_view uri, bool recursive);

public:
    workspace_configuration(file_manager& fm,
        utils::resource::resource_location location,
        const shared_json& global_settings,
        const lib_config& global_config,
        external_configuration_requests* ecr,
        watcher_registration_provider* watcher_provider);
    workspace_configuration(file_manager& fm,
        const shared_json& global_settings,
        const lib_config& global_config,
        std::shared_ptr<library> the_library); // test-only

    ~workspace_configuration();

    workspace_configuration(const workspace_configuration&) = delete;
    workspace_configuration(workspace_configuration&&) = delete;
    workspace_configuration& operator=(const workspace_configuration&) = delete;
    workspace_configuration& operator=(workspace_configuration&&) = delete;

    bool is_configuration_file(const utils::resource::resource_location& file) const;
    [[nodiscard]] utils::value_task<parse_config_file_result> parse_configuration_file(
        std::optional<utils::resource::resource_location> file = std::nullopt);
    [[nodiscard]] utils::value_task<utils::resource::resource_location> load_alternative_config_if_needed(
        const utils::resource::resource_location& file_location);

    const program* get_program(const utils::resource::resource_location& program) const;
    const processor_group* get_proc_grp(const utils::resource::resource_location& file) const;
    const processor_group* get_proc_grp_by_program(const program& p) const;
    processor_group* get_proc_grp_by_program(const program& p);

    bool settings_updated() const;
    [[nodiscard]] utils::value_task<std::optional<std::vector<index_t<processor_group, unsigned long long>>>>
    refresh_libraries(const std::vector<utils::resource::resource_location>& file_locations);

    void produce_diagnostics(std::vector<diagnostic>& target,
        const std::unordered_map<utils::resource::resource_location, std::vector<utils::resource::resource_location>>&
            used_configs_opened_files_map,
        bool include_advisory_cfg_diags) const;

    const processor_group& get_proc_grp(const proc_grp_id& p) const; // test only

    [[nodiscard]] utils::task update_external_configuration(
        const utils::resource::resource_location& normalized_location, std::string group_json);
    [[nodiscard]] utils::value_task<decltype(m_proc_grps)::iterator> make_external_proc_group(
        const utils::resource::resource_location& normalized_location, std::string group_json);

    void prune_external_processor_groups(const utils::resource::resource_location& location);

    [[nodiscard]] utils::value_task<std::pair<analyzer_configuration, index_t<processor_group, unsigned long long>>>
    get_analyzer_configuration(utils::resource::resource_location url) override;
    [[nodiscard]] opcode_suggestion_data get_opcode_suggestion_data(
        const utils::resource::resource_location& url) override;

    void change_processor_group_base(utils::resource::resource_location url);
};

} // namespace hlasm_plugin::parser_library::workspaces

#endif
