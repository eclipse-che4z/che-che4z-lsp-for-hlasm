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

#include "workspace_configuration.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <compare>
#include <deque>
#include <exception>
#include <functional>
#include <memory>
#include <regex>
#include <string_view>
#include <system_error>
#include <tuple>
#include <unordered_set>

#include "compiler_options.h"
#include "diagnostic_op.h"
#include "external_configuration_requests.h"
#include "file_manager.h"
#include "library_local.h"
#include "nlohmann/json.hpp"
#include "utils/async_busy_wait.h"
#include "utils/content_loader.h"
#include "utils/encoding.h"
#include "utils/factory.h"
#include "utils/path.h"
#include "utils/path_conversions.h"
#include "utils/platform.h"
#include "utils/string_operations.h"
#include "watcher_registration_provider.h"
#include "workspaces/configuration_provider.h"
#include "workspaces/program_configuration_storage.h"
#include "workspaces/wildcard.h"

namespace hlasm_plugin::parser_library::workspaces {
namespace {
const utils::resource::resource_location empty_alternative_cfg_root;

std::optional<std::filesystem::path> get_fs_abs_path(std::string_view path)
{
    if (path.empty())
        return std::nullopt;

    try
    {
        if (std::filesystem::path fs_path = path; utils::path::is_absolute(fs_path))
            return fs_path;

        return std::nullopt;
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

std::pair<utils::resource::resource_location, std::string> transform_to_resource_location(std::string_view path,
    const utils::resource::resource_location& base_resource_location,
    std::string_view path_suffix)
{
    using utils::resource::resource_location;
    std::pair<resource_location, std::string> result;
    auto& [rl, suffix] = result;

    if (resource_location::is_local(path))
    {
        rl = resource_location("file:" + utils::encoding::percent_encode_path_and_ignore_utf8(path.substr(5)));
        suffix = utils::encoding::percent_encode_path_and_ignore_utf8(path_suffix);
    }
    else if (utils::path::dissect_uri(path).has_value())
    {
        rl = resource_location(path);
        suffix = path_suffix;
    }
    else if (auto fs_path = get_fs_abs_path(path); fs_path.has_value())
    {
        rl = resource_location(utils::path::path_to_uri(utils::path::lexically_normal(*fs_path).string()));
        suffix = utils::encoding::percent_encode_path(path_suffix);
    }
    else
    {
        rl = resource_location::join(base_resource_location, utils::encoding::percent_encode_path(path));
        suffix = utils::encoding::percent_encode_path(path_suffix);
    }

    rl = rl.lexically_normal();

    return result;
}

std::optional<std::string> substitute_home_directory(std::string p)
{
    if (!p.starts_with("~"))
        return p;

    const auto& homedir = utils::platform::home();
    if (homedir.empty())
        return std::nullopt;

    const auto skip = (size_t)1 + (p.starts_with("~/") || p.starts_with("~\\"));
    return utils::path::join(homedir, std::move(p).substr(skip)).string();
}

library_local_options get_library_local_options(
    const config::library& lib, std::span<const std::string> fallback_macro_extensions)
{
    library_local_options opts;

    opts.optional_library = lib.optional;
    if (!lib.macro_extensions.empty())
        opts.extensions = lib.macro_extensions;
    else if (!fallback_macro_extensions.empty())
        opts.extensions = std::vector(fallback_macro_extensions.begin(), fallback_macro_extensions.end());

    return opts;
}

const nlohmann::json* find_member(std::string_view key, const nlohmann::json& j)
{
    if (j.is_object())
    {
        if (auto it = j.find(key); it == j.end())
            return nullptr;
        else
            return &it.value();
    }
    else if (j.is_array())
    {
        unsigned long long i = 0;
        const auto conv_result = std::from_chars(std::to_address(key.begin()), std::to_address(key.end()), i);
        if (conv_result.ec != std::errc() || conv_result.ptr != std::to_address(key.end()) || i >= j.size())
            return nullptr;

        return &j[i];
    }
    else
        return nullptr;
}

const nlohmann::json* find_subobject(std::string_view key, const nlohmann::json& input)
{
    const nlohmann::json* j = &input;

    while (true)
    {
        auto dot = key.find('.');
        auto subkey = key.substr(0, dot);

        j = find_member(subkey, *j);
        if (!j)
            return nullptr;

        if (dot == std::string_view::npos)
            break;
        else
            key.remove_prefix(dot + 1);
    }

    return j;
}

struct json_settings_replacer
{
    static const std::regex config_reference;

    const nlohmann::json& global_settings;
    global_settings_map& utilized_settings_values;
    const utils::resource::resource_location& location;

    std::match_results<std::string_view::iterator> matches;

    std::unordered_set<std::string, utils::hashers::string_hasher, std::equal_to<>> unavailable;

    void operator()(nlohmann::json& val)
    {
        if (val.is_structured())
        {
            for (auto& value : val)
                (*this)(value);
        }
        else if (val.is_string())
        {
            const auto& value = val.get<std::string_view>();

            if (auto replacement = try_replace(value); replacement.has_value())
                val = std::move(replacement).value();
        }
    }

    std::optional<std::string> try_replace(std::string_view s)
    {
        std::optional<std::string> result;
        if (!std::regex_search(s.begin(), s.end(), matches, config_reference))
            return result;

        auto& r = result.emplace();
        do
        {
            r.append(s.begin(), matches[0].first);
            s.remove_prefix(matches[0].second - s.begin());

            static constexpr std::string_view config_section = "config:";

            std::string_view key(matches[1].first, matches[1].second);
            if (key.starts_with(config_section))
            {
                auto reduced_key = key.substr(config_section.size());
                const auto* v = find_subobject(reduced_key, global_settings);
                if (v && v->is_string())
                    r.append(v->get<std::string_view>());
                else
                    unavailable.emplace(reduced_key);
                update_utilized_settings(reduced_key, v);
            }
            else if (key == "workspaceFolder")
            {
                // This seems broken in VSCode.
                // Our version attempts to do the right thing, while maintaining compatibility
                r.append(!location.is_local() ? location.get_uri() : location.get_path());
            }
            else
                unavailable.emplace(key);

        } while (std::regex_search(s.begin(), s.end(), matches, config_reference));

        r.append(s);

        return result;
    }

    void update_utilized_settings(std::string_view key, const nlohmann::json* j)
    {
        if (!j)
        {
            utilized_settings_values.emplace(
                std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(std::nullopt));
        }
        else
        {
            utilized_settings_values.emplace(
                std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(*j));
        }
    }
};

const std::regex json_settings_replacer::config_reference(R"(\$\{([^}]+)\})");
} // namespace

workspace_configuration::workspace_configuration(file_manager& fm,
    utils::resource::resource_location location,
    const shared_json& global_settings,
    const lib_config& global_config,
    external_configuration_requests* ecr,
    watcher_registration_provider* watch_provider)
    : m_file_manager(fm)
    , m_location(location)
    , m_proc_base(std::move(location))
    , m_global_settings(global_settings)
    , m_global_config(global_config)
    , m_external_configuration_requests(ecr)
    , m_watch_provider(watch_provider)
    , m_pgm_conf_store(std::make_unique<program_configuration_storage>(m_proc_grps))
{
    if (!m_location.empty())
    {
        auto hlasm_folder = utils::resource::resource_location::join(m_location, HLASM_PLUGIN_FOLDER);
        m_proc_grps_loc = utils::resource::resource_location::join(hlasm_folder, FILENAME_PROC_GRPS);
        m_pgm_conf_loc = utils::resource::resource_location::join(hlasm_folder, FILENAME_PGM_CONF);
    }
}

workspace_configuration::workspace_configuration(file_manager& fm,
    const shared_json& global_settings,
    const lib_config& global_config,
    std::shared_ptr<library> the_library)

    : m_file_manager(fm)
    , m_global_settings(global_settings)
    , m_global_config(global_config)
    , m_external_configuration_requests(nullptr)
    , m_pgm_conf_store(std::make_unique<program_configuration_storage>(m_proc_grps))
{
    static const std::string test_group = "test_group";
    auto [it, _] = m_proc_grps.try_emplace(basic_conf { test_group },
        std::piecewise_construct,
        std::forward_as_tuple(test_group, config::assembler_options(), std::vector<config::preprocessor_options>()),
        std::forward_as_tuple());
    it->second.first.add_library(std::move(the_library));
    m_pgm_conf_store->add_regex_conf(
        program { utils::resource::resource_location("**"), basic_conf { test_group }, {}, false },
        nullptr,
        empty_alternative_cfg_root);
}

workspace_configuration::~workspace_configuration() = default;

bool workspace_configuration::is_configuration_file(const utils::resource::resource_location& file) const
{
    return !m_location.empty() && !file.empty() && (is_config_file(file) || is_b4g_config_file(file));
}

template<typename T>
inline bool operator<(const std::pair<utils::resource::resource_location, library_options>& l,
    const std::tuple<const utils::resource::resource_location&, const T&>& r) noexcept
{
    const auto& [lx, ly] = l;
    return std::tie(lx, ly) < r;
}

template<typename T>
inline bool operator<(const std::tuple<const utils::resource::resource_location&, const T&>& l,
    const std::pair<utils::resource::resource_location, library_options>& r) noexcept
{
    const auto& [rx, ry] = r;
    return l < std::tie(rx, ry);
}

unsigned long long make_unique_id()
{
    static std::atomic<unsigned long long> unique_group_id = 0;
    return unique_group_id.fetch_add(1, std::memory_order_relaxed);
}

std::shared_ptr<library> workspace_configuration::get_local_library(
    const utils::resource::resource_location& url, const library_local_options& opts)
{
    if (const auto it = m_libraries.find(std::tie(url, opts)); it != m_libraries.end())
    {
        it->second.used = true;
        return it->second.lib;
    }

    auto watcher = add_watcher(url.get_uri(), false);
    auto result = std::make_shared<library_local>(m_file_manager, url, opts, m_proc_grps_current_loc);

    m_libraries.try_emplace(std::make_pair(url, library_options(opts)), result, std::move(watcher), true);

    return result;
}


utils::task workspace_configuration::process_processor_group(const config::processor_group& pg,
    std::span<const std::string> fallback_macro_extensions,
    const utils::resource::resource_location& alternative_root,
    std::vector<diagnostic>& diags)
{
    processor_group prc_grp(pg.name, pg.asm_options, pg.preprocessors);

    for (auto& lib_or_dataset : pg.libs)
    {
        // Workaround for poor symmetric transfer support in WebAssembly
        auto t = std::visit(
            [this, &alternative_root, &diags, &fallback_macro_extensions, &prc_grp](const auto& lib) {
                return process_processor_group_library(
                    lib, alternative_root, diags, fallback_macro_extensions, prc_grp);
            },
            lib_or_dataset);
        if (t.valid())
            co_await std::move(t);
    }

    auto next_id = make_unique_id();
    if (alternative_root.empty())
        m_proc_grps.try_emplace(basic_conf { prc_grp.name() }, std::move(prc_grp), next_id);
    else
        m_proc_grps.try_emplace(b4g_conf { prc_grp.name(), alternative_root }, std::move(prc_grp), next_id);

    co_return;
}

constexpr std::string_view external_uri_scheme = "hlasm-external";

utils::task workspace_configuration::process_processor_group_library(const config::dataset& dsn,
    const utils::resource::resource_location&,
    std::vector<diagnostic>&,
    std::span<const std::string>,
    processor_group& prc_grp)
{
    utils::resource::resource_location new_uri(utils::path::reconstruct_uri({
        .scheme = external_uri_scheme,
        .path = "/DATASET/" + utils::encoding::percent_encode_component(dsn.dsn),
    }));

    prc_grp.add_library(get_local_library(new_uri, { .optional_library = dsn.optional }));

    return {};
}

utils::task workspace_configuration::process_processor_group_library(const config::endevor& end,
    const utils::resource::resource_location&,
    std::vector<diagnostic>&,
    std::span<const std::string>,
    processor_group& prc_grp)
{
    utils::resource::resource_location new_uri(utils::path::reconstruct_uri({
        .scheme = external_uri_scheme,
        .path = "/ENDEVOR/" + utils::encoding::percent_encode_component(end.profile) + "/"
            + std::string(end.use_map ? "map" : "nomap") + "/"
            + utils::encoding::percent_encode_component(end.environment) + "/"
            + utils::encoding::percent_encode_component(end.stage) + "/"
            + utils::encoding::percent_encode_component(end.system) + "/"
            + utils::encoding::percent_encode_component(end.subsystem) + "/"
            + utils::encoding::percent_encode_component(end.type),
    }));

    prc_grp.add_library(get_local_library(new_uri, { .optional_library = end.optional }));

    return {};
}

utils::task workspace_configuration::process_processor_group_library(const config::endevor_dataset& end,
    const utils::resource::resource_location&,
    std::vector<diagnostic>&,
    std::span<const std::string>,
    processor_group& prc_grp)
{
    utils::resource::resource_location new_uri(utils::path::reconstruct_uri({
        .scheme = external_uri_scheme,
        .path = "/ENDEVOR/" + utils::encoding::percent_encode_component(end.profile) + "/"
            + utils::encoding::percent_encode_component(end.dsn),
    }));

    prc_grp.add_library(get_local_library(new_uri, { .optional_library = end.optional }));

    return {};
}

utils::task workspace_configuration::process_processor_group_library(const config::library& lib,
    const utils::resource::resource_location& alternative_root,
    std::vector<diagnostic>& diags,
    std::span<const std::string> fallback_macro_extensions,
    processor_group& prc_grp)
{
    const auto& root =
        lib.root_folder == config::processor_group_root_folder::alternate_root && !alternative_root.empty()
        ? alternative_root
        : m_proc_base;

    std::optional<std::string> lib_path = substitute_home_directory(lib.path);
    if (!lib_path.has_value())
    {
        diags.push_back(warning_L0006(m_proc_grps_current_loc, lib.path));
        return {};
    }

    auto lib_local_opts = get_library_local_options(lib, fallback_macro_extensions);

    if (auto first_wild_card = lib_path->find_first_of("*?"); first_wild_card == std::string::npos)
    {
        auto [rl, _] = transform_to_resource_location(*lib_path, root, {});
        prc_grp.add_library(get_local_library(rl.join(""), lib_local_opts));
        return {};
    }
    else
    {
        const size_t last_slash = lib_path->find_last_of("/\\", first_wild_card) + 1;
        const auto path_prefix = lib_path->substr(0, last_slash);

        auto [search_root, pattern] = transform_to_resource_location(path_prefix, root, lib_path->substr(last_slash));
        if (!pattern.ends_with("/"))
            pattern.push_back('/');

        search_root.join("");
        pattern.insert(0, search_root.get_uri());

        const auto [it, _] = m_library_prefixes.try_emplace(search_root, utils::factory([this, &search_root]() {
            return library_prefix_entry { add_watcher(search_root.get_uri(), true), true };
        }));
        it->second.used = true;

        return find_and_add_libs(std::move(search_root), std::move(pattern), prc_grp, std::move(lib_local_opts), diags);
    }
}

utils::task workspace_configuration::process_processor_group_and_cleanup_libraries(
    std::span<const config::processor_group> pgs,
    std::span<const std::string> fallback_macro_extensions,
    const utils::resource::resource_location& alternative_root,
    std::vector<diagnostic>& diags)
{
    for (auto& [_, l] : m_libraries)
        l.used = false; // mark
    for (auto& [_, l] : m_library_prefixes)
        l.used = false; // mark

    for (const auto& pg : pgs)
        co_await process_processor_group(pg, fallback_macro_extensions, alternative_root, diags);

    std::erase_if(m_libraries, [](const auto& kv) { return !kv.second.used; }); // sweep
    std::erase_if(m_library_prefixes, [](const auto& kv) { return !kv.second.used; }); // sweep
}

void workspace_configuration::process_program(const config::program_mapping& pgm, std::vector<diagnostic>& diags)
{
    std::optional<std::string> pgm_name = substitute_home_directory(pgm.program);
    if (!pgm_name.has_value())
    {
        diags.push_back(warning_L0006(m_pgm_conf_current_loc, pgm.program));
        return;
    }

    program prog {
        transform_to_resource_location(*pgm_name, m_location, {}).first,
        basic_conf {
            pgm.pgroup,
        },
        pgm.opts,
        false,
    };

    if (pgm_name->find_first_of("*?") == std::string::npos)
        m_pgm_conf_store->add_exact_conf(prog, nullptr, empty_alternative_cfg_root);
    else
        m_pgm_conf_store->add_regex_conf(prog, nullptr, empty_alternative_cfg_root);
}

bool workspace_configuration::is_config_file(const utils::resource::resource_location& file) const
{
    return !file.empty() && (file == m_proc_grps_loc || file == m_pgm_conf_loc);
}

bool workspace_configuration::is_b4g_config_file(const utils::resource::resource_location& file) const
{
    return file.filename() == B4G_CONF_FILE;
}

lib_config load_from_pgm_config(const config::pgm_conf& config)
{
    lib_config loaded;

    if (config.diagnostics_suppress_limit.has_value())
        loaded.diag_supress_limit = config.diagnostics_suppress_limit.value();

    return loaded;
}

[[nodiscard]] utils::value_task<std::variant<nlohmann::json, parse_config_file_result>> load_json_from_file(
    file_manager& fm, const utils::resource::resource_location& file)
{
    auto text = co_await fm.get_file_content(file);
    if (!text.has_value())
        co_return parse_config_file_result::not_found;

    try
    {
        co_return nlohmann::json::parse(text.value(), nullptr, true, true);
    }
    catch (const nlohmann::json::exception&)
    {
        co_return parse_config_file_result::error;
    }
}

template<typename... V, typename U>
bool equals(const std::variant<V...>& v, const U& u)
{
    return std::holds_alternative<U>(v) && std::get<U>(v) == u;
}

// open config files and parse them
utils::value_task<parse_config_file_result> workspace_configuration::load_and_process_config(
    std::vector<diagnostic>& diags)
{
    diags.clear();

    config::proc_grps proc_groups;
    config::pgm_conf pgm_config;
    global_settings_map utilized_settings_values;

    m_proc_grps.clear();
    m_pgm_conf_store->clear();
    m_b4g_config_cache.clear();

    auto [proc_gprs_result, proc_src] = co_await load_proc_config(proc_groups, utilized_settings_values, diags);

    auto [pgm_conf_loaded, pgm_src] = co_await load_pgm_config(pgm_config, utilized_settings_values, diags);

    m_proc_grps_current_loc = std::move(proc_src);
    m_pgm_conf_current_loc = std::move(pgm_src);

    m_utilized_settings_values = std::move(utilized_settings_values);

    if (proc_gprs_result != parse_config_file_result::parsed)
        co_return proc_gprs_result;

    co_await process_processor_group_and_cleanup_libraries(
        proc_groups.pgroups, proc_groups.macro_extensions, empty_alternative_cfg_root, diags);

    if (pgm_conf_loaded != parse_config_file_result::parsed)
    {
        m_local_config = {};
    }
    else
    {
        m_local_config = load_from_pgm_config(pgm_config);

        // process programs
        for (const auto& pgm : pgm_config.pgms)
            process_program(pgm, diags);
    }

    m_proc_grps_source = std::move(proc_groups);

    // we need to tolerate pgm_conf processing failure, because other products may provide the info
    co_return parse_config_file_result::parsed;
}

utils::value_task<std::pair<parse_config_file_result, utils::resource::resource_location>>
workspace_configuration::load_proc_config(
    config::proc_grps& proc_groups, global_settings_map& utilized_settings_values, std::vector<diagnostic>& diags)
{
    const auto current_settings = m_global_settings.load();
    json_settings_replacer json_visitor { *current_settings, utilized_settings_values, m_proc_base };

    auto config_source = m_proc_grps_loc;

    std::variant<nlohmann::json, parse_config_file_result> proc_json_or_err = parse_config_file_result::not_found;
    if (!m_proc_grps_loc.empty())
        proc_json_or_err = co_await load_json_from_file(m_file_manager, m_proc_grps_loc);

    if (equals(proc_json_or_err, parse_config_file_result::not_found))
    {
        static const std::string key = "hlasm.proc_grps";
        const auto* proc_conf = find_subobject(key, *current_settings);
        if (proc_conf && proc_conf->is_object())
        {
            proc_json_or_err = *proc_conf;
            config_source = m_location;
        }
        else if (proc_conf && !proc_conf->is_null())
        {
            proc_json_or_err = parse_config_file_result::error;
        }
        json_visitor.update_utilized_settings(key, proc_conf);
    }
    if (std::holds_alternative<parse_config_file_result>(proc_json_or_err))
    {
        auto reason = std::get<parse_config_file_result>(proc_json_or_err);
        if (reason == parse_config_file_result::error)
            diags.push_back(error_W0002(config_source));
        co_return { reason, config_source };
    }

    try
    {
        auto& proc_json = std::get<nlohmann::json>(proc_json_or_err);
        json_visitor(proc_json);
        proc_json.get_to(proc_groups);
    }
    catch (const nlohmann::json::exception&)
    {
        // could not load proc_grps
        diags.push_back(error_W0002(config_source));
        co_return { parse_config_file_result::error, config_source };
    }

    for (const auto& var : json_visitor.unavailable)
        diags.push_back(warn_W0007(config_source, var));

    for (const auto& pg : proc_groups.pgroups)
    {
        if (!pg.asm_options.valid())
            diags.push_back(error_W0005(config_source, pg.name, "processor group"));
        for (const auto& p : pg.preprocessors)
        {
            if (!p.valid())
                diags.push_back(error_W0006(config_source, pg.name, p.type()));
        }
    }

    co_return { parse_config_file_result::parsed, config_source };
}

utils::value_task<std::pair<parse_config_file_result, utils::resource::resource_location>>
workspace_configuration::load_pgm_config(
    config::pgm_conf& pgm_config, global_settings_map& utilized_settings_values, std::vector<diagnostic>& diags)
{
    const auto current_settings = m_global_settings.load();
    json_settings_replacer json_visitor { *current_settings, utilized_settings_values, m_proc_base };

    auto config_source = m_pgm_conf_loc;

    std::variant<nlohmann::json, parse_config_file_result> pgm_json_or_err = parse_config_file_result::not_found;
    if (!m_pgm_conf_loc.empty())
        pgm_json_or_err = co_await load_json_from_file(m_file_manager, m_pgm_conf_loc);

    if (equals(pgm_json_or_err, parse_config_file_result::not_found))
    {
        static const std::string key = "hlasm.pgm_conf";
        const auto* pgm_conf = find_subobject(key, *current_settings);
        if (pgm_conf && pgm_conf->is_object())
        {
            pgm_json_or_err = *pgm_conf;
            config_source = m_location;
        }
        else if (pgm_conf && !pgm_conf->is_null())
        {
            pgm_json_or_err = parse_config_file_result::error;
        }
        json_visitor.update_utilized_settings(key, pgm_conf);
    }
    if (std::holds_alternative<parse_config_file_result>(pgm_json_or_err))
    {
        auto reason = std::get<parse_config_file_result>(pgm_json_or_err);
        if (reason == parse_config_file_result::error)
            diags.push_back(error_W0003(config_source));
        co_return { reason, config_source };
    }

    try
    {
        auto& pgm_json = std::get<nlohmann::json>(pgm_json_or_err);
        json_visitor(pgm_json);
        pgm_json.get_to(pgm_config);
    }
    catch (const nlohmann::json::exception&)
    {
        diags.push_back(error_W0003(config_source));
        co_return { parse_config_file_result::error, config_source };
    }

    for (const auto& var : json_visitor.unavailable)
        diags.push_back(warn_W0007(config_source, var));

    for (const auto& pgm : pgm_config.pgms)
    {
        if (!pgm.opts.valid())
            diags.push_back(error_W0005(config_source, pgm.program, "program"));
    }

    co_return { parse_config_file_result::parsed, config_source };
}

bool workspace_configuration::settings_updated() const
{
    auto global_settings = m_global_settings.load();
    for (const auto& [key, value] : m_utilized_settings_values)
    {
        const auto* obj = find_subobject(key, *global_settings);
        if (obj == nullptr && !value.has_value())
            continue;
        if (!obj)
            return true;
        if (*obj == value)
            continue;
        return true;
    }
    return false;
}

utils::value_task<parse_config_file_result> workspace_configuration::parse_b4g_config_file(
    utils::resource::resource_location cfg_file_rl)
{
    // keep in sync with try_loading_alternative_configuration
    const auto alternative_root =
        utils::resource::resource_location::replace_filename(cfg_file_rl, "").join("..").lexically_normal();

    auto [it, inserted] = m_b4g_config_cache.try_emplace(cfg_file_rl);
    if (!inserted)
    {
        m_pgm_conf_store->remove_conf(std::to_address(it));

        std::erase_if(m_proc_grps, [&alternative_root](const auto& e) {
            const auto* b4g = std::get_if<b4g_conf>(&e.first);
            return b4g && b4g->bridge_json_uri == alternative_root;
        });
        it->second = {};
    }

    auto b4g_config_or_err = co_await load_json_from_file(m_file_manager, cfg_file_rl);
    if (std::holds_alternative<parse_config_file_result>(b4g_config_or_err))
        co_return std::get<parse_config_file_result>(b4g_config_or_err);

    const void* new_tag = std::to_address(it);
    auto& conf = it->second;
    try
    {
        auto& b4g_config_json = std::get<nlohmann::json>(b4g_config_or_err);
        conf.config.emplace(b4g_config_json.get<config::b4g_map>());
    }
    catch (const nlohmann::json::exception&)
    {
        conf.diags.push_back(error_B4G001(cfg_file_rl));
        co_return parse_config_file_result::error;
    }

    co_await process_processor_group_and_cleanup_libraries(
        m_proc_grps_source.pgroups, m_proc_grps_source.macro_extensions, alternative_root, conf.diags);

    const auto cfg_file_root = cfg_file_rl.parent();

    const auto create_pgm = [&alternative_root](
                                utils::resource::resource_location pgm_rl, std::string_view pgroup_name) {
        static const config::assembler_options empty_asm_opts {};

        return program {
            std::move(pgm_rl),
            b4g_conf {
                std::string(pgroup_name),
                alternative_root,
            },
            empty_asm_opts,
            false,
        };
    };

    for (const auto& [name, details] : conf.config.value().files)
        m_pgm_conf_store->add_exact_conf(
            create_pgm(utils::resource::resource_location::join(cfg_file_root, name).lexically_normal(),
                details.processor_group_name),
            new_tag,
            cfg_file_rl);

    if (const auto& def_grp = conf.config.value().default_processor_group_name; !def_grp.empty())
        m_pgm_conf_store->add_regex_conf(
            create_pgm(utils::resource::resource_location::join(cfg_file_root, "*"), def_grp), new_tag, cfg_file_rl);

    co_return parse_config_file_result::parsed;
}

utils::task workspace_configuration::find_and_add_libs(utils::resource::resource_location root,
    std::string path_pattern,
    processor_group& prc_grp,
    library_local_options opts,
    std::vector<diagnostic>& diags)
{
    std::regex path_validator = percent_encoded_pathmask_to_regex(path_pattern);

    std::unordered_set<std::string> processed_canonical_paths;
    std::deque<std::pair<std::string, utils::resource::resource_location>> dirs_to_search;

    if (std::error_code ec; dirs_to_search.emplace_back(m_file_manager.canonical(root, ec), root), ec)
    {
        if (!opts.optional_library)
            diags.push_back(error_L0001(m_proc_grps_current_loc, root));
        co_return;
    }

    constexpr size_t limit = 1000;
    for (bool first_ = true; !dirs_to_search.empty();)
    {
        const auto first = std::exchange(first_, false);
        if (processed_canonical_paths.size() > limit)
        {
            const auto presentable_pattern = utils::path::get_presentable_uri(path_pattern, false);
            diags.push_back(warning_L0005(m_proc_grps_current_loc, presentable_pattern, limit));
            break;
        }

        auto [canonical_path, dir] = std::move(dirs_to_search.front());
        dirs_to_search.pop_front();

        if (!processed_canonical_paths.insert(std::move(canonical_path)).second)
            continue;

        if (const auto dir_uri = dir.get_uri(); std::regex_match(dir_uri.begin(), dir_uri.end(), path_validator))
            prc_grp.add_library(get_local_library(dir, opts));

        auto [subdir_list, return_code] = co_await m_file_manager.list_directory_subdirs_and_symlinks(dir);
        if (return_code != utils::path::list_directory_rc::done)
        {
            if (!first || !opts.optional_library || return_code != utils::path::list_directory_rc::not_exists)
                diags.push_back(error_L0001(m_proc_grps_current_loc, dir));
            break;
        }

        for (auto& [subdir_canonical_path, subdir] : subdir_list)
        {
            if (processed_canonical_paths.contains(subdir_canonical_path))
                continue;

            dirs_to_search.emplace_back(std::move(subdir_canonical_path), subdir.lexically_normal());
        }
    }
}

workspace_configuration::watcher_registration_handle workspace_configuration::add_watcher(
    std::string_view uri, bool recursive)
{
    if (m_watch_provider)
        return watcher_registration_handle(m_watch_provider, m_watch_provider->add_watcher(uri, recursive));
    else
        return watcher_registration_handle();
}

void workspace_configuration::add_missing_diags(std::vector<diagnostic>& target,
    const utils::resource::resource_location& config_file_rl,
    const std::vector<utils::resource::resource_location>& opened_files,
    bool include_advisory_cfg_diags) const
{
    constexpr static diagnostic (*diags_matrix[2][2])(const utils::resource::resource_location&, std::string_view) = {
        { warn_B4G003, error_B4G002 },
        { warn_W0008, error_W0004 },
    };

    bool empty_cfg_rl = config_file_rl.empty();
    const auto& adjusted_conf_rl = empty_cfg_rl ? m_pgm_conf_current_loc : config_file_rl;

    for (const auto& categorized_missing_pgroups =
             m_pgm_conf_store->get_categorized_missing_pgroups(config_file_rl, opened_files);
         const auto& [missing_pgroup_name, used] : categorized_missing_pgroups)
    {
        if (!include_advisory_cfg_diags && !used)
            continue;

        target.push_back(diags_matrix[empty_cfg_rl][used](adjusted_conf_rl, missing_pgroup_name));
    }
}

void workspace_configuration::produce_diagnostics(std::vector<diagnostic>& target,
    const std::unordered_map<utils::resource::resource_location, std::vector<utils::resource::resource_location>>&
        used_configs_opened_files_map,
    bool include_advisory_cfg_diags) const
{
    for (auto& [key, value] : m_proc_grps)
    {
        if (const auto* e = std::get_if<external_conf>(&key); e && e->definition.use_count() <= 1)
            continue;
        const auto& [pg, _] = value;
        pg.copy_diagnostics(target);
    }

    for (const auto& diag : m_config_diags)
        target.push_back(diag);

    for (const auto& [config_rl, opened_files] : used_configs_opened_files_map)
    {
        if (const auto& b4g_config_cache_it = m_b4g_config_cache.find(config_rl);
            b4g_config_cache_it != m_b4g_config_cache.end())
        {
            for (const auto& d : b4g_config_cache_it->second.diags)
                target.push_back(d);
        }

        add_missing_diags(target, config_rl, opened_files, include_advisory_cfg_diags);
    }
}

utils::value_task<parse_config_file_result> workspace_configuration::parse_configuration_file(
    std::optional<utils::resource::resource_location> file)
{
    if (!file.has_value() || is_config_file(*file))
        return load_and_process_config(m_config_diags);

    if (!m_location.empty() && is_b4g_config_file(*file))
        return parse_b4g_config_file(std::move(*file));

    return utils::value_task<parse_config_file_result>::from_value(parse_config_file_result::not_found);
}

utils::value_task<std::optional<std::vector<index_t<processor_group, unsigned long long>>>>
workspace_configuration::refresh_libraries(const std::vector<utils::resource::resource_location>& file_locations)
{
    using return_type = std::optional<std::vector<index_t<processor_group, unsigned long long>>>;
    return_type result;
    std::unordered_set<utils::resource::resource_location> no_filename_rls;

    for (const auto& file_loc : file_locations)
        no_filename_rls.insert(utils::resource::resource_location::replace_filename(file_loc, ""));

    if (std::ranges::any_of(file_locations,
            [this, hlasm_folder = utils::resource::resource_location::join(m_location, HLASM_PLUGIN_FOLDER)](
                const auto& uri) { return is_configuration_file(uri) || uri == hlasm_folder; }))
    {
        // TODO: we could diff the configuration and really return only changed groups
        result.emplace();
        for (const auto& [_, proc_grp] : m_proc_grps)
            result->emplace_back(proc_grp.second);

        return parse_configuration_file()
            .then([](auto) {})
            .then(utils::value_task<return_type>::from_value(std::move(result)));
    }

    std::vector<utils::task> pending_refreshes;
    for (std::unordered_set<const library*> refreshed_libs; auto& [_, value] : m_proc_grps)
    {
        auto& [proc_grp, id] = value;
        bool pending_refresh = false;
        if (!proc_grp.refresh_needed(no_filename_rls, file_locations))
            continue;
        if (!result)
            result.emplace();
        result->emplace_back(id);
        for (const auto& lib : proc_grp.libraries())
        {
            if (!refreshed_libs.emplace(std::to_address(lib)).second || !lib->has_cached_content())
                continue;
            if (auto refresh = lib->refresh(); refresh.valid() && !refresh.done())
            {
                pending_refreshes.emplace_back(std::move(refresh));
                pending_refresh = true;
            }
        }
        if (!pending_refresh)
            proc_grp.invalidate_suggestions();
        else
            pending_refreshes.emplace_back([](auto& pg) -> utils::task {
                pg.invalidate_suggestions();
                co_return;
            }(proc_grp));
    }

    return utils::task::wait_all(std::move(pending_refreshes))
        .then(utils::value_task<return_type>::from_value(std::move(result)));
}

const processor_group* workspace_configuration::get_proc_grp_by_program(const program& pgm) const
{
    if (auto it = m_proc_grps.find(pgm.pgroup); it != m_proc_grps.end())
        return &it->second.first;

    return nullptr;
}

processor_group* workspace_configuration::get_proc_grp_by_program(const program& pgm)
{
    if (auto it = m_proc_grps.find(pgm.pgroup); it != m_proc_grps.end())
        return &it->second.first;

    return nullptr;
}

const processor_group& workspace_configuration::get_proc_grp(const proc_grp_id& p) const
{
    return m_proc_grps.at(p).first;
}

const program* workspace_configuration::get_program(const utils::resource::resource_location& file_location) const
{
    return m_pgm_conf_store->get_program(file_location.lexically_normal()).pgm;
}

const processor_group* workspace_configuration::get_proc_grp(const utils::resource::resource_location& file) const
{
    if (const auto* pgm = get_program(file); pgm)
        return get_proc_grp_by_program(*pgm);
    return nullptr;
}


utils::value_task<decltype(workspace_configuration::m_proc_grps)::iterator>
workspace_configuration::make_external_proc_group(
    const utils::resource::resource_location& normalized_location, std::string group_json)
{
    config::processor_group pg;
    global_settings_map utilized_settings_values;

    const auto current_settings = m_global_settings.load();
    json_settings_replacer json_visitor { *current_settings, utilized_settings_values, m_proc_base };

    std::vector<diagnostic> diags;

    auto proc_json = nlohmann::json::parse(group_json);
    json_visitor(proc_json);
    proc_json.get_to(pg);

    if (!pg.asm_options.valid())
        diags.push_back(error_W0005(normalized_location, pg.name, "external processor group"));
    for (const auto& p : pg.preprocessors)
    {
        if (!p.valid())
            diags.push_back(error_W0006(normalized_location, pg.name, p.type()));
    }

    processor_group prc_grp("", pg.asm_options, pg.preprocessors);

    for (auto& lib_or_dataset : pg.libs)
    {
        // Workaround for poor symmetric transfer support in WebAssembly
        auto t = std::visit(
            [this, &diags, &prc_grp](const auto& lib) {
                return process_processor_group_library(lib, empty_alternative_cfg_root, diags, {}, prc_grp);
            },
            lib_or_dataset);
        if (t.valid())
            co_await std::move(t);
    }
    m_utilized_settings_values.merge(std::move(utilized_settings_values));

    for (auto&& d : diags)
        prc_grp.add_external_diagnostic(std::move(d));

    co_return m_proc_grps
        .try_emplace(external_conf { std::make_shared<std::string>(std::move(group_json)) },
            std::move(prc_grp),
            make_unique_id())
        .first;
}

utils::task workspace_configuration::update_external_configuration(
    const utils::resource::resource_location& normalized_location, std::string group_json)
{
    if (std::string_view group_name(group_json); utils::trim_left(group_name, " \t\n\r"), group_name.starts_with("\""))
    {
        m_pgm_conf_store->update_exact_conf(
            program {
                normalized_location,
                basic_conf { nlohmann::json::parse(group_json).get<std::string>() },
                {},
                true,
            },
            nullptr,
            empty_alternative_cfg_root);
        co_return;
    }

    auto pg = m_proc_grps.find(tagged_string_view<external_conf> { group_json });
    if (pg == m_proc_grps.end())
        pg = co_await make_external_proc_group(normalized_location, std::move(group_json));

    m_pgm_conf_store->update_exact_conf(
        program {
            normalized_location,
            pg->first,
            {},
            true,
        },
        nullptr,
        empty_alternative_cfg_root);
}

void workspace_configuration::prune_external_processor_groups(const utils::resource::resource_location& location)
{
    m_pgm_conf_store->prune_external_processor_groups(location);

    std::erase_if(m_proc_grps, [](const auto& pg) {
        const auto* e = std::get_if<external_conf>(&pg.first);
        return e && e->definition.use_count() == 1;
    });
}

opcode_suggestion_data workspace_configuration::get_opcode_suggestion_data(
    const utils::resource::resource_location& url)
{
    opcode_suggestion_data result {};
    if (auto pgm = get_program(url))
    {
        if (auto proc_grp = get_proc_grp_by_program(*pgm); proc_grp)
        {
            proc_grp->apply_options_to(result.opts);
            result.proc_grp = proc_grp;
        }
        pgm->asm_opts.apply_options_to(result.opts);
    }

    return result;
}

utils::value_task<std::pair<analyzer_configuration, index_t<processor_group, unsigned long long>>>
workspace_configuration::get_analyzer_configuration(utils::resource::resource_location url)
{
    auto alt_config = co_await load_alternative_config_if_needed(url);
    const auto* pgm = get_program(url);
    const processor_group* proc_grp = nullptr;
    index_t<processor_group, unsigned long long> group_id;
    asm_option opts;
    if (pgm)
    {
        if (auto it = m_proc_grps.find(pgm->pgroup); it != m_proc_grps.end())
        {
            proc_grp = &it->second.first;
            group_id = it->second.second;
            proc_grp->apply_options_to(opts);
        }
        pgm->asm_opts.apply_options_to(opts);
    }

    auto relative_to_location = url.lexically_relative(m_location).lexically_normal();

    const auto& sysin_path = !pgm && (relative_to_location.empty() || relative_to_location.lexically_out_of_scope())
        ? url
        : relative_to_location;
    opts.sysin_member = sysin_path.filename();
    opts.sysin_dsn = sysin_path.parent().get_local_path_or_uri();

    co_return {
        analyzer_configuration {
            .libraries = proc_grp ? proc_grp->libraries() : std::vector<std::shared_ptr<library>>(),
            .opts = std::move(opts),
            .pp_opts = proc_grp ? proc_grp->preprocessors() : std::vector<preprocessor_options>(),
            .alternative_config_url = std::move(alt_config),
            .dig_suppress_limit = m_local_config.fill_missing_settings(m_global_config).diag_supress_limit.value(),
        },
        group_id,
    };
}

utils::value_task<utils::resource::resource_location> workspace_configuration::load_alternative_config_if_needed(
    const utils::resource::resource_location& file_location)
{
    using enum program_configuration_storage::cfg_affiliation;

    const auto rl = file_location.lexically_normal();
    auto [_, affiliation] = m_pgm_conf_store->get_program(rl);

    if (affiliation == exact_pgm || affiliation == exact_ext)
        co_return empty_alternative_cfg_root;

    if (m_external_configuration_requests)
    {
        struct resp
        {
            std::variant<int, std::string> result;
            void provide(std::string_view c) { result = std::string(c); }
            void error(int err, const char*) noexcept { result = err; }
        };
        auto [c, i] = make_workspace_manager_response(std::in_place_type<resp>);
        m_external_configuration_requests->read_external_configuration(rl.get_uri(), c);

        auto json_data = co_await utils::async_busy_wait(std::move(c), &i->result);
        if (std::holds_alternative<std::string>(json_data))
        {
            try
            {
                co_await update_external_configuration(rl, std::move(std::get<std::string>(json_data)));
                co_return empty_alternative_cfg_root;
            }
            catch (const nlohmann::json&)
            {
                // incompatible json in the response
                json_data = -1;
            }
        }

        if (std::get<int>(json_data) != 0)
        {
            // TODO: do we do something with the error?
            // this basically indicates either an error on the client side,
            //  or an allocation failure while processing the response
        }
    }

    if (affiliation == regex_pgm || m_location.empty())
        co_return empty_alternative_cfg_root;

    auto configuration_url = utils::resource::resource_location::replace_filename(rl, B4G_CONF_FILE);
    if (affiliation == exact_b4g || affiliation == regex_b4g)
        co_return configuration_url;

    if (auto it = m_b4g_config_cache.find(configuration_url); it == m_b4g_config_cache.end())
    {
        if (co_await parse_b4g_config_file(configuration_url) == parse_config_file_result::not_found)
            co_return empty_alternative_cfg_root;
        else
            co_return configuration_url;
    }
    else if (!it->second.config.has_value() && it->second.diags.empty()) // keep in sync with parse_b4g_config_file
        co_return empty_alternative_cfg_root;

    co_return configuration_url;
}

void workspace_configuration::change_processor_group_base(utils::resource::resource_location url)
{
    m_proc_base = std::move(url);
}

workspace_configuration::watcher_registration_handle::~watcher_registration_handle()
{
    // TODO: This can throw - there is nothing we can do about it, just catch silently?
    if (provider)
        provider->remove_watcher(id);
}

constexpr workspace_configuration::watcher_registration_handle::watcher_registration_handle(
    watcher_registration_handle&& o) noexcept
    : provider(std::exchange(o.provider, nullptr))
    , id(std::exchange(o.id, watcher_registration_id::INVALID))
{}

workspace_configuration::watcher_registration_handle& workspace_configuration::watcher_registration_handle::operator=(
    watcher_registration_handle&& o) noexcept
{
    watcher_registration_handle tmp(std::move(o));
    std::swap(provider, tmp.provider);
    std::swap(id, tmp.id);
    return *this;
}

} // namespace hlasm_plugin::parser_library::workspaces
