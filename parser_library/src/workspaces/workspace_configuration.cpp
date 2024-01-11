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

#include "external_configuration_requests.h"
#include "file_manager.h"
#include "library_local.h"
#include "nlohmann/json.hpp"
#include "utils/async_busy_wait.h"
#include "utils/content_loader.h"
#include "utils/encoding.h"
#include "utils/path.h"
#include "utils/path_conversions.h"
#include "utils/platform.h"
#include "utils/string_operations.h"
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

utils::resource::resource_location transform_to_resource_location(
    std::string_view path, const utils::resource::resource_location& base_resource_location)
{
    using utils::resource::resource_location;
    resource_location rl;

    if (resource_location::is_local(path))
        rl = resource_location("file:" + utils::encoding::percent_encode_and_ignore_utf8(path.substr(5)));
    else if (utils::path::is_uri(path))
        rl = resource_location(path);
    else if (auto fs_path = get_fs_abs_path(path); fs_path.has_value())
        rl = resource_location(utils::path::path_to_uri(utils::path::lexically_normal(*fs_path).string()));
    else
        rl = resource_location::join(base_resource_location, utils::encoding::percent_encode(path));

    return rl.lexically_normal();
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

std::optional<std::string_view> find_setting(std::string_view key, const nlohmann::json& m_j)
{
    const nlohmann::json* j = &m_j;

    while (true)
    {
        auto dot = key.find('.');
        auto subkey = key.substr(0, dot);

        j = find_member(subkey, *j);
        if (!j)
            return std::nullopt;

        if (dot == std::string_view::npos)
            break;
        else
            key.remove_prefix(dot + 1);
    }

    if (!j->is_string())
        return std::nullopt;
    else
        return j->get<std::string_view>();
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
                auto v = find_setting(reduced_key, global_settings);
                if (v.has_value())
                    r.append(*v);
                else
                    unavailable.emplace(key);
                utilized_settings_values.emplace(reduced_key, v);
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
};

const std::regex json_settings_replacer::config_reference(R"(\$\{([^}]+)\})");
} // namespace

workspace_configuration::workspace_configuration(file_manager& fm,
    utils::resource::resource_location location,
    const shared_json& global_settings,
    external_configuration_requests* ecr)
    : m_file_manager(fm)
    , m_location(std::move(location))
    , m_global_settings(global_settings)
    , m_external_configuration_requests(ecr)
    , m_pgm_conf_store(std::make_unique<program_configuration_storage>(m_proc_grps))
{
    auto hlasm_folder = utils::resource::resource_location::join(m_location, HLASM_PLUGIN_FOLDER);
    m_proc_grps_loc = utils::resource::resource_location::join(hlasm_folder, FILENAME_PROC_GRPS);
    m_pgm_conf_loc = utils::resource::resource_location::join(hlasm_folder, FILENAME_PGM_CONF);
}

workspace_configuration::~workspace_configuration() = default;

bool workspace_configuration::is_configuration_file(const utils::resource::resource_location& file) const
{
    return !m_location.empty() && (is_config_file(file) || is_b4g_config_file(file));
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

std::shared_ptr<library> workspace_configuration::get_local_library(
    const utils::resource::resource_location& url, const library_local_options& opts)
{
    if (auto it = m_libraries.find(std::tie(url, opts)); it != m_libraries.end())
    {
        it->second.second = true;
        return it->second.first;
    }

    auto result = std::make_shared<library_local>(m_file_manager, url, opts, m_proc_grps_loc);
    m_libraries.try_emplace(std::make_pair(url, library_options(opts)), result, true);
    return result;
}

utils::task workspace_configuration::process_processor_group(const config::processor_group& pg,
    std::span<const std::string> fallback_macro_extensions,
    const utils::resource::resource_location& alternative_root,
    std::vector<diagnostic_s>& diags)
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
    if (alternative_root.empty())
        m_proc_grps.try_emplace(basic_conf { prc_grp.name() }, std::move(prc_grp));
    else
        m_proc_grps.try_emplace(b4g_conf { prc_grp.name(), alternative_root }, std::move(prc_grp));

    co_return;
}

constexpr std::string_view external_uri_scheme = "hlasm-external";

utils::task workspace_configuration::process_processor_group_library(const config::dataset& dsn,
    const utils::resource::resource_location&,
    std::vector<diagnostic_s>&,
    std::span<const std::string>,
    processor_group& prc_grp)
{
    utils::path::dissected_uri new_uri_components;
    new_uri_components.scheme = external_uri_scheme;
    new_uri_components.path = "/DATASET/" + utils::encoding::percent_encode_component(dsn.dsn);
    if (!m_location.get_uri().empty())
        new_uri_components.fragment = utils::encoding::uri_friendly_base16_encode(m_location.get_uri());
    utils::resource::resource_location new_uri(utils::path::reconstruct_uri(new_uri_components));

    prc_grp.add_library(get_local_library(new_uri, { .optional_library = dsn.optional }));

    return {};
}

utils::task workspace_configuration::process_processor_group_library(const config::endevor& end,
    const utils::resource::resource_location&,
    std::vector<diagnostic_s>&,
    std::span<const std::string>,
    processor_group& prc_grp)
{
    utils::path::dissected_uri new_uri_components;
    new_uri_components.scheme = external_uri_scheme;
    new_uri_components.path = "/ENDEVOR/" + utils::encoding::percent_encode_component(end.profile) + "/"
        + std::string(end.use_map ? "map" : "nomap") + "/" + utils::encoding::percent_encode_component(end.environment)
        + "/" + utils::encoding::percent_encode_component(end.stage) + "/"
        + utils::encoding::percent_encode_component(end.system) + "/"
        + utils::encoding::percent_encode_component(end.subsystem) + "/"
        + utils::encoding::percent_encode_component(end.type);
    if (!m_location.get_uri().empty())
        new_uri_components.fragment = utils::encoding::uri_friendly_base16_encode(m_location.get_uri());
    utils::resource::resource_location new_uri(utils::path::reconstruct_uri(new_uri_components));

    prc_grp.add_library(get_local_library(new_uri, { .optional_library = end.optional }));

    return {};
}

utils::task workspace_configuration::process_processor_group_library(const config::endevor_dataset& end,
    const utils::resource::resource_location&,
    std::vector<diagnostic_s>&,
    std::span<const std::string>,
    processor_group& prc_grp)
{
    utils::path::dissected_uri new_uri_components;
    new_uri_components.scheme = external_uri_scheme;
    new_uri_components.path = "/ENDEVOR/" + utils::encoding::percent_encode_component(end.profile) + "/"
        + utils::encoding::percent_encode_component(end.dsn);
    if (!m_location.get_uri().empty())
        new_uri_components.fragment = utils::encoding::uri_friendly_base16_encode(m_location.get_uri());
    utils::resource::resource_location new_uri(utils::path::reconstruct_uri(new_uri_components));

    prc_grp.add_library(get_local_library(new_uri, { .optional_library = end.optional }));

    return {};
}

namespace {
void modify_hlasm_external_uri(
    utils::resource::resource_location& rl, const utils::resource::resource_location& workspace)
{
    auto rl_uri = rl.get_uri();
    if (!rl_uri.starts_with(external_uri_scheme))
        return;

    // mainly to support testing, but could be useful in general
    // hlasm-external:/path... is transformed into hlasm-external:/path...#<friendly workspace uri>
    utils::path::dissected_uri uri_components = utils::path::dissect_uri(rl_uri);
    if (uri_components.scheme == external_uri_scheme && !uri_components.fragment.has_value()
        && !workspace.get_uri().empty())
    {
        uri_components.fragment = utils::encoding::uri_friendly_base16_encode(workspace.get_uri());
        rl = utils::resource::resource_location(utils::path::reconstruct_uri(uri_components));
    }
}
} // namespace

utils::task workspace_configuration::process_processor_group_library(const config::library& lib,
    const utils::resource::resource_location& alternative_root,
    std::vector<diagnostic_s>& diags,
    std::span<const std::string> fallback_macro_extensions,
    processor_group& prc_grp)
{
    const auto& root =
        lib.root_folder == config::processor_group_root_folder::alternate_root && !alternative_root.empty()
        ? alternative_root
        : m_location;

    std::optional<std::string> lib_path = substitute_home_directory(lib.path);
    if (!lib_path.has_value())
    {
        diags.push_back(diagnostic_s::warning_L0006(m_proc_grps_loc, lib.path));
        return {};
    }

    auto lib_local_opts = get_library_local_options(lib, fallback_macro_extensions);
    auto rl = transform_to_resource_location(*lib_path, root);
    rl.join(""); // Ensure that this is a directory

    if (auto first_wild_card = rl.get_uri().find_first_of("*?"); first_wild_card == std::string::npos)
    {
        modify_hlasm_external_uri(rl, m_location);
        prc_grp.add_library(get_local_library(rl, lib_local_opts));
        return {};
    }
    else
    {
        utils::resource::resource_location search_root(
            rl.get_uri().substr(0, rl.get_uri().find_last_of("/", first_wild_card) + 1));
        return find_and_add_libs(std::move(search_root), std::move(rl), prc_grp, std::move(lib_local_opts), diags);
    }
}

utils::task workspace_configuration::process_processor_group_and_cleanup_libraries(
    std::span<const config::processor_group> pgs,
    std::span<const std::string> fallback_macro_extensions,
    const utils::resource::resource_location& alternative_root,
    std::vector<diagnostic_s>& diags)
{
    for (auto& [_, l] : m_libraries)
        l.second = false; // mark

    for (const auto& pg : pgs)
        co_await process_processor_group(pg, fallback_macro_extensions, alternative_root, diags);

    std::erase_if(m_libraries, [](const auto& kv) { return !kv.second.second; }); // sweep
}

void workspace_configuration::process_program(const config::program_mapping& pgm, std::vector<diagnostic_s>& diags)
{
    std::optional<std::string> pgm_name = substitute_home_directory(pgm.program);
    if (!pgm_name.has_value())
    {
        diags.push_back(diagnostic_s::warning_L0006(m_pgm_conf_loc, pgm.program));
        return;
    }

    program prog {
        transform_to_resource_location(*pgm_name, m_location),
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
    return file == m_proc_grps_loc || file == m_pgm_conf_loc;
}

bool workspace_configuration::is_b4g_config_file(const utils::resource::resource_location& file) const
{
    return file.filename() == B4G_CONF_FILE;
}

// open config files and parse them
utils::value_task<parse_config_file_result> workspace_configuration::load_and_process_config(
    std::vector<diagnostic_s>& diags)
{
    diags.clear();

    config::proc_grps proc_groups;
    global_settings_map utilized_settings_values;

    m_proc_grps.clear();
    m_pgm_conf_store->clear();
    m_b4g_config_cache.clear();

    if (auto l = co_await load_proc_config(proc_groups, utilized_settings_values, diags);
        l != parse_config_file_result::parsed)
        co_return l;

    config::pgm_conf pgm_config;
    const auto pgm_conf_loaded = co_await load_pgm_config(pgm_config, utilized_settings_values, diags);

    co_await process_processor_group_and_cleanup_libraries(
        proc_groups.pgroups, proc_groups.macro_extensions, empty_alternative_cfg_root, diags);

    if (pgm_conf_loaded != parse_config_file_result::parsed)
    {
        m_local_config = {};
    }
    else
    {
        m_local_config = lib_config::load_from_pgm_config(pgm_config);

        // process programs
        for (const auto& pgm : pgm_config.pgms)
            process_program(pgm, diags);
    }

    m_utilized_settings_values = std::move(utilized_settings_values);
    m_proc_grps_source = std::move(proc_groups);

    // we need to tolerate pgm_conf processing failure, because other products may provide the info
    co_return parse_config_file_result::parsed;
}

utils::value_task<parse_config_file_result> workspace_configuration::load_proc_config(
    config::proc_grps& proc_groups, global_settings_map& utilized_settings_values, std::vector<diagnostic_s>& diags)
{
    const auto current_settings = m_global_settings.load();
    json_settings_replacer json_visitor { *current_settings, utilized_settings_values, m_location };

    // proc_grps.json parse
    auto proc_grps_content = co_await m_file_manager.get_file_content(m_proc_grps_loc);
    if (!proc_grps_content.has_value())
        co_return parse_config_file_result::not_found;

    try
    {
        auto proc_json = nlohmann::json::parse(proc_grps_content.value());
        json_visitor(proc_json);
        proc_json.get_to(proc_groups);
    }
    catch (const nlohmann::json::exception&)
    {
        // could not load proc_grps
        diags.push_back(diagnostic_s::error_W0002(m_proc_grps_loc));
        co_return parse_config_file_result::error;
    }

    for (const auto& var : json_visitor.unavailable)
        diags.push_back(diagnostic_s::warn_W0007(m_proc_grps_loc, var));

    for (const auto& pg : proc_groups.pgroups)
    {
        if (!pg.asm_options.valid())
            diags.push_back(diagnostic_s::error_W0005(m_proc_grps_loc, pg.name, "processor group"));
        for (const auto& p : pg.preprocessors)
        {
            if (!p.valid())
                diags.push_back(diagnostic_s::error_W0006(m_proc_grps_loc, pg.name, p.type()));
        }
    }

    co_return parse_config_file_result::parsed;
}

utils::value_task<parse_config_file_result> workspace_configuration::load_pgm_config(
    config::pgm_conf& pgm_config, global_settings_map& utilized_settings_values, std::vector<diagnostic_s>& diags)
{
    const auto current_settings = m_global_settings.load();
    json_settings_replacer json_visitor { *current_settings, utilized_settings_values, m_location };

    // pgm_conf.json parse
    auto pgm_conf_content = co_await m_file_manager.get_file_content(m_pgm_conf_loc);
    if (!pgm_conf_content.has_value())
        co_return parse_config_file_result::not_found;

    try
    {
        auto pgm_json = nlohmann::json::parse(pgm_conf_content.value());
        json_visitor(pgm_json);
        pgm_json.get_to(pgm_config);
    }
    catch (const nlohmann::json::exception&)
    {
        diags.push_back(diagnostic_s::error_W0003(m_pgm_conf_loc));
        co_return parse_config_file_result::error;
    }

    for (const auto& var : json_visitor.unavailable)
        diags.push_back(diagnostic_s::warn_W0007(m_pgm_conf_loc, var));

    for (const auto& pgm : pgm_config.pgms)
    {
        if (!pgm.opts.valid())
            diags.push_back(diagnostic_s::error_W0005(m_pgm_conf_loc, pgm.program, "program"));
    }

    co_return parse_config_file_result::parsed;
}

bool workspace_configuration::settings_updated() const
{
    auto global_settings = m_global_settings.load();
    for (const auto& [key, value] : m_utilized_settings_values)
    {
        if (find_setting(key, *global_settings) != value)
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

    auto b4g_config_content = co_await m_file_manager.get_file_content(cfg_file_rl);
    if (!b4g_config_content.has_value())
        co_return parse_config_file_result::not_found;

    const void* new_tag = std::to_address(it);
    auto& conf = it->second;
    try
    {
        conf.config.emplace(nlohmann::json::parse(b4g_config_content.value()).get<config::b4g_map>());
    }
    catch (const nlohmann::json::exception&)
    {
        conf.diags.push_back(diagnostic_s::error_B4G001(cfg_file_rl));
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
    utils::resource::resource_location path_pattern,
    processor_group& prc_grp,
    library_local_options opts,
    std::vector<diagnostic_s>& diags)
{
    std::regex path_validator = percent_encoded_pathmask_to_regex(path_pattern.get_uri());

    std::unordered_set<std::string> processed_canonical_paths;
    std::deque<std::pair<std::string, utils::resource::resource_location>> dirs_to_search;

    if (std::error_code ec; dirs_to_search.emplace_back(m_file_manager.canonical(root, ec), root), ec)
    {
        if (!opts.optional_library)
            diags.push_back(diagnostic_s::error_L0001(m_proc_grps_loc, root));
        co_return;
    }

    constexpr size_t limit = 1000;
    for (bool first_ = true; !dirs_to_search.empty();)
    {
        const auto first = std::exchange(first_, false);
        if (processed_canonical_paths.size() > limit)
        {
            diags.push_back(diagnostic_s::warning_L0005(m_proc_grps_loc, path_pattern.to_presentable(), limit));
            break;
        }

        auto [canonical_path, dir] = std::move(dirs_to_search.front());
        dirs_to_search.pop_front();

        if (!processed_canonical_paths.insert(std::move(canonical_path)).second)
            continue;

        if (std::regex_match(dir.get_uri(), path_validator))
            prc_grp.add_library(get_local_library(dir, opts));

        auto [subdir_list, return_code] = co_await m_file_manager.list_directory_subdirs_and_symlinks(dir);
        if (return_code != utils::path::list_directory_rc::done)
        {
            if (!first || !opts.optional_library || return_code != utils::path::list_directory_rc::not_exists)
                diags.push_back(diagnostic_s::error_L0001(m_proc_grps_loc, dir));
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

void workspace_configuration::add_missing_diags(const diagnosable& target,
    const utils::resource::resource_location& config_file_rl,
    const std::vector<utils::resource::resource_location>& opened_files,
    bool include_advisory_cfg_diags) const
{
    constexpr static diagnostic_s (*diags_matrix[2][2])(const utils::resource::resource_location&, std::string_view) = {
        { diagnostic_s::warn_B4G003, diagnostic_s::error_B4G002 },
        { diagnostic_s::warn_W0008, diagnostic_s::error_W0004 },
    };

    bool empty_cfg_rl = config_file_rl.empty();
    const auto& adjusted_conf_rl = empty_cfg_rl ? m_pgm_conf_loc : config_file_rl;

    for (const auto& categorized_missing_pgroups =
             m_pgm_conf_store->get_categorized_missing_pgroups(config_file_rl, opened_files);
         const auto& [missing_pgroup_name, used] : categorized_missing_pgroups)
    {
        if (!include_advisory_cfg_diags && !used)
            continue;

        target.add_diagnostic(diags_matrix[empty_cfg_rl][used](adjusted_conf_rl, missing_pgroup_name));
    }
}

void workspace_configuration::produce_diagnostics(
    const diagnosable& target, const configuration_diagnostics_parameters& config_diag_params) const
{
    for (auto& [key, pg] : m_proc_grps)
    {
        if (const auto* e = std::get_if<external_conf>(&key); e && e->definition.use_count() <= 1)
            continue;
        pg.collect_diags();
        for (const auto& d : pg.diags())
            target.add_diagnostic(d);
        pg.diags().clear();
    }

    for (const auto& diag : m_config_diags)
        target.add_diagnostic(diag);

    for (const auto& [config_rl, opened_files] : config_diag_params.used_configs_opened_files_map)
    {
        if (const auto& b4g_config_cache_it = m_b4g_config_cache.find(config_rl);
            b4g_config_cache_it != m_b4g_config_cache.end())
        {
            for (const auto& d : b4g_config_cache_it->second.diags)
                target.add_diagnostic(d);
        }

        add_missing_diags(target, config_rl, opened_files, config_diag_params.include_advisory_cfg_diags);
    }
}

utils::value_task<parse_config_file_result> workspace_configuration::parse_configuration_file(
    std::optional<utils::resource::resource_location> file)
{
    if (!m_location.empty())
    {
        if (!file.has_value() || is_config_file(*file))
            return load_and_process_config(m_config_diags);

        if (is_b4g_config_file(*file))
            return parse_b4g_config_file(std::move(*file));
    }

    return utils::value_task<parse_config_file_result>::from_value(parse_config_file_result::not_found);
}

utils::value_task<std::optional<std::vector<const processor_group*>>> workspace_configuration::refresh_libraries(
    const std::vector<utils::resource::resource_location>& file_locations)
{
    std::optional<std::vector<const processor_group*>> result;
    std::unordered_set<utils::resource::resource_location, utils::resource::resource_location_hasher> no_filename_rls;

    for (const auto& file_loc : file_locations)
        no_filename_rls.insert(utils::resource::resource_location::replace_filename(file_loc, ""));

    if (std::any_of(file_locations.begin(),
            file_locations.end(),
            [this, hlasm_folder = utils::resource::resource_location::join(m_location, HLASM_PLUGIN_FOLDER)](
                const auto& uri) { return is_configuration_file(uri) || uri == hlasm_folder; }))
    {
        co_await parse_configuration_file();

        result.emplace();
        // TODO: we could diff the configuration and really return only changed groups
        for (const auto& [_, proc_grp] : m_proc_grps)
            result->emplace_back(&proc_grp);

        co_return result;
    }

    std::unordered_set<const library*> refreshed_libs;
    std::vector<utils::task> pending_refreshes;
    for (auto& [_, proc_grp] : m_proc_grps)
    {
        bool pending_refresh = false;
        if (!proc_grp.refresh_needed(no_filename_rls, file_locations))
            continue;
        if (!result)
            result.emplace();
        result->emplace_back(&proc_grp);
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

    for (auto& r : pending_refreshes)
        co_await std::move(r);

    co_return result;
}

const processor_group* workspace_configuration::get_proc_grp_by_program(const program& pgm) const
{
    if (auto it = m_proc_grps.find(pgm.pgroup); it != m_proc_grps.end())
        return &it->second;

    return nullptr;
}

processor_group* workspace_configuration::get_proc_grp_by_program(const program& pgm)
{
    if (auto it = m_proc_grps.find(pgm.pgroup); it != m_proc_grps.end())
        return &it->second;

    return nullptr;
}

const processor_group& workspace_configuration::get_proc_grp(const proc_grp_id& p) const { return m_proc_grps.at(p); }

const program* workspace_configuration::get_program(const utils::resource::resource_location& file_location) const
{
    return m_pgm_conf_store->get_program(file_location.lexically_normal()).pgm;
}

utils::value_task<decltype(workspace_configuration::m_proc_grps)::iterator>
workspace_configuration::make_external_proc_group(
    const utils::resource::resource_location& normalized_location, std::string group_json)
{
    config::processor_group pg;
    global_settings_map utilized_settings_values;

    const auto current_settings = m_global_settings.load();
    json_settings_replacer json_visitor { *current_settings, utilized_settings_values, m_location };

    std::vector<diagnostic_s> diags;

    auto proc_json = nlohmann::json::parse(group_json);
    json_visitor(proc_json);
    proc_json.get_to(pg);

    if (!pg.asm_options.valid())
        diags.push_back(diagnostic_s::error_W0005(normalized_location, pg.name, "external processor group"));
    for (const auto& p : pg.preprocessors)
    {
        if (!p.valid())
            diags.push_back(diagnostic_s::error_W0006(normalized_location, pg.name, p.type()));
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
        prc_grp.add_diagnostic(std::move(d));

    co_return m_proc_grps
        .try_emplace(external_conf { std::make_shared<std::string>(std::move(group_json)) }, std::move(prc_grp))
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
            void provide(sequence<char> c) { result = std::string(c); }
            void error(int err, const char*) noexcept { result = err; }
        };
        auto [c, i] = make_workspace_manager_response(std::in_place_type<resp>);
        m_external_configuration_requests->read_external_configuration(sequence<char>(rl.get_uri()), c);

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

} // namespace hlasm_plugin::parser_library::workspaces
